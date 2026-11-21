// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "net/socket/connect_job_rust_native.h"
#include "base/check.h"

#include <cstdlib>
#include <memory>
#include <set>
#include <utility>
#include <variant>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/trace_event/trace_event.h"
#include "net/base/connection_endpoint_metadata.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/trace_constants.h"
#include "net/base/url_util.h"
#include "net/cert/x509_util.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_values.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_connect_job.h"
#include "net/socket/transport_connect_job.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "net/ssl/ssl_info.h"
#include "third_party/boringssl/src/include/openssl/pool.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"

namespace net {

// Typed storage/provider primitives only. Sequencing/branches live in safe Rust.
struct SSLConnectJobFamilyRustBridge final
    : connection_jobs_upper_rust::Invocation {
  using Class = SSLConnectJob;
  static_assert(Class::STATE_TRANSPORT_CONNECT==0);
  static_assert(Class::STATE_TRANSPORT_CONNECT_COMPLETE==1);
  static_assert(Class::STATE_SOCKS_CONNECT==2);
  static_assert(Class::STATE_SOCKS_CONNECT_COMPLETE==3);
  static_assert(Class::STATE_TUNNEL_CONNECT==4);
  static_assert(Class::STATE_TUNNEL_CONNECT_COMPLETE==5);
  static_assert(Class::STATE_SSL_CONNECT==6);
  static_assert(Class::STATE_SSL_CONNECT_COMPLETE==7);
  static_assert(Class::STATE_NONE==8);
  static_assert(LOAD_STATE_IDLE==0 && LOAD_STATE_ESTABLISHING_PROXY_TUNNEL==9 && LOAD_STATE_CONNECTING==11 && LOAD_STATE_SSL_HANDSHAKE==12);

  explicit SSLConnectJobFamilyRustBridge(Class* job)
      : Invocation(static_cast<const ConnectJob*>(job)), mutable_owner(job), owner(job) { Register(&Clear); base_bridge.emplace(static_cast<ConnectJob*>(job)); }
  explicit SSLConnectJobFamilyRustBridge(const Class* job)
      : Invocation(static_cast<const ConnectJob*>(job)), mutable_owner(nullptr), owner(job) { Register(&Clear); base_bridge.emplace(static_cast<const ConnectJob*>(job)); }
  SSLConnectJobFamilyRustBridge()
      : Invocation(this), mutable_owner(nullptr), owner(nullptr) { Register(&Clear); }
  ~SSLConnectJobFamilyRustBridge() { base_bridge.reset(); Unregister(); }
  Class& M() { CHECK(Live()); CHECK(mutable_owner); return *mutable_owner; }
  const Class& R() const { CHECK(Live()); CHECK(owner); return *owner; }
  int64_t Run(uint32_t operation, int64_t argument = 0) {
    return connection_jobs_upper_rust::connection_jobs_rust_run(
        4, operation, argument, this, Service);
  }
  raw_ptr<Class> mutable_owner;
  raw_ptr<const Class> owner;
  std::optional<ConnectJobRustBridge> base_bridge; std::unique_ptr<StreamSocket> base_socket; std::optional<base::flat_set<std::string>> base_aliases; void PrepareSocket(){ base_bridge->socket_input=&base_socket; base_bridge->aliases=&base_aliases; }

  raw_ptr<ConnectionAttempts> attempts_output = nullptr;
  raw_ptr<ResolveErrorInfo> resolve_output = nullptr;
  raw_ptr<std::optional<ResolutionDetails>> resolution_output = nullptr;
  raw_ptr<scoped_refptr<SSLCertRequestInfo>> certificate_output = nullptr;
  std::optional<SSLConfig> config;
  raw_ptr<base::DictValue> log_output = nullptr;
  raw_ptr<SSLClientContext> context_input = nullptr;
  raw_ptr<const HostPortPair> host_input = nullptr;
  bool params_is_socks=false;
  bool params_is_http=false;
  SSLClientContext* Context() { return owner ? M().ssl_client_context() : context_input.get(); }
  const HostPortPair& Host() { return owner ? R().params_->host_and_port() : *host_input; }
  bool ech_capable=false;
  bool ech_enabled=false;

  static void Clear(Invocation* value) {
    auto& c = *static_cast<SSLConnectJobFamilyRustBridge*>(value);
    c.mutable_owner = nullptr;
    c.owner = nullptr;
  }

  static bool Service(void* context, uint32_t module, uint32_t action, int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<SSLConnectJobFamilyRustBridge*>(context);
    *value = 0;
    if(module==0 && c.base_bridge){ return ConnectJobRustBridge::Service(&*c.base_bridge,action,argument,value); }
    if (!c.Live() || module != 4) { return false; }
    if(action==UINT32_MAX){*value=1;return true;}
    switch (action) {
      case 0: {  // STATE
*value=c.R().next_state_;
        break;
      }
      case 1: {  // SET_STATE
c.M().next_state_=static_cast<Class::State>(argument);
        break;
      }
      case 2: {  // HAS_NESTED
*value=c.R().nested_connect_job_ != nullptr;
        break;
      }
      case 3: {  // HAS_NESTED_SOCKET
*value=c.R().nested_socket_ != nullptr;
        break;
      }
      case 4: {  // HAS_SSL_SOCKET
*value=c.R().ssl_socket_ != nullptr;
        break;
      }
      case 5: {  // NESTED_LOAD
*value=c.R().nested_connect_job_->GetLoadState();
        break;
      }
      case 6: {  // NESTED_ESTABLISHED
*value=c.R().nested_connect_job_->HasEstablishedConnection();
        break;
      }
      case 7: {  // COPY_ATTEMPTS
*c.attempts_output=c.R().connection_attempts_;
        break;
      }
      case 8: {  // COPY_RESOLVE
*c.resolve_output=c.R().resolve_error_info_;
        break;
      }
      case 9: {  // COPY_RESOLUTION
*c.resolution_output=c.R().resolution_details_;
        break;
      }
      case 10: {  // COPY_CERTIFICATE
*c.certificate_output=c.R().ssl_cert_request_info_;
        break;
      }
      case 11: {  // NEGOTIATION_STARTED
*value=c.R().ssl_negotiation_started_;
        break;
      }
      case 12: {  // PARAMS_CONNECTION_TYPE
*value=c.R().params_->GetConnectionType();
        break;
      }
      case 13: {  // CLEAR_TCP
c.M().tcp_connect_job_=nullptr;
        break;
      }
      case 14: {  // CLEAR_NESTED
c.M().nested_connect_job_.reset();
        break;
      }
      case 15: {  // HAPPY_EYEBALLS
*value=base::FeatureList::IsEnabled(features::kHappyEyeballsV2);
        break;
      }
      case 16: {  // HAS_ECH_RETRY
*value=c.R().ech_retry_configs_.has_value();
        break;
      }
      case 17: {  // CREATE_TCP
DCHECK(!c.R().nested_connect_job_); DCHECK(!c.R().TimerIsRunning());
auto job=std::make_unique<TcpConnectJob>(c.M().priority(),c.M().socket_tag(),
 c.M().common_connect_job_params(),c.M().params_->GetDirectConnectionParams(),
 &c.M(),&c.M().net_log(),std::nullopt,c.R().disable_stale_dns_);
c.M().tcp_connect_job_=job.get();c.M().nested_connect_job_=std::move(job);
        break;
      }
      case 18: {  // CREATE_TCP_RETRY
DCHECK(c.Context()&&c.Context()->IsEchEnabled(c.Host().host()));
DCHECK(c.R().service_endpoint_result_);
auto job=std::make_unique<TcpConnectJob>(c.M().priority(),c.M().socket_tag(),
 c.M().common_connect_job_params(),c.M().params_->GetDirectConnectionParams(),
 &c.M(),&c.M().net_log(),TcpConnectJob::ServiceEndpointOverride(
 *c.R().service_endpoint_result_,c.R().dns_aliases_),c.R().disable_stale_dns_);
c.M().tcp_connect_job_=job.get();c.M().nested_connect_job_=std::move(job);
        break;
      }
      case 19: {  // CREATE_TRANSPORT
c.M().nested_connect_job_=std::make_unique<TransportConnectJob>(
 c.M().priority(),c.M().socket_tag(),c.M().common_connect_job_params(),
 c.M().params_->GetDirectConnectionParams(),&c.M(),&c.M().net_log(),std::nullopt);
        break;
      }
      case 20: {  // CREATE_TRANSPORT_RETRY
DCHECK(c.Context()&&c.Context()->IsEchEnabled(c.Host().host()));
DCHECK(c.R().endpoint_result_);
c.M().nested_connect_job_=std::make_unique<TransportConnectJob>(
 c.M().priority(),c.M().socket_tag(),c.M().common_connect_job_params(),
 c.M().params_->GetDirectConnectionParams(),&c.M(),&c.M().net_log(),
 TransportConnectJob::EndpointResultOverride(*c.R().endpoint_result_,c.R().dns_aliases_));
        break;
      }
      case 21: {  // CONNECT_NESTED
*value=c.M().nested_connect_job_->Connect();
        break;
      }
      case 22: {  // SAVE_RESOLVE
c.M().resolve_error_info_=c.M().nested_connect_job_->GetResolveErrorInfo();
        break;
      }
      case 23: {  // SAVE_RESOLUTION
c.M().resolution_details_=c.M().nested_connect_job_->GetResolutionDetails();
        break;
      }
      case 24: {  // APPEND_ATTEMPTS
auto attempts=c.M().nested_connect_job_->GetConnectionAttempts();
c.M().connection_attempts_.insert(c.M().connection_attempts_.end(),attempts.begin(),attempts.end());
        break;
      }
      case 25: {  // TAKE_NESTED_SOCKET
c.M().nested_socket_=c.M().nested_connect_job_->PassSocket();
        break;
      }
      case 26: {  // SAVE_PEER
c.M().nested_socket_->GetPeerAddress(&c.M().server_address_);
        break;
      }
      case 27: {  // SAVE_ALIASES
c.M().dns_aliases_=c.M().nested_socket_->GetDnsAliases();
        break;
      }
      case 28: {  // SAVE_STALE
c.M().is_connected_via_stale_dns_=c.M().nested_connect_job_->IsConnectedViaStaleDns();
        break;
      }
      case 29: {  // CREATE_SOCKS
DCHECK(!c.R().nested_connect_job_);DCHECK(!c.R().TimerIsRunning());
c.M().nested_connect_job_=std::make_unique<SOCKSConnectJob>(c.M().priority(),
 c.M().socket_tag(),c.M().common_connect_job_params(),c.M().params_->GetSocksProxyConnectionParams(),
 &c.M(),&c.M().net_log());
        break;
      }
      case 30: {  // CREATE_TUNNEL
DCHECK(!c.R().nested_connect_job_);DCHECK(!c.R().TimerIsRunning());
c.M().nested_connect_job_=std::make_unique<HttpProxyConnectJob>(c.M().priority(),
 c.M().socket_tag(),c.M().common_connect_job_params(),c.M().params_->GetHttpProxyConnectionParams(),
 &c.M(),&c.M().net_log());
        break;
      }
      case 31: {  // SAVE_CERTIFICATE
c.M().ssl_cert_request_info_=c.M().nested_connect_job_->GetCertRequestInfo();
        break;
      }
      case 32: {  // HANDSHAKE_TIMER
DCHECK(!c.R().TimerIsRunning());c.base_bridge->duration=base::Seconds(30);
        break;
      }
      case 33: {  // COPY_CONNECT_TIMING
const auto& timing=c.R().nested_connect_job_->connect_timing();
c.M().connect_timing_.connect_start=timing.connect_start;
c.M().connect_timing_.domain_lookup_start=timing.domain_lookup_start;
c.M().connect_timing_.domain_lookup_end=timing.domain_lookup_end;
        break;
      }
      case 34: {  // SET_NEGOTIATION
c.M().ssl_negotiation_started_=true;
        break;
      }
      case 35: {  // STAMP_SSL_START
const auto now=base::TimeTicks::Now();if(!c.Live())return false;
c.M().connect_timing_.ssl_start=now;
        break;
      }
      case 36: {  // IS_TCP
*value=c.R().tcp_connect_job_ != nullptr;
        break;
      }
      case 37: {  // SAVE_ENDPOINT
c.M().endpoint_result_=c.M().nested_connect_job_->GetHostResolverEndpointResult();
        break;
      }
      case 38: {  // SAVE_SERVICE_ENDPOINT
c.M().service_endpoint_result_=c.M().tcp_connect_job_->PassServiceEndpoint();
        break;
      }
      case 39: {  // CONFIG_INIT
c.config.emplace(c.R().params_->ssl_config());
        break;
      }
      case 40: {  // CONFIG_IGNORE_ERRORS
c.config->ignore_certificate_errors=*c.M().common_connect_job_params()->ignore_certificate_errors;
        break;
      }
      case 41: {  // CONFIG_NAK
c.config->network_anonymization_key=c.R().params_->network_anonymization_key();
        break;
      }
      case 42: {  // STALE
*value=c.R().is_connected_via_stale_dns_;
        break;
      }
      case 43: {  // CONFIG_NO_EARLY_DATA
c.config->early_data_enabled=false;
        break;
      }
      case 44: {  // HAS_CONTEXT
*value=c.Context() != nullptr;
        break;
      }
      case 45: {  // ECH_PROVIDER
*value=c.Context()->IsEchEnabled(c.Host().host());
        break;
      }
      case 46: {  // HAS_ENDPOINT
*value=c.R().endpoint_result_.has_value();
        break;
      }
      case 47: {  // HAS_SERVICE_ENDPOINT
*value=c.R().service_endpoint_result_.has_value();
        break;
      }
      case 48: {  // CONFIG_ECH_RETRY
c.config->ech_config_list=*c.R().ech_retry_configs_;
        break;
      }
      case 49: {  // CONFIG_ECH_ENDPOINT
c.config->ech_config_list=c.R().endpoint_result_->metadata.ech_config_list;
        break;
      }
      case 50: {  // CONFIG_ECH_SERVICE
c.config->ech_config_list=c.R().service_endpoint_result_->metadata.ech_config_list;
        break;
      }
      case 51: {  // CONFIG_HAS_ECH
*value=!c.config->ech_config_list.empty();
        break;
      }
      case 52: {  // EXPECT_DIRECT
DCHECK_EQ(c.R().params_->GetConnectionType(),SSLSocketParams::DIRECT);
        break;
      }
      case 53: {  // SHOULD_TRUST_IDS
*value=c.Context()->config().ShouldAdvertiseTrustAnchorIDs();
        break;
      }
      case 54: {  // CONFIG_TRUST_IDS
c.config->trust_anchor_ids=c.Context()->config().SelectAllTrustAnchorIDs();
        break;
      }
      case 55: {  // CONFIG_PADDING
c.config->server_padding_to_request=c.Context()->config().RequestServerPadding();
        break;
      }
      case 56: {  // LOG_SSL
c.M().net_log().AddEvent(NetLogEventType::SSL_CONNECT_JOB_SSL_CONNECT,[&c]{
base::DictValue dict;c.log_output=&dict;c.Run(25);c.log_output=nullptr;return dict;});
        break;
      }
      case 57: {  // LOG_ECH_FLAG
c.log_output->Set("ech_enabled",argument != 0);
        break;
      }
      case 58: {  // LOG_ECH_BYTES
c.log_output->Set("ech_config_list",NetLogBinaryValue(c.config->ech_config_list));
        break;
      }
      case 59: {  // CONFIG_HAS_TRUST
*value=c.config->trust_anchor_ids.has_value();
        break;
      }
      case 60: {  // LOG_TRUST
c.log_output->Set("selected_trust_anchor_ids",x509_util::TrustAnchorIDsToString(
 x509_util::ParseTlsTrustAnchorIDs(*c.config->trust_anchor_ids)));
        break;
      }
      case 61: {  // CONFIG_HAS_PADDING
*value=c.config->server_padding_to_request.has_value();
        break;
      }
      case 62: {  // LOG_PADDING
c.log_output->Set("requested_server_padding",c.config->server_padding_to_request.value());
        break;
      }
      case 63: {  // CREATE_SSL
c.M().ssl_socket_=c.M().client_socket_factory()->CreateSSLClientSocket(
 c.M().ssl_client_context(),std::move(c.M().nested_socket_),c.R().params_->host_and_port(),*c.config);
        break;
      }
      case 64: {  // CONNECT_SSL
*value=c.M().ssl_socket_->Connect(c.R().callback_);
        break;
      }
      case 65: {  // STAMP_SSL_END
const auto now=base::TimeTicks::Now();if(!c.Live())return false;
c.M().connect_timing_.ssl_end=now;
        break;
      }
      case 66: {  // HAS_PEER
*value=!c.R().server_address_.address().empty();
        break;
      }
      case 67: {  // APPEND_PEER_ERROR
c.M().connection_attempts_.emplace_back(c.R().server_address_,static_cast<int>(argument));
        break;
      }
      case 68: {  // CLEAR_PEER
c.M().server_address_=IPEndPoint();
        break;
      }
      case 69: {  // DISABLE_STALE
c.M().disable_stale_dns_=true;
        break;
      }
      case 70: {  // LEGACY_FALLBACK
*value=c.R().disable_legacy_crypto_with_fallback_;
        break;
      }
      case 71: {  // DISABLE_FALLBACK
c.M().disable_legacy_crypto_with_fallback_=false;
        break;
      }
      case 72: {  // ENDPOINT_ECH
*value=!c.R().endpoint_result_->metadata.ech_config_list.empty();
        break;
      }
      case 73: {  // SERVICE_ECH
*value=!c.R().service_endpoint_result_->metadata.ech_config_list.empty();
        break;
      }
      case 74: {  // SAVE_ECH_RETRY
c.M().ech_retry_configs_=c.M().ssl_socket_->GetECHRetryConfigs();
        break;
      }
      case 75: {  // EXPECT_ECH_CAPABLE
DCHECK(argument != 0);
        break;
      }
      case 76: {  // LOG_ECH_RETRY
c.M().net_log().AddEvent(NetLogEventType::SSL_CONNECT_JOB_RESTART_WITH_ECH_CONFIG_LIST,[&c]{
 return base::DictValue().Set("bytes",NetLogBinaryValue(*c.R().ech_retry_configs_));});
        break;
      }
      case 77: {  // RECORD_SSL_RESULT
SSLClientSocket::RecordSSLConnectResult(c.M().ssl_socket_.get(),
 static_cast<int>(argument),c.ech_capable,c.ech_enabled,c.R().ech_retry_configs_,c.R().connect_timing_);
        break;
      }
      case 78: {  // SET_CAPABLE
c.ech_capable=argument != 0;
        break;
      }
      case 79: {  // SET_ENABLED
c.ech_enabled=argument != 0;
        break;
      }
      case 80: {  // PUBLISH_SSL
c.base_socket=std::move(c.M().ssl_socket_);c.base_aliases.emplace(std::move(c.M().dns_aliases_));c.PrepareSocket();
        break;
      }
      case 81: {  // CREATE_CERT_REQUEST
c.M().ssl_cert_request_info_=base::MakeRefCounted<SSLCertRequestInfo>();
        break;
      }
      case 82: {  // GET_CERT_REQUEST
c.M().ssl_socket_->GetSSLCertRequestInfo(c.M().ssl_cert_request_info_.get());
        break;
      }
      case 83: {  // RESET_TIMER
c.base_bridge->duration=base::TimeDelta();
        break;
      }
      case 84: {  // CLEAR_NESTED_SOCKET
c.M().nested_socket_=nullptr;
        break;
      }
      case 85: {  // CLEAR_SSL_SOCKET
c.M().ssl_socket_=nullptr;
        break;
      }
      case 86: {  // CLEAR_CERT
c.M().ssl_cert_request_info_=nullptr;
        break;
      }
      case 87: {  // CLEAR_NEGOTIATION
c.M().ssl_negotiation_started_=false;
        break;
      }
      case 88: {  // CLEAR_STALE
c.M().is_connected_via_stale_dns_=false;
        break;
      }
      case 89: {  // CLEAR_RESOLVE
c.M().resolve_error_info_=ResolveErrorInfo();
        break;
      }
      case 90: {  // PRIORITY_NESTED
c.M().nested_connect_job_->ChangePriority(static_cast<RequestPriority>(argument));
        break;
      }
      case 91: {  // NOTIFY_COMPLETE
*value=argument; // Direct safe BASE completion.
        break;
      }
      case 92: {  // BAD_STATE
NOTREACHED() << "bad state";
      }
      case 93: {  // EXPECT_OK
DCHECK_EQ(OK,argument);
        break;
      }
      case 94: {  // TRACE_DRIVE
TRACE_EVENT0(NetTracingCategory(),"SSLConnectJob::DoLoop");
*value=c.Run(26,argument);
        break;
      }
      case 95: {  // TRACE_SSL
TRACE_EVENT0(NetTracingCategory(),"SSLConnectJob::DoSSLConnect");
*value=c.Run(27);
        break;
      }
      case 96: {  // PARAMS_IS_SOCKS
*value=c.params_is_socks;
        break;
      }
      case 97: {  // PARAMS_IS_HTTP
*value=c.params_is_http;
        break;
      }
      default: return false;
    }
    // Only invocation-local liveness is read after any deleting callback.
    return c.Live();
  }
};


namespace {

// Handshake timeout and ECH selection are dispatched by the Rust policy.

}  // namespace

SSLSocketParams::SSLSocketParams(
    ConnectJobParams nested_params,
    const HostPortPair& host_and_port,
    const SSLConfig& ssl_config,
    NetworkAnonymizationKey network_anonymization_key)
    : nested_params_(nested_params),
      host_and_port_(host_and_port),
      ssl_config_(ssl_config),
      network_anonymization_key_(network_anonymization_key) {
  CHECK(!nested_params_.is_ssl());
}

SSLSocketParams::~SSLSocketParams() = default;

SSLSocketParams::ConnectionType SSLSocketParams::GetConnectionType() const {
  SSLConnectJobFamilyRustBridge c;c.params_is_socks=nested_params_.is_socks();
  c.params_is_http=nested_params_.is_http_proxy();
  return static_cast<ConnectionType>(c.Run(1));
}

std::unique_ptr<SSLConnectJob> SSLConnectJob::Factory::Create(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    scoped_refptr<SSLSocketParams> params,
    ConnectJob::Delegate* delegate,
    const NetLogWithSource* net_log) {
  return std::make_unique<SSLConnectJob>(priority, socket_tag,
                                         common_connect_job_params,
                                         std::move(params), delegate, net_log);
}

SSLConnectJob::SSLConnectJob(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    scoped_refptr<SSLSocketParams> params,
    ConnectJob::Delegate* delegate,
    const NetLogWithSource* net_log)
    : ConnectJob(
          priority,
          socket_tag,
          // The SSLConnectJob's timer is only started during the SSL handshake.
          base::TimeDelta(),
          common_connect_job_params,
          delegate,
          net_log,
          NetLogSourceType::SSL_CONNECT_JOB,
          NetLogEventType::SSL_CONNECT_JOB_CONNECT),
      params_(std::move(params)),
      callback_(base::BindRepeating(&SSLConnectJob::OnIOComplete,
                                    base::Unretained(this))) {}

SSLConnectJob::~SSLConnectJob() {
  connection_jobs_upper_rust::Invocation::Invalidate(static_cast<const ConnectJob*>(this));
  SSLConnectJobFamilyRustBridge c(this);c.Run(2);
}

LoadState SSLConnectJob::GetLoadState() const {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<LoadState>(c.Run(3));
}

bool SSLConnectJob::HasEstablishedConnection() const {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<bool>(c.Run(4));
}

void SSLConnectJob::OnConnectJobComplete(int result, ConnectJob* job) {
  DCHECK_EQ(job, nested_connect_job_.get());
  OnIOComplete(result);
}

void SSLConnectJob::OnNeedsProxyAuth(
    const HttpResponseInfo& response,
    HttpAuthController* auth_controller,
    base::OnceClosure restart_with_auth_callback,
    ConnectJob* job) {
  DCHECK_EQ(next_state_, STATE_TUNNEL_CONNECT_COMPLETE);

  // The timer shouldn't have started running yet, since the handshake only
  // starts after a tunnel has been established through the proxy.
  DCHECK(!TimerIsRunning());

  // Just pass the callback up to the consumer. This class doesn't need to do
  // anything once credentials are provided.
  NotifyDelegateOfProxyAuth(response, auth_controller,
                            std::move(restart_with_auth_callback));
}

ConnectionAttempts SSLConnectJob::GetConnectionAttempts() const {
  ConnectionAttempts result;SSLConnectJobFamilyRustBridge c(this);c.attempts_output=&result;c.Run(5);return result;
}

ResolveErrorInfo SSLConnectJob::GetResolveErrorInfo() const {
  ResolveErrorInfo result;SSLConnectJobFamilyRustBridge c(this);c.resolve_output=&result;c.Run(6);return result;
}

std::optional<ResolutionDetails> SSLConnectJob::GetResolutionDetails() const {
  std::optional<ResolutionDetails> result;SSLConnectJobFamilyRustBridge c(this);c.resolution_output=&result;c.Run(7);return result;
}

bool SSLConnectJob::IsSSLError() const {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<bool>(c.Run(9));
}

scoped_refptr<SSLCertRequestInfo> SSLConnectJob::GetCertRequestInfo() {
  scoped_refptr<SSLCertRequestInfo> result;SSLConnectJobFamilyRustBridge c(this);c.certificate_output=&result;c.Run(8);return result;
}

base::TimeDelta SSLConnectJob::HandshakeTimeoutForTesting() {
  SSLConnectJobFamilyRustBridge c;return base::Microseconds(c.Run(10));
}

void SSLConnectJob::OnIOComplete(int result) {
  SSLConnectJobFamilyRustBridge c(this);
  c.Run(11, result);
}

int SSLConnectJob::DoLoop(int result) {
  SSLConnectJobFamilyRustBridge c(this);
  TRACE_EVENT0(NetTracingCategory(),"SSLConnectJob::DoLoop");
  DCHECK_NE(next_state_,STATE_NONE);
  return static_cast<int>(c.Run(12, result));
}

int SSLConnectJob::DoTransportConnect() {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(13));
}

