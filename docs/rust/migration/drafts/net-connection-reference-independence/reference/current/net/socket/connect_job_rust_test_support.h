// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECT_JOB_RUST_TEST_SUPPORT_H_
#define NET_SOCKET_CONNECT_JOB_RUST_TEST_SUPPORT_H_

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/test/task_environment.h"
#include "net/http/http_response_info.h"
#include "net/log/net_log.h"
#include "net/log/net_log_entry.h"
#include "net/socket/connect_job.h"
#include "net/socket/connect_job_rust_original.h"
#include "net/socket/stream_socket.h"

namespace net {

// Read-only friend on both classes. No field, virtual method or layout change.
// Never compare padding/object representations or inspect a dead receiver.
struct ConnectJobRustTestAccess {
  struct State {
    bool delegate;
    bool timer;
    bool socket;
    int priority;
    std::optional<int> error;
    int64_t start;
    int64_t end;
    bool operator==(const State&) const = default;
  };
  template <class Base>
  static State Read(const Base& job, base::TimeTicks origin) {
    return {job.delegate_ != nullptr,
            job.timer_.IsRunning(),
            job.socket_ != nullptr,
            static_cast<int>(job.priority_),
            job.net_error_,
            job.connect_timing_.connect_start.is_null()
                ? -1
                : (job.connect_timing_.connect_start - origin).InMicroseconds(),
            job.connect_timing_.connect_end.is_null()
                ? -1
                : (job.connect_timing_.connect_end - origin).InMicroseconds()};
  }
};

namespace connect_job_rust_test {
using State = ConnectJobRustTestAccess::State;
enum class Mark : int {
  kDerivedDestroy,
  kConnect,
  kPriority,
  kTimeout,
  kComplete,
  kProxyAuth,
  kRestart,
  kDone,
  kSocketLog,
  kAliases,
  kSocketDestroy,
  kState,
  kLog,
  kLogParams,
  kReturn,
};
struct Event {
  Mark mark;
  int64_t a = 0;
  int64_t b = 0;
  std::optional<State> state;
  bool operator==(const Event&) const = default;
};
struct Trace {
  ~Trace();
  Trace(const Trace&);
  Trace(Trace&&) noexcept;
  Trace& operator=(const Trace&);
  Trace& operator=(Trace&&) noexcept;

  explicit Trace(bool record);
  void Add(Mark mark, int64_t a = 0, int64_t b = 0,
           std::optional<State> state = std::nullopt);
  bool record;
  uint32_t expected_source = 0;
  uint64_t checksum = 0;
  std::vector<Event> events;
};

// Called only on the suite's single sequence. OnAddEntry never calls a NetLog
// method; the read-only probe reads job fields/timer without logging or mutation.
// No deleting/log-recursive observer is used (invalid in the original API).
class Observer final : public NetLog::ThreadSafeObserver {
 public:
  explicit Observer(Trace& trace);
  ~Observer() override;
  void OnAddEntry(const NetLogEntry& entry) override;
  std::function<std::optional<State>()> probe;

 private:
  raw_ref<Trace> trace_;
};

class Socket final : public StreamSocket {
 public:
  Socket(Trace& trace, int id);
  ~Socket() override;
  int Read(IOBuffer*, int, CompletionOnceCallback) override;
  int Write(IOBuffer*, int, CompletionOnceCallback,
            const NetworkTrafficAnnotationTag&) override;
  int SetReceiveBufferSize(int32_t) override;
  int SetSendBufferSize(int32_t) override;
  int Connect(CompletionOnceCallback) override;
  void Disconnect() override;
  bool IsConnected() const override;
  bool IsConnectedAndIdle() const override;
  int GetPeerAddress(IPEndPoint*) const override;
  int GetLocalAddress(IPEndPoint*) const override;
  const NetLogWithSource& NetLog() const override;
  bool WasEverUsed() const override;
  NextProto GetNegotiatedProtocol() const override;
  bool GetSSLInfo(SSLInfo*) override;
  int64_t GetTotalReceivedBytes() const override;
  void ApplySocketTag(const SocketTag&) override;
  void SetDnsAliases(base::flat_set<std::string> aliases) override;
  std::function<void()> on_aliases;

