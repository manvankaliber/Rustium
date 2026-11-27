// Source-only shared BASE primitive projection. No job storage change.
#ifndef NET_SOCKET_CONNECT_JOB_RUST_NATIVE_H_
#define NET_SOCKET_CONNECT_JOB_RUST_NATIVE_H_
#include "net/socket/connect_job.h"
#include "net/socket/connect_job_rust_ffi.h"
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
#include "net/socket/connection_jobs_upper_rust_ffi.h"
namespace net {
// Invocation-local observation, not a receiver field or a heap registry.
// ConnectJob is sequence-bound. Its base destructor invalidates old synchronous
// invocations before registering its own destructor work.
struct NET_EXPORT_PRIVATE ConnectJobRustBridge : connection_jobs_upper_rust::Invocation {
  using Op = connect_job_rust::Operation;
  using Action = connect_job_rust::Action;
  static ConnectJobRustBridge*& Active();
  raw_ptr<ConnectJobRustBridge> previous;
  raw_ptr<ConnectJob> mutable_owner;
  raw_ptr<const ConnectJob> owner;
  raw_ptr<ConnectJob::Delegate> saved_delegate = nullptr;
  raw_ptr<std::unique_ptr<StreamSocket>> socket_input = nullptr;
  raw_ptr<std::unique_ptr<StreamSocket>> socket_output = nullptr;
  raw_ptr<std::optional<base::flat_set<std::string>>> aliases = nullptr;
  raw_ptr<const HttpResponseInfo> response = nullptr;
  raw_ptr<HttpAuthController> auth_controller = nullptr;
  raw_ptr<base::OnceClosure> restart = nullptr;
  raw_ptr<OnConnectJobCompleteCallback> done = nullptr;
  raw_ptr<ConnectionAttempts> attempts = nullptr;
  raw_ptr<std::optional<ResolutionDetails>> resolution = nullptr;
  raw_ptr<scoped_refptr<SSLCertRequestInfo>> certificate = nullptr;
  raw_ptr<std::optional<HostResolverEndpointResult>> endpoint = nullptr;
  base::TimeDelta duration;

  explicit ConnectJobRustBridge(ConnectJob* job,
                                base::TimeDelta timer_duration = {});
  explicit ConnectJobRustBridge(const ConnectJob* job);
  ~ConnectJobRustBridge();
  bool Live() const { return owner != nullptr; }
  ConnectJob* Mutable() const {
    CHECK(mutable_owner);
    return mutable_owner.get();
  }
  void Invalidate() {
    mutable_owner = nullptr;
    owner = nullptr;
    saved_delegate = nullptr;
    socket_input = nullptr;
    socket_output = nullptr;
    aliases = nullptr;
    response = nullptr;
    auth_controller = nullptr;
    restart = nullptr;
    done = nullptr;
    attempts = nullptr;
    resolution = nullptr;
    certificate = nullptr;
    endpoint = nullptr;
  }
  static void BeforeDestruction(ConnectJob* job) {
    connection_jobs_upper_rust::Invocation::Invalidate(job);
  }

