// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "net/socket/connect_job_rust_native.h"
#include "base/check.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "http_proxy_client_socket.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/http_user_agent_settings.h"
#include "net/base/net_errors.h"
#include "net/base/proxy_chain.h"
#include "net/base/session_usage.h"
#include "net/base/task/task_runner.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/quic/quic_context.h"
#include "net/quic/quic_http_utils.h"
#include "net/quic/quic_proxy_client_socket.h"
#include "net/quic/quic_session_key.h"
#include "net/quic/quic_session_pool.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/next_proto.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_client_socket_pool.h"
#include "net/socket/transport_connect_job.h"
#include "net/spdy/multiplexed_session_creation_initiator.h"
#include "net/spdy/spdy_proxy_client_socket.h"
#include "net/spdy/spdy_session.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/spdy/spdy_stream.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace net {
namespace {
GURL MakeProxyUrl(const HttpProxySocketParams&);
int32_t SelectConnectionFamilyIntParam(const std::string&,int32_t);
scoped_refptr<HttpAuthController> CreateConnectionFamilyAuth(const HttpProxySocketParams&,const CommonConnectJobParams*,HostResolver*);
}
namespace { const scoped_refptr<base::SingleThreadTaskRunner>& TaskRunner(RequestPriority); }

// Typed storage/provider primitives only. Sequencing/branches live in safe Rust.
struct HttpProxyConnectJobFamilyRustBridge final
    : connection_jobs_upper_rust::Invocation {
  using Class = HttpProxyConnectJob;
  static_assert(Class::STATE_BEGIN_CONNECT==0);
  static_assert(Class::STATE_TRANSPORT_CONNECT==1);
  static_assert(Class::STATE_TRANSPORT_CONNECT_COMPLETE==2);
  static_assert(Class::STATE_HTTP_PROXY_CONNECT==3);
  static_assert(Class::STATE_HTTP_PROXY_CONNECT_COMPLETE==4);
  static_assert(Class::STATE_SPDY_PROXY_CREATE_STREAM==5);
  static_assert(Class::STATE_SPDY_PROXY_CREATE_STREAM_COMPLETE==6);
  static_assert(Class::STATE_QUIC_PROXY_CREATE_SESSION==7);
  static_assert(Class::STATE_QUIC_PROXY_CREATE_STREAM==8);
  static_assert(Class::STATE_QUIC_PROXY_CREATE_STREAM_COMPLETE==9);
  static_assert(Class::STATE_RESTART_WITH_AUTH==10);
  static_assert(Class::STATE_RESTART_WITH_AUTH_COMPLETE==11);
  static_assert(Class::STATE_NONE==12);
  static_assert(LOAD_STATE_IDLE==0 && LOAD_STATE_ESTABLISHING_PROXY_TUNNEL==9 && LOAD_STATE_CONNECTING==11 && LOAD_STATE_SSL_HANDSHAKE==12);
  static_assert(ProxyServer::SCHEME_HTTP==4 && ProxyServer::SCHEME_HTTPS==32 && ProxyServer::SCHEME_QUIC==64);

  explicit HttpProxyConnectJobFamilyRustBridge(Class* job)
      : Invocation(static_cast<const ConnectJob*>(job)), mutable_owner(job), owner(job) { Register(&Clear); base_bridge.emplace(static_cast<ConnectJob*>(job)); }
  explicit HttpProxyConnectJobFamilyRustBridge(const Class* job)
      : Invocation(static_cast<const ConnectJob*>(job)), mutable_owner(nullptr), owner(job) { Register(&Clear); base_bridge.emplace(static_cast<const ConnectJob*>(job)); }
  HttpProxyConnectJobFamilyRustBridge()
      : Invocation(this), mutable_owner(nullptr), owner(nullptr) { Register(&Clear); }
  ~HttpProxyConnectJobFamilyRustBridge() { base_bridge.reset(); Unregister(); }
  Class& M() { CHECK(Live()); CHECK(mutable_owner); return *mutable_owner; }
  const Class& R() const { CHECK(Live()); CHECK(owner); return *owner; }
  int64_t Run(uint32_t operation, int64_t argument = 0) {
    return connection_jobs_upper_rust::connection_jobs_rust_run(
        6, operation, argument, this, Service);
  }
  raw_ptr<Class> mutable_owner;
  raw_ptr<const Class> owner;
  std::optional<ConnectJobRustBridge> base_bridge; std::unique_ptr<StreamSocket> base_socket; std::optional<base::flat_set<std::string>> base_aliases; void PrepareSocket(){ base_bridge->socket_input=&base_socket; base_bridge->aliases=&base_aliases; }

  raw_ptr<ResolveErrorInfo> resolve_output=nullptr;
  raw_ptr<scoped_refptr<SSLCertRequestInfo>> certificate_output=nullptr;
  raw_ptr<std::string> text_output=nullptr;
  std::optional<SpdySessionKey> key;
  base::WeakPtr<SpdySession> spdy;
  std::optional<base::expected<base::WeakPtr<SpdySession>,int>> session_result;
  raw_ptr<const HttpProxySocketParams> params_input=nullptr;
  raw_ptr<const NetworkQualityEstimator> estimator=nullptr;
  std::optional<base::TimeDelta> rtt;
  base::TimeDelta timeout,min_timeout,max_timeout,latency;
  int32_t ssl_multiplier=0,plain_multiplier=0;
  bool https=false;
  int http_version=0,scheme=0,connect_result=0;
  std::string histogram;
  void (*load_timeout_config)(HttpProxyConnectJobFamilyRustBridge*)=nullptr;
  RequestPriority priority_input=DEFAULT_PRIORITY;
  raw_ptr<const scoped_refptr<base::SingleThreadTaskRunner>> runner_output=nullptr;
  std::optional<GURL> url_output;
  raw_ptr<const CommonConnectJobParams> common_input=nullptr;
  raw_ptr<HostResolver> resolver_input=nullptr;
  scoped_refptr<HttpAuthController> auth_output;
  bool param_valid=false;
  int32_t param_value=0;

  static void Clear(Invocation* value) {
    auto& c = *static_cast<HttpProxyConnectJobFamilyRustBridge*>(value);
    c.mutable_owner = nullptr;
    c.owner = nullptr;
    c.resolve_output = nullptr;
    c.certificate_output = nullptr;
    c.text_output = nullptr;
    c.params_input = nullptr;
    c.estimator = nullptr;
    c.runner_output = nullptr;
    c.common_input = nullptr;
    c.resolver_input = nullptr;
  }

  static bool Service(void* context, uint32_t module, uint32_t action, int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<HttpProxyConnectJobFamilyRustBridge*>(context);
    *value = 0;
    if(module==0 && c.base_bridge){ return ConnectJobRustBridge::Service(&*c.base_bridge,action,argument,value); }
    if (!c.Live() || module != 6) { return false; }
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
      case 3: {  // NESTED_LOAD
*value=c.R().nested_connect_job_->GetLoadState();
        break;
      }
      case 4: {  // NESTED_ESTABLISHED
*value=c.R().nested_connect_job_->HasEstablishedConnection();
        break;
      }
      case 5: {  // ESTABLISHED
*value=c.R().has_established_connection_;
        break;
      }
      case 6: {  // SET_ESTABLISHED
c.M().has_established_connection_=true;
        break;
      }
      case 7: {  // COPY_RESOLVE
*c.resolve_output=c.R().resolve_error_info_;
        break;
      }
      case 8: {  // HAS_CERT
*value=c.R().ssl_cert_request_info_ != nullptr;
        break;
      }
      case 9: {  // COPY_CERT
*c.certificate_output=c.R().ssl_cert_request_info_;
        break;
      }
      case 10: {  // SCHEME
*value=c.R().params_->proxy_server().scheme();
        break;
      }
      case 11: {  // STAMP_START
const auto now=base::TimeTicks::Now();if(!c.Live())return false;c.M().connect_start_time_=now;
        break;
      }
      case 12: {  // ALTERNATE_TIMER
c.base_bridge->duration=Class::AlternateNestedConnectionTimeout(*c.R().params_,c.M().network_quality_estimator());
        break;
      }
      case 13: {  // CREATE_TRANSPORT
c.M().nested_connect_job_=TransportConnectJob::Factory::CreateJob(
c.M().priority(),c.M().socket_tag(),c.M().common_connect_job_params(),
c.M().params_->transport_params(),&c.M(),&c.M().net_log());
        break;
      }
      case 14: {  // TUNNEL
*value=c.R().params_->tunnel();
        break;
      }
      case 15: {  // FIND_EXISTING_SPDY
c.spdy=c.M().common_connect_job_params()->spdy_session_pool->FindAvailableSession(
c.M().CreateSpdySessionKey(),false,false,c.M().net_log());*value=static_cast<bool>(c.spdy);
        break;
      }
      case 16: {  // CREATE_SSL
DCHECK(c.R().params_->is_over_ssl());
c.M().nested_connect_job_=std::make_unique<SSLConnectJob>(c.M().priority(),
c.M().socket_tag(),c.M().common_connect_job_params(),c.M().params_->ssl_params(),&c.M(),&c.M().net_log());
        break;
      }
      case 17: {  // EXPECT_HTTPS
DCHECK_EQ(c.R().params_->proxy_server().scheme(),ProxyServer::SCHEME_HTTPS);
        break;
      }
      case 18: {  // CONNECT_NESTED
*value=c.M().nested_connect_job_->Connect();
        break;
      }
      case 19: {  // SAVE_RESOLVE
c.M().resolve_error_info_=c.M().nested_connect_job_->GetResolveErrorInfo();
        break;
      }
      case 20: {  // FIRST_PROXY
*value=c.R().params_->proxy_chain_index()==0;
        break;
      }
      case 21: {  // ERROR_LATENCY
Class::EmitConnectLatency(NextProto::kProtoUnknown,c.R().params_->proxy_server().scheme(),
Class::HttpConnectResult::kError,base::TimeTicks::Now()-c.R().connect_start_time_);
        break;
      }
      case 22: {  // SAVE_CERT
c.M().ssl_cert_request_info_=c.M().nested_connect_job_->GetCertRequestInfo();
        break;
      }
      case 23: {  // MULTI_PROXY
*value=c.R().params_->proxy_chain().is_multi_proxy();
        break;
      }
      case 24: {  // MARK_PROXY_CERT
DCHECK(c.R().ssl_cert_request_info_);c.M().ssl_cert_request_info_->is_proxy=true;
        break;
      }
      case 25: {  // NEGOTIATED
*value=static_cast<int>(c.M().nested_connect_job_->socket()->GetNegotiatedProtocol());
        break;
      }
      case 26: {  // SUCCESS_LATENCY
Class::EmitConnectLatency(static_cast<NextProto>(argument),c.R().params_->proxy_server().scheme(),
Class::HttpConnectResult::kSuccess,base::TimeTicks::Now()-c.R().connect_start_time_);
        break;
      }
      case 27: {  // PUBLISH_UNTUNNELED
c.base_socket=c.M().nested_connect_job_->PassSocket();c.base_aliases.emplace();c.PrepareSocket();
        break;
      }
      case 28: {  // HANDSHAKE_TIMER
c.base_bridge->duration=base::Seconds(30);
        break;
      }
      case 29: {  // CREATE_HTTP_SOCKET
DCHECK(c.R().params_->tunnel());
c.M().transport_socket_=std::make_unique<HttpProxyClientSocket>(c.M().nested_connect_job_->PassSocket(),
c.M().GetUserAgent(),c.R().params_->endpoint(),c.R().params_->proxy_chain(),c.R().params_->proxy_chain_index(),
c.R().http_auth_controller_,c.M().common_connect_job_params()->proxy_delegate,c.R().params_->traffic_annotation());
        break;
      }
      case 30: {  // CLEAR_NESTED
c.M().nested_connect_job_.reset();
        break;
      }
      case 31: {  // CONNECT_SOCKET
*value=c.M().transport_socket_->Connect(base::BindOnce(&Class::OnIOComplete,base::Unretained(&c.M())));
        break;
      }
      case 32: {  // POST_AUTH
TaskRunner(c.R().priority())->PostTask(FROM_HERE,
base::BindOnce(&Class::OnAuthChallenge,c.M().weak_ptr_factory_.GetWeakPtr()));
        break;
      }
      case 33: {  // PUBLISH_SOCKET
c.base_socket=std::move(c.M().transport_socket_);c.base_aliases.reset();c.PrepareSocket();
        break;
      }
      case 34: {  // FIND_SPDY
c.key.emplace(c.M().CreateSpdySessionKey());
c.spdy=c.M().common_connect_job_params()->spdy_session_pool->FindAvailableSession(*c.key,false,false,c.M().net_log());
*value=static_cast<bool>(c.spdy);
        break;
      }
      case 35: {  // CREATE_SPDY
c.session_result.emplace(c.M().common_connect_job_params()->spdy_session_pool->CreateAvailableSessionFromSocket(
*c.key,c.M().nested_connect_job_->PassSocket(),c.R().nested_connect_job_->connect_timing(),c.M().net_log(),
SpdySessionInitiator::kHttpProxyConnectJob));
        break;
      }
      case 36: {  // SESSION_OK
*value=c.session_result->has_value();
        break;
      }
      case 37: {  // SESSION_ERROR
*value=c.session_result->error();
        break;
      }
      case 38: {  // TAKE_SESSION
c.spdy=std::move(c.session_result->value());
        break;
      }
      case 39: {  // NEW_STREAM_REQUEST
c.M().spdy_stream_request_=std::make_unique<SpdyStreamRequest>();
        break;
      }
      case 40: {  // REQUEST_SPDY_STREAM
*value=c.M().spdy_stream_request_->StartRequest(SPDY_BIDIRECTIONAL_STREAM,c.spdy,
GURL("https://"+c.R().params_->endpoint().ToString()),false,Class::kH2QuicTunnelPriority,
c.M().socket_tag(),c.spdy->net_log(),base::BindOnce(&Class::OnIOComplete,base::Unretained(&c.M())),c.R().params_->traffic_annotation());
        break;
      }
      case 41: {  // CLEAR_STREAM_REQUEST
c.M().spdy_stream_request_.reset();
        break;
      }
      case 42: {  // CREATE_SPDY_SOCKET
auto stream=c.M().spdy_stream_request_->ReleaseStream();
c.M().spdy_stream_request_.reset();DCHECK(stream.get());
c.M().transport_socket_=std::make_unique<SpdyProxyClientSocket>(stream,c.R().params_->proxy_chain(),
c.R().params_->proxy_chain_index(),c.M().GetUserAgent(),c.R().params_->endpoint(),c.M().net_log(),
c.R().http_auth_controller_,c.M().common_connect_job_params()->proxy_delegate);
        break;
      }
      case 43: {  // CREATE_QUIC_REQUEST
DCHECK(c.R().params_->tunnel());DCHECK(!c.M().common_connect_job_params()->quic_supported_versions->empty());
c.M().quic_session_request_=std::make_unique<QuicSessionRequest>(c.M().common_connect_job_params()->quic_session_pool);
        break;
      }
      case 44: {  // REQUEST_QUIC_SESSION
const auto& config=c.R().params_->quic_ssl_config().value();
const auto& server=c.R().params_->proxy_server().host_port_pair();
auto proxies=c.R().params_->proxy_chain().Prefix(c.R().params_->proxy_chain_index());
for(const auto& proxy:proxies.proxy_servers()){CHECK(proxy.is_quic());}
*value=c.M().quic_session_request_->Request(url::SchemeHostPort(url::kHttpsScheme,server.host(),server.port()),
SupportedQuicVersionForProxying(),proxies,c.R().params_->traffic_annotation(),c.M().http_user_agent_settings(),
SessionUsage::kProxy,config.privacy_mode,Class::kH2QuicTunnelPriority,c.M().socket_tag(),
c.R().params_->network_anonymization_key(),c.R().params_->secure_dns_policy(),false,config.GetCertVerifyFlags(),
GURL("https://"+server.ToString()),c.R().params_->target_network(),c.M().net_log(),&c.M().quic_net_error_details_,
MultiplexedSessionCreationInitiator::kUnknown,std::nullopt,CompletionOnceCallback(),
base::BindOnce(&Class::OnIOComplete,base::Unretained(&c.M())));
        break;
      }
      case 45: {  // CLEAR_QUIC_REQUEST
c.M().quic_session_request_.reset();
        break;
      }
      case 46: {  // TAKE_QUIC_SESSION
c.M().quic_session_=c.M().quic_session_request_->ReleaseSessionHandle();
        break;
      }
      case 47: {  // REQUEST_QUIC_STREAM
*value=c.M().quic_session_->RequestStream(false,
base::BindOnce(&Class::OnIOComplete,base::Unretained(&c.M())),c.R().params_->traffic_annotation());
        break;
      }
      case 48: {  // CREATE_QUIC_SOCKET
auto stream=c.M().quic_session_->ReleaseStream();
const uint8_t urgency=ConvertRequestPriorityToQuicPriority(Class::kH2QuicTunnelPriority);
stream->SetPriority(quic::QuicStreamPriority(quic::HttpStreamPriority{urgency,kDefaultPriorityIncremental}));
c.M().transport_socket_=std::make_unique<QuicProxyClientSocket>(std::move(stream),std::move(c.M().quic_session_),
c.R().params_->proxy_chain(),c.R().params_->proxy_chain_index(),c.M().GetUserAgent(),c.R().params_->endpoint(),
c.M().net_log(),c.R().http_auth_controller_,c.M().common_connect_job_params()->proxy_delegate);
        break;
      }
      case 49: {  // RESTART_SOCKET
DCHECK(c.R().transport_socket_);*value=c.M().transport_socket_->RestartWithAuth(
base::BindOnce(&Class::OnIOComplete,base::Unretained(&c.M())));
        break;
      }
      case 50: {  // SOCKET_CONNECTED
*value=c.R().transport_socket_->IsConnected();
        break;
      }
      case 51: {  // RESTARTED
*value=c.R().has_restarted_;
        break;
      }
      case 52: {  // SET_RESTARTED
c.M().has_restarted_=true;
        break;
      }
      case 53: {  // HAS_AUTH
*value=c.R().http_auth_controller_ != nullptr;
        break;
      }
      case 54: {  // AUTH_CLOSED
c.M().http_auth_controller_->OnConnectionClosed();
        break;
      }
      case 55: {  // CLEAR_SOCKET
c.M().transport_socket_.reset();
        break;
      }
      case 56: {  // HAS_SOCKET
*value=c.R().transport_socket_ != nullptr;
        break;
      }
      case 57: {  // PRIORITY_NESTED
c.M().nested_connect_job_->ChangePriority(static_cast<RequestPriority>(argument));
        break;
      }
      case 58: {  // PRIORITY_SOCKET
c.M().transport_socket_->SetStreamPriority(static_cast<RequestPriority>(argument));
        break;
      }
      case 59: {  // TIMEOUT_LATENCY
Class::EmitConnectLatency(NextProto::kProtoUnknown,c.R().params_->proxy_server().scheme(),
Class::HttpConnectResult::kTimedOut,base::TimeTicks::Now()-c.R().connect_start_time_);
        break;
      }
      case 60: {  // STOP_TIMER
c.base_bridge->duration=base::TimeDelta();
        break;
      }
      case 61: {  // NOTIFY_AUTH
c.M().NotifyDelegateOfProxyAuth(*c.M().transport_socket_->GetConnectResponseInfo(),
c.M().transport_socket_->GetAuthController().get(),base::BindOnce(&Class::RestartWithAuthCredentials,c.M().weak_ptr_factory_.GetWeakPtr()));
        break;
      }
      case 62: {  // POST_RESTART
DCHECK(c.R().transport_socket_);TaskRunner(c.R().priority())->PostTask(FROM_HERE,
base::BindOnce(&Class::OnIOComplete,c.M().weak_ptr_factory_.GetWeakPtr(),OK));
        break;
      }
      case 63: {  // HAS_UA
*value=c.R().http_user_agent_settings() != nullptr;
        break;
      }
      case 64: {  // GET_UA
*c.text_output=c.R().http_user_agent_settings()->GetUserAgent();
        break;
      }
      case 65: {  // EMPTY_TEXT
c.text_output->clear();
        break;
      }
      case 66: {  // BUILD_KEY
auto prefix=c.R().params_->proxy_chain().Prefix(c.R().params_->proxy_chain_index());
DCHECK(c.R().params_->proxy_chain_index()!=0||prefix.is_direct());
c.key.emplace(c.R().params_->proxy_server().host_port_pair(),PRIVACY_MODE_DISABLED,prefix,
SessionUsage::kProxy,c.R().socket_tag(),c.R().params_->network_anonymization_key(),c.R().params_->secure_dns_policy(),true,c.R().params_->target_network());
        break;
      }
      case 67: {  // NOTIFY_COMPLETE
*value=argument; // Direct safe BASE completion.
        break;
      }
      case 68: {  // BAD_STATE
NOTREACHED() << "bad state";
      }
      case 69: {  // EXPECT_OK
DCHECK_EQ(OK,argument);
        break;
      }
      case 70: {  // EXPECT_NOT_PENDING
DCHECK_NE(ERR_IO_PENDING,argument);
        break;
      }
      case 71: {  // HAS_ESTIMATOR
*value=c.estimator != nullptr;
        break;
      }
      case 72: {  // READ_RTT
c.rtt=c.estimator->GetHttpRTT();*value=c.rtt.has_value();
        break;
      }
      case 73: {  // READ_TIMEOUT_CONFIG
c.load_timeout_config(&c);
        break;
      }
      case 74: {  // IS_HTTPS_INPUT
*value=c.https;
        break;
      }
      case 75: {  // SSL_MULTIPLIER
*value=c.ssl_multiplier;
        break;
      }
      case 76: {  // PLAIN_MULTIPLIER
*value=c.plain_multiplier;
        break;
      }
      case 77: {  // MULTIPLY_RTT
c.timeout=static_cast<int32_t>(argument)*c.rtt.value();
        break;
      }
      case 78: {  // BELOW_MIN
*value=c.timeout<c.min_timeout;
        break;
      }
      case 79: {  // ABOVE_MAX
*value=c.max_timeout<c.timeout;
        break;
      }
      case 80: {  // TAKE_MIN
c.timeout=c.min_timeout;
        break;
      }
      case 81: {  // TAKE_MAX
c.timeout=c.max_timeout;
        break;
      }
      case 82: {  // TIMEOUT_MICROS
*value=c.timeout.InMicroseconds();
        break;
      }
      case 83: {  // HTTP_VERSION_INPUT
*value=c.http_version;
        break;
      }
      case 84: {  // SCHEME_INPUT
*value=c.scheme;
        break;
      }
      case 85: {  // RESULT_INPUT
*value=c.connect_result;
        break;
      }
      case 86: {  // HISTOGRAM_PREFIX
c.histogram="Net.HttpProxy.ConnectLatency.";
        break;
      }
      case 87: {  // HISTOGRAM_HTTP1
c.histogram+="Http1";
        break;
      }
      case 88: {  // HISTOGRAM_HTTP2
c.histogram+="Http2";
        break;
      }
      case 89: {  // HISTOGRAM_HTTP3
c.histogram+="Http3";
        break;
      }
      case 90: {  // HISTOGRAM_HTTP
c.histogram+=".Http";
        break;
      }
      case 91: {  // HISTOGRAM_HTTPS
c.histogram+=".Https";
        break;
      }
      case 92: {  // HISTOGRAM_QUIC
c.histogram+=".Quic";
        break;
      }
      case 93: {  // HISTOGRAM_SUCCESS
c.histogram+=".Success";
        break;
      }
      case 94: {  // HISTOGRAM_ERROR
c.histogram+=".Error";
        break;
      }
      case 95: {  // HISTOGRAM_TIMEOUT
c.histogram+=".TimedOut";
        break;
      }
      case 96: {  // EMIT_HISTOGRAM
base::UmaHistogramMediumTimes(c.histogram,c.latency);
        break;
      }
      case 97: {  // TASK_SCHEDULER
*value=features::kNetTaskSchedulerHttpProxyConnectJob.Get();
        break;
      }
      case 98: {  // TASK_NET
c.runner_output=&net::GetTaskRunner(c.priority_input);
        break;
      }
      case 99: {  // TASK_DEFAULT
c.runner_output=&base::SingleThreadTaskRunner::GetCurrentDefault();
        break;
      }
      case 100: {  // PARAM_OVER_SSL
*value=c.params_input->is_over_ssl();
        break;
      }
      case 101: {  // PARAM_OVER_QUIC
*value=c.params_input->is_over_quic();
        break;
      }
      case 102: {  // URL_HTTPS
c.url_output.emplace("https://"+c.params_input->proxy_server().host_port_pair().ToString());
        break;
      }
      case 103: {  // URL_HTTP
c.url_output.emplace("http://"+c.params_input->proxy_server().host_port_pair().ToString());
        break;
      }
      case 104: {  // PARAM_TUNNEL
*value=c.params_input->tunnel();
        break;
      }
      case 105: {  // CREATE_AUTH
c.auth_output=base::MakeRefCounted<HttpAuthController>(HttpAuth::AUTH_PROXY,
MakeProxyUrl(*c.params_input),c.params_input->network_anonymization_key(),
c.common_input->http_auth_cache,c.common_input->http_auth_handler_factory,c.resolver_input);
        break;
      }
      case 106: {  // PARAM_PRESENT
*value=c.param_valid;
        break;
      }
      case 107: {  // PARAM_VALUE
*value=c.param_value;
        break;
      }
      default: return false;
    }
    // Only invocation-local liveness is read after any deleting callback.
    return c.Live();
  }
};


