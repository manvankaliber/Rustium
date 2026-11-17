// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/host_resolver_manager.h"
#include "net/dns/dns_resolution_policy_ffi.h"
#include "net/dns/dns_client_rust_services.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/circular_deque.h"
#include "base/containers/flat_set.h"
#include "base/containers/linked_list.h"
#include "base/debug/debugger.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/safe_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/no_destructor.h"
#include "base/notimplemented.h"
#include "base/numerics/safe_conversions.h"
#include "base/observer_list.h"
#include "base/rand_util.h"
#include "base/sequence_checker.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/types/optional_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "net/base/address_family.h"
#include "net/base/address_list.h"
#include "net/base/completion_once_callback.h"
#include "net/base/ech_mode.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_change_notifier.h"
#include "net/base/network_interfaces.h"
#include "net/base/prioritized_dispatcher.h"
#include "net/base/request_priority.h"
#include "net/base/trace_constants.h"
#include "net/base/url_util.h"
#include "net/dns/dns_alias_utility.h"
#include "net/dns/dns_client.h"
#include "net/dns/dns_names_util.h"
#include "net/dns/dns_response.h"
#include "net/dns/dns_response_result_extractor.h"
#include "net/dns/dns_transaction.h"
#include "net/dns/dns_util.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver_dns_task.h"
#include "net/dns/host_resolver_internal_result.h"
#include "net/dns/host_resolver_manager_job.h"
#include "net/dns/host_resolver_manager_request_impl.h"
#include "net/dns/host_resolver_manager_service_endpoint_request_impl.h"
#include "net/dns/host_resolver_mdns_listener_impl.h"
#include "net/dns/host_resolver_mdns_task.h"
#include "net/dns/host_resolver_nat64_task.h"
#include "net/dns/host_resolver_proc.h"
#include "net/dns/host_resolver_system_task.h"
#include "net/dns/httpssvc_metrics.h"
#include "net/dns/loopback_only.h"
#include "net/dns/mdns_client.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/public/dns_query_type.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/dns/public/resolve_error_info.h"
#include "net/dns/public/secure_dns_mode.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/dns/public/util.h"
#include "net/dns/record_parsed.h"
#include "net/dns/resolve_context.h"
#include "net/dns/test_dns_config_service.h"
#include "net/http/http_network_session.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/url_request/url_request_context.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_MDNS)
#include "net/dns/mdns_client_impl.h"
#endif

#if BUILDFLAG(IS_WIN)
#include <Winsock2.h>

#include "net/base/winsock_init.h"
#endif

#if BUILDFLAG(IS_POSIX)
#include <net/if.h>

#include "net/base/sys_addrinfo.h"
#include <ifaddrs.h>
#endif


namespace net {

namespace {

// Limit the size of hostnames that will be resolved to combat issues in
// some platform's resolvers.
const size_t kMaxHostLength = 4096;

// Time between IPv6 probes, i.e. for how long results of each IPv6 probe are
// cached.
const int kIPv6ProbePeriodMs = 1000;

// Google DNS address used for IPv6 probes.
const uint8_t kIPv6ProbeAddress[] = {0x20, 0x01, 0x48, 0x60, 0x48, 0x60,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x88, 0x88};

// True if |hostname| ends with either ".local" or ".local.".
bool ResemblesMulticastDNSName(std::string_view hostname) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bytes(hostname); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 46, native_service);
  return rust_reply.a!=0;
}

bool ConfigureAsyncDnsNoFallbackFieldTrial() {
  using RWords = DnsResolutionWords;

std::string group;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { group=base::FieldTrialList::FindFullName("AsyncDns");
        return {}; }
      case 2: { return RWords::Bytes(group); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 47, native_service);
  return rust_reply.a!=0;
}

base::DictValue NetLogIPv6AvailableParams(bool ipv6_available, bool cached) {
  base::DictValue dict;
  dict.Set("ipv6_available", ipv6_available);
  dict.Set("cached", cached);
  return dict;
}

// Maximum of 64 concurrent resolver calls (excluding retries).
// Between 2010 and 2020, the limit was set to 6 because of a report of a broken
// home router that would fail in the presence of more simultaneous queries.
// In 2020, we conducted an experiment to see if this kind of router was still
// present on the Internet, and found no evidence of any remaining issues, so
// we increased the limit to 64 at that time.
const size_t kDefaultMaxSystemTasks = 64u;

PrioritizedDispatcher::Limits GetDispatcherLimits(
    const HostResolver::ManagerOptions& options) {
  using RWords = DnsResolutionWords;

std::optional<PrioritizedDispatcher::Limits> limits;std::string group;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { limits.emplace(NUM_PRIORITIES,options.max_concurrent_resolves);
        return {}; }
      case 2: { return RWords::Unsigned(static_cast<uint64_t>(limits->total_jobs)); }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(HostResolver::ManagerOptions::kDefaultParallelism)); }
      case 4: { limits->total_jobs=args.a;
        return {}; }
      case 5: { group=base::FieldTrialList::FindFullName("HostResolverDispatch");
        return {}; }
      case 6: { return RWords::Bytes(group); }
      case 7: { return RWords::Unsigned(static_cast<uint64_t>(NUM_PRIORITIES)); }
      case 8: { NOTREACHED(); }
      case 9: { size_t value=0;bool ok=base::StringToSizeT(std::string_view(group).substr(args.a,args.b),&value);return {.a=value,.b=ok}; }
      case 10: { return RWords::Unsigned(static_cast<uint64_t>(MINIMUM_PRIORITY)); }
      case 11: { limits->reserved_slots.clear();
        return {}; }
      case 12: { limits->reserved_slots.push_back(args.a);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 54, native_service);
  return std::move(limits).value();
}

base::DictValue NetLogResults(const HostCache::Entry& results) {
  base::DictValue dict;
  dict.Set("results", results.NetLogParams());
  return dict;
}

base::DictValue NetLogResults(
    const std::set<std::unique_ptr<HostResolverInternalResult>>& results) {
  using RWords = DnsResolutionWords;

std::optional<base::ListValue> list;base::DictValue dict;decltype(results.begin()) it;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { list.emplace(base::ListValue::with_capacity(results.size()));
        return {}; }
      case 2: { it=results.begin();
        return {}; }
      case 3: { return RWords::Bool(it!=results.end()); }
      case 4: { list->Append((*it)->ToValue());
        return {}; }
      case 5: { ++it;
        return {}; }
      case 6: { dict.Set("results",std::move(*list));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 49, native_service);
  return dict;
}

std::vector<IPEndPoint> FilterAddresses(std::vector<IPEndPoint> addresses,
                                        DnsQueryTypeSet query_types) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!query_types.Has(DnsQueryType::UNSPECIFIED));DCHECK(!query_types.empty());
        return {}; }
      case 2: { DCHECK(HasAddressType(query_types));return {.a=query_types.Has(DnsQueryType::A),.b=query_types.Has(DnsQueryType::AAAA)}; }
      case 3: { return RWords::Signed(static_cast<int64_t>(ADDRESS_FAMILY_UNSPECIFIED)); }
      case 4: { return RWords::Unsigned(static_cast<uint64_t>(addresses.size())); }
      case 5: { return RWords::Signed(static_cast<int64_t>(addresses.at(args.a).GetFamily())); }
      case 6: { addresses.at(args.a)=std::move(addresses.at(args.b));
        return {}; }
      case 7: { addresses.erase(addresses.begin()+base::checked_cast<ptrdiff_t>(args.a),addresses.end());
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 50, native_service);
  return addresses;
}

int GetPortForGloballyReachableCheck() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(base::FeatureList::IsEnabled(features::kUseAlternativePortForGloballyReachableCheck)); }
      case 2: { return RWords::Signed(static_cast<int64_t>(features::kAlternativePortForGloballyReachableCheck.Get())); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 43, native_service);
  return static_cast<int>(rust_reply.x);
}

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
//
// LINT.IfChange(DnsClientCapability)
enum class DnsClientCapability {
  kSecureDisabledInsecureDisabled = 0,
  kSecureDisabledInsecureEnabled = 1,
  kSecureEnabledInsecureDisabled = 2,
  kSecureEnabledInsecureEnabled = 3,
  kMaxValue = kSecureEnabledInsecureEnabled,
};
// LINT.ThenChange(/tools/metrics/histograms/metadata/net/enums.xml:DnsClientCapability)

void RecordDnsClientCapabilityMetrics(const DnsClient* dns_client) {
  using RWords = DnsResolutionWords;


std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(dns_client!=nullptr); }
      case 2: { return RWords::Bool(dns_client->CanUseSecureDnsTransactions()); }
      case 3: { return RWords::Bool(dns_client->CanUseInsecureDnsTransactions(std::nullopt)); }
      case 4: { base::UmaHistogramEnumeration("Net.DNS.DnsConfig.DnsClientCapability",static_cast<DnsClientCapability>(args.a));
        return {}; }
      case 9000: { if(client_frame_depth==0){const auto* owner=(dns_client);if(owner)client_frame=owner->MakeRustPolicyServices(nullptr);}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 44, native_service);
  (void)rust_reply;
}
}  // namespace

//-----------------------------------------------------------------------------

bool ResolveLocalHostname(std::string_view host,
                          std::vector<IPEndPoint>* address_list) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { address_list->clear();
        return {}; }
      case 2: { return RWords::Bool(IsLocalHostname(host)); }
      case 3: { address_list->emplace_back(IPAddress::IPv6Localhost(),0);
        return {}; }
      case 4: { address_list->emplace_back(IPAddress::IPv4Localhost(),0);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 45, native_service);
  return rust_reply.a!=0;
}

