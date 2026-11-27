// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/tcp_client_socket.h"

#include <memory>
#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ref.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "net/base/features.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/port_util.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/socket/socket_performance_watcher.h"
#include "net/socket/tcp_client_socket_rust_ffi.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

#if defined(TCP_CLIENT_SOCKET_OBSERVES_SUSPEND)
#include "base/power_monitor/power_monitor.h"
#endif

namespace net {

class NetLogWithSource;

namespace policy = tcp_client_socket_rust;
// Stack-owned service bridge. Rust receives this bridge, never `this`. Every
// native boundary commits a copied policy snapshot, then snapshots reentrant
// changes. A weak generation detects both destruction and nested Disconnect.
// No Rust borrow crosses a native write to C++ state.
struct TCPClientSocket::RustContext {
  explicit RustContext(TCPClientSocket* owner)
      : weak(owner->weak_ptr_factory_.GetWeakPtr()) {}
  base::WeakPtr<TCPClientSocket> weak;
  raw_ptr<const IPEndPoint> input = nullptr;
  raw_ptr<IPEndPoint> output = nullptr;
  raw_ptr<IOBuffer> buffer = nullptr;
  int length = 0;
  raw_ptr<CompletionOnceCallback> callback = nullptr;
  raw_ptr<const NetworkTrafficAnnotationTag> annotation = nullptr;
  // Only Suspend owns a detached write callback. Keep that object absent in
  // other invocations instead of destroying an empty DLL-backed holder.
  std::optional<CompletionOnceCallback> suspended_write;
  std::optional<base::TimeDelta> rtt;