namespace {

// HttpProxyConnectJobs will time out after this many seconds.  Note this is in
// addition to the timeout for the transport socket.
// Timeout selection is implemented by the Rust family policy.

class HttpProxyTimeoutExperiments {
 public:
  HttpProxyTimeoutExperiments() { Init(); }

  ~HttpProxyTimeoutExperiments() = default;

  void Init() {
    min_proxy_connection_timeout_ =
        base::Seconds(GetInt32Param("min_proxy_connection_timeout_seconds", 8));
    max_proxy_connection_timeout_ = base::Seconds(
        GetInt32Param("max_proxy_connection_timeout_seconds", 30));
    ssl_http_rtt_multiplier_ = GetInt32Param("ssl_http_rtt_multiplier", 10);
    non_ssl_http_rtt_multiplier_ =
        GetInt32Param("non_ssl_http_rtt_multiplier", 5);

    DCHECK_LT(0, ssl_http_rtt_multiplier_);
    DCHECK_LT(0, non_ssl_http_rtt_multiplier_);
    DCHECK_LE(base::TimeDelta(), min_proxy_connection_timeout_);
    DCHECK_LE(base::TimeDelta(), max_proxy_connection_timeout_);
    DCHECK_LE(min_proxy_connection_timeout_, max_proxy_connection_timeout_);
  }