class HostResolverManager::ProbeRequestImpl
    : public HostResolver::ProbeRequest,
      public ResolveContext::DohStatusObserver {
 public:
  ProbeRequestImpl(base::WeakPtr<ResolveContext> context,
                   base::WeakPtr<HostResolverManager> resolver)
      : context_(std::move(context)), resolver_(std::move(resolver)) {}

  ProbeRequestImpl(const ProbeRequestImpl&) = delete;
  ProbeRequestImpl& operator=(const ProbeRequestImpl&) = delete;

  ~ProbeRequestImpl() override {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(context_.get()!=nullptr); }
      case 2: { context_->UnregisterDohStatusObserver(this);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 56, native_service);
  (void)rust_reply;
}

  int Start() override {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(resolver_);DCHECK(!runner_);
        return {}; }
      case 2: { return RWords::Bool(context_.get()!=nullptr); }
      case 3: { return RWords::Signed(static_cast<int64_t>(ERR_CONTEXT_SHUT_DOWN)); }
      case 4: { context_->RegisterDohStatusObserver(this);
        return {}; }
      case 5: { StartRunner(false);
        return {}; }
      case 6: { return RWords::Signed(static_cast<int64_t>(ERR_IO_PENDING)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 51, native_service);
  return static_cast<int>(rust_reply.x);
}

  // ResolveContext::DohStatusObserver
  void OnSessionChanged() override { CancelRunner(); }

  void OnDohServerUnavailable(bool network_change) override {
    // Start the runner asynchronously, as this may trigger reentrant calls into
    // HostResolverManager, which are not allowed during notification handling.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&ProbeRequestImpl::StartRunner,
                       weak_ptr_factory_.GetWeakPtr(), network_change));
  }

 private:
  void StartRunner(bool network_change) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(resolver_);DCHECK(!resolver_->invalidation_in_progress_);
        return {}; }
      case 2: { return RWords::Bool(context_.get()!=nullptr); }
      case 3: { return RWords::Bool(runner_!=nullptr); }
      case 4: { runner_=resolver_->CreateDohProbeRunner(context_.get());
        return {}; }
      case 5: { runner_->Start(network_change);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 52, native_service);
  (void)rust_reply;
}

  void CancelRunner() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(runner_!=nullptr); }
      case 2: { base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(FROM_HERE,std::move(runner_));
        return {}; }
      case 3: { weak_ptr_factory_.InvalidateWeakPtrs();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 53, native_service);
  (void)rust_reply;
}

  base::WeakPtr<ResolveContext> context_;

  std::unique_ptr<DnsProbeRunner> runner_;
  base::WeakPtr<HostResolverManager> resolver_;

  base::WeakPtrFactory<ProbeRequestImpl> weak_ptr_factory_{this};
};

//-----------------------------------------------------------------------------

HostResolverManager::HostResolverManager(
    const HostResolver::ManagerOptions& options,
    SystemDnsConfigChangeNotifier* system_dns_config_notifier,
    NetLog* net_log)
    : HostResolverManager(PassKey(),
                          options,
                          system_dns_config_notifier,
                          handles::kInvalidNetworkHandle,
                          net_log) {}

HostResolverManager::HostResolverManager(
    base::PassKey<HostResolverManager>,
    const HostResolver::ManagerOptions& options,
    SystemDnsConfigChangeNotifier* system_dns_config_notifier,
    handles::NetworkHandle target_network,
    NetLog* net_log)
    : host_resolver_system_params_(nullptr, options.max_system_retry_attempts),
      net_log_(net_log),
      system_dns_config_notifier_(system_dns_config_notifier),
      target_network_(target_network),
      check_ipv6_on_wifi_(options.check_ipv6_on_wifi),
      ipv6_reachability_override_(base::FeatureList::IsEnabled(
          features::kEnableIPv6ReachabilityOverride)),
      is_happy_eyeballs_v3_enabled_(
          base::FeatureList::IsEnabled(features::kHappyEyeballsV3)),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      https_svcb_options_(
          options.https_svcb_options
              ? *options.https_svcb_options
              : HostResolver::HttpsSvcbOptions::FromFeatures()) {
  using RWords = DnsResolutionWords;

std::optional<PrioritizedDispatcher::Limits> limits;NetworkChangeNotifier::ConnectionType connection_type=NetworkChangeNotifier::CONNECTION_UNKNOWN;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { limits.emplace(GetDispatcherLimits(options));
        return {}; }
      case 2: { dispatcher_=std::make_unique<PrioritizedDispatcher>(*limits);
        return {}; }
      case 3: { max_queued_jobs_=limits->total_jobs*100u;
        return {}; }
      case 4: { DCHECK_GE(dispatcher_->num_priorities(),static_cast<size_t>(NUM_PRIORITIES));
        return {}; }
      case 5: {
#if BUILDFLAG(IS_WIN)
EnsureWinsockInit();
#endif
#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_APPLE)
RunLoopbackProbeJob();
#endif
        return {}; }
      case 6: { return RWords::Bool(IsBoundToNetwork()); }
      case 7: { NetworkChangeNotifier::AddIPAddressObserver(this);
        return {}; }
      case 8: { NetworkChangeNotifier::AddConnectionTypeObserver(this);
        return {}; }
      case 9: { return RWords::Bool(system_dns_config_notifier_!=nullptr); }
      case 10: { system_dns_config_notifier_->AddObserver(this);
        return {}; }
      case 11: { EnsureSystemHostResolverCallReady();
        return {}; }
      case 12: { connection_type=NetworkChangeNotifier::GetNetworkConnectionType(target_network);
        return {}; }
      case 13: { connection_type=NetworkChangeNotifier::GetConnectionType();
        return {}; }
      case 14: { UpdateConnectionType(connection_type);
        return {}; }
      case 15: {
#if defined(ENABLE_BUILT_IN_DNS)
CHECK((options.insecure_dns_mode!=InsecureDnsMode::kEnabledPlatform&&options.insecure_dns_mode!=InsecureDnsMode::kEnabledPlatformNoSystem)||features::IsDnsPlatformSupported());dns_client_=DnsClient::CreateClient(net_log_);dns_client_->SetInsecureEnabled(options.insecure_dns_mode,options.additional_types_via_insecure_dns_enabled);dns_client_->SetConfigOverrides(options.dns_config_overrides);
#else
DCHECK(options.dns_config_overrides==DnsConfigOverrides());
#endif
        return {}; }
      case 16: { allow_fallback_to_systemtask_=!ConfigureAsyncDnsNoFallbackFieldTrial();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 55, native_service);
  (void)rust_reply;
}

HostResolverManager::~HostResolverManager() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
        return {}; }
      case 2: { dispatcher_->SetLimitsToZero();
        return {}; }
      case 3: { jobs_.clear();
        return {}; }
      case 4: { return RWords::Signed(static_cast<int64_t>(target_network_)); }
      case 5: { NetworkChangeNotifier::RemoveIPAddressObserver(this);
        return {}; }
      case 6: { NetworkChangeNotifier::RemoveConnectionTypeObserver(this);
        return {}; }
      case 7: { return RWords::Bool(system_dns_config_notifier_!=nullptr); }
      case 8: { system_dns_config_notifier_->RemoveObserver(this);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 37, native_service);
  (void)rust_reply;
}

// static
std::unique_ptr<HostResolverManager>
HostResolverManager::CreateNetworkBoundHostResolverManager(
    const HostResolver::ManagerOptions& options,
    handles::NetworkHandle target_network,
    NetLog* net_log) {
  NOTIMPLEMENTED();
  return nullptr;
}

std::unique_ptr<HostResolver::ResolveHostRequest>
HostResolverManager::CreateRequest(
    std::variant<url::SchemeHostPort, HostPortPair> host,
    NetworkAnonymizationKey network_anonymization_key,
    handles::NetworkHandle target_network,
    NetLogWithSource net_log,
    std::optional<ResolveHostParameters> optional_parameters,
    ResolveContext* resolve_context) {
  return CreateRequest(HostResolver::Host(std::move(host)),
                       std::move(network_anonymization_key), target_network,
                       std::move(net_log), std::move(optional_parameters),
                       resolve_context);
}

std::unique_ptr<HostResolver::ResolveHostRequest>
HostResolverManager::CreateRequest(
    HostResolver::Host host,
    NetworkAnonymizationKey network_anonymization_key,
    handles::NetworkHandle target_network,
    NetLogWithSource net_log,
    std::optional<ResolveHostParameters> optional_parameters,
    ResolveContext* resolve_context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!invalidation_in_progress_);

  DCHECK_EQ(resolve_context->GetTargetNetwork(), target_network_);
  // ResolveContexts must register (via RegisterResolveContext()) before use to
  // ensure cached data is invalidated on network and configuration changes.
  DCHECK(registered_contexts_.HasObserver(resolve_context));
  // Multi-network support for Cronet and CCT was originally implemented by
  // creating multiple URLRequestContexts/HostResolverManagers. Until this
  // historical artifact is removed, make sure these two mechanisms are not used
  // at the same time.
  // TODO(crbug.com/495684670): Clean this up once multi-network Cronet and CCT
  // no longer depend on network-bound URLRequestContexts.
  CHECK(target_network_ == handles::kInvalidNetworkHandle ||
        target_network == handles::kInvalidNetworkHandle);

  return std::make_unique<RequestImpl>(
      std::move(net_log), std::move(host), std::move(network_anonymization_key),
      target_network, std::move(optional_parameters),
      resolve_context->GetWeakPtr(), weak_ptr_factory_.GetWeakPtr(),
      tick_clock_);
}

