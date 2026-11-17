// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/connect_job.h"
#include "net/socket/connect_job_rust_ffi.h"
#include "net/socket/connect_job_rust_native.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/trace_event/trace_event.h"
#include "net/base/connection_endpoint_metadata.h"
#include "net/base/net_errors.h"
#include "net/base/trace_constants.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_auth_controller.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/socket_tag.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/stream_socket.h"
#include "net/socket/transport_connect_job.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/ssl/ssl_cert_request_info.h"

namespace net {

static_assert(OK == 0);

namespace connection_jobs_upper_rust {
Invocation*& Invocation::Active() {
  static thread_local Invocation* active = nullptr;
  return active;
}
Invocation::Invocation(const void* value) : identity(value) {}
Invocation::~Invocation() { DCHECK(!registered); }
void Invocation::Register(ClearObservers clear) {
  CHECK(!registered);
  clear_observers = clear;
  previous = Active();
  Active() = this;
  registered = true;
}
void Invocation::Unregister() {
  CHECK(registered);
  CHECK(Active() == this);
  Active() = previous.get();
  registered = false;
}
void Invocation::Invalidate(const void* value) {
  for (auto* call = Active(); call; call = call->previous.get()) {
    if (call->identity == value) {
      if (call->clear_observers) {
        call->clear_observers(call);
      }
      call->identity = nullptr;
    }
  }
}
}  // namespace connection_jobs_upper_rust

ConnectJobRustBridge*& ConnectJobRustBridge::Active() {
  static thread_local ConnectJobRustBridge* active = nullptr;
  return active;
}
ConnectJobRustBridge::ConnectJobRustBridge(ConnectJob* job,
                                         base::TimeDelta timer_duration)
    : Invocation(job), previous(Active()), mutable_owner(job), owner(job),
      duration(timer_duration) {
  Active() = this;
  Register([](Invocation* value) {
    static_cast<ConnectJobRustBridge*>(value)->Invalidate();
  });
}
ConnectJobRustBridge::ConnectJobRustBridge(const ConnectJob* job)
    : Invocation(job), previous(Active()), mutable_owner(nullptr), owner(job) {
  Active() = this;
  Register([](Invocation* value) {
    static_cast<ConnectJobRustBridge*>(value)->Invalidate();
  });
}
ConnectJobRustBridge::~ConnectJobRustBridge() {
  Unregister();
  DCHECK_EQ(Active(), this);
  Active() = previous.get();
}

CommonConnectJobParams::CommonConnectJobParams(
    ClientSocketFactory* client_socket_factory,
    HostResolver* host_resolver,
    HttpAuthCache* http_auth_cache,
    HttpAuthHandlerFactory* http_auth_handler_factory,
    SpdySessionPool* spdy_session_pool,
    const quic::ParsedQuicVersionVector* quic_supported_versions,
    QuicSessionPool* quic_session_pool,
    ProxyDelegate* proxy_delegate,
    const HttpUserAgentSettings* http_user_agent_settings,
    SSLClientContext* ssl_client_context,
    SocketPerformanceWatcherFactory* socket_performance_watcher_factory,
    NetworkQualityEstimator* network_quality_estimator,
    NetLog* net_log,
    WebSocketEndpointLockManager* websocket_endpoint_lock_manager,
    HttpServerProperties* http_server_properties,
    const NextProtoVector* alpn_protos,
    const SSLConfig::ApplicationSettings* application_settings,
    const bool* ignore_certificate_errors,
    const bool* enable_early_data)
    : client_socket_factory(client_socket_factory),
      host_resolver(host_resolver),
      http_auth_cache(http_auth_cache),
      http_auth_handler_factory(http_auth_handler_factory),
      spdy_session_pool(spdy_session_pool),
      quic_supported_versions(quic_supported_versions),
      quic_session_pool(quic_session_pool),
      proxy_delegate(proxy_delegate),
      http_user_agent_settings(http_user_agent_settings),
      ssl_client_context(ssl_client_context),
      socket_performance_watcher_factory(socket_performance_watcher_factory),
      network_quality_estimator(network_quality_estimator),
      net_log(net_log),
      websocket_endpoint_lock_manager(websocket_endpoint_lock_manager),
      http_server_properties(http_server_properties),
      alpn_protos(alpn_protos),
      application_settings(application_settings),
      ignore_certificate_errors(ignore_certificate_errors),
      enable_early_data(enable_early_data) {}

CommonConnectJobParams::CommonConnectJobParams(
    const CommonConnectJobParams& other) = default;

CommonConnectJobParams::~CommonConnectJobParams() = default;

CommonConnectJobParams& CommonConnectJobParams::operator=(
    const CommonConnectJobParams& other) = default;


ConnectJob::ConnectJob(RequestPriority priority,
                       const SocketTag& socket_tag,
                       base::TimeDelta timeout_duration,
                       const CommonConnectJobParams* common_connect_job_params,
                       Delegate* delegate,
                       const NetLogWithSource* net_log,
                       NetLogSourceType net_log_source_type,
                       NetLogEventType net_log_connect_event_type)
    : timeout_duration_(timeout_duration),
      priority_(priority),
      socket_tag_(socket_tag),
      common_connect_job_params_(common_connect_job_params),
      delegate_(delegate),
      top_level_job_(connect_job_rust::connect_job_rust_top_level(net_log != nullptr)),
      net_log_(ConnectJobRustBridge::SelectNetLog(
          top_level_job_, net_log, common_connect_job_params, net_log_source_type)),
      net_log_connect_event_type_(net_log_connect_event_type) {
  DCHECK(delegate);
  ConnectJobRustBridge c(this);
  c.Run(ConnectJobRustBridge::Op::kInit);
}

ConnectJob::~ConnectJob() {
  ConnectJobRustBridge::BeforeDestruction(this);
  ConnectJobRustBridge c(this);
  c.Run(ConnectJobRustBridge::Op::kDestroy);
}

std::unique_ptr<StreamSocket> ConnectJob::PassSocket() {
  std::unique_ptr<StreamSocket> result;
  ConnectJobRustBridge c(this);
  c.socket_output = &result;
  c.Run(ConnectJobRustBridge::Op::kPassSocket);
  return result;
}

void ConnectJob::ChangePriority(RequestPriority priority) {
  ConnectJobRustBridge c(this);
  c.Run(ConnectJobRustBridge::Op::kChangePriority, priority);
}

int ConnectJob::Connect() {
  ConnectJobRustBridge c(this, timeout_duration_);
  return static_cast<int>(c.Run(ConnectJobRustBridge::Op::kConnect));
}

ConnectionAttempts ConnectJob::GetConnectionAttempts() const {
  ConnectionAttempts result;
  ConnectJobRustBridge c(this);
  c.attempts = &result;
  c.Run(ConnectJobRustBridge::Op::kDefaultAttempts);
  return result;
}

std::optional<ResolutionDetails> ConnectJob::GetResolutionDetails() const {
  std::optional<ResolutionDetails> result;
  ConnectJobRustBridge c(this);
  c.resolution = &result;
  c.Run(ConnectJobRustBridge::Op::kDefaultResolution);
  return result;
}

bool ConnectJob::IsSSLError() const {
  ConnectJobRustBridge c(this);
  return c.Run(ConnectJobRustBridge::Op::kDefaultSslError) != 0;
}

scoped_refptr<SSLCertRequestInfo> ConnectJob::GetCertRequestInfo() {
  scoped_refptr<SSLCertRequestInfo> result;
  ConnectJobRustBridge c(static_cast<const ConnectJob*>(this));
  c.certificate = &result;
  c.Run(ConnectJobRustBridge::Op::kDefaultCertificate);
  return result;
}

void ConnectJob::set_done_closure(OnConnectJobCompleteCallback done_closure) {
  ConnectJobRustBridge c(this);
  c.done = &done_closure;
  c.Run(ConnectJobRustBridge::Op::kSetDoneClosure);
}

std::optional<HostResolverEndpointResult>
ConnectJob::GetHostResolverEndpointResult() const {
  std::optional<HostResolverEndpointResult> result;
  ConnectJobRustBridge c(this);
  c.endpoint = &result;
  c.Run(ConnectJobRustBridge::Op::kDefaultEndpoint);
  return result;
}

bool ConnectJob::IsConnectedViaStaleDns() const {
  ConnectJobRustBridge c(this);
  return c.Run(ConnectJobRustBridge::Op::kDefaultStaleDns) != 0;
}

void ConnectJob::SetSocket(
    std::unique_ptr<StreamSocket> socket,
    std::optional<base::flat_set<std::string>> dns_aliases) {
  ConnectJobRustBridge c(this);
  c.socket_input = &socket;
  c.aliases = &dns_aliases;
  c.Run(ConnectJobRustBridge::Op::kSetSocket);
}

void ConnectJob::NotifyDelegateOfCompletion(int rv) {
  ConnectJobRustBridge c(this);
  c.Run(ConnectJobRustBridge::Op::kNotifyCompletion, rv);
}

void ConnectJob::NotifyDelegateOfProxyAuth(
    const HttpResponseInfo& response,
    HttpAuthController* auth_controller,
    base::OnceClosure restart_with_auth_callback) {
  ConnectJobRustBridge c(this);
  c.response = &response;
  c.auth_controller = auth_controller;
  c.restart = &restart_with_auth_callback;
  c.Run(ConnectJobRustBridge::Op::kNotifyProxyAuth);
}

void ConnectJob::ResetTimer(base::TimeDelta remaining_time) {
  ConnectJobRustBridge c(this, remaining_time);
  c.Run(ConnectJobRustBridge::Op::kResetTimer);
}

bool ConnectJob::TimerIsRunning() const {
  ConnectJobRustBridge c(this);
  return c.Run(ConnectJobRustBridge::Op::kTimerIsRunning) != 0;
}

void ConnectJob::LogConnectStart() {
  ConnectJobRustBridge c(this);
  c.Run(ConnectJobRustBridge::Op::kLogStart);
}

void ConnectJob::StopTimerAndLogConnectCompletion(int net_error) {
  ConnectJobRustBridge c(this);
  c.Run(ConnectJobRustBridge::Op::kStopAndLogCompletion, net_error);
}

void ConnectJob::OnTimeout() {
  ConnectJobRustBridge c(this);
  connect_job_rust::connect_job_rust_timeout(&c, ConnectJobRustBridge::Service);
}

void ConnectJob::OnTimedOutInternal() {
  ConnectJobRustBridge c(static_cast<const ConnectJob*>(this));
  c.Run(ConnectJobRustBridge::Op::kDefaultTimeoutHook);
}

}  // namespace net