  base::TimeDelta min_proxy_connection_timeout() const {
    return min_proxy_connection_timeout_;
  }
  base::TimeDelta max_proxy_connection_timeout() const {
    return max_proxy_connection_timeout_;
  }
  int32_t ssl_http_rtt_multiplier() const { return ssl_http_rtt_multiplier_; }
  int32_t non_ssl_http_rtt_multiplier() const {
    return non_ssl_http_rtt_multiplier_;
  }

 private:
  // Returns the value of the parameter |param_name| for the field trial
  // "NetAdaptiveProxyConnectionTimeout". If the value of the parameter is
  // unavailable, then |default_value| is available.
  static int32_t GetInt32Param(const std::string& param_name,
                               int32_t default_value) {
  return SelectConnectionFamilyIntParam(param_name,default_value);
}

  // For secure proxies, the connection timeout is set to
  // |ssl_http_rtt_multiplier_| times the HTTP RTT estimate. For insecure
  // proxies, the connection timeout is set to |non_ssl_http_rtt_multiplier_|
  // times the HTTP RTT estimate. In either case, the connection timeout
  // is clamped to be between |min_proxy_connection_timeout_| and
  // |max_proxy_connection_timeout_|.
  base::TimeDelta min_proxy_connection_timeout_;
  base::TimeDelta max_proxy_connection_timeout_;
  int32_t ssl_http_rtt_multiplier_;
  int32_t non_ssl_http_rtt_multiplier_;
};

HttpProxyTimeoutExperiments* GetProxyTimeoutExperiments() {
  static HttpProxyTimeoutExperiments proxy_timeout_experiments;
  return &proxy_timeout_experiments;
}

// Make a URL for a proxy, for use in proxy auth challenges.
GURL MakeProxyUrl(const HttpProxySocketParams& params) {
  HttpProxyConnectJobFamilyRustBridge c;c.params_input=&params;c.Run(31);return std::move(*c.url_output);
}

const scoped_refptr<base::SingleThreadTaskRunner>& TaskRunner(
    net::RequestPriority priority) {
  HttpProxyConnectJobFamilyRustBridge c;c.priority_input=priority;c.Run(30);return *c.runner_output;
}

}  // namespace