std::unique_ptr<HostResolver::ProbeRequest>
HostResolverManager::CreateDohProbeRequest(ResolveContext* context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  return std::make_unique<ProbeRequestImpl>(context->GetWeakPtr(),
                                            weak_ptr_factory_.GetWeakPtr());
}

std::unique_ptr<HostResolver::MdnsListener>
HostResolverManager::CreateMdnsListener(const HostPortPair& host,
                                        DnsQueryType query_type) {
  using RWords = DnsResolutionWords;

std::unique_ptr<HostResolverMdnsListenerImpl> listener;MDnsClient* client=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);DCHECK_NE(DnsQueryType::UNSPECIFIED,query_type);
        return {}; }
      case 2: { listener=std::make_unique<HostResolverMdnsListenerImpl>(host,query_type);
        return {}; }
      case 3: { return RWords::Signed(static_cast<int64_t>(GetOrCreateMdnsClient(&client))); }
      case 4: { listener->set_inner_listener(client->CreateListener(DnsQueryTypeToQtype(query_type),host.host(),listener.get()));
        return {}; }
      case 5: { listener->set_initialization_error(static_cast<int>(args.x));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 40, native_service);
  return listener;
}

std::unique_ptr<HostResolver::ServiceEndpointRequest>
HostResolverManager::CreateServiceEndpointRequest(
    HostResolver::Host host,
    NetworkAnonymizationKey network_anonymization_key,
    handles::NetworkHandle target_network,
    NetLogWithSource net_log,
    ResolveHostParameters parameters,
    ResolveContext* resolve_context) {
  using RWords = DnsResolutionWords;

base::WeakPtr<ResolveContext> context;std::unique_ptr<ServiceEndpointRequestImpl> result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);DCHECK(!invalidation_in_progress_);DCHECK_EQ(resolve_context->GetTargetNetwork(),target_network_);
        return {}; }
      case 2: { return RWords::Bool(resolve_context!=nullptr); }
      case 3: { DCHECK(registered_contexts_.HasObserver(resolve_context));
        return {}; }
      case 4: { context=resolve_context->GetWeakPtr();
        return {}; }
      case 5: { CHECK(target_network_==handles::kInvalidNetworkHandle || target_network==handles::kInvalidNetworkHandle);
        return {}; }
      case 6: { result=std::make_unique<ServiceEndpointRequestImpl>(std::move(host),std::move(network_anonymization_key),target_network,std::move(net_log),std::move(parameters),context,weak_ptr_factory_.GetWeakPtr(),tick_clock_);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 57, native_service);
  return result;
}

void HostResolverManager::SetInsecureDnsClientEnabled(
    InsecureDnsMode mode,
    bool additional_dns_types_enabled) {
  using RWords = DnsResolutionWords;


std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_); CHECK((mode != InsecureDnsMode::kEnabledPlatform && mode != InsecureDnsMode::kEnabledPlatformNoSystem) || features::IsDnsPlatformSupported());
        return {}; }
      case 2: { return RWords::Bool(dns_client_ != nullptr); }
      case 3: { return RWords::Bool(dns_client_->CanUseInsecureDnsTransactions(std::nullopt)); }
      case 4: { return RWords::Bool(dns_client_->CanQueryAdditionalTypesViaInsecureDns(std::nullopt)); }
      case 5: { dns_client_->SetInsecureEnabled(mode, additional_dns_types_enabled);
        return {}; }
      case 6: { AbortInsecureDnsTasks(ERR_NETWORK_CHANGED, false);
        return {}; }
      case 9000: { if(client_frame_depth==0){const auto* owner=(dns_client_.get());if(owner)client_frame=owner->MakeRustPolicyServices(nullptr);}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 5, native_service);
  (void)rust_reply;
}

base::DictValue HostResolverManager::GetDnsConfigAsValue() const {
  using RWords = DnsResolutionWords;

base::DictValue result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
        return {}; }
      case 2: { return RWords::Bool(dns_client_!=nullptr); }
      case 3: { result=dns_client_->GetDnsConfigAsValueForNetLog();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 38, native_service);
  return result;
}

void HostResolverManager::SetDnsConfigOverrides(DnsConfigOverrides overrides) {
  using RWords = DnsResolutionWords;


std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
        return {}; }
      case 2: { return RWords::Bool(dns_client_ != nullptr); }
      case 3: { return RWords::Bool(overrides == DnsConfigOverrides()); }
      case 4: { DCHECK(dns_client_);
        return {}; }
      case 5: { return RWords::Bool(dns_client_->CanUseSecureDnsTransactions()); }
      case 6: { return RWords::Bool(dns_client_->CanUseInsecureDnsTransactions(std::nullopt)); }
      case 7: { return RWords::Bool(dns_client_->SetConfigOverrides(std::move(overrides))); }
      case 8: { NetworkChangeNotifier::TriggerNonSystemDnsChange();
        return {}; }
      case 9: { InvalidateCaches();
        return {}; }
      case 10: { UpdateJobsForChangedConfig();
        return {}; }
      case 9000: { if(client_frame_depth==0){const auto* owner=(dns_client_.get());if(owner)client_frame=owner->MakeRustPolicyServices(nullptr);}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 6, native_service);
  (void)rust_reply;
}

void HostResolverManager::RegisterResolveContext(ResolveContext* context) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { registered_contexts_.AddObserver(context);
        return {}; }
      case 2: { return RWords::Bool(dns_client_!=nullptr); }
      case 3: { context->InvalidateCachesAndPerSessionData(dns_client_->GetCurrentSession(),false);
        return {}; }
      case 4: { context->InvalidateCachesAndPerSessionData(nullptr,false);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 39, native_service);
  (void)rust_reply;
}

void HostResolverManager::DeregisterResolveContext(
    const ResolveContext* context) {
  registered_contexts_.RemoveObserver(context);

  // Destroy Jobs when their context is closed.
  RemoveAllJobs(context);
}

void HostResolverManager::SetTickClockForTesting(
    const base::TickClock* tick_clock) {
  tick_clock_ = tick_clock;
}

void HostResolverManager::SetIPv6ReachabilityOverride(
    bool reachability_override) {
  ipv6_reachability_override_ = reachability_override;
}

void HostResolverManager::SetIsHappyEyeballsV3Enabled(bool enabled) {
  is_happy_eyeballs_v3_enabled_ = enabled;
}

bool HostResolverManager::IsHappyEyeballsV3Enabled() const {
  return is_happy_eyeballs_v3_enabled_;
}

void HostResolverManager::SetMaxQueuedJobsForTesting(size_t value) {
  DCHECK_EQ(0u, dispatcher_->num_queued_jobs());
  DCHECK_GE(value, 0u);
  max_queued_jobs_ = value;
}

void HostResolverManager::SetHaveOnlyLoopbackAddresses(bool result) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(additional_resolver_flags_)); }
      case 2: { return RWords::Unsigned(static_cast<uint64_t>(HOST_RESOLVER_LOOPBACK_ONLY)); }
      case 3: { return RWords::Bool(result); }
      case 4: { additional_resolver_flags_ = static_cast<HostResolverFlags>(args.a);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 4, native_service);
  (void)rust_reply;
}

void HostResolverManager::SetMdnsSocketFactoryForTesting(
    std::unique_ptr<MDnsSocketFactory> socket_factory) {
  DCHECK(!mdns_client_);
  mdns_socket_factory_ = std::move(socket_factory);
}

void HostResolverManager::SetMdnsClientForTesting(
    std::unique_ptr<MDnsClient> client) {
  mdns_client_ = std::move(client);
}

void HostResolverManager::SetDnsClientForTesting(
    std::unique_ptr<DnsClient> dns_client) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(dns_client);
        return {}; }
      case 2: { return RWords::Bool(dns_client_ != nullptr); }
      case 3: { return RWords::Bool(dns_client->GetSystemConfigForTesting().has_value()); }
      case 4: { dns_client->SetSystemConfig(dns_client_->GetSystemConfigForTesting());
        return {}; }
      case 5: { dns_client->SetConfigOverrides(dns_client_->GetConfigOverridesForTesting());
        return {}; }
      case 6: { dns_client_ = std::move(dns_client);
        return {}; }
      case 7: { InvalidateCaches();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 10, native_service);
  (void)rust_reply;
}

void HostResolverManager::SetLastIPv6ProbeResultForTesting(
    bool last_ipv6_probe_result) {
  SetLastIPv6ProbeResult(last_ipv6_probe_result);
}

