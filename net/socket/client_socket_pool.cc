// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "base/check.h"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/proxy_chain.h"
#include "net/base/session_usage.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_pool_manager.h"
#include "net/socket/connect_job.h"
#include "net/socket/connect_job_factory.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/stream_socket.h"
#include "net/spdy/spdy_session.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/ssl/ssl_config.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

namespace net {
namespace {
OnHostResolutionCallbackResult OnHostResolution(SpdySessionPool*,const SpdySessionKey&,bool,const HostPortPair&,const HostResolverEndpointsOrServiceEndpoints&,const base::flat_set<std::string>&);
SocketPoolAdditionalCapacity CreateConnectionFamilyCapacity(size_t,const ProxyChain&);
NetworkAnonymizationKey SelectConnectionFamilyKey(NetworkAnonymizationKey&);
}

// Typed storage/provider primitives only. Sequencing/branches live in safe Rust.
struct ClientSocketPoolFamilyRustBridge final
    : connection_jobs_upper_rust::Invocation {
  using Class = ClientSocketPool;

  explicit ClientSocketPoolFamilyRustBridge(Class* job)
      : Invocation(job), mutable_owner(job), owner(job) { Register(&Clear);  }
  explicit ClientSocketPoolFamilyRustBridge(const Class* job)
      : Invocation(job), mutable_owner(nullptr), owner(job) { Register(&Clear);  }
  ClientSocketPoolFamilyRustBridge()
      : Invocation(this), mutable_owner(nullptr), owner(nullptr) { Register(&Clear); }
  ~ClientSocketPoolFamilyRustBridge() {  Unregister(); }
  Class& M() { CHECK(Live()); CHECK(mutable_owner); return *mutable_owner; }
  const Class& R() const { CHECK(Live()); CHECK(owner); return *owner; }
  int64_t Run(uint32_t operation, int64_t argument = 0) {
    return connection_jobs_upper_rust::connection_jobs_rust_run(
        8, operation, argument, this, Service);
  }
  raw_ptr<Class> mutable_owner;
  raw_ptr<const Class> owner;


  raw_ptr<const Class::GroupId> group_input=nullptr;
  raw_ptr<const ProxyChain> proxy_input=nullptr;
  raw_ptr<NetworkAnonymizationKey> nak_input=nullptr;
  std::optional<NetworkAnonymizationKey> nak_output;
  std::optional<SocketPoolAdditionalCapacity> capacity_output;
  size_t capacity_input=0;
  raw_ptr<std::string> text_output=nullptr;
  std::string_view piece;
  raw_ptr<base::DictValue> dict_output=nullptr;
  raw_ptr<const NetLogWithSource> log_input=nullptr;
  raw_ptr<scoped_refptr<Class::SocketParams>> params_input=nullptr;
  raw_ptr<const std::optional<NetworkTrafficAnnotationTag>> annotation_input=nullptr;
  RequestPriority priority_input=DEFAULT_PRIORITY;
  raw_ptr<SocketTag> tag_input=nullptr;
  raw_ptr<ConnectJob::Delegate> delegate_input=nullptr;
  std::unique_ptr<ConnectJob> job_output;
  OnHostResolutionCallback resolution_callback;
  raw_ptr<SpdySessionPool> spdy_input=nullptr;
  raw_ptr<const SpdySessionKey> key_input=nullptr;
  bool websockets_input=false;
  raw_ptr<const HostPortPair> host_input=nullptr;
  raw_ptr<const HostResolverEndpointsOrServiceEndpoints> endpoints_input=nullptr;
  raw_ptr<const base::flat_set<std::string>> aliases_input=nullptr;
  base::TimeDelta duration_input;
  ConnectJobFactory::AlpnMode alpn=ConnectJobFactory::AlpnMode::kHttpAll;
  bool force_tunnel=false;

  static void Clear(Invocation* value) {
    auto& c = *static_cast<ClientSocketPoolFamilyRustBridge*>(value);
    c.mutable_owner = nullptr;
    c.owner = nullptr;
    c.group_input = nullptr;
    c.proxy_input = nullptr;
    c.nak_input = nullptr;
    c.text_output = nullptr;
    c.dict_output = nullptr;
    c.log_input = nullptr;
    c.params_input = nullptr;
    c.annotation_input = nullptr;
    c.tag_input = nullptr;
    c.delegate_input = nullptr;
    c.spdy_input = nullptr;
    c.key_input = nullptr;
    c.host_input = nullptr;
    c.endpoints_input = nullptr;
    c.aliases_input = nullptr;
  }

  static bool Service(void* context, uint32_t module, uint32_t action, int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<ClientSocketPoolFamilyRustBridge*>(context);
    *value = 0;

    if (!c.Live() || module != 8) { return false; }
    if(action==UINT32_MAX){*value=1;return true;}
    switch (action) {
      case 0: {  // PRIVACY
*value=static_cast<int>(c.group_input->privacy_mode());
        break;
      }
      case 1: {  // SECURE_DNS
*value=static_cast<int>(c.group_input->secure_dns_policy());
        break;
      }
      case 2: {  // DISABLE_FETCH
*value=c.group_input->disable_cert_network_fetches();
        break;
      }
      case 3: {  // TARGET_NETWORK
*value=c.group_input->target_network();
        break;
      }
      case 4: {  // PARTITION
*value=NetworkAnonymizationKey::IsPartitioningEnabled();
        break;
      }
      case 5: {  // PIECE
switch(argument){case 0:c.piece="";break;case 1:c.piece="pm/";break;
case 2:c.piece="pmwocc/";break;case 3:c.piece="pmpsa/";break;
case 4:c.piece="dsd/";break;case 5:c.piece="dns_bootstrap/";break;
default:NOTREACHED();}
        break;
      }
      case 6: {  // APPEND_PIECE
c.text_output->append(c.piece);
        break;
      }
      case 7: {  // APPEND_DISABLE
c.text_output->append("disable_cert_network_fetches/");
        break;
      }
      case 8: {  // APPEND_TARGET
c.text_output->append(base::StrCat({"target_network=",base::ToString(c.group_input->target_network()),"/"}));
        break;
      }
      case 9: {  // APPEND_DESTINATION
c.text_output->append(c.group_input->destination().Serialize());
        break;
      }
      case 10: {  // APPEND_NAK
c.text_output->append(base::StrCat({" <",c.group_input->network_anonymization_key().ToDebugString(),">"}));
        break;
      }
      case 11: {  // CLEAR_TEXT
c.text_output->clear();
        break;
      }
      case 12: {  // COPY_NAK
c.nak_output.emplace(std::move(*c.nak_input));
        break;
      }
      case 13: {  // EMPTY_NAK
c.nak_output.emplace();
        break;
      }
      case 14: {  // DIRECT_INPUT
*value=c.proxy_input->is_direct();
        break;
      }
      case 15: {  // RANDOM_PROXY
*value=ClientSocketPoolManager::allow_size_randomization_for_proxy();
        break;
      }
      case 16: {  // CREATE_CAPACITY
c.capacity_output.emplace(SocketPoolAdditionalCapacity::Create(c.capacity_input));
        break;
      }
      case 17: {  // EMPTY_CAPACITY
c.capacity_output.emplace(SocketPoolAdditionalCapacity::CreateEmpty());
        break;
      }
      case 18: {  // SECONDS_INPUT
*value=c.duration_input.InSeconds();
        break;
      }
      case 19: {  // CHECK_SECONDS
DCHECK_GT(argument,0);
        break;
      }
      case 20: {  // DICT_GROUP
c.dict_output->Set("group_id",c.group_input->ToString());
        break;
      }
      case 21: {  // LOG_REQUEST
c.log_input->AddEvent(NetLogEventType::TCP_CLIENT_SOCKET_POOL_REQUESTED_SOCKET,[&c]{return Class::NetLogGroupIdParams(*c.group_input);});
        break;
      }
      case 22: {  // USING_SSL
*value=GURL::SchemeIsCryptographic(c.group_input->destination().scheme());
        break;
      }
      case 23: {  // DIRECT_OWNER
*value=c.R().GetProxyChain().is_direct();
        break;
      }
      case 24: {  // INSTALL_RESOLUTION_CALLBACK
c.resolution_callback=base::BindRepeating(&OnHostResolution,c.R().common_connect_job_params_->spdy_session_pool,
SpdySessionKey(HostPortPair::FromSchemeHostPort(c.group_input->destination()),c.group_input->privacy_mode(),
c.R().GetProxyChain(),SessionUsage::kDestination,*c.tag_input,c.group_input->network_anonymization_key(),
c.group_input->secure_dns_policy(),c.group_input->disable_cert_network_fetches(),c.group_input->target_network()),c.R().is_for_websockets_);
        break;
      }
      case 25: {  // WEBSOCKETS
*value=c.R().is_for_websockets_;
        break;
      }
      case 26: {  // CREATE_JOB
c.job_output=c.R().connect_job_factory_->CreateConnectJob(c.group_input->destination(),c.R().GetProxyChain(),
*c.annotation_input,(*c.params_input)->allowed_bad_certs(),c.alpn,c.force_tunnel,c.group_input->privacy_mode(),
c.resolution_callback,c.priority_input,*c.tag_input,c.group_input->network_anonymization_key(),
c.group_input->secure_dns_policy(),c.group_input->disable_cert_network_fetches(),c.R().common_connect_job_params_,
c.group_input->target_network(),c.delegate_input);
        break;
      }
      case 27: {  // HTTP11
c.alpn=ConnectJobFactory::AlpnMode::kHttp11Only;
        break;
      }
      case 28: {  // HTTP_ALL
c.alpn=ConnectJobFactory::AlpnMode::kHttpAll;
        break;
      }
      case 29: {  // TUNNEL
c.force_tunnel=argument != 0;
        break;
      }
      case 30: {  // EXPAND_ALLOCATE
const auto next=c.R().additional_capacity_.NextExpandabilityBeforeAllocation(c.R().Expandability(),c.R().SocketsInUse(),c.R().SocketSoftCap());
if(!c.Live())return false;c.M().expandability_=next;
        break;
      }
      case 31: {  // EXPAND_RELEASE
const auto next=c.R().additional_capacity_.NextExpandabilityAfterRelease(c.R().Expandability(),c.R().SocketsInUse(),c.R().SocketSoftCap());
if(!c.Live())return false;c.M().expandability_=next;
        break;
      }
      case 32: {  // HAS_OVERRIDE
*value=c.R().socket_soft_cap_override_for_test_.has_value();
        break;
      }
      case 33: {  // OVERRIDE_CAP
*value=static_cast<int64_t>(*c.R().socket_soft_cap_override_for_test_);
        break;
      }
      case 34: {  // BASE_CAP
*value=static_cast<int64_t>(c.R().socket_soft_cap_);
        break;
      }
      case 35: {  // HOST_CHECK
DCHECK(*c.host_input==c.key_input->host_port_pair());
        break;
      }
      case 36: {  // HOST_LEGACY
*value=std::get_if<base::span<const HostResolverEndpointResult>>(c.endpoints_input.get())!=nullptr;
        break;
      }
      case 37: {  // HOST_NOTIFY_LEGACY
*value=static_cast<int>(c.spdy_input->OnHostResolutionComplete(*c.key_input,c.websockets_input,
*std::get_if<base::span<const HostResolverEndpointResult>>(c.endpoints_input.get()),*c.aliases_input));
        break;
      }
      case 38: {  // HOST_NOTIFY_SERVICE
*value=static_cast<int>(c.spdy_input->OnHostResolutionComplete(*c.key_input,c.websockets_input,
std::get<base::span<const ServiceEndpoint>>(*c.endpoints_input),*c.aliases_input));
        break;
      }
      case 39: {  // BAD_STATE
NOTREACHED();
      }
      default: return false;
    }
    // Only invocation-local liveness is read after any deleting callback.
    return c.Live();
  }
};


namespace {

// The maximum duration, in seconds, to keep used idle persistent sockets alive.
// Idle timeout state is owned by the safe Rust family core.

// Invoked by the transport socket pool after host resolution is complete
// to allow the connection to be aborted, if a matching SPDY session can
// be found. Returns OnHostResolutionCallbackResult::kMayBeDeletedAsync if such
// a session is found, as it will post a task that may delete the calling
// ConnectJob. Also returns kMayBeDeletedAsync if there may already be such
// a task posted.
OnHostResolutionCallbackResult OnHostResolution(
    SpdySessionPool* spdy_session_pool,
    const SpdySessionKey& spdy_session_key,
    bool is_for_websockets,
    const HostPortPair& host_port_pair,
    const HostResolverEndpointsOrServiceEndpoints& endpoint_results,
    const base::flat_set<std::string>& aliases) {

  ClientSocketPoolFamilyRustBridge c;c.spdy_input=spdy_session_pool;c.key_input=&spdy_session_key;
  c.websockets_input=is_for_websockets;c.host_input=&host_port_pair;c.endpoints_input=&endpoint_results;
  c.aliases_input=&aliases;return static_cast<OnHostResolutionCallbackResult>(c.Run(0));
}

}  // namespace

ClientSocketPool::SocketParams::SocketParams(
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs)
    : allowed_bad_certs_(allowed_bad_certs) {}

ClientSocketPool::SocketParams::~SocketParams() = default;

scoped_refptr<ClientSocketPool::SocketParams>
ClientSocketPool::SocketParams::CreateForHttpForTesting() {
  return base::MakeRefCounted<SocketParams>(
      /*allowed_bad_certs=*/std::vector<SSLConfig::CertAndStatus>());
}

// static
std::string_view ClientSocketPool::GroupId::GetPrivacyModeGroupIdPrefix(
    PrivacyMode privacy_mode) {
  ClientSocketPoolFamilyRustBridge c;c.Run(1,privacy_mode);return c.piece;
}

// static
std::string_view ClientSocketPool::GroupId::GetSecureDnsPolicyGroupIdPrefix(
    SecureDnsPolicy secure_dns_policy) {
  ClientSocketPoolFamilyRustBridge c;c.Run(2,static_cast<int>(secure_dns_policy));return c.piece;
}

ClientSocketPool::GroupId::GroupId()
    : privacy_mode_(PrivacyMode::PRIVACY_MODE_DISABLED) {}

ClientSocketPool::GroupId::GroupId(
    url::SchemeHostPort destination,
    PrivacyMode privacy_mode,
    NetworkAnonymizationKey network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    handles::NetworkHandle target_network)
    : destination_(std::move(destination)),
      privacy_mode_(privacy_mode),
      network_anonymization_key_(
          SelectConnectionFamilyKey(network_anonymization_key)),
      secure_dns_policy_(secure_dns_policy),
      disable_cert_network_fetches_(disable_cert_network_fetches),
      target_network_(target_network) {
  DCHECK(destination_.IsValid());

  // ClientSocketPool only expected to be used for HTTP/HTTPS/WS/WSS cases, and
  // "ws"/"wss" schemes should be converted to "http"/"https" equivalent first.
  DCHECK(destination_.scheme() == url::kHttpScheme ||
         destination_.scheme() == url::kHttpsScheme);
}

ClientSocketPool::GroupId::GroupId(const GroupId& group_id) = default;

ClientSocketPool::GroupId::~GroupId() = default;

ClientSocketPool::GroupId& ClientSocketPool::GroupId::operator=(
    const GroupId& group_id) = default;

ClientSocketPool::GroupId& ClientSocketPool::GroupId::operator=(
    GroupId&& group_id) = default;

std::string ClientSocketPool::GroupId::ToString() const {
  std::string result;ClientSocketPoolFamilyRustBridge c;c.group_input=this;c.text_output=&result;c.Run(3);return result;
}

ClientSocketPool::~ClientSocketPool() { connection_jobs_upper_rust::Invocation::Invalidate(this); }

// static
base::TimeDelta ClientSocketPool::used_idle_socket_timeout() {
  ClientSocketPoolFamilyRustBridge c;return base::Seconds(c.Run(4));
}

// static
void ClientSocketPool::set_used_idle_socket_timeout(base::TimeDelta timeout) {
  ClientSocketPoolFamilyRustBridge c;c.duration_input=timeout;c.Run(5);
}

ClientSocketPool::ClientSocketPool(
    size_t socket_soft_cap,
    const ProxyChain& proxy_chain,
    bool is_for_websockets,
    const CommonConnectJobParams* common_connect_job_params,
    std::unique_ptr<ConnectJobFactory> connect_job_factory)
    : socket_soft_cap_(socket_soft_cap),
      additional_capacity_(
          CreateConnectionFamilyCapacity(socket_soft_cap,proxy_chain)),
      proxy_chain_(proxy_chain),
      is_for_websockets_(is_for_websockets),
      common_connect_job_params_(common_connect_job_params),
      connect_job_factory_(std::move(connect_job_factory)) {}

void ClientSocketPool::NetLogTcpClientSocketPoolRequestedSocket(
    const NetLogWithSource& net_log,
    const GroupId& group_id) {
  ClientSocketPoolFamilyRustBridge c(this);c.log_input=&net_log;c.group_input=&group_id;c.Run(6);
}

base::DictValue ClientSocketPool::NetLogGroupIdParams(const GroupId& group_id) {
  base::DictValue result;ClientSocketPoolFamilyRustBridge c;c.dict_output=&result;c.group_input=&group_id;c.Run(7);return result;
}

std::unique_ptr<ConnectJob> ClientSocketPool::CreateConnectJob(
    GroupId group_id,
    scoped_refptr<SocketParams> socket_params,
    const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    RequestPriority request_priority,
    SocketTag socket_tag,
    ConnectJob::Delegate* delegate) {

  ClientSocketPoolFamilyRustBridge c(this);c.group_input=&group_id;c.params_input=&socket_params;
  c.annotation_input=&proxy_annotation_tag;c.priority_input=request_priority;c.tag_input=&socket_tag;
  c.delegate_input=delegate;c.Run(8);return std::move(c.job_output);
}

void ClientSocketPool::UpdateExpandabilityBeforeAllocation() {
  ClientSocketPoolFamilyRustBridge c(this);c.Run(9);
}

void ClientSocketPool::UpdateExpandabilityAfterRelease() {
  ClientSocketPoolFamilyRustBridge c(this);c.Run(10);
}

namespace {
SocketPoolAdditionalCapacity CreateConnectionFamilyCapacity(size_t cap,const ProxyChain& proxy){
 ClientSocketPoolFamilyRustBridge c;c.capacity_input=cap;c.proxy_input=&proxy;c.Run(11);return std::move(*c.capacity_output);
}
NetworkAnonymizationKey SelectConnectionFamilyKey(NetworkAnonymizationKey& key){
 ClientSocketPoolFamilyRustBridge c;c.nak_input=&key;c.Run(12);return std::move(*c.nak_output);
}
}  // namespace
size_t ClientSocketPool::SocketSoftCap() const {
 ClientSocketPoolFamilyRustBridge c(this);return static_cast<size_t>(c.Run(13));
}

}  // namespace net