HttpProxySocketParams::HttpProxySocketParams(
    ConnectJobParams nested_params,
    const HostPortPair& endpoint,
    const ProxyChain& proxy_chain,
    size_t proxy_chain_index,
    bool tunnel,
    const NetworkTrafficAnnotationTag traffic_annotation,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    handles::NetworkHandle target_network)
    : HttpProxySocketParams(std::move(nested_params),
                            std::nullopt,
                            endpoint,
                            proxy_chain,
                            proxy_chain_index,
                            tunnel,
                            std::move(traffic_annotation),
                            network_anonymization_key,
                            secure_dns_policy,
                            target_network) {}

HttpProxySocketParams::HttpProxySocketParams(
    SSLConfig quic_ssl_config,
    const HostPortPair& endpoint,
    const ProxyChain& proxy_chain,
    size_t proxy_chain_index,
    bool tunnel,
    const NetworkTrafficAnnotationTag traffic_annotation,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    handles::NetworkHandle target_network)
    : HttpProxySocketParams(std::nullopt,
                            std::move(quic_ssl_config),
                            endpoint,
                            proxy_chain,
                            proxy_chain_index,
                            tunnel,
                            std::move(traffic_annotation),
                            network_anonymization_key,
                            secure_dns_policy,
                            target_network) {}