// static
bool HostResolverManager::IsLocalTask(TaskType task) {
  using RWords = DnsResolutionWords;
static_assert(static_cast<int>(TaskType::SYSTEM)==0);
static_assert(static_cast<int>(TaskType::DNS)==1);
static_assert(static_cast<int>(TaskType::SECURE_DNS)==2);
static_assert(static_cast<int>(TaskType::MDNS)==3);
static_assert(static_cast<int>(TaskType::CACHE_LOOKUP)==4);
static_assert(static_cast<int>(TaskType::INSECURE_CACHE_LOOKUP)==5);
static_assert(static_cast<int>(TaskType::SECURE_CACHE_LOOKUP)==6);
static_assert(static_cast<int>(TaskType::CONFIG_PRESET)==7);
static_assert(static_cast<int>(TaskType::NAT64)==8);
static_assert(static_cast<int>(TaskType::HOSTS)==9);
static_assert(static_cast<int>(TaskType::DNS_PLATFORM)==10);
static_assert(static_cast<int>(SecureDnsMode::kOff)==0 && static_cast<int>(SecureDnsMode::kAutomatic)==1 && static_cast<int>(SecureDnsMode::kSecure)==2);
static_assert(static_cast<int>(InsecureDnsMode::kDisabled)==0 && static_cast<int>(InsecureDnsMode::kEnabledBuiltIn)==1 && static_cast<int>(InsecureDnsMode::kEnabledPlatform)==2 && static_cast<int>(InsecureDnsMode::kEnabledPlatformNoSystem)==3);
static_assert(static_cast<int>(SecureDnsPolicy::kAllow)==0 && static_cast<int>(SecureDnsPolicy::kDisable)==1 && static_cast<int>(SecureDnsPolicy::kBootstrap)==2);
static_assert(static_cast<int>(ResolveHostParameters::CacheUsage::ALLOWED)==0 && static_cast<int>(ResolveHostParameters::CacheUsage::STALE_ALLOWED)==1 && static_cast<int>(ResolveHostParameters::CacheUsage::STALE_ALLOWED_WHILE_REFRESHING)==2 && static_cast<int>(ResolveHostParameters::CacheUsage::DISALLOWED)==3);
static_assert(static_cast<int>(DnsQueryType::UNSPECIFIED)==0 && static_cast<int>(EchMode::kDisabled)==0 && static_cast<int>(EchMode::kOpportunistic)==1 && static_cast<int>(EchMode::kStrict)==2);
static_assert(kMaxHostLength==4096 && kIPv6ProbePeriodMs==1000 && kDefaultMaxSystemTasks==64);
static_assert(ADDRESS_FAMILY_UNSPECIFIED==0 && ADDRESS_FAMILY_IPV4==1 && ADDRESS_FAMILY_IPV6==2);


  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(task)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 1, native_service);
  return rust_reply.a != 0;
}

void HostResolverManager::InitializeJobKeyAndIPAddress(
    const NetworkAnonymizationKey& network_anonymization_key,
    const ResolveHostParameters& parameters,
    const NetLogWithSource& source_net_log,
    JobKey& out_job_key,
    IPAddress& out_ip_address) {
  using RWords = DnsResolutionWords;

DnsQueryTypeSet effective_types;const DnsConfig* config=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { out_job_key.network_anonymization_key=network_anonymization_key; out_job_key.source=parameters.source;
        return {}; }
      case 2: { return RWords::Bool(out_ip_address.AssignFromIPLiteral(out_job_key.host.GetHostnameWithoutBrackets())); }
      case 3: { out_job_key.secure_dns_mode=GetEffectiveSecureDnsMode(parameters.secure_dns_policy);
        return {}; }
      case 4: { return RWords::Unsigned(static_cast<uint64_t>(parameters.dns_query_type)); }
      case 5: { out_job_key.query_types={static_cast<DnsQueryType>(args.a)};
        return {}; }
      case 6: { effective_types={DnsQueryType::A,DnsQueryType::AAAA};
        return {}; }
      case 7: { return RWords::Bool(dns_client_ != nullptr); }
      case 8: { return RWords::Bool(!config->nameservers.empty()); }
      case 9: { return RWords::Bool(!config->doh_config.servers().empty()); }
      case 10: { return RWords::Bool(config->use_local_ipv6); }
      case 11: { return RWords::Bool(last_ipv6_probe_result_); }
      case 12: { return RWords::Bool(ipv6_reachability_override_); }
      case 13: { return RWords::Signed(static_cast<int64_t>(out_job_key.GetTargetNetwork())); }
      case 14: { out_job_key.flags|=HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6; effective_types.Remove(DnsQueryType::AAAA);
        return {}; }
      case 15: { return RWords::Bool(base::FeatureList::IsEnabled(features::kUseDnsHttpsSvcb)); }
      case 16: { return RWords::Bool(out_job_key.host.HasScheme()); }
      case 17: { return RWords::Bool(out_job_key.host.GetScheme()==url::kHttpScheme || out_job_key.host.GetScheme()==url::kHttpsScheme || out_job_key.host.GetScheme()==url::kWsScheme || out_job_key.host.GetScheme()==url::kWssScheme); }
      case 18: { effective_types.Put(DnsQueryType::HTTPS);
        return {}; }
      case 19: { out_job_key.query_types=effective_types;
        return {}; }
      case 20: { config=&dns_client_->GetEffectiveConfig();
        return {}; }
      case 21: { return {.a=parameters.include_canonical_name,.b=parameters.loopback_only,.x=parameters.avoid_multicast_resolution,.y=additional_resolver_flags_}; }
      case 22: { return {.a=HOST_RESOLVER_CANONNAME,.b=HOST_RESOLVER_LOOPBACK_ONLY,.x=HOST_RESOLVER_AVOID_MULTICAST}; }
      case 23: { out_job_key.flags=static_cast<HostResolverFlags>(args.a);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 13, native_service);
  (void)rust_reply;
}

HostCache::Entry HostResolverManager::ResolveLocally(
    bool only_ipv6_reachable,
    const JobKey& job_key,
    const IPAddress& ip_address,
    ResolveHostParameters::CacheUsage cache_usage,
    SecureDnsPolicy secure_dns_policy,
    HostResolverSource source,
    const NetLogWithSource& source_net_log,
    HostCache* cache,
    std::deque<TaskType>* out_tasks,
    std::optional<HostCache::EntryStaleness>* out_stale_info,
    bool record_metrics) {
  using RWords = DnsResolutionWords;

std::optional<HostCache::Entry> resolved; std::set<std::unique_ptr<HostResolverInternalResult>> host_results;std::string validation_hostname;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(out_stale_info); *out_stale_info=std::nullopt;
        return {}; }
      case 2: { CreateTaskSequence(job_key,cache_usage,secure_dns_policy,out_tasks);
        return {}; }
      case 3: { source_net_log.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_TASK_SEQUENCE_CREATED,[&]{ base::ListValue list; for(TaskType t:*out_tasks) list.Append(static_cast<int>(t)); return base::DictValue().Set("tasks",std::move(list));});
        return {}; }
      case 4: { return RWords::Bool(ip_address.IsValid()); }
      case 5: { return RWords::Unsigned(static_cast<uint64_t>(job_key.source)); }
      case 6: { validation_hostname=job_key.host.GetHostname();return RWords::Bytes(validation_hostname); }
      case 7: { return RWords::Bool(IsCanonicalizedHostCompliant(job_key.host.GetHostname())); }
      case 8: { resolved.emplace(ERR_NAME_NOT_RESOLVED,HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      case 9: { return RWords::Unsigned(static_cast<uint64_t>(job_key.host.GetHostname().size())); }
      case 10: { return {.a=static_cast<uint64_t>((job_key.flags&HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6)!=0),.b=ip_address.IsIPv4(),.x=ip_address.IsLoopback(),.y=static_cast<int64_t>(source)}; }
      case 11: { return RWords::Bool(only_ipv6_reachable); }
      case 12: { out_tasks->push_front(TaskType::NAT64); resolved.emplace(ERR_DNS_CACHE_MISS,HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      case 13: { resolved=ResolveAsIP(job_key.query_types,job_key.flags & HOST_RESOLVER_CANONNAME,ip_address);
        return {}; }
      case 14: { resolved=ServeLocalhost(job_key.host.GetHostname(),job_key.query_types,job_key.flags & HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6);
        return {}; }
      case 15: { return RWords::Bool(resolved.has_value()); }
      case 16: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->size())); }
      case 17: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->front())); }
      case 18: { out_tasks->pop_front();
        return {}; }
      case 19: { resolved=MaybeServeFromCache(cache,job_key.ToCacheKey(args.a!=0),cache_usage,args.b!=0,source_net_log,out_stale_info,record_metrics);
        return {}; }
      case 20: { DCHECK(out_stale_info->has_value()); source_net_log.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_CACHE_HIT,[&]{return NetLogResults(*resolved);});
        return {}; }
      case 21: { DCHECK(!out_stale_info->has_value());
        return {}; }
      case 22: { resolved=MaybeReadFromConfig(job_key);
        return {}; }
      case 23: { source_net_log.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_CONFIG_PRESET_MATCH,[&]{return NetLogResults(*resolved);});
        return {}; }
      case 24: { StartBootstrapFollowup(job_key,cache,source_net_log);
        return {}; }
      case 25: { host_results=ServeFromHosts(job_key.host.GetHostname(),job_key.query_types,job_key.flags & HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6,*out_tasks);
        return {}; }
      case 26: { return RWords::Unsigned(static_cast<uint64_t>(host_results.size())); }
      case 27: { return RWords::Bool((*std::next(host_results.begin(),args.a))->type()==HostResolverInternalResult::Type::kData); }
      case 28: { source_net_log.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_HOSTS_HIT,[&]{return NetLogResults(host_results);});
        return {}; }
      case 29: { resolved.emplace(host_results,base::Time::Now(),tick_clock_->NowTicks(),HostCache::Entry::SOURCE_HOSTS);
        return {}; }
      case 30: { NOTREACHED(); }
      case 31: { resolved.emplace(ERR_DNS_CACHE_MISS,HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      case 32: { std::string().swap(validation_hostname);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 14, native_service);
  return std::move(resolved).value();
}

void HostResolverManager::CreateAndStartJob(JobKey key,
                                            std::deque<TaskType> tasks,
                                            RequestImpl* request) {
  using RWords = DnsResolutionWords;

JobMap::iterator it;Job* job=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!tasks.empty());
        return {}; }
      case 2: { it=jobs_.find(key);
        return {}; }
      case 3: { return RWords::Bool(it==jobs_.end()); }
      case 4: { job=AddJobWithoutRequest(key,request->parameters().cache_usage,request->host_cache(),std::move(tasks),request->priority(),request->source_net_log());
        return {}; }
      case 5: { job=it->second.get();
        return {}; }
      case 6: { job->AddRequest(request);
        return {}; }
      case 7: { job->RunNextTask();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 18, native_service);
  (void)rust_reply;
}

