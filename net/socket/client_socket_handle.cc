// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "base/check.h"

#include <utility>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/notreached.h"
#include "base/trace_event/trace_event.h"
#include "net/base/net_errors.h"
#include "net/base/trace_constants.h"
#include "net/log/net_log_event_type.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/connect_job.h"

namespace net {

// Typed storage/provider primitives only. Sequencing/branches live in safe Rust.
struct ClientSocketHandleFamilyRustBridge final
    : connection_jobs_upper_rust::Invocation {
  using Class = ClientSocketHandle;

  explicit ClientSocketHandleFamilyRustBridge(Class* job)
      : Invocation(static_cast<const StreamSocketHandle*>(job)), mutable_owner(job), owner(job) { Register(&Clear);  }
  explicit ClientSocketHandleFamilyRustBridge(const Class* job)
      : Invocation(static_cast<const StreamSocketHandle*>(job)), mutable_owner(nullptr), owner(job) { Register(&Clear);  }
  ClientSocketHandleFamilyRustBridge()
      : Invocation(this), mutable_owner(nullptr), owner(nullptr) { Register(&Clear); }
  ~ClientSocketHandleFamilyRustBridge() {  Unregister(); }
  Class& M() { CHECK(Live()); CHECK(mutable_owner); return *mutable_owner; }
  const Class& R() const { CHECK(Live()); CHECK(owner); return *owner; }
  int64_t Run(uint32_t operation, int64_t argument = 0) {
    return connection_jobs_upper_rust::connection_jobs_rust_run(
        7, operation, argument, this, Service);
  }
  raw_ptr<Class> mutable_owner;
  raw_ptr<const Class> owner;


  raw_ptr<const ClientSocketPool::GroupId> group_input=nullptr;
  raw_ptr<scoped_refptr<ClientSocketPool::SocketParams>> params_input=nullptr;
  raw_ptr<const std::optional<NetworkTrafficAnnotationTag>> annotation_input=nullptr;
  RequestPriority priority_input=DEFAULT_PRIORITY;
  raw_ptr<const SocketTag> tag_input=nullptr;
  ClientSocketPool::RespectLimits respect_input=ClientSocketPool::RespectLimits::ENABLED;
  raw_ptr<CompletionOnceCallback> callback_input=nullptr;
  raw_ptr<const ClientSocketPool::ProxyAuthCallback> auth_input=nullptr;
  raw_ptr<ClientSocketPool> pool_input=nullptr;
  raw_ptr<const NetLogWithSource> log_input=nullptr;
  raw_ptr<HigherLayeredPool> higher_input=nullptr;
  const char* reason_input=nullptr;
  raw_ptr<ConnectJob> job_input=nullptr;
  CompletionOnceCallback saved_callback;
  bool cancel_input=false,cancel_job_input=false;

  static void Clear(Invocation* value) {
    auto& c = *static_cast<ClientSocketHandleFamilyRustBridge*>(value);
    c.mutable_owner = nullptr;
    c.owner = nullptr;
    c.group_input = nullptr;
    c.params_input = nullptr;
    c.annotation_input = nullptr;
    c.tag_input = nullptr;
    c.callback_input = nullptr;
    c.auth_input = nullptr;
    c.pool_input = nullptr;
    c.log_input = nullptr;
    c.higher_input = nullptr;
    c.job_input = nullptr;
  }

  static bool Service(void* context, uint32_t module, uint32_t action, int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<ClientSocketHandleFamilyRustBridge*>(context);
    *value = 0;
    
    if (!c.Live() || module != 7) { return false; }
    if(action==UINT32_MAX){*value=1;return true;}
    switch (action) {
      case 0: {  // INVALIDATE_WEAK
c.M().weak_factory_.InvalidateWeakPtrs();
        break;
      }
      case 1: {  // REQUEST_SOURCE
c.M().requesting_source_=c.log_input->source();
        break;
      }
      case 2: {  // CHECK_INPUT_GROUP
CHECK(c.group_input->destination().IsValid());
        break;
      }
      case 3: {  // STORE_POOL
c.M().pool_=c.pool_input;
        break;
      }
      case 4: {  // STORE_GROUP
c.M().group_id_=*c.group_input;
        break;
      }
      case 5: {  // REQUEST_SOCKET
auto callback=base::BindOnce(&Class::OnIOComplete,base::Unretained(&c.M()));
*value=c.M().pool_->RequestSocket(*c.group_input,std::move(*c.params_input),*c.annotation_input,
c.priority_input,*c.tag_input,c.respect_input,&c.M(),std::move(callback),*c.auth_input,*c.log_input);
        break;
      }
      case 6: {  // STORE_CALLBACK
c.M().callback_=std::move(*c.callback_input);
        break;
      }
      case 7: {  // HAS_SOCKET
*value=c.R().socket()!=nullptr;
        break;
      }
      case 8: {  // HAS_POOL
*value=c.R().pool_!=nullptr;
        break;
      }
      case 9: {  // POOL_PRIORITY
c.M().pool_->SetPriority(c.R().group_id_,&c.M(),static_cast<RequestPriority>(argument));
        break;
      }
      case 10: {  // INITIALIZED
*value=c.R().is_initialized();
        break;
      }
      case 11: {  // DISCONNECT
c.M().socket()->Disconnect();
        break;
      }
      case 12: {  // LOAD_DIAGNOSTICS
CHECK(!c.R().is_initialized());CHECK(c.R().group_id_.destination().IsValid());
        break;
      }
      case 13: {  // POOL_LOAD
*value=c.R().pool_->GetLoadState(c.R().group_id_,&c.R());
        break;
      }
      case 14: {  // POOL_STALLED
*value=c.R().pool_->IsStalled();
        break;
      }
      case 15: {  // CHECK_ADD_HIGHER
CHECK(c.higher_input);CHECK(!c.R().higher_pool_);
        break;
      }
      case 16: {  // ADD_HIGHER
c.M().pool_->AddHigherLayeredPool(c.higher_input);
        break;
      }
      case 17: {  // STORE_HIGHER
c.M().higher_pool_=c.higher_input;
        break;
      }
      case 18: {  // CHECK_REMOVE_HIGHER
CHECK(c.R().higher_pool_);CHECK_EQ(c.R().higher_pool_,c.higher_input);
        break;
      }
      case 19: {  // REMOVE_HIGHER
c.M().pool_->RemoveHigherLayeredPool(c.higher_input);
        break;
      }
      case 20: {  // CLEAR_HIGHER
c.M().higher_pool_=nullptr;
        break;
      }
      case 21: {  // HAS_HIGHER
*value=c.R().higher_pool_!=nullptr;
        break;
      }
      case 22: {  // CURRENT_HIGHER_INPUT
c.higher_input=c.R().higher_pool_.get();
        break;
      }
      case 23: {  // CLOSE_IDLE
c.M().pool_->CloseIdleSocketsInGroup(c.R().group_id_,c.reason_input);
        break;
      }
      case 24: {  // SAVE_ATTEMPTS
c.M().connection_attempts_=c.job_input->GetConnectionAttempts();
        break;
      }
      case 25: {  // SAVE_RESOLVE
c.M().resolve_error_info_=c.job_input->GetResolveErrorInfo();
        break;
      }
      case 26: {  // SAVE_SSL_ERROR
c.M().is_ssl_error_=c.job_input->IsSSLError();
        break;
      }
      case 27: {  // SAVE_CERT
c.M().ssl_cert_request_info_=c.job_input->GetCertRequestInfo();
        break;
      }
      case 28: {  // TAKE_CALLBACK
c.saved_callback=std::move(c.M().callback_);
        break;
      }
      case 29: {  // RESET_CALLBACK
c.M().callback_.Reset();
        break;
      }
      case 30: {  // INVOKE_CALLBACK
std::move(c.saved_callback).Run(static_cast<int>(argument));
        break;
      }
      case 31: {  // EXPECT_NOT_PENDING
CHECK_NE(ERR_IO_PENDING,argument);
        break;
      }
      case 32: {  // SET_INITIALIZED
c.M().set_is_initialized(argument != 0);
        break;
      }
      case 33: {  // CHECK_GENERATION
CHECK_NE(-1,c.R().group_generation_)<<"Pool should have set |group_generation_| to a valid value.";
        break;
      }
      case 34: {  // BEGIN_SOCKET_USE
DCHECK(c.R().socket());c.M().socket()->NetLog().BeginEventReferencingSource(
NetLogEventType::SOCKET_IN_USE,c.R().requesting_source_);
        break;
      }
      case 35: {  // VALID_GROUP
*value=c.R().group_id_.destination().IsValid();
        break;
      }
      case 36: {  // CHECK_POOL
CHECK(c.R().pool_);
        break;
      }
      case 37: {  // END_SOCKET_USE
c.M().socket()->NetLog().EndEvent(NetLogEventType::SOCKET_IN_USE);
        break;
      }
      case 38: {  // RELEASE_SOCKET
c.M().pool_->ReleaseSocket(c.R().group_id_,c.M().PassSocket(),c.R().group_generation_);
        break;
      }
      case 39: {  // CANCEL_REQUEST
c.M().pool_->CancelRequest(c.R().group_id_,&c.M(),argument != 0);
        break;
      }
      case 40: {  // BAD_STATE
NOTREACHED();
      }
      case 41: {  // DROP_SOCKET
c.M().PassSocket();
        break;
      }
      case 42: {  // CLEAR_GROUP
c.M().group_id_=ClientSocketPool::GroupId();
        break;
      }
      case 43: {  // CLEAR_REUSE
c.M().set_reuse_type(Class::SocketReuseType::kUnused);
        break;
      }
      case 44: {  // CLEAR_POOL
c.M().pool_=nullptr;
        break;
      }
      case 45: {  // CLEAR_IDLE
c.M().idle_time_=base::TimeDelta();
        break;
      }
      case 46: {  // CLEAR_TIMING
c.M().set_connect_timing(LoadTimingInfo::ConnectTiming());
        break;
      }
      case 47: {  // CLEAR_RESOLUTION
c.M().set_resolution_details(std::nullopt);
        break;
      }
      case 48: {  // CLEAR_GENERATION
c.M().group_generation_=-1;
        break;
      }
      case 49: {  // RESET_RESOLVE
c.M().resolve_error_info_=ResolveErrorInfo(OK);
        break;
      }
      case 50: {  // RESET_SSL_ERROR
c.M().is_ssl_error_=false;
        break;
      }
      case 51: {  // RESET_CERT
c.M().ssl_cert_request_info_=nullptr;
        break;
      }
      case 52: {  // CANCEL_INPUT
*value=c.cancel_input;
        break;
      }
      case 53: {  // CANCEL_JOB_INPUT
*value=c.cancel_job_input;
        break;
      }
      case 54: {  // RESET_DIAGNOSTICS
DCHECK(c.cancel_input||!c.cancel_job_input);
        break;
      }
      default: return false;
    }
    // Only invocation-local liveness is read after any deleting callback.
    return c.Live();
  }
};


ClientSocketHandle::ClientSocketHandle()
    : resolve_error_info_(ResolveErrorInfo(OK)) {}

ClientSocketHandle::~ClientSocketHandle() {
  connection_jobs_upper_rust::Invocation::Invalidate(static_cast<const StreamSocketHandle*>(this));
  ClientSocketHandleFamilyRustBridge c(this);c.Run(0);
}

int ClientSocketHandle::Init(
    const ClientSocketPool::GroupId& group_id,
    scoped_refptr<ClientSocketPool::SocketParams> socket_params,
    const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    RequestPriority priority,
    const SocketTag& socket_tag,
    ClientSocketPool::RespectLimits respect_limits,
    CompletionOnceCallback callback,
    const ClientSocketPool::ProxyAuthCallback& proxy_auth_callback,
    ClientSocketPool* pool,
    const NetLogWithSource& net_log) {

  ClientSocketHandleFamilyRustBridge c(this);c.group_input=&group_id;c.params_input=&socket_params;
  c.annotation_input=&proxy_annotation_tag;c.priority_input=priority;c.tag_input=&socket_tag;
  c.respect_input=respect_limits;c.callback_input=&callback;c.auth_input=&proxy_auth_callback;
  c.pool_input=pool;c.log_input=&net_log;return static_cast<int>(c.Run(1));
}

void ClientSocketHandle::SetPriority(RequestPriority priority) {
  ClientSocketHandleFamilyRustBridge c(this);
  c.Run(2, priority);
}

void ClientSocketHandle::Reset() {
  ClientSocketHandleFamilyRustBridge c(this);
  c.Run(3);
}

void ClientSocketHandle::ResetAndCloseSocket() {
  ClientSocketHandleFamilyRustBridge c(this);
  c.Run(4);
}

LoadState ClientSocketHandle::GetLoadState() const {
  ClientSocketHandleFamilyRustBridge c(this);
  return static_cast<LoadState>(c.Run(5));
}

bool ClientSocketHandle::IsPoolStalled() const {
  ClientSocketHandleFamilyRustBridge c(this);
  return static_cast<bool>(c.Run(6));
}

void ClientSocketHandle::AddHigherLayeredPool(HigherLayeredPool* higher_pool) {
  ClientSocketHandleFamilyRustBridge c(this);c.higher_input=higher_pool;c.Run(7);
}

void ClientSocketHandle::RemoveHigherLayeredPool(
    HigherLayeredPool* higher_pool) {
  ClientSocketHandleFamilyRustBridge c(this);c.higher_input=higher_pool;c.Run(8);
}

void ClientSocketHandle::CloseIdleSocketsInGroup(
    const char* net_log_reason_utf8) {
  ClientSocketHandleFamilyRustBridge c(this);c.reason_input=net_log_reason_utf8;c.Run(9);
}

void ClientSocketHandle::SetAdditionalErrorState(ConnectJob* connect_job) {
  ClientSocketHandleFamilyRustBridge c(this);c.job_input=connect_job;c.Run(10);
}

void ClientSocketHandle::OnIOComplete(int result) {
  TRACE_EVENT0(NetTracingCategory(),"ClientSocketHandle::OnIOComplete");
  ClientSocketHandleFamilyRustBridge c(this);
  c.Run(11, result);
}

void ClientSocketHandle::HandleInitCompletion(int result) {
  ClientSocketHandleFamilyRustBridge c(this);
  c.Run(12, result);
}

void ClientSocketHandle::ResetInternal(bool cancel, bool cancel_connect_job) {
  ClientSocketHandleFamilyRustBridge c(this);c.cancel_input=cancel;c.cancel_job_input=cancel_connect_job;c.Run(13);
}

void ClientSocketHandle::ResetErrorState() {
  ClientSocketHandleFamilyRustBridge c(this);
  c.Run(14);
}


}  // namespace net