HttpProxySocketParams::HttpProxySocketParams(
    std::optional<ConnectJobParams> nested_params,
    std::optional<SSLConfig> quic_ssl_config,
    const HostPortPair& endpoint,
    const ProxyChain& proxy_chain,
    size_t proxy_chain_index,
    bool tunnel,
    const NetworkTrafficAnnotationTag traffic_annotation,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    handles::NetworkHandle target_network)
    : nested_params_(std::move(nested_params)),
      quic_ssl_config_(std::move(quic_ssl_config)),
      endpoint_(endpoint),
      proxy_chain_(proxy_chain),
      proxy_chain_index_(proxy_chain_index),
      tunnel_(tunnel),
      network_anonymization_key_(network_anonymization_key),
      traffic_annotation_(traffic_annotation),
      secure_dns_policy_(secure_dns_policy),
      target_network_(target_network) {
  DCHECK(!proxy_chain_.is_direct());
  DCHECK(proxy_chain_.IsValid());
  CHECK(proxy_chain_index_ < proxy_chain_.length());

  // This is either a connection to an HTTP proxy,an SSL proxy, or a QUIC proxy.
  DCHECK(nested_params_ || quic_ssl_config_);
  DCHECK(!(nested_params_ && quic_ssl_config_));

  // Only supports proxy endpoints without scheme for now.
  // TODO(crbug.com/40181080): Handle scheme.
  if (is_over_transport()) {
    DCHECK(std::holds_alternative<HostPortPair>(
        nested_params_->transport()->destination()));
  } else if (is_over_ssl() && nested_params_->ssl()->GetConnectionType() ==
                                  SSLSocketParams::ConnectionType::DIRECT) {
    DCHECK(std::holds_alternative<HostPortPair>(
        nested_params_->ssl()->GetDirectConnectionParams()->destination()));
  }
}