HostResolverManager::Job* HostResolverManager::AddJobWithoutRequest(
    JobKey key,
    ResolveHostParameters::CacheUsage cache_usage,
    HostCache* host_cache,
    std::deque<TaskType> tasks,
    RequestPriority priority,
    const NetLogWithSource& source_net_log) {
  auto new_job =
      std::make_unique<Job>(weak_ptr_factory_.GetWeakPtr(), key, cache_usage,
                            host_cache, std::move(tasks), priority,
                            source_net_log, tick_clock_, https_svcb_options_);
  auto insert_result = jobs_.emplace(std::move(key), std::move(new_job));
  auto& iterator = insert_result.first;
  bool is_new = insert_result.second;
  CHECK(is_new);
  auto& job = iterator->second;
  job->OnAddedToJobMap(iterator);
  return job.get();
}

void HostResolverManager::CreateAndStartJobForServiceEndpointRequest(
    JobKey key,
    std::deque<TaskType> tasks,
    ServiceEndpointRequestImpl* request) {
  using RWords = DnsResolutionWords;

JobMap::iterator it;Job* job=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { CHECK(!tasks.empty());
        return {}; }
      case 2: { it=jobs_.find(key);
        return {}; }
      case 3: { return RWords::Bool(it==jobs_.end()); }
      case 4: { job=AddJobWithoutRequest(key,request->parameters().cache_usage,request->host_cache(),std::move(tasks),request->priority(),request->net_log());
        return {}; }
      case 5: { job=it->second.get();
        return {}; }
      case 6: { job->AddServiceEndpointRequest(request);
        return {}; }
      case 7: { job->RunNextTask();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 19, native_service);
  (void)rust_reply;
}

HostCache::Entry HostResolverManager::ResolveAsIP(DnsQueryTypeSet query_types,
                                                  bool resolve_canonname,
                                                  const IPAddress& ip_address) {
  using RWords = DnsResolutionWords;

base::flat_set<std::string> aliases;std::optional<HostCache::Entry> result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(ip_address.IsValid());DCHECK(!query_types.Has(DnsQueryType::UNSPECIFIED));
        return {}; }
      case 2: { return RWords::Bool(query_types.Has(AddressFamilyToDnsQueryType(GetAddressFamily(ip_address)))); }
      case 3: { result.emplace(ERR_NAME_NOT_RESOLVED,HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      case 4: { return RWords::Bool(resolve_canonname); }
      case 5: { aliases={ip_address.ToString()};
        return {}; }
      case 6: { result.emplace(OK,std::vector<IPEndPoint>{IPEndPoint(ip_address,0)},std::move(aliases),HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 15, native_service);
  return std::move(result).value();
}

std::optional<HostCache::Entry> HostResolverManager::MaybeServeFromCache(
    HostCache* cache,
    const HostCache::Key& key,
    ResolveHostParameters::CacheUsage cache_usage,
    bool ignore_secure,
    const NetLogWithSource& source_net_log,
    std::optional<HostCache::EntryStaleness>* out_stale_info,
    bool record_metrics) {
  using RWords = DnsResolutionWords;

std::optional<HostCache::Key> effective_key; const std::pair<const HostCache::Key, HostCache::Entry>* cache_result = nullptr; HostCache::EntryStaleness staleness; std::optional<HostCache::Entry> result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(out_stale_info); *out_stale_info = std::nullopt;
        return {}; }
      case 2: { return RWords::Bool(cache != nullptr); }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(cache_usage)); }
      case 4: { effective_key.emplace(key);
        return {}; }
      case 5: { return RWords::Unsigned(static_cast<uint64_t>(effective_key->host_resolver_source)); }
      case 6: { effective_key->host_resolver_source = HostResolverSource::ANY;
        return {}; }
      case 7: { cache_result = cache->LookupStale(*effective_key, tick_clock_->NowTicks(), &staleness, ignore_secure, record_metrics);
        return {}; }
      case 8: { DCHECK(cache_usage == ResolveHostParameters::CacheUsage::ALLOWED || cache_usage == ResolveHostParameters::CacheUsage::STALE_ALLOWED_WHILE_REFRESHING); cache_result = cache->Lookup(*effective_key, tick_clock_->NowTicks(), ignore_secure, record_metrics); staleness = HostCache::kNotStale;
        return {}; }
      case 9: { return RWords::Bool(cache_result != nullptr); }
      case 10: { *out_stale_info = std::move(staleness); source_net_log.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_CACHE_HIT, [&] { return NetLogResults(cache_result->second); }); result = cache_result->second;
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 7, native_service);
  return result;
}

std::optional<HostCache::Entry> HostResolverManager::MaybeReadFromConfig(
    const JobKey& key) {
  using RWords = DnsResolutionWords;

std::optional<std::vector<IPEndPoint>> preset;std::vector<IPEndPoint> filtered;std::optional<HostCache::Entry> result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(HasAddressType(key.query_types));
        return {}; }
      case 2: { return RWords::Bool(key.host.HasScheme()); }
      case 3: { preset=dns_client_->GetPresetAddrs(key.host.AsSchemeHostPort());
        return {}; }
      case 4: { return RWords::Bool(preset.has_value()); }
      case 5: { filtered=FilterAddresses(std::move(*preset),key.query_types);
        return {}; }
      case 6: { return RWords::Unsigned(static_cast<uint64_t>(filtered.size())); }
      case 7: { return RWords::Bool(base::FeatureList::IsEnabled(features::kEnableBootstrapIPRandomizationForDoh)); }
      case 8: { base::RandomShuffle(filtered.begin(),filtered.end());
        return {}; }
      case 9: { result.emplace(OK,std::move(filtered),base::flat_set<std::string>{},HostCache::Entry::SOURCE_CONFIG);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 16, native_service);
  return result;
}

void HostResolverManager::StartBootstrapFollowup(
    JobKey key,
    HostCache* host_cache,
    const NetLogWithSource& source_net_log) {
  using RWords = DnsResolutionWords;

Job* job=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_EQ(SecureDnsMode::kOff,key.secure_dns_mode);DCHECK(host_cache);
        return {}; }
      case 2: { key.secure_dns_mode=SecureDnsMode::kSecure;
        return {}; }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(jobs_.count(key))); }
      case 4: { job=AddJobWithoutRequest(key,ResolveHostParameters::CacheUsage::ALLOWED,host_cache,{TaskType::SECURE_DNS},RequestPriority::LOW,source_net_log);
        return {}; }
      case 5: { job->RunNextTask();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 17, native_service);
  (void)rust_reply;
}

std::set<std::unique_ptr<HostResolverInternalResult>>
HostResolverManager::ServeFromHosts(std::string_view hostname,
                                    DnsQueryTypeSet query_types,
                                    bool default_family_due_to_no_ipv6,
                                    const std::deque<TaskType>& tasks) {
  using RWords = DnsResolutionWords;

const DnsHosts* hosts=nullptr;DnsHosts::const_iterator selected;std::string effective;std::unique_ptr<HostResolverInternalResult> ipv4,ipv6;std::set<std::unique_ptr<HostResolverInternalResult>> result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!query_types.Has(DnsQueryType::UNSPECIFIED));
        return {}; }
      case 2: { return RWords::Bool(dns_client_!=nullptr); }
      case 3: { return {.a=query_types.Has(DnsQueryType::A),.b=query_types.Has(DnsQueryType::AAAA)}; }
      case 4: { return RWords::Unsigned(static_cast<uint64_t>(tasks.size())); }
      case 5: { return RWords::Unsigned(static_cast<uint64_t>(tasks.front())); }
      case 6: { hosts=dns_client_->GetHosts();
        return {}; }
      case 7: { return RWords::Bool(hosts!=nullptr); }
      case 8: { return RWords::Unsigned(static_cast<uint64_t>(hosts->size())); }
      case 9: { effective=base::ToLowerASCII(hostname);
        return {}; }
      case 10: { return RWords::Bool(query_types.Has(args.a?DnsQueryType::AAAA:DnsQueryType::A)); }
      case 11: { selected=hosts->find(DnsHostsKey(effective,args.a?ADDRESS_FAMILY_IPV6:ADDRESS_FAMILY_IPV4));
        return {}; }
      case 12: { return RWords::Bool(selected!=hosts->end()); }
      case 13: { auto item=std::make_unique<HostResolverInternalDataResult>(std::string(hostname),args.a?DnsQueryType::AAAA:DnsQueryType::A,tick_clock_->NowTicks(),base::Time::Now(),HostResolverInternalResult::Source::kHosts,std::vector<IPEndPoint>{{selected->second,0}},std::vector<std::string>{},std::vector<HostPortPair>{}); (args.a?ipv6:ipv4)=std::move(item);
        return {}; }
      case 14: { auto item=std::make_unique<HostResolverInternalErrorResult>(std::string(hostname),args.a?DnsQueryType::AAAA:DnsQueryType::A,std::nullopt,std::nullopt,HostResolverInternalResult::Source::kHosts,ERR_NAME_NOT_RESOLVED);(args.a?ipv6:ipv4)=std::move(item);
        return {}; }
      case 15: { return RWords::Bool(default_family_due_to_no_ipv6); }
      case 16: { return RWords::Bool(ipv4!=nullptr); }
      case 17: { return RWords::Unsigned(ipv4->type()==HostResolverInternalResult::Type::kError?1:0); }
      case 18: { return RWords::Unsigned(static_cast<uint64_t>(ipv4->AsData().endpoints().size())); }
      case 19: { return RWords::Bool(ipv4->AsData().endpoints().at(args.a).address().IsLoopback()); }
      case 20: { CHECK(!ipv6);query_types.Put(DnsQueryType::AAAA);
        return {}; }
      case 21: { result=ServeFromHosts(hostname,query_types,false,tasks);
        return {}; }
      case 22: { return RWords::Bool(ipv6!=nullptr); }
      case 23: { result.insert(std::move(ipv6));
        return {}; }
      case 24: { result.insert(std::move(ipv4));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 21, native_service);
  return result;
}

