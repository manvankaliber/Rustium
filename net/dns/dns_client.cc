// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ref.h"
#include "net/dns/dns_client.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/notimplemented.h"
#include "base/rand_util.h"
#include "base/values.h"
#include "net/base/features.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/dns/address_sorter.h"
#include "net/dns/dns_session.h"
#include "net/dns/dns_transaction.h"
#include "net/dns/dns_util.h"
#include "net/dns/public/dns_over_https_config.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/public/secure_dns_mode.h"
#include "net/dns/resolve_context.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/socket/client_socket_factory.h"
#include "net/third_party/uri_template/uri_template.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

#include "net/dns/dns_request_family_ffi.h"
#include "net/dns/dns_client_rust_services.h"
#include "net/dns/public/dns_over_https_server_config.h"
namespace net {
namespace {
using namespace dns_request_family;
class DnsClientImpl final : public DnsClient {
 public:
  DnsClientImpl(NetLog* net_log, const RandIntCallback& random)
      : net_log_(net_log), rand_int_callback_(random) {
    UpdateSession(BuildEffectiveConfig());
  }
  DnsClientImpl(const DnsClientImpl&) = delete;
  DnsClientImpl& operator=(const DnsClientImpl&) = delete;
  ~DnsClientImpl() override = default;
  std::unique_ptr<DnsClientRustServices> MakeRustPolicyServices(ResolveContext* context) const override {
    auto frame = std::make_unique<Frame>(*this);
    frame->resolve_context = context; frame->query_only = true;
    return frame;
  }
  bool CanUseSecureDnsTransactions() const override { return Run(ClientEvent::kSecure).x; }
  bool CanUseInsecureDnsTransactions(std::optional<EchMode> ech) const override {
    return Run(ClientEvent::kInsecure, W(Ech(ech))).x;
  }
  bool CanQueryAdditionalTypesViaInsecureDns(std::optional<EchMode> ech) const override {
    return Run(ClientEvent::kAdditional, W(Ech(ech))).x;
  }
  void SetInsecureEnabled(InsecureDnsMode mode, bool additional) override {
    Frame f(*this); f.Run(ClientEvent::kSetInsecure, W(static_cast<int>(mode), 0, 0, additional));
  }
  InsecureDnsMode GetInsecureDnsMode(std::optional<EchMode> ech) const override {
    return static_cast<InsecureDnsMode>(Run(ClientEvent::kMode, W(Ech(ech))).x);
  }
  bool FallbackFromSecureTransactionPreferred(ResolveContext* context) const override {
    Frame f(*this); f.resolve_context = context; return f.Run(ClientEvent::kSecureFallback).x;
  }
  bool FallbackFromInsecureTransactionPreferred(std::optional<EchMode> ech) const override {
    return Run(ClientEvent::kInsecureFallback, W(Ech(ech))).x;
  }
  bool SetSystemConfig(std::optional<DnsConfig> config) override {
    Frame f(*this); f.input_system = std::move(config); return f.Run(ClientEvent::kSetSystem).x;
  }
  bool SetConfigOverrides(DnsConfigOverrides overrides) override {
    Frame f(*this); f.input_overrides = std::move(overrides); return f.Run(ClientEvent::kSetOverrides).x;
  }
  void ReplaceCurrentSession() override {
    DCHECK(session_); Frame f(*this); f.Run(ClientEvent::kReplaceSession);
  }
  DnsSession* GetCurrentSession() override { DCHECK(session_); return session_.get(); }
  const DnsConfig& GetEffectiveConfig() const override { DCHECK(session_); return session_->config(); }
  const DnsHosts* GetHosts() const override { return &GetEffectiveConfig().hosts; }
  std::optional<std::vector<IPEndPoint>> GetPresetAddrs(const url::SchemeHostPort& endpoint) const override {
    DCHECK(endpoint.IsValid()); Frame f(*this); f.endpoint = &endpoint;
    f.Run(ClientEvent::kPreset); return std::move(f.preset);
  }
  DnsTransactionFactory* GetTransactionFactory() override { DCHECK(factory_); return factory_.get(); }
  AddressSorter* GetAddressSorter() override { return address_sorter_.get(); }
  void IncrementInsecureFallbackFailures() override { Frame f(*this); f.Run(ClientEvent::kIncrementFailures); }
  void ClearInsecureFallbackFailures() override { Frame f(*this); f.Run(ClientEvent::kClearFailures); }
  base::DictValue GetDnsConfigAsValueForNetLog() const override {
    Frame f(*this); f.Run(ClientEvent::kConfigLog); return std::move(f.log);
  }
  std::optional<DnsConfig> GetSystemConfigForTesting() const override { return system_config_; }
  DnsConfigOverrides GetConfigOverridesForTesting() const override { return config_overrides_; }
  void SetTransactionFactoryForTesting(std::unique_ptr<DnsTransactionFactory> factory) override { factory_ = std::move(factory); }
  void SetAddressSorterForTesting(std::unique_ptr<AddressSorter> sorter) override { address_sorter_ = std::move(sorter); }
 private:
  class Frame : public DnsClientRustServices {
   public:
    explicit Frame(const DnsClientImpl& owner) : c(owner) {}
    explicit Frame(DnsClientImpl& owner) : c(owner), m(&owner) {}
    bool query_only = false;
    DnsResolverWords Dispatch(uint32_t op, DnsResolverWords in) override {
      if (query_only) {
        CHECK(op == static_cast<uint32_t>(ClientOp::kRead) ||
              op == static_cast<uint32_t>(ClientOp::kCheck) ||
              op == static_cast<uint32_t>(ClientOp::kMetric) ||
              op == static_cast<uint32_t>(ClientOp::kFeature));
        if (op == static_cast<uint32_t>(ClientOp::kRead)) {
          CHECK(in.x==1 || in.x==2 || in.x==3 || in.x==4 || in.x==5 ||
                in.x==6 || in.x==7 || in.x==9 || in.x==10 || in.x==20);
        }
      }
      return Native(static_cast<ClientOp>(op), in);
    }
    DnsResolverWords Run(ClientEvent event, DnsResolverWords in = {}) {
      return rustium_dns_request_family_run(3, static_cast<uint32_t>(event), in, this, &Call);
    }
    std::optional<DnsConfig> input_system;
    std::optional<DnsConfigOverrides> input_overrides;
    std::optional<DnsConfig> config;
    raw_ptr<ResolveContext> resolve_context = nullptr;
    raw_ptr<const url::SchemeHostPort> endpoint = nullptr;
    std::optional<std::vector<IPEndPoint>> preset;
    base::DictValue log;
   private:
    static DnsResolverWords Call(void* context, uint32_t op, DnsResolverWords in) {
      return static_cast<Frame*>(context)->Dispatch(op, in);
    }
    DnsClientImpl& Mutable() { CHECK(m); return *m; }
    const DnsOverHttpsServerConfig& Server(uint64_t i) const { return c->GetEffectiveConfig().doh_config.servers().at(i); }
    DnsResolverWords Native(ClientOp op, DnsResolverWords in) {
      switch (op) {
        case ClientOp::kRead:
          switch (in.x) {
            case 1: return W(c->GetEffectiveConfig().doh_config.servers().size());
            case 2: return W(static_cast<int>(c->insecure_dns_mode_));
            case 3: return W(c->GetEffectiveConfig().unhandled_options);
            case 4: return W(c->GetEffectiveConfig().nameservers.size());
            case 5: return W(c->GetEffectiveConfig().dns_over_tls_active);
            case 6: return W(c->can_query_additional_types_via_insecure_);
            case 7: return W(features::IsDnsPlatformSupported());
            case 9: DCHECK(c->session_); CHECK(resolve_context); return W(resolve_context->NumAvailableDohServers(c->session_.get()));
            case 10: return W(c->insecure_fallback_failures_);
            case 12: return W(config->doh_config.servers().size());
            case 13: return W(config->unhandled_options);
            case 14: return W(config->allow_dns_over_https_upgrade);
            case 15: return W(static_cast<int>(config->secure_dns_mode));
            case 16: return W(config->dns_over_tls_hostname.empty());
            case 17: return W(config->fallback_doh_nameservers.size());
            case 18: return W(!!c->net_log_);
            case 19: return W(config->should_perform_doh_fallback_upgrade);
            case 20: return W(!!c->session_);
          }
          NOTREACHED();
        case ClientOp::kWrite:
          switch (in.x) {
            case 1: Mutable().insecure_dns_mode_ = static_cast<InsecureDnsMode>(in.y); break;
            case 2: Mutable().can_query_additional_types_via_insecure_ = in.y; break;
            case 3: Mutable().insecure_fallback_failures_ = static_cast<int>(in.y); break;
            case 4: config->should_perform_doh_fallback_upgrade = in.y; break;
            case 5: preset.emplace(); break;
            default: NOTREACHED();
          }
          return {};
        case ClientOp::kCheck: DCHECK(in.x); return {};
        case ClientOp::kSystemSame: return W(input_system == c->system_config_);
        case ClientOp::kStoreSystem: Mutable().system_config_ = std::move(input_system); return {};
        case ClientOp::kOverridesSame: return W(*input_overrides == c->config_overrides_);
        case ClientOp::kStoreOverrides: Mutable().config_overrides_ = std::move(*input_overrides); return {};
        case ClientOp::kHasOverridesAll: return W(c->config_overrides_.OverridesEverything());
        case ClientOp::kApplyOverrides:
          switch (in.x) {
            case 0: config = c->config_overrides_.ApplyOverrides(DnsConfig()); break;
            case 1: config = c->config_overrides_.ApplyOverrides(c->system_config_.value_or(DnsConfig())); break;
            case 2: config = c->session_->config(); break;
            default: NOTREACHED();
          }
          return {};
        case ClientOp::kClearNameservers: config->nameservers.clear(); return {};
        case ClientOp::kUpgradeDot:
          config->doh_config = DnsOverHttpsConfig(GetDohUpgradeServersFromDotHostname(config->dns_over_tls_hostname)); return {};
        case ClientOp::kUpgradeNameservers:
          switch (in.x) {
            case 0: config->doh_config = DnsOverHttpsConfig(GetDohUpgradeServersFromNameservers(config->nameservers)); break;
            case 1: config->doh_config = DnsOverHttpsConfig(GetDohUpgradeServersFromNameservers(config->fallback_doh_nameservers)); break;
            default: NOTREACHED();
          }
          return {};
        case ClientOp::kNameserverCount: return W(0, config->nameservers.size());
        case ClientOp::kNameserverPublic: return W(config->nameservers.at(in.a).address().IsPubliclyRoutable());
        case ClientOp::kNameserverLoopback: return W(config->nameservers.at(in.a).address().IsLoopback());
        case ClientOp::kFeature:
          switch (in.x) {
            case 0: return W(base::FeatureList::IsEnabled(features::kDohFallbackAllowedWithLocalNameservers));
            case 1: return W(base::FeatureList::IsEnabled(features::kAddAutomaticWithDohFallbackMode));
            default: NOTREACHED();
          }
        case ClientOp::kMetric:
          switch (in.x) {
            case 0: UMA_HISTOGRAM_BOOLEAN("Net.DNS.UpgradeConfig.DotUpgradeSucceeded", in.y); break;
            case 1: UMA_HISTOGRAM_BOOLEAN("Net.DNS.UpgradeConfig.HasPublicInsecureNameserver", in.y); break;
            case 2: UMA_HISTOGRAM_ENUMERATION("Net.DNS.UpgradeConfigFailed.LocalNameserverState", static_cast<DnsConfigLocalNameserverState>(in.y)); break;
            case 3: UMA_HISTOGRAM_BOOLEAN("Net.DNS.UpgradeConfig.InsecureUpgradeWithFallbackSucceeded", in.y); break;
            case 4: UMA_HISTOGRAM_BOOLEAN("Net.DNS.UpgradeConfig.InsecureUpgradeSucceeded", in.y); break;
            case 5: UMA_HISTOGRAM_BOOLEAN("Net.DNS.UpgradeConfig.Ineligible.DohSpecified", in.y); break;
            case 6: UMA_HISTOGRAM_BOOLEAN("Net.DNS.UpgradeConfig.Ineligible.UnhandledOptions", in.y); break;
            case 7: base::UmaHistogramEnumeration("Net.DNS.FallbackFromSecureTransactionPreferred", static_cast<FallbackFromSecureTransactionPreferredReason>(in.y)); break;
            default: NOTREACHED();
          }
          return {};
        case ClientOp::kClearFactory: Mutable().factory_.reset(); return {};
        case ClientOp::kNewSession:
          Mutable().session_ = base::MakeRefCounted<DnsSession>(std::move(*config), c->rand_int_callback_, c->net_log_); return {};
        case ClientOp::kNewFactory: Mutable().factory_ = DnsTransactionFactory::CreateFactory(c->session_.get()); return {};
        case ClientOp::kConfigSame: return W(c->session_->config() == *config);
        case ClientOp::kLogChanged:
          c->net_log_->AddGlobalEntry(NetLogEventType::DNS_CONFIG_CHANGED, [owner = &c.get()] { return owner->GetDnsConfigAsValueForNetLog(); }); return {};
        case ClientOp::kPresetCount: return W(0, c->GetEffectiveConfig().doh_config.servers().size());
        case ClientOp::kPresetMatches: {
          std::string uri; const bool valid = uri_template::Expand(Server(in.a).server_template(), {}, &uri);
          DCHECK(valid); return W(url::SchemeHostPort(GURL(uri)) == *endpoint);
        }
        case ClientOp::kPresetGroupCount: return W(0, Server(in.a).endpoints().size());
        case ClientOp::kPresetAddressCount: return W(0, Server(in.a).endpoints().at(in.b).size());
        case ClientOp::kPresetAppend:
          preset->emplace_back(Server(in.a).endpoints().at(in.b).at(in.x), endpoint->port()); return {};
        case ClientOp::kPrepareLog: log = c->GetEffectiveConfig().ToDict(); return {};
        case ClientOp::kSetLogBool:
          switch (in.x) {
            case 0: log.Set("can_use_secure_dns_transactions", !!in.y); break;
            case 1: log.Set("can_use_insecure_dns_transactions", !!in.y); break;
            default: NOTREACHED();
          }
          return {};
      }
      NOTREACHED();
    }
    raw_ref<const DnsClientImpl> c;
    raw_ptr<DnsClientImpl> m = nullptr;
  };
  static int Ech(std::optional<EchMode> ech) { return ech ? static_cast<int>(*ech) : -1; }
  DnsResolverWords Run(ClientEvent event, DnsResolverWords input = {}) const {
    Frame f(*this); return f.Run(event, input);
  }
  DnsConfig BuildEffectiveConfig() const {
    Frame f(*this); f.Run(ClientEvent::kBuild); return std::move(*f.config);
  }
  void UpdateSession(DnsConfig config) {
    Frame f(*this); f.config = std::move(config); f.Run(ClientEvent::kSession);
  }
  InsecureDnsMode insecure_dns_mode_ = InsecureDnsMode::kDisabled;
  bool can_query_additional_types_via_insecure_ = false;
  int insecure_fallback_failures_ = 0;
  std::optional<DnsConfig> system_config_;
  DnsConfigOverrides config_overrides_;
  scoped_refptr<DnsSession> session_;
  std::unique_ptr<DnsTransactionFactory> factory_;
  std::unique_ptr<AddressSorter> address_sorter_ = AddressSorter::CreateAddressSorter();
  raw_ptr<NetLog> net_log_;
  const RandIntCallback rand_int_callback_;
};

}  // namespace
std::unique_ptr<DnsClientRustServices> DnsClient::MakeRustPolicyServices(ResolveContext*) const { return nullptr; }

bool DnsClient::UseDnsPlatformDueToEchMode(std::optional<EchMode> ech) {
  auto provider = [](void*, uint32_t op, DnsResolverWords input) -> DnsResolverWords {
    CHECK_EQ(op, static_cast<uint32_t>(dns_request_family::ClientOp::kRead));
    CHECK_EQ(input.x, 7);
    return dns_request_family::W(features::IsDnsPlatformSupported());
  };
  return rustium_dns_request_family_run(
      3, static_cast<uint32_t>(dns_request_family::ClientEvent::kUsePlatform),
      dns_request_family::W(ech ? static_cast<int>(*ech) : -1), nullptr, provider).x;
}

// static
std::unique_ptr<DnsClient> DnsClient::CreateClient(NetLog* net_log) {
  return std::make_unique<DnsClientImpl>(
      net_log, base::BindRepeating(&base::RandIntInclusive));
}

// static
std::unique_ptr<DnsClient> DnsClient::CreateClientForTesting(
    NetLog* net_log,
    const RandIntCallback& rand_int_callback) {
  return std::make_unique<DnsClientImpl>(net_log, rand_int_callback);
}

}  // namespace net