HttpProxySocketParams::~HttpProxySocketParams() = default;

std::unique_ptr<HttpProxyConnectJob> HttpProxyConnectJob::Factory::Create(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    scoped_refptr<HttpProxySocketParams> params,
    ConnectJob::Delegate* delegate,
    const NetLogWithSource* net_log) {
  return std::make_unique<HttpProxyConnectJob>(
      priority, socket_tag, common_connect_job_params, std::move(params),
      delegate, net_log);
}

HttpProxyConnectJob::HttpProxyConnectJob(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    scoped_refptr<HttpProxySocketParams> params,
    ConnectJob::Delegate* delegate,
    const NetLogWithSource* net_log)
    : ConnectJob(priority,
                 socket_tag,
                 base::TimeDelta() /* The socket takes care of timeouts */,
                 common_connect_job_params,
                 delegate,
                 net_log,
                 NetLogSourceType::HTTP_PROXY_CONNECT_JOB,
                 NetLogEventType::HTTP_PROXY_CONNECT_JOB_CONNECT),
      params_(std::move(params)),
      http_auth_controller_(
          CreateConnectionFamilyAuth(*params_,common_connect_job_params,host_resolver())) {}

HttpProxyConnectJob::~HttpProxyConnectJob() { connection_jobs_upper_rust::Invocation::Invalidate(static_cast<const ConnectJob*>(this)); }