std::optional<HostCache::Entry> HostResolverManager::ServeLocalhost(
    std::string_view hostname,
    DnsQueryTypeSet query_types,
    bool default_family_due_to_no_ipv6) {
  using RWords = DnsResolutionWords;

std::vector<IPEndPoint> addresses;std::optional<HostCache::Entry> result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!query_types.Has(DnsQueryType::UNSPECIFIED));
        return {}; }
      case 2: { return {.a=query_types.Has(DnsQueryType::A),.b=query_types.Has(DnsQueryType::AAAA)}; }
      case 3: { return RWords::Bool(ResolveLocalHostname(hostname,&addresses)); }
      case 4: { return RWords::Bool(default_family_due_to_no_ipv6); }
      case 5: { return RWords::Bool(query_types.Has(DnsQueryType::A)); }
      case 6: { return RWords::Bool(query_types.Has(DnsQueryType::AAAA)); }
      case 7: { query_types.Put(DnsQueryType::AAAA);
        return {}; }
      case 8: { result.emplace(OK,FilterAddresses(std::move(addresses),query_types),base::flat_set<std::string>{},HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 20, native_service);
  return result;
}

void HostResolverManager::CacheResult(HostCache* cache,
                                      const HostCache::Key& key,
                                      const HostCache::Entry& entry,
                                      base::TimeDelta ttl) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(cache != nullptr); }
      case 2: { return RWords::Signed(static_cast<int64_t>(entry.error())); }
      case 3: { return RWords::Bool(ttl.is_positive()); }
      case 4: { cache->Set(key, entry, tick_clock_->NowTicks(), ttl);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 3, native_service);
  (void)rust_reply;
}

std::unique_ptr<HostResolverManager::Job> HostResolverManager::RemoveJob(
    JobMap::iterator job_it) {
  CHECK(job_it != jobs_.end());
  CHECK(job_it->second);
  CHECK(jobs_.find(job_it->first) != jobs_.end());

  std::unique_ptr<Job> job;
  job_it->second.swap(job);
  jobs_.erase(job_it);
  job->OnRemovedFromJobMap();

  return job;
}

SecureDnsMode HostResolverManager::GetEffectiveSecureDnsMode(
    SecureDnsPolicy secure_dns_policy) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(secure_dns_policy)); }
      case 2: { return RWords::Bool(dns_client_ != nullptr); }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(dns_client_->GetEffectiveConfig().secure_dns_mode)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 2, native_service);
  return static_cast<SecureDnsMode>(rust_reply.a);
}

bool HostResolverManager::ShouldForceSystemResolverDueToTestOverride() const {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(HostResolverProc::GetDefault()!=nullptr); }
      case 2: { return RWords::Bool(system_resolver_disabled_for_testing_); }
      case 3: { DCHECK(dns_client_);DCHECK(std::ranges::none_of(dns_client_->GetEffectiveConfig().nameservers,&IPAddress::IsPubliclyRoutable,&IPEndPoint::address));
        return {}; }
      case 4: { return RWords::Bool(host_resolver_system_params_.resolver_proc.get()!=nullptr); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 41, native_service);
  return rust_reply.a!=0;
}

// static
void HostResolverManager::PushDnsTasks(const DnsClient& dns_client,
                                       bool dns_tasks_allowed,
                                       bool allow_fallback_to_systemtask,
                                       bool system_task_allowed,
                                       SecureDnsMode secure_dns_mode,
                                       InsecureDnsMode insecure_dns_mode,
                                       bool allow_cache,
                                       bool prioritize_local_lookups,
                                       ResolveContext* resolve_context,
                                       std::deque<TaskType>* out_tasks) {
  using RWords = DnsResolutionWords;


std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { out_tasks->push_front(static_cast<TaskType>(args.a));
        return {}; }
      case 2: { out_tasks->push_back(static_cast<TaskType>(args.a));
        return {}; }
      case 3: { out_tasks->pop_front();
        return {}; }
      case 4: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->front())); }
      case 5: { return RWords::Bool(dns_client.CanUseSecureDnsTransactions()); }
      case 6: { return RWords::Bool(dns_client.FallbackFromSecureTransactionPreferred(resolve_context)); }
      case 7: { DCHECK_EQ(static_cast<uint64_t>(out_tasks->front()), args.a);DCHECK(args.b==0||args.x!=0);
        return {}; }
      case 8: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->size())); }
      case 9: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->at(args.a))); }
      case 10: { NOTREACHED(); }
      case 11: { return {.a=dns_tasks_allowed, .b=allow_fallback_to_systemtask, .x=system_task_allowed, .y=static_cast<int64_t>(secure_dns_mode)}; }
      case 12: { return {.a=static_cast<uint64_t>(insecure_dns_mode), .b=allow_cache, .x=prioritize_local_lookups}; }
      case 9000: { if(client_frame_depth==0){const auto* owner=(&dns_client);if(owner)client_frame=owner->MakeRustPolicyServices(resolve_context);}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 11, native_service);
  (void)rust_reply;
}

void HostResolverManager::CreateTaskSequence(
    const JobKey& job_key,
    ResolveHostParameters::CacheUsage cache_usage,
    SecureDnsPolicy secure_dns_policy,
    std::deque<TaskType>* out_tasks) {
  using RWords = DnsResolutionWords;

EchMode ech=EchMode::kOpportunistic;
std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { out_tasks->push_front(static_cast<TaskType>(args.a));
        return {}; }
      case 2: { out_tasks->push_back(static_cast<TaskType>(args.a));
        return {}; }
      case 3: { out_tasks->pop_front();
        return {}; }
      case 4: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->front())); }
      case 5: { return RWords::Bool(dns_client_->CanUseSecureDnsTransactions()); }
      case 6: { return RWords::Bool(dns_client_->FallbackFromSecureTransactionPreferred(&*job_key.resolve_context)); }
      case 7: { DCHECK_EQ(static_cast<uint64_t>(out_tasks->front()), args.a);DCHECK(args.b==0||args.x!=0);
        return {}; }
      case 8: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->size())); }
      case 9: { return RWords::Unsigned(static_cast<uint64_t>(out_tasks->at(args.a))); }
      case 10: { NOTREACHED(); }
      case 11: { DCHECK(out_tasks->empty());
        return {}; }
      case 12: { ech=job_key.GetEchMode();return RWords::Unsigned(static_cast<uint64_t>(ech)); }
      case 13: { return RWords::Unsigned(static_cast<uint64_t>(cache_usage)); }
      case 14: { return RWords::Unsigned(static_cast<uint64_t>(job_key.secure_dns_mode)); }
      case 15: { return RWords::Unsigned(static_cast<uint64_t>(secure_dns_policy)); }
      case 16: { DCHECK_EQ(SecureDnsMode::kOff, job_key.secure_dns_mode);
        return {}; }
      case 17: { return {.a=job_key.query_types.Has(DnsQueryType::A),.b=job_key.query_types.Has(DnsQueryType::AAAA)}; }
      case 18: { return RWords::Unsigned(static_cast<uint64_t>(job_key.source)); }
      case 19: { RecordDnsClientCapabilityMetrics(dns_client_.get());
        return {}; }
      case 20: { return RWords::Bool((job_key.flags & HOST_RESOLVER_CANONNAME) != 0); }
      case 21: { return RWords::Bool(ResemblesMulticastDNSName(job_key.host.GetHostname())); }
      case 22: { return RWords::Bool(dns_client_ != nullptr); }
      case 23: { return RWords::Bool(dns_client_->CanUseInsecureDnsTransactions(ech)); }
      case 24: { return RWords::Bool(dns_client_->FallbackFromInsecureTransactionPreferred(ech)); }
      case 25: { return RWords::Bool(dns_client_->CanQueryAdditionalTypesViaInsecureDns(ech)); }
      case 26: { return RWords::Unsigned(static_cast<uint64_t>(dns_client_->GetInsecureDnsMode(ech))); }
      case 27: { return RWords::Bool(ShouldForceSystemResolverDueToTestOverride()); }
      case 28: { return RWords::Bool(allow_fallback_to_systemtask_); }
      case 29: { DCHECK(std::ranges::find(*out_tasks, TaskType::DNS) == out_tasks->end()); DCHECK(std::ranges::find(*out_tasks, TaskType::DNS_PLATFORM) == out_tasks->end()); DCHECK(std::ranges::find(*out_tasks, TaskType::MDNS) == out_tasks->end());
        return {}; }
      case 30: { base::UmaHistogramEnumeration("Net.DNS.DnsConfig.DnsClientCapability",static_cast<DnsClientCapability>(args.a));
        return {}; }
      case 31: { return RWords::Bool(dns_client_->CanUseInsecureDnsTransactions(std::nullopt)); }
      case 9000: { if(client_frame_depth==0){const auto* owner=(dns_client_.get());if(owner)client_frame=owner->MakeRustPolicyServices(job_key.resolve_context.get());}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 12, native_service);
  (void)rust_reply;
}