 private:
  raw_ref<Trace> trace_;
  int id_;
  NetLogWithSource log_;
};

template <class Base> struct Types;
template <> struct Types<ConnectJob> { using Params = CommonConnectJobParams; };
template <> struct Types<ConnectJobRustOriginal> {
  using Params = CommonConnectJobParamsRustOriginal;
};
template <class Base>
typename Types<Base>::Params MakeParams() {
  return typename Types<Base>::Params(
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr, NetLog::Get(), nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr);
}

template <class Base>
class Job final : public Base {
 public:
  Job(Trace& trace, const typename Types<Base>::Params* params,
      typename Base::Delegate* delegate, base::TimeDelta timeout,
      const NetLogWithSource* log = nullptr)
      : Base(DEFAULT_PRIORITY, SocketTag(), timeout, params, delegate, log,
             NetLogSourceType::TRANSPORT_CONNECT_JOB,
             NetLogEventType::TRANSPORT_CONNECT_JOB_CONNECT), trace_(trace) {}
  ~Job() override { trace_->Add(Mark::kDerivedDestroy); }
  using Base::NotifyDelegateOfCompletion;
  using Base::NotifyDelegateOfProxyAuth;
  using Base::ResetTimer;
  using Base::SetSocket;
  using Base::TimerIsRunning;
  LoadState GetLoadState() const override { return LOAD_STATE_IDLE; }
  bool HasEstablishedConnection() const override { return false; }
  ResolveErrorInfo GetResolveErrorInfo() const override { return ResolveErrorInfo(OK); }
  int result = ERR_IO_PENDING;
  std::function<void()> on_connect;
  std::function<void(RequestPriority)> on_priority;
  std::function<void()> on_timeout;

 private:
  int ConnectInternal() override {
    trace_->Add(Mark::kConnect, result, this->TimerIsRunning());
    const int rv = result;
    // Cache all callback state before invoking a terminal deleting callback.
    auto callback = on_connect;
    if (callback) { callback(); }
    return rv;
  }
  void ChangePriorityInternal(RequestPriority priority) override {
    trace_->Add(Mark::kPriority, priority, this->priority());
    auto callback = on_priority;
    if (callback) { callback(priority); }
  }
  void OnTimedOutInternal() override {
    trace_->Add(Mark::kTimeout, this->socket() != nullptr,
               this->TimerIsRunning());
    if (on_timeout) { on_timeout(); }
  }
  raw_ref<Trace> trace_;
};

template <class Base>
class Delegate final : public Base::Delegate {
 public:
  explicit Delegate(Trace& trace) : trace_(trace) {}
  void OnConnectJobComplete(int rv, Base* job) override {
    trace_->Add(Mark::kComplete, rv, 0,
               ConnectJobRustTestAccess::Read(*job, origin));
    auto callback = complete;
    if (callback) { callback(job); }
  }
  void OnNeedsProxyAuth(const HttpResponseInfo& response,
                       HttpAuthController* controller,
                       base::OnceClosure restart, Base* job) override {
    trace_->Add(Mark::kProxyAuth, &response == expected_response,
               controller == nullptr,
               ConnectJobRustTestAccess::Read(*job, origin));
    auto callback = auth;
    if (callback) { callback(job, std::move(restart)); }
  }
  base::TimeTicks origin;
  raw_ptr<const HttpResponseInfo> expected_response = nullptr;
  std::function<void(Base*)> complete;
  std::function<void(Base*, base::OnceClosure)> auth;

