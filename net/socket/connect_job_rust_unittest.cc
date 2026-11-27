// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connect_job_rust_test_support.h"

#include <algorithm>
#include <string>

#include "net/ssl/ssl_cert_request_info.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::connect_job_rust_test {
namespace {
std::vector<Event> Select(const Trace& trace, Mark mark) {
  std::vector<Event> selected;
  for (const auto& event : trace.events) {
    if (event.mark == mark) { selected.push_back(event); }
  }
  return selected;
}
size_t Index(const Trace& trace, Mark mark, int64_t a) {
  auto it = std::find_if(trace.events.begin(), trace.events.end(),
                         [&](const Event& event) {
                           return event.mark == mark && event.a == a;
                         });
  EXPECT_NE(it, trace.events.end());
  return static_cast<size_t>(it - trace.events.begin());
}
void ExpectOriginalContract(Scenario kind, const Trace& trace) {
  const auto completed = Select(trace, Mark::kComplete);
  const auto snapshots = Select(trace, Mark::kState);
  for (const auto& event : completed) {
    ASSERT_TRUE(event.state);
    EXPECT_FALSE(event.state->delegate);
    EXPECT_FALSE(event.state->timer);
    EXPECT_EQ(event.state->error, event.a);
    EXPECT_GE(event.state->end, 0);
  }
  switch (kind) {
    case Scenario::kTopConstructor:
    case Scenario::kNestedConstructor: {
      ASSERT_EQ(1u, snapshots.size());
      EXPECT_TRUE(snapshots.front().state->delegate);
      EXPECT_FALSE(snapshots.front().state->timer);
      EXPECT_FALSE(snapshots.front().state->error);
      EXPECT_EQ(-1, snapshots.front().state->start);
      size_t outer_events = 0;
      for (const auto& event : trace.events) {
        outer_events += event.mark == Mark::kLog &&
                        event.a == static_cast<int>(NetLogEventType::CONNECT_JOB);
      }
      // Count actual event headers rather than dictionary records (a=12345/-3).
      EXPECT_EQ(kind == Scenario::kTopConstructor ? 2u : 0u, outer_events);
      break;
    }
    case Scenario::kSyncSuccess:
    case Scenario::kSyncError:
    case Scenario::kAsyncSuccess:
    case Scenario::kAsyncError: {
      const bool sync = kind == Scenario::kSyncSuccess || kind == Scenario::kSyncError;
      EXPECT_EQ(sync ? 0u : 1u, completed.size());
      ASSERT_FALSE(snapshots.empty());
      EXPECT_FALSE(snapshots.front().state->delegate);
      EXPECT_FALSE(snapshots.front().state->timer);
      bool saw_end = false;
      for (const auto& event : trace.events) {
        if (event.mark == Mark::kLog && event.state &&
            event.a == static_cast<int>(NetLogEventType::TRANSPORT_CONNECT_JOB_CONNECT) &&
            event.b == static_cast<int>(NetLogEventPhase::END)) {
          saw_end = true;
          // Intentionally different historical observation order.
          EXPECT_EQ(sync, event.state->delegate);
          EXPECT_FALSE(event.state->timer);
        }
      }
      EXPECT_TRUE(saw_end);
      break;
    }
    case Scenario::kPendingCancel: {
      EXPECT_TRUE(completed.empty());
      const auto done = Select(trace, Mark::kDone);
      ASSERT_EQ(1u, done.size());
      EXPECT_EQ(ERR_ABORTED, done.front().b);
      EXPECT_TRUE(done.front().state->delegate);
      EXPECT_FALSE(done.front().state->timer);
      break;
    }
    case Scenario::kTimeout:
    case Scenario::kTimeoutDelete: {
      ASSERT_EQ(1u, completed.size());
      EXPECT_EQ(ERR_TIMED_OUT, completed.front().a);
      EXPECT_FALSE(completed.front().state->socket);
      const auto hooks = Select(trace, Mark::kTimeout);
      ASSERT_EQ(1u, hooks.size());
      EXPECT_EQ(0, hooks.front().a);  // Socket already destroyed at virtual hook.
      EXPECT_EQ(0, hooks.front().b);  // Timer no longer running.
      EXPECT_LT(Index(trace, Mark::kSocketDestroy, 1),
                Index(trace, Mark::kTimeout, 0));
      EXPECT_LT(Index(trace, Mark::kTimeout, 0),
                Index(trace, Mark::kComplete, ERR_TIMED_OUT));
      break;
    }
    case Scenario::kResetTimer:
      ASSERT_EQ(3u, snapshots.size());
      EXPECT_TRUE(snapshots.front().state->timer);
      EXPECT_FALSE(snapshots.at(1).state->timer);
      EXPECT_FALSE(snapshots.back().state->timer);
      ASSERT_EQ(1u, completed.size());
      EXPECT_EQ(ERR_TIMED_OUT, completed.front().a);
      break;
    case Scenario::kPriority:
    case Scenario::kPriorityDelete:
      for (const auto& event : Select(trace, Mark::kPriority)) {
        EXPECT_EQ(event.a, event.b);  // Stored before derived virtual observes it.
      }
      break;
    case Scenario::kSocketAliases:
    case Scenario::kAliasReentry:
      EXPECT_LT(Index(trace, Mark::kSocketLog, 2), Index(trace, Mark::kAliases, 2));
      EXPECT_LT(Index(trace, Mark::kAliases, 2), Index(trace, Mark::kSocketDestroy, 1));
      EXPECT_LT(Index(trace, Mark::kAliases, 3), Index(trace, Mark::kSocketDestroy, 2));
      break;
    case Scenario::kPassSocket:
      ASSERT_EQ(1u, snapshots.size());
      EXPECT_FALSE(snapshots.front().state->socket);
      EXPECT_EQ(1u, Select(trace, Mark::kSocketDestroy).size());
      break;
    case Scenario::kProxyAuth:
    case Scenario::kAuthDelete: {
      const auto auth = Select(trace, Mark::kProxyAuth);
      ASSERT_EQ(1u, auth.size());
      EXPECT_EQ(1, auth.front().a);  // Same response reference forwarded.
      EXPECT_EQ(1, auth.front().b);  // Null controller forwarded exactly.
      EXPECT_TRUE(auth.front().state->delegate);
      EXPECT_TRUE(auth.front().state->timer);
      EXPECT_EQ(kind == Scenario::kProxyAuth ? 1u : 0u,
                Select(trace, Mark::kRestart).size());
      EXPECT_TRUE(completed.empty());
      break;
    }
    case Scenario::kDoneReplace: {
      const auto done = Select(trace, Mark::kDone);
      ASSERT_EQ(3u, done.size());
      EXPECT_EQ(1, done.front().a);
      EXPECT_EQ(ERR_ABORTED, done.front().b);
      EXPECT_EQ(2, done.at(1).a);
      EXPECT_EQ(ERR_FAILED, done.at(1).b);
      EXPECT_EQ(3, done.back().a);
      EXPECT_EQ(ERR_FAILED, done.back().b);
      EXPECT_LT(Index(trace, Mark::kDerivedDestroy, 0), Index(trace, Mark::kDone, 3));
      EXPECT_LT(Index(trace, Mark::kDone, 3), Index(trace, Mark::kSocketDestroy, 1));
      EXPECT_TRUE(done.back().state->socket);
      break;
    }
    case Scenario::kConstQueries: {
      const auto returns = Select(trace, Mark::kReturn);
      ASSERT_EQ(33u, returns.size());
      EXPECT_EQ(1, returns.back().a);  // All named fields unchanged.
      break;
    }
    case Scenario::kCompleteDelete:
    case Scenario::kCompletionReentry:
      ASSERT_EQ(1u, completed.size());
      EXPECT_EQ(OK, completed.front().a);
      EXPECT_LT(Index(trace, Mark::kComplete, OK),
                Index(trace, Mark::kSocketDestroy, 1));
      break;
    case Scenario::kPendingSelfDelete:
      EXPECT_TRUE(completed.empty());
      EXPECT_EQ(1u, Select(trace, Mark::kDerivedDestroy).size());
      break;
  }
}

class ConnectJobRustContractTest : public testing::TestWithParam<Scenario> {
 protected:
  base::test::TaskEnvironment environment{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};
TEST_P(ConnectJobRustContractTest, ExactRegularLifecycleAndVirtualOrder) {
  const Inputs<ConnectJobRustOriginal> original_inputs;
  const Inputs<ConnectJob> rust_inputs;
  const Trace original = Run<ConnectJobRustOriginal>(GetParam(), environment, original_inputs);
  const Trace rust = Run<ConnectJob>(GetParam(), environment, rust_inputs);
  ExpectOriginalContract(GetParam(), original);
  ExpectOriginalContract(GetParam(), rust);
  EXPECT_EQ(original.events, rust.events);
  EXPECT_EQ(original.checksum, rust.checksum);
}
INSTANTIATE_TEST_SUITE_P(
    BaseOnly, ConnectJobRustContractTest,
    testing::Values(Scenario::kTopConstructor, Scenario::kNestedConstructor,
                    Scenario::kSyncSuccess, Scenario::kSyncError,
                    Scenario::kPendingCancel, Scenario::kAsyncSuccess,
                    Scenario::kAsyncError, Scenario::kTimeout,
                    Scenario::kResetTimer, Scenario::kPriority,
                    Scenario::kSocketAliases, Scenario::kPassSocket,
                    Scenario::kProxyAuth, Scenario::kDoneReplace,
                    Scenario::kConstQueries, Scenario::kCompleteDelete,
                    Scenario::kAuthDelete, Scenario::kPriorityDelete,
                    Scenario::kCompletionReentry, Scenario::kAliasReentry,
                    Scenario::kPendingSelfDelete, Scenario::kTimeoutDelete));

TEST(ConnectJobRustStructureTest, IndependentParamsCopyAndAssignment) {
  auto original = MakeParams<ConnectJobRustOriginal>();
  auto rust = MakeParams<ConnectJob>();
  const CommonConnectJobParamsRustOriginal original_copy(original);
  const CommonConnectJobParams rust_copy(rust);
  original = original_copy;
  rust = rust_copy;
  EXPECT_EQ(original.net_log.get(), rust.net_log.get());
  EXPECT_EQ(nullptr, original.host_resolver.get());
  EXPECT_EQ(nullptr, rust.host_resolver.get());
  EXPECT_EQ(nullptr, original.ignore_certificate_errors.get());
  EXPECT_EQ(nullptr, rust.ignore_certificate_errors.get());
}
TEST(ConnectJobRustStructureTest, CertificateDefaultIsNullWithoutStateMutation) {
  base::test::TaskEnvironment environment{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  Trace trace(false);
  auto params = MakeParams<ConnectJob>();
  auto original_params = MakeParams<ConnectJobRustOriginal>();
  Delegate<ConnectJob> delegate(trace);
  Delegate<ConnectJobRustOriginal> original_delegate(trace);
  Job<ConnectJob> job(trace, &params, &delegate, base::TimeDelta());
  Job<ConnectJobRustOriginal> original(
      trace, &original_params, &original_delegate, base::TimeDelta());
  const auto now = base::TimeTicks::Now();
  const auto before = ConnectJobRustTestAccess::Read(
      static_cast<const ConnectJob&>(job), now);
  const auto original_before = ConnectJobRustTestAccess::Read(
      static_cast<const ConnectJobRustOriginal&>(original), now);
  EXPECT_EQ(nullptr, job.GetCertRequestInfo());
  EXPECT_EQ(nullptr, original.GetCertRequestInfo());
  EXPECT_EQ(before, ConnectJobRustTestAccess::Read(
                        static_cast<const ConnectJob&>(job), now));
  EXPECT_EQ(original_before, ConnectJobRustTestAccess::Read(
                                 static_cast<const ConnectJobRustOriginal&>(original), now));
}
}  // namespace
}  // namespace net::connect_job_rust_test
