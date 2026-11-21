// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "net/socket/connect_job_rust_native.h"
#include "base/check.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/connect_job_params.h"
#include "net/socket/socks5_client_socket.h"
#include "net/socket/socks_client_socket.h"
#include "net/socket/transport_connect_job.h"

namespace net {

// Typed storage/provider primitives only. Sequencing/branches live in safe Rust.
struct SOCKSConnectJobFamilyRustBridge final
    : connection_jobs_upper_rust::Invocation {
  using Class = SOCKSConnectJob;
  static_assert(Class::STATE_TRANSPORT_CONNECT==0);
  static_assert(Class::STATE_TRANSPORT_CONNECT_COMPLETE==1);
  static_assert(Class::STATE_SOCKS_CONNECT==2);
  static_assert(Class::STATE_SOCKS_CONNECT_COMPLETE==3);
  static_assert(Class::STATE_NONE==4);
  static_assert(LOAD_STATE_IDLE==0 && LOAD_STATE_ESTABLISHING_PROXY_TUNNEL==9 && LOAD_STATE_CONNECTING==11 && LOAD_STATE_SSL_HANDSHAKE==12);

  explicit SOCKSConnectJobFamilyRustBridge(Class* job)
      : Invocation(static_cast<const ConnectJob*>(job)), mutable_owner(job), owner(job) { Register(&Clear); base_bridge.emplace(static_cast<ConnectJob*>(job)); }
  explicit SOCKSConnectJobFamilyRustBridge(const Class* job)
      : Invocation(static_cast<const ConnectJob*>(job)), mutable_owner(nullptr), owner(job) { Register(&Clear); base_bridge.emplace(static_cast<const ConnectJob*>(job)); }
  SOCKSConnectJobFamilyRustBridge()
      : Invocation(this), mutable_owner(nullptr), owner(nullptr) { Register(&Clear); }
  ~SOCKSConnectJobFamilyRustBridge() { base_bridge.reset(); Unregister(); }
  Class& M() { CHECK(Live()); CHECK(mutable_owner); return *mutable_owner; }
  const Class& R() const { CHECK(Live()); CHECK(owner); return *owner; }
  int64_t Run(uint32_t operation, int64_t argument = 0) {
    return connection_jobs_upper_rust::connection_jobs_rust_run(
        5, operation, argument, this, Service);
  }
  raw_ptr<Class> mutable_owner;
  raw_ptr<const Class> owner;
  std::optional<ConnectJobRustBridge> base_bridge; std::unique_ptr<StreamSocket> base_socket; std::optional<base::flat_set<std::string>> base_aliases; void PrepareSocket(){ base_bridge->socket_input=&base_socket; base_bridge->aliases=&base_aliases; }
  raw_ptr<ResolveErrorInfo> resolve_output = nullptr;

  static void Clear(Invocation* value) {
    auto& c = *static_cast<SOCKSConnectJobFamilyRustBridge*>(value);
    c.mutable_owner = nullptr;
    c.owner = nullptr;
  }

  static bool Service(void* context, uint32_t module, uint32_t action, int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<SOCKSConnectJobFamilyRustBridge*>(context);
    *value = 0;
    if(module==0 && c.base_bridge){ return ConnectJobRustBridge::Service(&*c.base_bridge,action,argument,value); }
    if (!c.Live() || module != 5) { return false; }
    if(action==UINT32_MAX){*value=1;return true;}
    switch (action) {
      case 0: {  // GET_STATE
*value = c.R().next_state_;
        break;
      }
      case 1: {  // SET_STATE
c.M().next_state_ = static_cast<Class::State>(argument);
        break;
      }
      case 2: {  // HAS_TRANSPORT
*value = c.R().transport_connect_job_ != nullptr;
        break;
      }
      case 3: {  // TRANSPORT_LOAD
*value = c.R().transport_connect_job_->GetLoadState();
        break;
      }
      case 4: {  // COPY_RESOLVE
*c.resolve_output = c.R().resolve_error_info_;
        break;
      }
      case 5: {  // CLEAR_TRANSPORT
c.M().transport_connect_job_.reset();
        break;
      }
      case 6: {  // CREATE_TRANSPORT
DCHECK(!c.R().transport_connect_job_);
c.M().transport_connect_job_ = TransportConnectJob::Factory::CreateJob(
  c.M().priority(), c.M().socket_tag(), c.M().common_connect_job_params(),
  c.M().socks_params_->transport_params(), &c.M(), &c.M().net_log());
        break;
      }
      case 7: {  // CONNECT_TRANSPORT
*value = c.M().transport_connect_job_->Connect();
        break;
      }
      case 8: {  // SAVE_TRANSPORT_RESOLVE
c.M().resolve_error_info_ = c.M().transport_connect_job_->GetResolveErrorInfo();
        break;
      }
      case 9: {  // RESET_HANDSHAKE_TIMER
c.base_bridge->duration=base::Seconds(30);
        break;
      }
      case 10: {  // IS_V5
*value = c.R().socks_params_->is_socks_v5();
        break;
      }
      case 11: {  // CREATE_V5
c.M().socket_ = std::make_unique<SOCKS5ClientSocket>(
  c.M().transport_connect_job_->PassSocket(), c.M().socks_params_->destination(),
  c.M().socks_params_->traffic_annotation());
        break;
      }
      case 12: {  // CREATE_V4
auto socket = std::make_unique<SOCKSClientSocket>(
  c.M().transport_connect_job_->PassSocket(), c.M().socks_params_->destination(),
  c.M().socks_params_->network_anonymization_key(), c.M().priority(), c.M().host_resolver(),
  c.M().socks_params_->transport_params()->secure_dns_policy(),
  c.M().socks_params_->traffic_annotation());
c.M().socks_socket_ptr_ = socket.get(); c.M().socket_ = std::move(socket);
        break;
      }
      case 13: {  // CONNECT_SOCKET
*value = c.M().socket_->Connect(base::BindOnce(
  &Class::OnIOComplete, base::Unretained(&c.M())));
        break;
      }
      case 14: {  // SAVE_V4_RESOLVE
c.M().resolve_error_info_ = c.M().socks_socket_ptr_->GetResolveErrorInfo();
        break;
      }
      case 15: {  // DISCONNECT_SOCKET
c.M().socket_->Disconnect();
        break;
      }
      case 16: {  // PUBLISH_SOCKET
c.base_socket=std::move(c.M().socket_);c.base_aliases.reset();c.PrepareSocket();
        break;
      }
      case 17: {  // NOTIFY
*value=argument; // Completion uses direct safe BASE policy.
        break;
      }
      case 18: {  // PRIORITY
c.M().transport_connect_job_->ChangePriority(static_cast<RequestPriority>(argument));
        break;
      }
      case 19: {  // BAD_STATE
NOTREACHED() << "bad state";
      }
      case 20: {  // EXPECT_OK
DCHECK_EQ(OK, argument);
        break;
      }
      default: return false;
    }
    // Only invocation-local liveness is read after any deleting callback.
    return c.Live();
  }
};


// SOCKSConnectJobs will time out if the SOCKS handshake takes longer than this.
// Timeout selection is implemented by the Rust family policy.

SOCKSSocketParams::SOCKSSocketParams(
    ConnectJobParams nested_params,
    bool socks_v5,
    const HostPortPair& host_port_pair,
    const NetworkAnonymizationKey& network_anonymization_key,
    const NetworkTrafficAnnotationTag& traffic_annotation)
    : transport_params_(nested_params.take_transport()),
      destination_(host_port_pair),
      socks_v5_(socks_v5),
      network_anonymization_key_(network_anonymization_key),
      traffic_annotation_(traffic_annotation) {}

SOCKSSocketParams::~SOCKSSocketParams() = default;

std::unique_ptr<SOCKSConnectJob> SOCKSConnectJob::Factory::Create(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    scoped_refptr<SOCKSSocketParams> socks_params,
    ConnectJob::Delegate* delegate,
    const NetLogWithSource* net_log) {
  return std::make_unique<SOCKSConnectJob>(
      priority, socket_tag, common_connect_job_params, std::move(socks_params),
      delegate, net_log);
}

SOCKSConnectJob::SOCKSConnectJob(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    scoped_refptr<SOCKSSocketParams> socks_params,
    ConnectJob::Delegate* delegate,
    const NetLogWithSource* net_log)
    : ConnectJob(priority,
                 socket_tag,
                 base::TimeDelta(),
                 common_connect_job_params,
                 delegate,
                 net_log,
                 NetLogSourceType::SOCKS_CONNECT_JOB,
                 NetLogEventType::SOCKS_CONNECT_JOB_CONNECT),
      socks_params_(std::move(socks_params)) {}

SOCKSConnectJob::~SOCKSConnectJob() {
  connection_jobs_upper_rust::Invocation::Invalidate(static_cast<const ConnectJob*>(this));
  SOCKSConnectJobFamilyRustBridge c(this);
  c.Run(0);
}

LoadState SOCKSConnectJob::GetLoadState() const {
  SOCKSConnectJobFamilyRustBridge c(this);
  return static_cast<LoadState>(c.Run(1));
}

bool SOCKSConnectJob::HasEstablishedConnection() const {
  SOCKSConnectJobFamilyRustBridge c(this);
  return c.Run(2) != 0;
}

ResolveErrorInfo SOCKSConnectJob::GetResolveErrorInfo() const {
  ResolveErrorInfo result;
  SOCKSConnectJobFamilyRustBridge c(this);
  c.resolve_output=&result;
  c.Run(3);
  return result;
}

base::TimeDelta SOCKSConnectJob::HandshakeTimeoutForTesting() {
  SOCKSConnectJobFamilyRustBridge c;
  return base::Microseconds(c.Run(4));
}

void SOCKSConnectJob::OnIOComplete(int result) {
  SOCKSConnectJobFamilyRustBridge c(this);
  c.Run(5, result);
}

void SOCKSConnectJob::OnConnectJobComplete(int result, ConnectJob* job) {
  DCHECK(transport_connect_job_);
  DCHECK_EQ(next_state_, STATE_TRANSPORT_CONNECT_COMPLETE);
  OnIOComplete(result);
}

void SOCKSConnectJob::OnNeedsProxyAuth(
    const HttpResponseInfo& response,
    HttpAuthController* auth_controller,
    base::OnceClosure restart_with_auth_callback,
    ConnectJob* job) {
  // A SOCKSConnectJob can't be on top of an HttpProxyConnectJob.
  NOTREACHED();
}

int SOCKSConnectJob::DoLoop(int result) {
  DCHECK_NE(next_state_, STATE_NONE);
  SOCKSConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(6, result));
}

int SOCKSConnectJob::DoTransportConnect() {
  SOCKSConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(7));
}

int SOCKSConnectJob::DoTransportConnectComplete(int result) {
  SOCKSConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(8, result));
}

int SOCKSConnectJob::DoSOCKSConnect() {
  SOCKSConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(9));
}

int SOCKSConnectJob::DoSOCKSConnectComplete(int result) {
  SOCKSConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(10, result));
}

int SOCKSConnectJob::ConnectInternal() {
  SOCKSConnectJobFamilyRustBridge c(this);
  return static_cast<int>(c.Run(11));
}

void SOCKSConnectJob::ChangePriorityInternal(RequestPriority priority) {
  SOCKSConnectJobFamilyRustBridge c(this);
  c.Run(12, priority);
}


}  // namespace net