namespace {

bool RequestWillUseWiFi(handles::NetworkHandle network) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Signed(static_cast<int64_t>(network)); }
      case 2: { return RWords::Signed(static_cast<int64_t>(NetworkChangeNotifier::GetConnectionType())); }
      case 3: { return RWords::Signed(static_cast<int64_t>(NetworkChangeNotifier::GetNetworkConnectionType(network))); }
      case 4: { return RWords::Signed(static_cast<int64_t>(NetworkChangeNotifier::CONNECTION_WIFI)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 48, native_service);
  return rust_reply.a!=0;
}

}  // namespace

void HostResolverManager::FinishIPv6ReachabilityCheck(
    CompletionOnceCallback callback,
    int rv) {
  using RWords = DnsResolutionWords;

std::vector<CompletionOnceCallback> callbacks;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { SetLastIPv6ProbeResult(args.a!=0);
        return {}; }
      case 2: { return RWords::Signed(static_cast<int64_t>(rv)); }
      case 3: { std::move(callback).Run(OK);
        return {}; }
      case 4: { return RWords::Unsigned(static_cast<uint64_t>(ipv6_request_callbacks_.size())); }
      case 5: { ipv6_request_callbacks_.swap(callbacks);
        return {}; }
      case 6: { return RWords::Unsigned(static_cast<uint64_t>(callbacks.size())); }
      case 7: { std::move(callbacks.at(args.a)).Run(OK);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 34, native_service);
  (void)rust_reply;
}

int HostResolverManager::StartIPv6ReachabilityCheck(
    handles::NetworkHandle target_network,
    const NetLogWithSource& net_log,
    ClientSocketFactory* client_socket_factory,
    CompletionOnceCallback callback) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { CHECK(target_network_==handles::kInvalidNetworkHandle || target_network==handles::kInvalidNetworkHandle);
        return {}; }
      case 2: { return RWords::Signed(static_cast<int64_t>(target_network)); }
      case 3: { return RWords::Bool(check_ipv6_on_wifi_); }
      case 4: { return RWords::Bool(RequestWillUseWiFi(target_network_)); }
      case 5: { probing_ipv6_=false;last_ipv6_probe_result_=false;last_ipv6_probe_time_=base::TimeTicks();
        return {}; }
      case 6: { return RWords::Bool(probing_ipv6_); }
      case 7: { ipv6_request_callbacks_.push_back(std::move(callback));
        return {}; }
      case 8: { return RWords::Signed(static_cast<int64_t>(ERR_IO_PENDING)); }
      case 9: { return RWords::Bool(last_ipv6_probe_time_.is_null()); }
      case 10: { return RWords::Signed(static_cast<int64_t>((tick_clock_->NowTicks()-last_ipv6_probe_time_).InMilliseconds())); }
      case 11: { probing_ipv6_=true;
        return {}; }
      case 12: { return RWords::Signed(StartGloballyReachableCheck(IPAddress(kIPv6ProbeAddress),target_network,net_log,client_socket_factory,base::BindOnce(&HostResolverManager::FinishIPv6ReachabilityCheck,weak_ptr_factory_.GetWeakPtr(),std::move(callback)))); }
      case 13: { SetLastIPv6ProbeResult(args.a!=0);
        return {}; }
      case 14: { net_log.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_IPV6_REACHABILITY_CHECK,[&]{return NetLogIPv6AvailableParams(last_ipv6_probe_result_,args.a!=0);});
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 35, native_service);
  return static_cast<int>(rust_reply.x);
}

void HostResolverManager::SetLastIPv6ProbeResult(bool last_ipv6_probe_result) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { probing_ipv6_=false;
        return {}; }
      case 2: { last_ipv6_probe_result_=last_ipv6_probe_result;
        return {}; }
      case 3: { last_ipv6_probe_time_=tick_clock_->NowTicks();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 33, native_service);
  (void)rust_reply;
}

int HostResolverManager::StartGloballyReachableCheck(
    const IPAddress& dest,
    handles::NetworkHandle target_network,
    const NetLogWithSource& net_log,
    ClientSocketFactory* client_socket_factory,
    CompletionOnceCallback callback) {
  using RWords = DnsResolutionWords;

std::unique_ptr<DatagramClientSocket> probing_socket;DatagramClientSocket* socket=nullptr;scoped_refptr<base::RefCountedData<std::unique_ptr<DatagramClientSocket>>> refcounted_socket;int last_rv=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { probing_socket=client_socket_factory->CreateDatagramClientSocket(DatagramSocket::DEFAULT_BIND,target_network,net_log.net_log(),net_log.source());socket=probing_socket.get();refcounted_socket=base::MakeRefCounted<base::RefCountedData<std::unique_ptr<DatagramClientSocket>>>(std::move(probing_socket));
        return {}; }
      case 2: { last_rv=socket->ConnectAsync(IPEndPoint(dest,GetPortForGloballyReachableCheck()),base::BindOnce(&HostResolverManager::RunFinishGloballyReachableCheck,weak_ptr_factory_.GetWeakPtr(),refcounted_socket,std::move(callback)));return RWords::Signed(last_rv); }
      case 3: { return RWords::Signed(static_cast<int64_t>(ERR_IO_PENDING)); }
      case 4: { return RWords::Bool(FinishGloballyReachableCheck(socket,static_cast<int>(last_rv))); }
      case 5: { return RWords::Signed(static_cast<int64_t>(ERR_FAILED)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 36, native_service);
  return static_cast<int>(rust_reply.x);
}

bool HostResolverManager::FinishGloballyReachableCheck(
    DatagramClientSocket* socket,
    int rv) {
  using RWords = DnsResolutionWords;

IPEndPoint endpoint;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Signed(static_cast<int64_t>(rv)); }
      case 2: { return RWords::Signed(static_cast<int64_t>(socket->GetLocalAddress(&endpoint))); }
      case 3: { return RWords::Bool(endpoint.address().IsLinkLocal()); }
      case 4: { return RWords::Bool(endpoint.address().IsIPv6()); }
      case 5: { const uint8_t prefix[]={0x20,0x01,0,0};return RWords::Bool(IPAddressStartsWith(endpoint.address(),prefix)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 32, native_service);
  return rust_reply.a!=0;
}

void HostResolverManager::RunFinishGloballyReachableCheck(
    scoped_refptr<base::RefCountedData<std::unique_ptr<DatagramClientSocket>>>
        socket,
    CompletionOnceCallback callback,
    int rv) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(FinishGloballyReachableCheck(socket->data.get(),rv)); }
      case 2: { std::move(callback).Run(args.a?OK:ERR_FAILED);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 42, native_service);
  (void)rust_reply;
}

void HostResolverManager::RunLoopbackProbeJob() {
  RunHaveOnlyLoopbackAddressesJob(
      base::BindOnce(&HostResolverManager::SetHaveOnlyLoopbackAddresses,
                     weak_ptr_factory_.GetWeakPtr()));
}

void HostResolverManager::RemoveAllJobs(const ResolveContext* context) {
  using RWords = DnsResolutionWords;

std::vector<std::unique_ptr<Job>> jobs_to_destroy;JobMap::iterator it;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { it=jobs_.begin();
        return {}; }
      case 2: { return RWords::Bool(it!=jobs_.end()); }
      case 3: { return RWords::Bool(&*it->first.resolve_context==context); }
      case 4: { jobs_to_destroy.push_back(RemoveJob(it++));
        return {}; }
      case 5: { ++it;
        return {}; }
      case 6: { jobs_to_destroy.clear();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 22, native_service);
  (void)rust_reply;
}

void HostResolverManager::AbortJobsWithoutTargetNetwork(bool in_progress_only) {
  using RWords = DnsResolutionWords;

std::vector<std::unique_ptr<Job>> jobs_to_abort;JobMap::iterator it;std::optional<PrioritizedDispatcher::Limits> limits;base::WeakPtr<HostResolverManager> self;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { it=jobs_.begin();
        return {}; }
      case 2: { return RWords::Bool(it!=jobs_.end()); }
      case 3: { return RWords::Bool(it->second->HasTargetNetwork()); }
      case 4: { return RWords::Bool(in_progress_only); }
      case 5: { return RWords::Bool(it->second->is_running()); }
      case 6: { jobs_to_abort.push_back(RemoveJob(it++));
        return {}; }
      case 7: { ++it;
        return {}; }
      case 8: { limits.emplace(dispatcher_->GetLimits());
        return {}; }
      case 9: { dispatcher_->SetLimits(PrioritizedDispatcher::Limits(limits->reserved_slots.size(),0));
        return {}; }
      case 10: { self=weak_ptr_factory_.GetWeakPtr();
        return {}; }
      case 11: { return RWords::Bool(self.get()!=nullptr); }
      case 12: { return RWords::Unsigned(static_cast<uint64_t>(jobs_to_abort.size())); }
      case 13: { jobs_to_abort.at(args.a)->Abort();
        return {}; }
      case 14: { dispatcher_->SetLimits(*limits);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 23, native_service);
  (void)rust_reply;
}

