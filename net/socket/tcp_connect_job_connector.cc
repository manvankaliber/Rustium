// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/tcp_connect_job_connector.h"
#include "net/socket/tcp_connect_job_connector_rust_ffi.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"

#include <memory>
#include <optional>
#include <string_view>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ref.h"
#include "base/notreached.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/connection_attempts.h"
#include "net/socket/socket_performance_watcher.h"
#include "net/socket/socket_performance_watcher_factory.h"
#include "net/socket/websocket_stream_socket.h"

namespace net {


// Invocation-local observer. Invalidated at the most-derived destructor entry.
// No typed native pointer crosses the Rust ABI or survives a service call.
struct tcp_connect_job_connectorRustBridge : connection_jobs_upper_rust::Invocation {

  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kNone)==0);
  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kWaitForIPEndPoint)==1);
  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kObtainIPEndPoint)==2);
  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kTcpConnect)==3);
  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kTcpConnectComplete)==4);
  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kWaitForCryptoReady)==5);
  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kVerifyIPEndPointUsable)==6);
  static_assert(static_cast<int>(TcpConnectJob::Connector::State::kDone)==7);

  using Op = tcp_connect_job_connector_rust::Operation;
  using Action = tcp_connect_job_connector_rust::Action;
  raw_ptr<const TcpConnectJob::Connector> owner;
  raw_ptr<TcpConnectJob::Connector> mutable_owner = nullptr;

  TcpConnectJob::IPEndPointInfo ip_result=base::unexpected(ERR_IO_PENDING);
  raw_ptr<const ServiceEndpoint> endpoint=nullptr;
  std::unique_ptr<SocketPerformanceWatcher> watcher;

  explicit tcp_connect_job_connectorRustBridge(TcpConnectJob::Connector* value) : Invocation(value), owner(value), mutable_owner(value) {
    Register(&Clear);
  }
  explicit tcp_connect_job_connectorRustBridge(const TcpConnectJob::Connector* value) : Invocation(value), owner(value) {
    Register(&Clear);
  }
  ~tcp_connect_job_connectorRustBridge() { Unregister(); }
  static void Clear(Invocation* value) {
    auto& c = *static_cast<tcp_connect_job_connectorRustBridge*>(value);
    c.owner = nullptr; c.mutable_owner = nullptr;
    c.endpoint=nullptr;
  }
  int64_t Run(Op operation, int64_t argument = 0) {
    return connection_jobs_rust_run(10, static_cast<uint32_t>(operation),
                                    argument, this, &Service);
  }
  static bool Service(void* context, uint32_t module, uint32_t action,
                      int64_t argument, int64_t* output) {
    auto& c = *static_cast<tcp_connect_job_connectorRustBridge*>(context);
    *output = 0;
    if (!c.owner) return false;
    if (action == UINT32_MAX) return true;
    CHECK_EQ(module, 10u);
    const auto& j = *c.owner;
    // Mutable access is requested only in action arms that require it. Const
    // wrappers never obtain a mutable receiver and never commit a snapshot.
    auto Mutable = [&]() -> TcpConnectJob::Connector& { CHECK(c.mutable_owner); return *c.mutable_owner; };
    switch (static_cast<Action>(action)) {
      case Action::State: { *output=static_cast<int>(j.next_state_); break; }
      case Action::SetState: { Mutable().next_state_=static_cast<TcpConnectJob::Connector::State>(argument); break; }
      case Action::CheckNotDone: { DCHECK_NE(j.next_state_, TcpConnectJob::Connector::State::kDone); break; }
      case Action::CheckLoop: { DCHECK_NE(j.next_state_, TcpConnectJob::Connector::State::kNone); break; }
      case Action::CheckResult: { DCHECK_NE(argument,ERR_IO_PENDING); break; }
      case Action::CheckOk: { DCHECK_EQ(argument,OK); break; }
      case Action::CheckNoAddress: { DCHECK(!j.current_address_); break; }
      case Action::CheckAddress: { DCHECK(j.current_address_); break; }
      case Action::CheckNoMore: { CHECK_EQ(argument,ERR_NAME_NOT_RESOLVED); break; }
      case Action::NotReached: { NOTREACHED(); }
      case Action::GetIp: { c.ip_result=j.parent_->GetNextIPEndPoint(j); *output=c.ip_result.has_value(); break; }
      case Action::IpError: { *output=c.ip_result.error(); break; }
      case Action::TakeIp: { Mutable().current_address_=std::move(c.ip_result).value(); break; }
      case Action::FindEndpoint: { c.endpoint=j.parent_->FindServiceEndpoint(*j.current_address_); break; }
      case Action::HasEndpoint: { *output=c.endpoint!=nullptr; break; }
      case Action::IsStale: { *output=j.parent_->IsStaleConnector(j); break; }
      case Action::SaveMetadata: { Mutable().current_service_endpoint_=*c.endpoint; break; }
      case Action::ClearMetadata: { Mutable().current_service_endpoint_=std::nullopt; break; }
      case Action::HasMetadata: { *output=j.current_service_endpoint_.has_value(); break; }
      case Action::UseMetadata: { c.endpoint=&j.current_service_endpoint_.value(); break; }
      case Action::StartNull: { *output=j.start_time_.is_null(); break; }
      case Action::StampStart: { const auto now=base::TimeTicks::Now();if(c.owner) Mutable().start_time_=now; break; }
      case Action::HasWatcher: { *output=j.parent_->socket_performance_watcher_factory()!=nullptr; break; }
      case Action::MakeWatcher: { c.watcher=j.parent_->socket_performance_watcher_factory()->CreateSocketPerformanceWatcher(
 SocketPerformanceWatcherFactory::PROTOCOL_TCP,j.current_address_->address()); break; }
      case Action::MakeSocket: { const auto& log=j.parent_->net_log();
 auto socket=j.parent_->client_socket_factory()->CreateTransportClientSocket(
 AddressList(*j.current_address_),j.parent_->params_->target_network(),std::move(c.watcher),
 j.parent_->network_quality_estimator(),log.net_log(),log.source());
 if(c.owner) Mutable().transport_socket_=std::move(socket); break; }
      case Action::ApplyTag: { Mutable().transport_socket_->ApplySocketTag(j.parent_->socket_tag()); break; }
      case Action::HasWebsocket: { *output=j.parent_->websocket_endpoint_lock_manager()!=nullptr; break; }
      case Action::WrapWebsocket: { Mutable().transport_socket_=std::make_unique<WebSocketStreamSocket>(
 *j.parent_->websocket_endpoint_lock_manager(),*j.current_address_,std::move(Mutable().transport_socket_),
 j.parent_->params_->network_anonymization_key()); break; }
      case Action::StartEvent: { j.parent_->net_log().AddEvent(NetLogEventType::TCP_CONNECT_JOB_CONNECTOR_CONNECT_START,[&] {
 base::DictValue d;d.Set("address",j.current_address_->ToString());d.Set("connector",j.name_);
 j.transport_socket_->NetLog().source().AddToEventParameters(d);return d;}); break; }
      case Action::Connect: { *output=Mutable().transport_socket_->Connect(base::BindOnce(
 &TcpConnectJob::Connector::OnIOComplete,base::Unretained(c.mutable_owner.get()))); break; }
      case Action::CompleteEvent: { j.parent_->net_log().AddEvent(NetLogEventType::TCP_CONNECT_JOB_CONNECTOR_CONNECT_COMPLETE,[&] {
 return j.NetLogDict();}); break; }
      case Action::CompleteErrorEvent: { j.parent_->net_log().AddEvent(NetLogEventType::TCP_CONNECT_JOB_CONNECTOR_CONNECT_COMPLETE,[&] {
 return j.NetLogDict().Set("net_error",static_cast<int>(argument));}); break; }
      case Action::Established: { j.parent_->has_established_connection_=true; break; }
      case Action::Crypto: { *output=j.parent_->EndpointsCryptoReady(); break; }
      case Action::UsableEvent: { j.parent_->net_log().AddEvent(NetLogEventType::TCP_CONNECT_JOB_VERIFY_IP_ENDPOINT_USABLE,[&] {
 return j.NetLogDict().Set("is_usable",argument!=0);}); break; }
      case Action::FinalMetadata: { Mutable().final_service_endpoint_=*c.endpoint; break; }
      case Action::AddressV6: { *output=j.current_address_->GetFamily()==ADDRESS_FAMILY_IPV6; break; }
      case Action::FreshPrefer: { j.parent_->fresh_state_.prefer_ipv6=argument!=0; break; }
      case Action::StalePrefer: { j.parent_->stale_state_.prefer_ipv6=argument!=0; break; }
      case Action::RecordAttempt: { j.parent_->connection_attempts_.emplace_back(*j.current_address_,static_cast<int>(argument)); break; }
      case Action::ResetSocket: { Mutable().transport_socket_.reset(); break; }
      case Action::ResetAddress: { Mutable().current_address_.reset(); break; }
      case Action::DoneEvent: { j.parent_->net_log().AddEvent(NetLogEventType::TCP_CONNECT_JOB_CONNECTOR_DONE,[&]{return j.NetLogDict();}); break; }
      case Action::DoneErrorEvent: { j.parent_->net_log().AddEvent(NetLogEventType::TCP_CONNECT_JOB_CONNECTOR_DONE,[&]{
 return j.NetLogDict().Set("net_error",static_cast<int>(argument));}); break; }
      case Action::ParentComplete: { j.parent_->OnConnectorComplete(static_cast<int>(argument),Mutable()); break; }

      default: NOTREACHED();
    }
    return c.owner != nullptr;
  }
};
TcpConnectJob::Connector::Connector(TcpConnectJob* parent,
                                    std::string_view name)
    : parent_(*parent), name_(name) {}