 private:
  raw_ref<Trace> trace_;
};

enum class Scenario : unsigned {
  kTopConstructor, kNestedConstructor, kSyncSuccess, kSyncError,
  kPendingCancel, kAsyncSuccess, kAsyncError, kTimeout, kResetTimer,
  kPriority, kSocketAliases, kPassSocket, kProxyAuth, kDoneReplace,
  kConstQueries, kCompleteDelete, kAuthDelete, kPriorityDelete,
  kCompletionReentry, kAliasReentry, kPendingSelfDelete,
  kTimeoutDelete,
};
inline constexpr std::array<const char*, 15> kWorkloadNames = {
    "ctor_top", "ctor_nested", "sync_ok_timer", "sync_error",
    "pending_cancel", "async_ok", "async_error", "timeout", "reset_timer",
    "priority_changes", "socket_aliases", "pass_socket", "proxy_auth",
    "done_replace", "const_queries"};

template <class Base>
struct Inputs {
  typename Types<Base>::Params params = MakeParams<Base>();
  // Construct the auth-response fixture before measurement. Its own policy is
  // outside ConnectJob BASE; no migrated response-info constructor is timed.
  HttpResponseInfo auth_response;
};

// No production subclass or socket implementation is called. The task clock,
// timer/callback ownership and NetLog transport are explicit native services.
// With record=false, log capturing is off and transcript vectors are not filled;
// checksums still retain all virtual callbacks, return values and named states.
template <class Base>
Trace Run(Scenario kind, base::test::TaskEnvironment& environment,
          const Inputs<Base>& inputs, bool record = true) {
  Trace trace(record);
  const auto origin = base::TimeTicks::Now();
  Delegate<Base> delegate(trace);
  delegate.origin = origin;
  std::optional<Observer> observer;
  if (record) { observer.emplace(trace); }
  std::optional<NetLogWithSource> nested_log;
  if (kind == Scenario::kNestedConstructor) {
    nested_log.emplace(NetLogWithSource::Make(
        NetLog::Get(), NetLogSourceType::TRANSPORT_CONNECT_JOB));
  }
  auto job = std::make_unique<Job<Base>>(
      trace, &inputs.params, &delegate, base::Seconds(10),
      nested_log ? &*nested_log : nullptr);
  Base* const base_job = job.get();
  if (observer) {
    observer->probe = [&]() -> std::optional<State> {
      return ConnectJobRustTestAccess::Read(*base_job, origin);
    };
  }
  const auto state = [&] {
    trace.Add(Mark::kState, 0, 0,
               ConnectJobRustTestAccess::Read(static_cast<const Base&>(*job), origin));
  };
  const auto connect = [&] {
    const int rv = job->Connect();
    trace.Add(Mark::kReturn, rv);
  };
  const auto done = [&](int id) {
    job->set_done_closure(base::BindOnce(
        [](Trace* trace, Base* job, base::TimeTicks origin, int id, int rv) {
          trace->Add(Mark::kDone, id, rv,
                     ConnectJobRustTestAccess::Read(*job, origin));
        }, &trace, base_job, origin, id));
  };
  switch (kind) {
    case Scenario::kTopConstructor:
    case Scenario::kNestedConstructor:
      state();
      break;
    case Scenario::kSyncSuccess:
    case Scenario::kSyncError:
      job->result = kind == Scenario::kSyncSuccess ? OK : ERR_FAILED;
      connect();
      state();
      environment.FastForwardBy(base::Seconds(20));
      state();
      break;
    case Scenario::kPendingCancel:
      connect();
      done(1);
      state();
      break;
    case Scenario::kAsyncSuccess:
    case Scenario::kAsyncError:
      connect();
      job->NotifyDelegateOfCompletion(kind == Scenario::kAsyncSuccess ? OK : ERR_FAILED);
      state();
      environment.FastForwardBy(base::Seconds(20));
      break;
    case Scenario::kTimeout:
    case Scenario::kTimeoutDelete:
      if (kind == Scenario::kTimeoutDelete) {
        delegate.complete = [&](Base*) { job.reset(); };
      }
      job->SetSocket(std::make_unique<Socket>(trace, 1), std::nullopt);
      job->on_timeout = [&] { job->ChangePriority(HIGHEST); };
      connect();
      done(1);
      environment.FastForwardBy(base::Seconds(10) - base::Microseconds(1));
      state();
      environment.FastForwardBy(base::Microseconds(1));
      if (job) { state(); }
      break;
    case Scenario::kResetTimer:
      connect();
      environment.FastForwardBy(base::Seconds(9));
      job->ResetTimer(base::Seconds(10));
      environment.FastForwardBy(base::Seconds(9));
      state();
      job->ResetTimer(base::TimeDelta());
      environment.FastForwardBy(base::Seconds(20));
      state();
      job->ResetTimer(base::Microseconds(1));
      environment.FastForwardBy(base::Microseconds(1));
      state();
      break;
    case Scenario::kPriority:
      job->on_priority = [&](RequestPriority priority) {
        if (priority == HIGHEST) { job->ChangePriority(LOWEST); }
      };
      job->ChangePriority(HIGHEST);
      job->ChangePriority(MEDIUM);
      state();
      break;
    case Scenario::kSocketAliases:
    case Scenario::kAliasReentry: {
      job->SetSocket(std::make_unique<Socket>(trace, 1), std::nullopt);
      auto incoming = std::make_unique<Socket>(trace, 2);
      if (kind == Scenario::kAliasReentry) {
        incoming->on_aliases = [&] {
          // Original-defined: old socket is still installed during this virtual.
          trace.Add(Mark::kReturn, job->socket() != nullptr);
          job->ChangePriority(HIGHEST);
        };
      }
      job->SetSocket(std::move(incoming), base::flat_set<std::string>{"a", "b"});
      trace.Add(Mark::kReturn, job->socket()->GetDnsAliases().size());
      job->SetSocket(std::make_unique<Socket>(trace, 3), base::flat_set<std::string>{});
      job->SetSocket(nullptr, base::flat_set<std::string>{"ignored-for-null"});
      state();
      break;
    }
    case Scenario::kPassSocket: {
      job->SetSocket(std::make_unique<Socket>(trace, 1), std::nullopt);
      auto socket = job->PassSocket();
      trace.Add(Mark::kReturn, socket != nullptr, job->PassSocket() == nullptr);
      state();
      socket.reset();
      break;
    }
    case Scenario::kProxyAuth:
    case Scenario::kAuthDelete: {
      const HttpResponseInfo& response = inputs.auth_response;
      delegate.expected_response = &response;
      delegate.auth = [&](Base*, base::OnceClosure restart) {
        if (kind == Scenario::kAuthDelete) {
          job.reset();
        } else {
          std::move(restart).Run();
        }
      };
      connect();
      job->NotifyDelegateOfProxyAuth(response, nullptr,
          base::BindOnce([](Trace* trace) { trace->Add(Mark::kRestart); }, &trace));
      if (job) { state(); }
      break;
    }
    case Scenario::kDoneReplace:
      job->SetSocket(std::make_unique<Socket>(trace, 1), std::nullopt);
      done(1);
      done(2);  // Original assignment invokes previous callback immediately.
      connect();
      job->NotifyDelegateOfCompletion(ERR_FAILED);
      done(3);  // Callback2 now sees the final error, not its install-time state.
      break;
    case Scenario::kConstQueries: {
      // Use a genuinely const receiver; this must never enter mutable dispatch.
      // The separate mutable fixture above remains alive but is not queried.
      // This branch's constructor/destructor logs cannot use the first job's
      // state probe. Named snapshots below inspect the actual const receiver.
      if (observer) { observer->probe = {}; }
      const Job<Base> constant(trace, &inputs.params, &delegate, base::TimeDelta());
      const State before = ConnectJobRustTestAccess::Read(
          static_cast<const Base&>(constant), origin);
      for (int i = 0; i < 8; ++i) {
        trace.Add(Mark::kReturn, constant.GetConnectionAttempts().empty(),
                   !constant.GetResolutionDetails().has_value());
        trace.Add(Mark::kReturn, constant.IsSSLError(),
                   constant.GetHostResolverEndpointResult().has_value());
        trace.Add(Mark::kReturn, constant.IsConnectedViaStaleDns(), constant.priority());
        trace.Add(Mark::kReturn, constant.net_log().source().type ==
                                     NetLogSourceType::TRANSPORT_CONNECT_JOB);
      }
      trace.Add(Mark::kReturn,
                 before == ConnectJobRustTestAccess::Read(
                               static_cast<const Base&>(constant), origin));
      break;
    }
    case Scenario::kCompleteDelete:
    case Scenario::kCompletionReentry:
      delegate.complete = [&](Base*) {
        if (kind == Scenario::kCompleteDelete) {
          job.reset();
        } else {
          job->ChangePriority(HIGHEST);
          auto transferred = job->PassSocket();
          trace.Add(Mark::kReturn, transferred != nullptr);
        }
      };
      job->SetSocket(std::make_unique<Socket>(trace, 1), std::nullopt);
      connect();
      job->NotifyDelegateOfCompletion(OK);
      if (job) { state(); }
      break;
    case Scenario::kPriorityDelete:
      job->on_priority = [&](RequestPriority) { job.reset(); };
      job->ChangePriority(HIGHEST);
      break;
    case Scenario::kPendingSelfDelete:
      job->on_connect = [&] { job.reset(); };
      // Original reads only local rv after ConnectInternal returning pending.
      // No use of the deleted object or callback wrapper after that return.
      connect();
      break;
  }
  job.reset();
  if (observer) { observer->probe = {}; }
  return trace;
}

static_assert(sizeof(ConnectJob) == sizeof(ConnectJobRustOriginal));
static_assert(alignof(ConnectJob) == alignof(ConnectJobRustOriginal));
static_assert(sizeof(CommonConnectJobParams) == sizeof(CommonConnectJobParamsRustOriginal));
static_assert(alignof(CommonConnectJobParams) == alignof(CommonConnectJobParamsRustOriginal));

}  // namespace connect_job_rust_test
}  // namespace net
#endif  // NET_SOCKET_CONNECT_JOB_RUST_TEST_SUPPORT_H_