  static NetLogWithSource SelectNetLog(
      bool top_level,
      const NetLogWithSource* borrowed,
      const CommonConnectJobParams* common,
      NetLogSourceType source_type) {
    // This is typed materialization of the Rust-selected branch.
    if (top_level) {
      return NetLogWithSource::Make(common->net_log, source_type);
    }
    return *borrowed;
  }
  int64_t Run(Op operation, int64_t argument = 0) {
    return connect_job_rust::connect_job_rust_run(
        static_cast<uint32_t>(operation), argument, this, Service);
  }
  static bool Service(void* opaque,
                      uint32_t wire_action,
                      int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<ConnectJobRustBridge*>(opaque);
    *value = 0;
    if (!c.Live()) {
      return false;
    }
    switch (static_cast<Action>(wire_action)) {
      case Action::kTopLevel:
        *value = c.owner->top_level_job_;
        break;
      case Action::kHasDelegate:
        *value = c.owner->delegate_ != nullptr;
        break;
      case Action::kDurationMicros:
        *value = c.duration.InMicroseconds();
        break;
      case Action::kStartTimer: {
        auto* job = c.Mutable();
        job->timer_.Start(FROM_HERE, c.duration, job, &ConnectJob::OnTimeout);
        break;
      }
      case Action::kStopTimer:
        c.Mutable()->timer_.Stop();
        break;
      case Action::kStampStartNow: {
        const auto now = base::TimeTicks::Now();
        if (!c.Live()) {
          return false;
        }
        c.Mutable()->connect_timing_.connect_start = now;
        break;
      }
      case Action::kBeginConnectEvent: {
        auto* job = c.Mutable();
        job->net_log_.BeginEvent(job->net_log_connect_event_type_);
        break;
      }
      case Action::kConnectInternal:
        *value = c.Mutable()->ConnectInternal();
        break;
      case Action::kStoreFinalError:
        c.Mutable()->net_error_ = static_cast<int>(argument);
        break;
      case Action::kStampEndNow: {
        const auto now = base::TimeTicks::Now();
        if (!c.Live()) {
          return false;
        }
        c.Mutable()->connect_timing_.connect_end = now;
        break;
      }
      case Action::kEndConnectEvent: {
        auto* job = c.Mutable();
        job->net_log_.EndEventWithNetErrorCode(
            job->net_log_connect_event_type_, static_cast<int>(argument));
        break;
      }
      case Action::kTakeDelegate: {
        auto* job = c.Mutable();
        c.saved_delegate = job->delegate_;
        job->delegate_ = nullptr;
        break;
      }
      case Action::kClearDelegate:
        c.Mutable()->delegate_ = nullptr;
        break;
      case Action::kInvokeCompletionDelegate: {
        auto* job = c.Mutable();
        auto* delegate = c.saved_delegate.get();
        c.saved_delegate = nullptr;
        delegate->OnConnectJobComplete(static_cast<int>(argument), job);
        break;
      }
      case Action::kTraceCompletion: {
        // The RAII trace must cover the whole inner sequence and terminal call.
        TRACE_EVENT0(NetTracingCategory(), "ConnectJob::NotifyDelegateOfCompletion");
        *value = connect_job_rust::connect_job_rust_run(
            static_cast<uint32_t>(Op::kCompletionInsideTrace), argument,
            &c, Service);
        break;
      }
      case Action::kInvokeProxyAuthDelegate: {
        auto* job = c.Mutable();
        auto* delegate = job->delegate_.get();
        delegate->OnNeedsProxyAuth(*c.response, c.auth_controller.get(),
                                  std::move(*c.restart), job);
        break;
      }
      case Action::kHasInputSocket:
        *value = c.socket_input && static_cast<bool>(*c.socket_input);
        break;
      case Action::kLogInputSocketSource: {
        // The input remains owned by the wrapper parameter, independently of
        // receiver deletion. Copy its source before touching the receiver.
        const auto source = (*c.socket_input)->NetLog().source();
        if (!c.Live()) {
          return false;
        }
        c.Mutable()->net_log_.AddEventReferencingSource(
            NetLogEventType::CONNECT_JOB_SET_SOCKET, source);
        break;
      }
      case Action::kHasDnsAliases:
        *value = c.aliases && c.aliases->has_value();
        break;
      case Action::kMoveDnsAliases:
        (*c.socket_input)->SetDnsAliases(std::move(c.aliases->value()));
        break;
      case Action::kMoveInputSocket:
        c.Mutable()->socket_ = std::move(*c.socket_input);
        break;
      case Action::kClearSocket:
        c.Mutable()->socket_.reset();
        break;
      case Action::kTimedOutInternal:
        c.Mutable()->OnTimedOutInternal();
        break;
      case Action::kTimedOutEvent:
        c.Mutable()->net_log_.AddEvent(NetLogEventType::CONNECT_JOB_TIMED_OUT);
        break;
      case Action::kBeginTopEvent:
        c.Mutable()->net_log_.BeginEvent(NetLogEventType::CONNECT_JOB);
        break;
      case Action::kEndTopEvent:
        c.Mutable()->net_log_.EndEvent(NetLogEventType::CONNECT_JOB);
        break;
      case Action::kStorePriority:
        c.Mutable()->priority_ = static_cast<RequestPriority>(argument);
        break;
      case Action::kChangePriorityInternal:
        c.Mutable()->ChangePriorityInternal(static_cast<RequestPriority>(argument));
        break;
      case Action::kPassSocket:
        *c.socket_output = std::move(c.Mutable()->socket_);
        break;
      case Action::kInstallDoneClosure: {
        auto* job = c.Mutable();
        // Preserve the original ScopedClosureRunner assignment and its
        // immediate old-callback release. Only the fallback selection is Rust.
        job->done_closure_ = base::ScopedClosureRunner(base::BindOnce(
            [](ConnectJob* receiver, OnConnectJobCompleteCallback closure) {
              const auto error = receiver->net_error_;
              const int selected = connect_job_rust::connect_job_rust_done_error(
                  error.has_value(), error.value_or(0));
              std::move(closure).Run(selected);
            },
            base::Unretained(job), std::move(*c.done)));
        break;
      }
      case Action::kTimerRunning:
        *value = c.owner->timer_.IsRunning();
        break;
      case Action::kEmptyAttempts:
        c.attempts->clear();
        break;
      case Action::kNoResolution:
        c.resolution->reset();
        break;
      case Action::kNoCertificate:
        *c.certificate = nullptr;
        break;
      case Action::kNoEndpoint:
        c.endpoint->reset();
        break;
      default:
        return false;
    }
    // Never read any receiver field after a potentially deleting callback.
    return c.Live();
  }
};


}
#endif