const RequestPriority HttpProxyConnectJob::kH2QuicTunnelPriority =
    DEFAULT_PRIORITY;

LoadState HttpProxyConnectJob::GetLoadState() const {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<LoadState>(c.Run(0));
}

bool HttpProxyConnectJob::HasEstablishedConnection() const {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<bool>(c.Run(1));
}

ResolveErrorInfo HttpProxyConnectJob::GetResolveErrorInfo() const {
  ResolveErrorInfo result;HttpProxyConnectJobFamilyRustBridge c(this);c.resolve_output=&result;c.Run(2);return result;
}

bool HttpProxyConnectJob::IsSSLError() const {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<bool>(c.Run(3));
}

scoped_refptr<SSLCertRequestInfo> HttpProxyConnectJob::GetCertRequestInfo() {
  scoped_refptr<SSLCertRequestInfo> result;HttpProxyConnectJobFamilyRustBridge c(this);c.certificate_output=&result;c.Run(4);return result;
}

void HttpProxyConnectJob::OnConnectJobComplete(int result, ConnectJob* job) {
  DCHECK_EQ(nested_connect_job_.get(), job);
  DCHECK_EQ(next_state_, STATE_TRANSPORT_CONNECT_COMPLETE);
  OnIOComplete(result);
}

void HttpProxyConnectJob::OnNeedsProxyAuth(
    const HttpResponseInfo& response,
    HttpAuthController* auth_controller,
    base::OnceClosure restart_with_auth_callback,
    ConnectJob* job) {
  // None of the nested ConnectJob used by this class can encounter auth
  // challenges. Instead, the challenges are returned by the ProxyClientSocket
  // implementations after nested_connect_job_ has already established a
  // connection.
  NOTREACHED();
}