  static policy::State Snapshot(TCPClientSocket* s) {
    return {s->current_address_index_,   s->next_connect_state_,
            s->previously_disconnected_, s->total_received_bytes_,
            s->was_ever_used_,           s->was_disconnected_on_suspend_};
  }
  static void Commit(TCPClientSocket* s, policy::State state) {
    s->current_address_index_ = static_cast<int>(state.index);
    s->next_connect_state_ = static_cast<ConnectState>(state.phase);
    s->previously_disconnected_ = state.previous != 0;
    s->total_received_bytes_ = state.received;
    s->was_ever_used_ = state.used != 0;
    s->was_disconnected_on_suspend_ = state.suspended != 0;
  }
  static bool Call(void* opaque,
                   policy::State* state,
                   uint32_t action,
                   int64_t value,
                   int64_t* output) {
    auto& c = *static_cast<RustContext*>(opaque);
    TCPClientSocket* s = c.weak.get();
    if (!s) {
      *output = ERR_ABORTED;
      return false;
    }
    Commit(s, *state);
    int64_t result = OK;
    using A = policy::Action;
    switch (static_cast<A>(action)) {
      case A::kSave:
        break;
      case A::kValidateConnect:
        DCHECK(!s->read_callback_);
        DCHECK(!s->write_callback_);
        break;
      case A::kValid:
        result = s->socket_->IsValid();
        break;
      case A::kCount:
        result = s->addresses_.size();
        break;
      case A::kHasBind:
        result = !!s->bind_address_;
        break;
      case A::kOpen:
        result = s->socket_->Open(
            value == -1 ? s->addresses_[s->current_address_index_].GetFamily()
                        : static_cast<AddressFamily>(value));
        break;
      case A::kBindInput:
        result = s->socket_->Bind(*c.input);
        break;
      case A::kBindSaved:
        result = s->socket_->Bind(*s->bind_address_);
        break;
      case A::kSaveBind:
        s->bind_address_ = std::make_unique<IPEndPoint>(*c.input);
        break;
      case A::kClearBind:
        s->bind_address_.reset();
        break;
      case A::kClose:
        s->socket_->Close();
        break;
      case A::kInvalidate:
        s->weak_ptr_factory_.InvalidateWeakPtrs();
        c.weak = s->weak_ptr_factory_.GetWeakPtr();
        break;
      case A::kDefaults:
        s->socket_->SetDefaultOptionsForClient();
        break;
      case A::kHasNetwork:
        result = s->network_ != handles::kInvalidNetworkHandle;
        break;
      case A::kBindNetwork:
        result = s->socket_->BindToNetwork(s->network_);
        break;
      case A::kPortAllowed:
        result = IsPortAllowedForIpEndpoint(
            s->addresses_[s->current_address_index_]);
        break;
      case A::kHasBefore:
        result = !!s->before_connect_callback_;
        break;
      case A::kBefore: {
        // Copy the callback because it can delete its owning socket while Run
        // is active. Never touch s after Run until the weak check below.
        auto before = s->before_connect_callback_;
        result = before.Run();
        break;
      }
      case A::kWatcher:
        result = !!s->socket_->socket_performance_watcher();
        break;
      case A::kNotify:
        s->socket_->socket_performance_watcher()->OnConnectionChanged();
        break;
      case A::kStartAttempt:
        s->start_connect_attempt_ = base::TimeTicks::Now();
        break;
      case A::kHasAttempt:
        result = s->start_connect_attempt_.has_value();
        break;
      case A::kEmit:
        s->EmitConnectAttemptHistograms(static_cast<int>(value));
        break;
      case A::kStopTimer:
        s->start_connect_attempt_ = std::nullopt;
        s->connect_attempt_timer_.Stop();
        break;
      case A::kTimeoutEnabled:
        result =
            base::FeatureList::IsEnabled(features::kTimeoutTcpConnectAttempt);
        break;
      case A::kHasRtt:
        c.rtt = s->network_quality_estimator_
                    ? s->network_quality_estimator_->GetTransportRTT()
                    : std::nullopt;
        result = c.rtt.has_value();
        break;
      case A::kMinTimeout:
        result = features::kTimeoutTcpConnectAttemptMin.Get().InMicroseconds();
        break;
      case A::kMaxTimeout:
        result = features::kTimeoutTcpConnectAttemptMax.Get().InMicroseconds();
        break;
      case A::kAdaptiveTimeout:
        result =
            (*c.rtt * features::kTimeoutTcpConnectAttemptRTTMultiplier.Get())
                .InMicroseconds();
        break;
      case A::kStartTimer:
        DCHECK(!s->connect_attempt_timer_.IsRunning());
        s->connect_attempt_timer_.Start(
            FROM_HERE, base::Microseconds(value),
            base::BindOnce(&TCPClientSocket::OnConnectAttemptTimeout,
                           base::Unretained(s)));
        break;
      case A::kConnect:
        result = s->ConnectInternal(s->addresses_[s->current_address_index_]);
        break;
      case A::kLogStart:
        s->socket_->StartLoggingMultipleConnectAttempts(s->addresses_);
        break;
      case A::kLogEnd:
        s->socket_->EndLoggingMultipleConnectAttempts(static_cast<int>(value));
        break;
      case A::kResetCallbacks: {
        // Releasing callback captures may run arbitrary destructors. Detach
        // all ownership first and never access the owner during their release.
        auto connect = std::move(s->connect_callback_);
        auto read = std::move(s->read_callback_);
        auto write = std::move(s->write_callback_);
        connect.Reset();
        read.Reset();
        write.Reset();
        break;
      }
      case A::kStoreConnect:
        s->connect_callback_ = std::move(*c.callback);
        break;
      case A::kRead:
        result =
            s->socket_->Read(c.buffer, c.length,
                             base::BindOnce(&TCPClientSocket::DidCompleteRead,
                                            base::Unretained(s)));
        break;
      case A::kReadReady:
        result = s->socket_->ReadIfReady(
            c.buffer, c.length,
            base::BindOnce(&TCPClientSocket::DidCompleteRead,
                           base::Unretained(s)));
        break;
      case A::kWrite:
        result =
            s->socket_->Write(c.buffer, c.length,
                              base::BindOnce(&TCPClientSocket::DidCompleteWrite,
                                             base::Unretained(s)),
                              *c.annotation);
        break;
      case A::kStoreRead:
        s->read_callback_ = std::move(*c.callback);
        break;
      case A::kStoreWrite:
        s->write_callback_ = std::move(*c.callback);
        break;
      case A::kResetRead:
        s->read_callback_.Reset();
        break;
      case A::kCancelRead:
        result = s->socket_->CancelReadIfReady();
        break;
      case A::kTakeWrite:
        c.suspended_write = std::move(s->write_callback_);
        break;
      case A::kRunWrite:
        if (c.suspended_write && *c.suspended_write) {
          std::move(*c.suspended_write).Run(static_cast<int>(value));
        }
        break;
      case A::kCompleteRead:
        s->DidCompleteRead(static_cast<int>(value));
        break;
      case A::kReadPending:
        result = !!s->read_callback_;
        break;
      case A::kInvokeConnect:
        std::move(s->connect_callback_).Run(static_cast<int>(value));
        break;
      case A::kInvokeRead:
        std::move(s->read_callback_).Run(static_cast<int>(value));
        break;
      case A::kInvokeWrite:
        std::move(s->write_callback_).Run(static_cast<int>(value));
        break;
      case A::kLocalAddress:
        result = s->socket_->GetLocalAddress(c.output);
        break;
      case A::kCopyBind:
        *c.output = *s->bind_address_;
        break;
    }
    // The native primitive may delete the owner or invalidate this generation.
    // Check again before reading any owner state; within this callback-free
    // snapshot use that checked pointer instead of repeating the validity load.
    TCPClientSocket* surviving_owner = c.weak.get();
    if (!surviving_owner) {
      *output = ERR_ABORTED;
      return false;
    }
    *state = Snapshot(surviving_owner);
    *output = result;
    return true;
  }
  int64_t RunValue(policy::Operation op, int64_t value = 0) {
    TCPClientSocket* owner = weak.get();
    if (!owner) {
      return ERR_ABORTED;
    }
    policy::State state = Snapshot(owner);
    int64_t result = tcp_client_socket_rust_run_fast(
        &state, static_cast<uint32_t>(op), value, this, &Call);
    owner = weak.get();
    if (!owner) {
      return ERR_ABORTED;
    }
    // No callback runs between Rust's terminal Save and this liveness check.
    // Publish directly instead of making another service round trip merely to
    // commit and immediately snapshot the same scalar fields again.
    Commit(owner, state);
    return result;
  }
  int Run(policy::Operation op, int64_t value = 0) {
    return static_cast<int>(RunValue(op, value));
  }
};

TCPClientSocket::TCPClientSocket(
    const AddressList& addresses,
    std::unique_ptr<SocketPerformanceWatcher> socket_performance_watcher,
    NetworkQualityEstimator* network_quality_estimator,
    net::NetLog* net_log,
    const net::NetLogSource& source,
    handles::NetworkHandle network)
    : TCPClientSocket(TCPSocket::Create(std::move(socket_performance_watcher),
                                        net_log,
                                        source),
                      addresses,
                      -1 /* current_address_index */,
                      nullptr /* bind_address */,
                      network_quality_estimator,
                      network) {}

TCPClientSocket::TCPClientSocket(std::unique_ptr<TCPSocket> connected_socket,
                                 const IPEndPoint& peer_address)
    : TCPClientSocket(std::move(connected_socket),
                      AddressList(peer_address),
                      0 /* current_address_index */,
                      nullptr /* bind_address */,
                      // TODO(https://crbug.com/1123197: Pass non-null
                      // NetworkQualityEstimator
                      nullptr /* network_quality_estimator */,
                      handles::kInvalidNetworkHandle) {}

TCPClientSocket::TCPClientSocket(
    std::unique_ptr<TCPSocket> unconnected_socket,
    const AddressList& addresses,
    std::unique_ptr<IPEndPoint> bound_address,
    NetworkQualityEstimator* network_quality_estimator)
    : TCPClientSocket(std::move(unconnected_socket),
                      addresses,
                      -1 /* current_address_index */,
                      std::move(bound_address),
                      network_quality_estimator,
                      handles::kInvalidNetworkHandle) {}

TCPClientSocket::~TCPClientSocket() {
  Disconnect();
#if defined(TCP_CLIENT_SOCKET_OBSERVES_SUSPEND)
  base::PowerMonitor::GetInstance()->RemovePowerSuspendObserver(this);
#endif  // defined(TCP_CLIENT_SOCKET_OBSERVES_SUSPEND)
}

std::unique_ptr<TCPClientSocket> TCPClientSocket::CreateFromBoundSocket(
    std::unique_ptr<TCPSocket> bound_socket,
    const AddressList& addresses,
    const IPEndPoint& bound_address,
    NetworkQualityEstimator* network_quality_estimator) {
  return base::WrapUnique(new TCPClientSocket(
      std::move(bound_socket), addresses, -1 /* current_address_index */,
      std::make_unique<IPEndPoint>(bound_address), network_quality_estimator,
      handles::kInvalidNetworkHandle));
}

int TCPClientSocket::Bind(const IPEndPoint& address) {
  RustContext c(this);
  c.input = &address;
  return c.Run(policy::Operation::kBind, address.GetFamily());
}

bool TCPClientSocket::SetKeepAlive(bool enable, int delay) {
  return socket_->SetKeepAlive(enable, delay);
}

bool TCPClientSocket::SetNoDelay(bool no_delay) {
  return socket_->SetNoDelay(no_delay);
}

void TCPClientSocket::SetBeforeConnectCallback(
    const BeforeConnectCallback& before_connect_callback) {
  DCHECK_EQ(CONNECT_STATE_NONE, next_connect_state_);
  before_connect_callback_ = before_connect_callback;
}

int TCPClientSocket::Connect(CompletionOnceCallback callback) {
  DCHECK(callback);
  RustContext c(this);
  c.callback = &callback;
  return c.Run(policy::Operation::kConnect);
}

TCPClientSocket::TCPClientSocket(
    std::unique_ptr<TCPSocket> socket,
    const AddressList& addresses,
    int current_address_index,
    std::unique_ptr<IPEndPoint> bind_address,
    NetworkQualityEstimator* network_quality_estimator,
    handles::NetworkHandle network)
    : socket_(std::move(socket)),
      bind_address_(std::move(bind_address)),
      addresses_(addresses),
      current_address_index_(current_address_index),
      network_quality_estimator_(network_quality_estimator),
      network_(network) {
  DCHECK(socket_);
  RustContext(this).Run(policy::Operation::kInit);
#if defined(TCP_CLIENT_SOCKET_OBSERVES_SUSPEND)
  base::PowerMonitor::GetInstance()->AddPowerSuspendObserver(this);
#endif  // defined(TCP_CLIENT_SOCKET_OBSERVES_SUSPEND)
}

int TCPClientSocket::ReadCommon(IOBuffer* buf,
                                int buf_len,
                                CompletionOnceCallback callback,
                                bool read_if_ready) {
  DCHECK(callback);
  DCHECK(!read_callback_);
  RustContext c(this);
  c.buffer = buf;
  c.length = buf_len;
  c.callback = &callback;
  return c.Run(read_if_ready ? policy::Operation::kReadReady
                             : policy::Operation::kRead);
}

void TCPClientSocket::OnConnectAttemptTimeout() {
  DidCompleteConnect(ERR_TIMED_OUT);
}

int TCPClientSocket::ConnectInternal(const IPEndPoint& endpoint) {
  // |socket_| is owned by this class and the callback won't be run once
  // |socket_| is gone. Therefore, it is safe to use base::Unretained() here.
  return socket_->Connect(endpoint,
                          base::BindOnce(&TCPClientSocket::DidCompleteConnect,
                                         base::Unretained(this)));
}

void TCPClientSocket::Disconnect() {
  RustContext(this).Run(policy::Operation::kDisconnect);
}

bool TCPClientSocket::IsConnected() const {
  return socket_->IsConnected();
}

bool TCPClientSocket::IsConnectedAndIdle() const {
  return socket_->IsConnectedAndIdle();
}

int TCPClientSocket::GetPeerAddress(IPEndPoint* address) const {
  return socket_->GetPeerAddress(address);
}

int TCPClientSocket::GetLocalAddress(IPEndPoint* address) const {
  DCHECK(address);
  struct ReadOnlyContext {
    raw_ref<const TCPClientSocket> socket;
    raw_ref<IPEndPoint> address;
  } context{raw_ref(*this), raw_ref(*address)};
  // A genuinely const socket must not enter the state-writing bridge or acquire
  // a WeakPtr through its non-const factory. The native address query retains
  // the original getter's synchronous primitive boundary.
  const auto read_only = [](void* opaque, policy::State state, uint32_t action,
                            int64_t) -> policy::Reply {
    const auto& c = *static_cast<ReadOnlyContext*>(opaque);
    int result = OK;
    switch (static_cast<policy::Action>(action)) {
      case policy::Action::kSave:
        break;
      case policy::Action::kValid:
        result = c.socket->socket_->IsValid();
        break;
      case policy::Action::kHasBind:
        result = !!c.socket->bind_address_;
        break;
      case policy::Action::kCopyBind:
        c.address.get() = *c.socket->bind_address_;
        break;
      case policy::Action::kLocalAddress:
        result = c.socket->socket_->GetLocalAddress(&c.address.get());
        break;
      default:
        NOTREACHED();
    }
    // Rust's final Save is an observation-only no-op here. Do not read the
    // owner again after the primitive returns or write back a policy snapshot.
    return {state, result, 1};
  };
  return static_cast<int>(policy::tcp_client_socket_rust_run(
      {}, static_cast<uint32_t>(policy::Operation::kLocalAddress), 0, &context,
      read_only));
}

const NetLogWithSource& TCPClientSocket::NetLog() const {
  return socket_->net_log();
}

bool TCPClientSocket::WasEverUsed() const {
  return was_ever_used_;
}

NextProto TCPClientSocket::GetNegotiatedProtocol() const {
  return NextProto::kProtoUnknown;
}

bool TCPClientSocket::GetSSLInfo(SSLInfo* ssl_info) {
  return false;
}

int TCPClientSocket::Read(IOBuffer* buf,
                          int buf_len,
                          CompletionOnceCallback callback) {
  return ReadCommon(buf, buf_len, std::move(callback), /*read_if_ready=*/false);
}

int TCPClientSocket::ReadIfReady(IOBuffer* buf,
                                 int buf_len,
                                 CompletionOnceCallback callback) {
  return ReadCommon(buf, buf_len, std::move(callback), /*read_if_ready=*/true);
}

int TCPClientSocket::CancelReadIfReady() {
  DCHECK(read_callback_);
  return RustContext(this).Run(policy::Operation::kCancelRead);
}

int TCPClientSocket::Write(
    IOBuffer* buf,
    int buf_len,
    CompletionOnceCallback callback,
    const NetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK(callback);
  DCHECK(!write_callback_);
  RustContext c(this);
  c.buffer = buf;
  c.length = buf_len;
  c.callback = &callback;
  c.annotation = &traffic_annotation;
  return c.Run(policy::Operation::kWrite);
}

int TCPClientSocket::SetReceiveBufferSize(int32_t size) {
  return socket_->SetReceiveBufferSize(size);
}

int TCPClientSocket::SetSendBufferSize(int32_t size) {
  return socket_->SetSendBufferSize(size);
}

SocketDescriptor TCPClientSocket::SocketDescriptorForTesting() const {
  return socket_->SocketDescriptorForTesting();
}

int64_t TCPClientSocket::GetTotalReceivedBytes() const {
  return total_received_bytes_;
}

void TCPClientSocket::ApplySocketTag(const SocketTag& tag) {
  socket_->ApplySocketTag(tag);
}

void TCPClientSocket::OnSuspend() {
#if defined(TCP_CLIENT_SOCKET_OBSERVES_SUSPEND)
  RustContext(this).Run(policy::Operation::kSuspend);
#endif
}

void TCPClientSocket::DidCompleteConnect(int result) {
  DCHECK(connect_callback_);
  RustContext(this).Run(policy::Operation::kCompleteConnect, result);
}

void TCPClientSocket::DidCompleteRead(int result) {
  DCHECK(read_callback_);
  RustContext(this).Run(policy::Operation::kCompleteRead, result);
}

void TCPClientSocket::DidCompleteWrite(int result) {
  DCHECK(write_callback_);
  RustContext(this).Run(policy::Operation::kCompleteWrite, result);
}

int TCPClientSocket::OpenSocket(AddressFamily family) {
  return RustContext(this).Run(policy::Operation::kOpen, family);
}

void TCPClientSocket::EmitConnectAttemptHistograms(int result) {
  // This should only be called in response to completing a connect attempt.
  DCHECK(start_connect_attempt_);

  base::TimeDelta duration =
      base::TimeTicks::Now() - start_connect_attempt_.value();

  // Histogram the total time the connect attempt took, grouped by success and
  // failure. Note that failures also include cases when the connect attempt
  // was cancelled by the client before the handshake completed.
  if (result == OK) {
    DEPRECATED_UMA_HISTOGRAM_MEDIUM_TIMES(
        "Net.TcpConnectAttempt.Latency.Success", duration);
  } else {
    DEPRECATED_UMA_HISTOGRAM_MEDIUM_TIMES("Net.TcpConnectAttempt.Latency.Error",
                                          duration);
  }
}

base::TimeDelta TCPClientSocket::GetConnectAttemptTimeout() {
  RustContext c(this);
  return base::Microseconds(c.RunValue(policy::Operation::kTimeout));
}

}  // namespace net