void HostResolverManager::AbortInsecureDnsTasks(int error, bool fallback_only) {
  using RWords = DnsResolutionWords;

std::vector<base::OnceClosure> closures;JobMap::iterator it;std::optional<PrioritizedDispatcher::Limits> limits;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { it=jobs_.begin();
        return {}; }
      case 2: { return RWords::Bool(it!=jobs_.end()); }
      case 3: { closures.push_back(it->second->GetAbortInsecureDnsTaskClosure(error,fallback_only));
        return {}; }
      case 4: { ++it;
        return {}; }
      case 5: { limits.emplace(dispatcher_->GetLimits());
        return {}; }
      case 6: { dispatcher_->SetLimits(PrioritizedDispatcher::Limits(limits->reserved_slots.size(),0));
        return {}; }
      case 7: { return RWords::Unsigned(static_cast<uint64_t>(closures.size())); }
      case 8: { std::move(closures.at(args.a)).Run();
        return {}; }
      case 9: { dispatcher_->SetLimits(*limits);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 24, native_service);
  (void)rust_reply;
}

// TODO(crbug.com/40641277): Consider removing this and its usage.
void HostResolverManager::TryServingAllJobsFromHosts() {
  using RWords = DnsResolutionWords;

base::WeakPtr<HostResolverManager> self;JobMap::iterator it;Job* job=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(dns_client_!=nullptr); }
      case 2: { self=weak_ptr_factory_.GetWeakPtr();
        return {}; }
      case 3: { it=jobs_.begin();
        return {}; }
      case 4: { return RWords::Bool(self.get()!=nullptr); }
      case 5: { return RWords::Bool(it!=jobs_.end()); }
      case 6: { job=it->second.get();++it;
        return {}; }
      case 7: { job->ServeFromHosts();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 25, native_service);
  (void)rust_reply;
}

void HostResolverManager::OnIPAddressChanged(
    NetworkChangeNotifier::IPAddressChangeType change_type) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!IsBoundToNetwork());last_ipv6_probe_time_=base::TimeTicks();
        return {}; }
      case 2: { probe_weak_ptr_factory_.InvalidateWeakPtrs();
        return {}; }
      case 3: { InvalidateCaches();
        return {}; }
      case 4: {
#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_APPLE)
RunLoopbackProbeJob();
#endif
        return {}; }
      case 5: { AbortJobsWithoutTargetNetwork(true);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 29, native_service);
  (void)rust_reply;
}

void HostResolverManager::OnConnectionTypeChanged(
    NetworkChangeNotifier::ConnectionType type) {
  DCHECK(!IsBoundToNetwork());
  UpdateConnectionType(type);
}

void HostResolverManager::OnSystemDnsConfigChanged(const DnsConfig& config) {
  using RWords = DnsResolutionWords;


std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!IsBoundToNetwork());if(HostResolverProc::GetDefault()&&system_resolver_disabled_for_testing_){DCHECK(std::ranges::none_of(config.nameservers,&IPAddress::IsPubliclyRoutable,&IPEndPoint::address));}
        return {}; }
      case 2: { return RWords::Bool(dns_client_!=nullptr); }
      case 3: { return RWords::Bool(dns_client_->CanUseSecureDnsTransactions()); }
      case 4: { return RWords::Bool(dns_client_->CanUseInsecureDnsTransactions(std::nullopt)); }
      case 5: { return RWords::Bool(dns_client_->SetSystemConfig(config)); }
      case 6: { InvalidateCaches();
        return {}; }
      case 7: { UpdateJobsForChangedConfig();
        return {}; }
      case 9000: { if(client_frame_depth==0){const auto* owner=(dns_client_.get());if(owner)client_frame=owner->MakeRustPolicyServices(nullptr);}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 26, native_service);
  (void)rust_reply;
}

void HostResolverManager::UpdateJobsForChangedConfig() {
  using RWords = DnsResolutionWords;

base::WeakPtr<HostResolverManager> self;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { self = weak_ptr_factory_.GetWeakPtr();
        return {}; }
      case 2: { AbortJobsWithoutTargetNetwork(false);
        return {}; }
      case 3: { return RWords::Bool(self.get() != nullptr); }
      case 4: { TryServingAllJobsFromHosts();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 9, native_service);
  (void)rust_reply;
}

void HostResolverManager::OnFallbackResolve(int dns_task_error) {
  using RWords = DnsResolutionWords;


std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(dns_client_); DCHECK_NE(OK, dns_task_error);
        return {}; }
      case 2: { return RWords::Bool(dns_client_->FallbackFromInsecureTransactionPreferred(std::nullopt)); }
      case 3: { dns_client_->IncrementInsecureFallbackFailures();
        return {}; }
      case 4: { AbortInsecureDnsTasks(ERR_FAILED, true);
        return {}; }
      case 9000: { if(client_frame_depth==0){const auto* owner=(dns_client_.get());if(owner)client_frame=owner->MakeRustPolicyServices(nullptr);}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 8, native_service);
  (void)rust_reply;
}

int HostResolverManager::GetOrCreateMdnsClient(MDnsClient** out_client) {
  using RWords = DnsResolutionWords;
#if BUILDFLAG(ENABLE_MDNS)


  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(mdns_client_!=nullptr); }
      case 2: { return RWords::Bool(mdns_socket_factory_!=nullptr); }
      case 3: { mdns_socket_factory_=std::make_unique<MDnsSocketFactoryImpl>(net_log_);
        return {}; }
      case 4: { mdns_client_=MDnsClient::CreateDefault();
        return {}; }
      case 5: { return RWords::Bool(mdns_client_->IsListening()); }
      case 6: { return RWords::Signed(static_cast<int64_t>(mdns_client_->StartListening(mdns_socket_factory_.get()))); }
      case 7: { DCHECK_NE(ERR_IO_PENDING,args.x);DCHECK(args.x!=OK || mdns_client_->IsListening());
        return {}; }
      case 8: { *out_client=mdns_client_.get();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 31, native_service);
  return static_cast<int>(rust_reply.x);
#else
NOTREACHED();
#endif
}

void HostResolverManager::InvalidateCaches(bool network_change) {
  using RWords = DnsResolutionWords;

std::optional<decltype(registered_contexts_.begin())> it;
#if DCHECK_IS_ON()
base::WeakPtr<HostResolverManager> self_ptr;size_t num_jobs=0;
#endif

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);DCHECK(!invalidation_in_progress_);
#if DCHECK_IS_ON()
self_ptr=weak_ptr_factory_.GetWeakPtr();num_jobs=jobs_.size();
#endif
        return {}; }
      case 2: { invalidation_in_progress_=true;
        return {}; }
      case 3: { it.emplace(registered_contexts_.begin());
        return {}; }
      case 4: { return RWords::Bool(*it!=registered_contexts_.end()); }
      case 5: { return RWords::Bool(dns_client_!=nullptr); }
      case 6: { (**it).InvalidateCachesAndPerSessionData(dns_client_->GetCurrentSession(),network_change);
        return {}; }
      case 7: { (**it).InvalidateCachesAndPerSessionData(nullptr,network_change);
        return {}; }
      case 8: { ++*it;
        return {}; }
      case 9: { invalidation_in_progress_=false;
        return {}; }
      case 10: {
#if DCHECK_IS_ON()
DCHECK(self_ptr);DCHECK_EQ(num_jobs,jobs_.size());
#endif
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 28, native_service);
  (void)rust_reply;
}

void HostResolverManager::UpdateConnectionType(
    NetworkChangeNotifier::ConnectionType type) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { host_resolver_system_params_.unresponsive_delay=GetTimeDeltaForConnectionTypeFromFieldTrialOrDefault("DnsUnresponsiveDelayMsByConnectionType",HostResolverSystemTask::Params::kDnsDefaultUnresponsiveDelay,type);
        return {}; }
      case 2: { return RWords::Bool(type==NetworkChangeNotifier::CONNECTION_NONE); }
      case 3: { return RWords::Bool(dns_client_!=nullptr); }
      case 4: { dns_client_->ReplaceCurrentSession();
        return {}; }
      case 5: { InvalidateCaches(true);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 27, native_service);
  (void)rust_reply;
}

std::unique_ptr<DnsProbeRunner> HostResolverManager::CreateDohProbeRunner(
    ResolveContext* resolve_context) {
  using RWords = DnsResolutionWords;

std::unique_ptr<DnsProbeRunner> runner;
std::unique_ptr<DnsClientRustServices> client_frame;uint32_t client_frame_depth=0;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(resolve_context);DCHECK(registered_contexts_.HasObserver(resolve_context));
        return {}; }
      case 2: { return RWords::Bool(dns_client_!=nullptr); }
      case 3: { return RWords::Bool(dns_client_->CanUseSecureDnsTransactions()); }
      case 4: { runner=dns_client_->GetTransactionFactory()->CreateDohProbeRunner(resolve_context);
        return {}; }
      case 9000: { if(client_frame_depth==0){const auto* owner=(dns_client_.get());if(owner)client_frame=owner->MakeRustPolicyServices(nullptr);}++client_frame_depth;return RWords::Bool(client_frame!=nullptr);}
      case 9001: CHECK_GT(client_frame_depth,0u);if(--client_frame_depth==0)client_frame.reset();return {};
      default: if(operation>=10000 && client_frame) return client_frame->Dispatch(operation-10000,args); NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(1, 30, native_service);
  return runner;
}

}  // namespace net