base::TimeDelta HttpProxyConnectJob::AlternateNestedConnectionTimeout(
    const HttpProxySocketParams& params,
    const NetworkQualityEstimator* network_quality_estimator) {

  HttpProxyConnectJobFamilyRustBridge c;c.https=params.proxy_server().is_https();c.estimator=network_quality_estimator;
  c.load_timeout_config=[](HttpProxyConnectJobFamilyRustBridge* input){
  input->ssl_multiplier=GetProxyTimeoutExperiments()->ssl_http_rtt_multiplier();
  input->plain_multiplier=GetProxyTimeoutExperiments()->non_ssl_http_rtt_multiplier();
  input->min_timeout=GetProxyTimeoutExperiments()->min_proxy_connection_timeout();
  input->max_timeout=GetProxyTimeoutExperiments()->max_proxy_connection_timeout();};
  return base::Microseconds(c.Run(5));
}

base::TimeDelta HttpProxyConnectJob::TunnelTimeoutForTesting() {
  HttpProxyConnectJobFamilyRustBridge c;return base::Microseconds(c.Run(6));
}

void HttpProxyConnectJob::UpdateFieldTrialParametersForTesting() {
  GetProxyTimeoutExperiments()->Init();
}

int HttpProxyConnectJob::ConnectInternal() {
  DCHECK_EQ(next_state_,STATE_NONE);
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(7));
}

ProxyServer::Scheme HttpProxyConnectJob::GetProxyServerScheme() const {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<ProxyServer::Scheme>(c.Run(8));
}

void HttpProxyConnectJob::OnIOComplete(int result) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  c.Run(9, result);
}

void HttpProxyConnectJob::RestartWithAuthCredentials() {
  DCHECK_EQ(next_state_,STATE_NONE);
  HttpProxyConnectJobFamilyRustBridge c(this);
  c.Run(10);
}

int HttpProxyConnectJob::DoLoop(int result) {
  DCHECK_NE(next_state_,STATE_NONE);
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(11, result));
}

int HttpProxyConnectJob::DoBeginConnect() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(12));
}

int HttpProxyConnectJob::DoTransportConnect() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(13));
}

int HttpProxyConnectJob::DoTransportConnectComplete(int result) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(14, result));
}

int HttpProxyConnectJob::DoHttpProxyConnect() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(15));
}

int HttpProxyConnectJob::DoHttpProxyConnectComplete(int result) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(16, result));
}

int HttpProxyConnectJob::DoSpdyProxyCreateStream() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(17));
}

int HttpProxyConnectJob::DoSpdyProxyCreateStreamComplete(int result) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(18, result));
}

int HttpProxyConnectJob::DoQuicProxyCreateSession() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(19));
}

int HttpProxyConnectJob::DoQuicProxyCreateStream(int result) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(20, result));
}

int HttpProxyConnectJob::DoQuicProxyCreateStreamComplete(int result) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(21, result));
}

int HttpProxyConnectJob::DoRestartWithAuth() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(22));
}

int HttpProxyConnectJob::DoRestartWithAuthComplete(int result) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(23, result));
}

void HttpProxyConnectJob::ChangePriorityInternal(RequestPriority priority) {
  HttpProxyConnectJobFamilyRustBridge c(this);
  c.Run(24, priority);
}

void HttpProxyConnectJob::OnTimedOutInternal() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  c.Run(25);
}

void HttpProxyConnectJob::OnAuthChallenge() {
  HttpProxyConnectJobFamilyRustBridge c(this);
  c.Run(26);
}

std::string HttpProxyConnectJob::GetUserAgent() const {
  std::string result;HttpProxyConnectJobFamilyRustBridge c(this);c.text_output=&result;c.Run(27);return result;
}

SpdySessionKey HttpProxyConnectJob::CreateSpdySessionKey() const {
  HttpProxyConnectJobFamilyRustBridge c(this);c.Run(28);return std::move(*c.key);
}

// static
void HttpProxyConnectJob::EmitConnectLatency(NextProto http_version,
                                             ProxyServer::Scheme scheme,
                                             HttpConnectResult result,
                                             base::TimeDelta latency) {
  HttpProxyConnectJobFamilyRustBridge c;c.http_version=static_cast<int>(http_version);
  c.scheme=scheme;c.connect_result=static_cast<int>(result);c.latency=latency;c.Run(29);
}

namespace {
int32_t SelectConnectionFamilyIntParam(const std::string& name,int32_t fallback){
 HttpProxyConnectJobFamilyRustBridge c;c.param_value=0;
 c.param_valid=base::StringToInt(base::GetFieldTrialParamValue("NetAdaptiveProxyConnectionTimeout",name),&c.param_value);
 return static_cast<int32_t>(c.Run(33,fallback));
}
scoped_refptr<HttpAuthController> CreateConnectionFamilyAuth(const HttpProxySocketParams& params,const CommonConnectJobParams* common,HostResolver* resolver){
 HttpProxyConnectJobFamilyRustBridge c;c.params_input=&params;c.common_input=common;c.resolver_input=resolver;c.Run(34);return std::move(c.auth_output);
}
}  // namespace

}  // namespace net
