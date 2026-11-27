// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/transport_connect_sub_job.h"
#include "net/socket/transport_connect_sub_job_rust_ffi.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/connection_attempts.h"
#include "net/socket/socket_performance_watcher.h"
#include "net/socket/socket_performance_watcher_factory.h"
#include "net/socket/websocket_stream_socket.h"

namespace net {


// Invocation-local observer. Invalidated at the most-derived destructor entry.
// No typed native pointer crosses the Rust ABI or survives a service call.
struct transport_connect_sub_jobRustBridge : connection_jobs_upper_rust::Invocation {

  static_assert(TransportConnectSubJob::STATE_NONE==0);
  static_assert(TransportConnectSubJob::STATE_TRANSPORT_CONNECT==1);
  static_assert(TransportConnectSubJob::STATE_TRANSPORT_CONNECT_COMPLETE==2);
  static_assert(TransportConnectSubJob::STATE_DONE==3);

  using Op = transport_connect_sub_job_rust::Operation;
  using Action = transport_connect_sub_job_rust::Action;
  raw_ptr<const TransportConnectSubJob> owner;
  raw_ptr<TransportConnectSubJob> mutable_owner = nullptr;
  std::unique_ptr<SocketPerformanceWatcher> watcher;

  explicit transport_connect_sub_jobRustBridge(TransportConnectSubJob* value) : Invocation(value), owner(value), mutable_owner(value) {
    Register(&Clear);
  }
  explicit transport_connect_sub_jobRustBridge(const TransportConnectSubJob* value) : Invocation(value), owner(value) {
    Register(&Clear);
  }
  ~transport_connect_sub_jobRustBridge() { Unregister(); }
  static void Clear(Invocation* value) {
    auto& c = *static_cast<transport_connect_sub_jobRustBridge*>(value);
    c.owner = nullptr; c.mutable_owner = nullptr;

  }
  int64_t Run(Op operation, int64_t argument = 0) {
    return connection_jobs_rust_run(9, static_cast<uint32_t>(operation),
                                    argument, this, &Service);
  }
  static bool Service(void* context, uint32_t module, uint32_t action,
                      int64_t argument, int64_t* output) {
    auto& c = *static_cast<transport_connect_sub_jobRustBridge*>(context);
    *output = 0;
    if (!c.owner) return false;
    if (action == UINT32_MAX) return true;
    CHECK_EQ(module, 9u);
    const auto& j = *c.owner;
    // Mutable access is requested only in action arms that require it. Const
    // wrappers never obtain a mutable receiver and never commit a snapshot.
    auto Mutable = [&]() -> TransportConnectSubJob& { CHECK(c.mutable_owner); return *c.mutable_owner; };
    switch (static_cast<Action>(action)) {
      case Action::State: { *output = j.next_state_; break; }
      case Action::SetState: { Mutable().next_state_ = static_cast<TransportConnectSubJob::State>(argument); break; }
      case Action::CheckNone: { DCHECK_EQ(j.next_state_, TransportConnectSubJob::STATE_NONE); break; }
      case Action::CheckLoop: { DCHECK_NE(j.next_state_, TransportConnectSubJob::STATE_NONE); break; }
      case Action::CheckOk: { DCHECK_EQ(argument, OK); break; }
      case Action::NotReached: { NOTREACHED(); }
      case Action::HasWatcherFactory: { *output = j.parent_job_->socket_performance_watcher_factory() != nullptr; break; }
      case Action::MakeWatcher: { c.watcher = j.parent_job_->socket_performance_watcher_factory()->CreateSocketPerformanceWatcher(
 SocketPerformanceWatcherFactory::PROTOCOL_TCP, j.CurrentAddress().address()); break; }
      case Action::MakeSocket: { const auto& log = j.parent_job_->net_log();
 auto socket = j.parent_job_->client_socket_factory()->CreateTransportClientSocket(
 AddressList(j.CurrentAddress()), j.parent_job_->params_->target_network(),
 std::move(c.watcher), j.parent_job_->network_quality_estimator(), log.net_log(), log.source());
 if(c.owner) Mutable().transport_socket_ = std::move(socket); break; }
      case Action::AttemptEvent: { j.parent_job_->net_log().AddEvent(NetLogEventType::TRANSPORT_CONNECT_JOB_CONNECT_ATTEMPT, [&] {
 auto dict = base::DictValue().Set("address", j.CurrentAddress().ToString());
 j.transport_socket_->NetLog().source().AddToEventParameters(dict); return dict; }); break; }
      case Action::ApplyTag: { Mutable().transport_socket_->ApplySocketTag(j.parent_job_->socket_tag()); break; }
      case Action::HasWebsocket: { *output = j.parent_job_->websocket_endpoint_lock_manager() != nullptr; break; }
      case Action::WrapWebsocket: { Mutable().transport_socket_ = std::make_unique<WebSocketStreamSocket>(
 *j.parent_job_->websocket_endpoint_lock_manager(), j.CurrentAddress(), std::move(Mutable().transport_socket_),
 j.parent_job_->params_->network_anonymization_key()); break; }
      case Action::Connect: { *output = Mutable().transport_socket_->Connect(base::BindOnce(
 &TransportConnectSubJob::OnIOComplete, base::Unretained(c.mutable_owner.get()))); break; }
      case Action::ResetSocket: { Mutable().transport_socket_.reset(); break; }
      case Action::RecordAttempt: { j.parent_job_->connection_attempts_.emplace_back(j.CurrentAddress(), static_cast<int>(argument)); break; }
      case Action::Index: { *output = j.current_address_index_; break; }
      case Action::Count: { *output = j.addresses_.size(); break; }
      case Action::Next: { ++Mutable().current_address_index_; break; }
      case Action::ParentComplete: { j.parent_job_->OnSubJobComplete(static_cast<int>(argument), c.mutable_owner.get()); break; }

      default: NOTREACHED();
    }
    return c.owner != nullptr;
  }
};
TransportConnectSubJob::TransportConnectSubJob(
    std::vector<IPEndPoint> addresses,
    TransportConnectJob* parent_job,
    SubJobType type)
    : parent_job_(parent_job), addresses_(std::move(addresses)), type_(type) {}

TransportConnectSubJob::~TransportConnectSubJob() { transport_connect_sub_jobRustBridge::Invalidate(this); }

// Start connecting.
int TransportConnectSubJob::Start() {
transport_connect_sub_jobRustBridge c(this); return c.Run(transport_connect_sub_jobRustBridge::Op::Start);
}

LoadState TransportConnectSubJob::GetLoadState() const {
transport_connect_sub_jobRustBridge c(this); return static_cast<LoadState>(c.Run(transport_connect_sub_jobRustBridge::Op::Load));
}

const IPEndPoint& TransportConnectSubJob::CurrentAddress() const {
  DCHECK_LT(current_address_index_, addresses_.size());
  return addresses_[current_address_index_];
}

void TransportConnectSubJob::OnIOComplete(int result) {
transport_connect_sub_jobRustBridge c(this); c.Run(transport_connect_sub_jobRustBridge::Op::Io, result);
}

int TransportConnectSubJob::DoLoop(int result) {
transport_connect_sub_jobRustBridge c(this); return c.Run(transport_connect_sub_jobRustBridge::Op::Loop, result);
}

int TransportConnectSubJob::DoTransportConnect() {
transport_connect_sub_jobRustBridge c(this); return c.Run(transport_connect_sub_jobRustBridge::Op::Connect);
}

int TransportConnectSubJob::DoTransportConnectComplete(int result) {
transport_connect_sub_jobRustBridge c(this); return c.Run(transport_connect_sub_jobRustBridge::Op::Complete, result);
}

}  // namespace net