TcpConnectJob::Connector::~Connector() { tcp_connect_job_connectorRustBridge::Invalidate(this); }

int TcpConnectJob::Connector::TryAdvanceState() {
tcp_connect_job_connectorRustBridge c(this); return c.Run(tcp_connect_job_connectorRustBridge::Op::Advance);
}

LoadState TcpConnectJob::Connector::GetLoadState() const {
tcp_connect_job_connectorRustBridge c(this); return static_cast<LoadState>(c.Run(tcp_connect_job_connectorRustBridge::Op::Load));
}

std::unique_ptr<StreamSocket> TcpConnectJob::Connector::PassSocket() {
  DCHECK_EQ(next_state_, State::kDone);
  DCHECK(transport_socket_);
  return std::move(transport_socket_);
}

ServiceEndpoint TcpConnectJob::Connector::PassFinalServiceEndpoint() {
  DCHECK_EQ(next_state_, State::kDone);
  DCHECK(final_service_endpoint_);
  return std::move(final_service_endpoint_).value();
}

void TcpConnectJob::Connector::OnIOComplete(int result) {
tcp_connect_job_connectorRustBridge c(this); c.Run(tcp_connect_job_connectorRustBridge::Op::Io, result);
}

int TcpConnectJob::Connector::DoLoop(int result) {
tcp_connect_job_connectorRustBridge c(this); return c.Run(tcp_connect_job_connectorRustBridge::Op::Loop, result);
}