int SSLConnectJob::DoTransportConnectComplete(int result) {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(14, result));
}

int SSLConnectJob::DoSOCKSConnect() {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(15));
}

int SSLConnectJob::DoSOCKSConnectComplete(int result) {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(16, result));
}

int SSLConnectJob::DoTunnelConnect() {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(17));
}

int SSLConnectJob::DoTunnelConnectComplete(int result) {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(18, result));
}

int SSLConnectJob::DoSSLConnect() {
  SSLConnectJobFamilyRustBridge c(this);
  TRACE_EVENT0(NetTracingCategory(),"SSLConnectJob::DoSSLConnect");
  return static_cast<int>(c.Run(19));
}

int SSLConnectJob::DoSSLConnectComplete(int result) {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(20, result));
}

SSLConnectJob::State SSLConnectJob::GetInitialState(
    SSLSocketParams::ConnectionType connection_type) {
  SSLConnectJobFamilyRustBridge c;return static_cast<State>(c.Run(21,connection_type));
}

int SSLConnectJob::ConnectInternal() {
  SSLConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(22));
}

void SSLConnectJob::ResetStateForRestart() {
  SSLConnectJobFamilyRustBridge c(this);
  c.Run(23);
}

void SSLConnectJob::ChangePriorityInternal(RequestPriority priority) {
  SSLConnectJobFamilyRustBridge c(this);
  c.Run(24, priority);
}


}  // namespace net