int TcpConnectJob::Connector::DoObtainIPEndPoint() {
tcp_connect_job_connectorRustBridge c(this); return c.Run(tcp_connect_job_connectorRustBridge::Op::Obtain);
}

int TcpConnectJob::Connector::DoTcpConnect() {
tcp_connect_job_connectorRustBridge c(this); return c.Run(tcp_connect_job_connectorRustBridge::Op::Connect);
}

int TcpConnectJob::Connector::DoTcpConnectComplete(int result) {
tcp_connect_job_connectorRustBridge c(this); return c.Run(tcp_connect_job_connectorRustBridge::Op::Complete, result);
}

int TcpConnectJob::Connector::DoVerifyIPEndPointUsable() {
tcp_connect_job_connectorRustBridge c(this); return c.Run(tcp_connect_job_connectorRustBridge::Op::Verify);
}

int TcpConnectJob::Connector::OnEndpointFailed(int error) {
tcp_connect_job_connectorRustBridge c(this); return c.Run(tcp_connect_job_connectorRustBridge::Op::Failed, error);
}

void TcpConnectJob::Connector::OnDone(int result) {
tcp_connect_job_connectorRustBridge c(this); c.Run(tcp_connect_job_connectorRustBridge::Op::Done, result);
}

base::DictValue TcpConnectJob::Connector::NetLogDict() const {
  return base::DictValue().Set("connector", name_);
}

}  // namespace net
