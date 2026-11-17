// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <type_traits>

#include "base/test/task_environment.h"
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"
#include "net/socket/connection_jobs_upper_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::connection_jobs_const_layout_test {
namespace {
// Compile against both actual headers in the same ordinary runner. No guessed
// byte sizes, offset arithmetic, padding comparison or old-DLL layout casts.
static_assert(sizeof(SSLConnectJob) == sizeof(SSLConnectJobRustOriginal));
static_assert(alignof(SSLConnectJob) == alignof(SSLConnectJobRustOriginal));
static_assert(sizeof(SOCKSConnectJob) == sizeof(SOCKSConnectJobRustOriginal));
static_assert(alignof(SOCKSConnectJob) == alignof(SOCKSConnectJobRustOriginal));
static_assert(sizeof(HttpProxyConnectJob) == sizeof(HttpProxyConnectJobRustOriginal));
static_assert(alignof(HttpProxyConnectJob) == alignof(HttpProxyConnectJobRustOriginal));
static_assert(sizeof(SSLSocketParams) == sizeof(SSLSocketParamsRustOriginal));
static_assert(alignof(SSLSocketParams) == alignof(SSLSocketParamsRustOriginal));
static_assert(sizeof(SOCKSSocketParams) == sizeof(SOCKSSocketParamsRustOriginal));
static_assert(alignof(SOCKSSocketParams) == alignof(SOCKSSocketParamsRustOriginal));
static_assert(sizeof(HttpProxySocketParams) == sizeof(HttpProxySocketParamsRustOriginal));
static_assert(alignof(HttpProxySocketParams) == alignof(HttpProxySocketParamsRustOriginal));
static_assert(alignof(ClientSocketHandle) == alignof(ClientSocketHandleRustOriginal));
static_assert(alignof(ClientSocketPool) == alignof(ClientSocketPoolRustOriginal));
static_assert(alignof(StreamSocketHandle) == alignof(StreamSocketHandleRustOriginal));
static_assert(alignof(SocketPoolAdditionalCapacity) == alignof(SocketPoolAdditionalCapacityRustOriginal));

using Timing = std::array<base::TimeTicks, 6>;
template <class Job>
Timing ReadTiming(const Job& job) {
  const auto& timing = job.connect_timing();
  return {timing.domain_lookup_start, timing.domain_lookup_end,
          timing.connect_start, timing.connect_end,
          timing.ssl_start, timing.ssl_end};
}

template <class Types>
void CheckConstTls(base::test::TaskEnvironment& environment) {
  using namespace connection_jobs_job_workloads;
  Fixture<Types> fixture(Case::kTlsSync, environment);
  Delegate<Types> delegate;
  using Tls = typename Types::Tls;
  const Tls job(DEFAULT_PRIORITY, SocketTag(), &*fixture.common,
                fixture.tls_params, &delegate, nullptr);
  const auto before = ReadTiming(job);
  const auto priority = job.priority();
  const auto source = job.net_log().source();
  for (unsigned i = 0; i < 8; ++i) {
    EXPECT_TRUE(job.GetConnectionAttempts().empty());
    EXPECT_FALSE(job.GetResolutionDetails().has_value());
    EXPECT_EQ(ResolveErrorInfo(OK), job.GetResolveErrorInfo());
    EXPECT_FALSE(job.IsSSLError());
    EXPECT_FALSE(job.HasEstablishedConnection());
    EXPECT_EQ(before, ReadTiming(job));
    EXPECT_EQ(priority, job.priority());
    EXPECT_EQ(source.id, job.net_log().source().id);
    EXPECT_EQ(source.type, job.net_log().source().type);
  }
  // GetLoadState on an unstarted original SSL job would read its uninitialized
  // next_state_, so this ordinary const test intentionally uses defined APIs.
}

TEST(ConnectionJobsConstBoundaryTest, GenuinelyConstTlsQueriesRemainReadOnly) {
  base::test::TaskEnvironment environment{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  CheckConstTls<connection_jobs_job_workloads::Original>(environment);
  CheckConstTls<connection_jobs_job_workloads::Production>(environment);
}

TEST(ConnectionJobsConstBoundaryTest, EmptyConstStreamDoesNotMutateOutput) {
  const connection_jobs_upper_test::Stream<connection_jobs_upper_test::Original> original;
  const connection_jobs_upper_test::Stream<connection_jobs_upper_test::Production> rust;
  LoadTimingInfo expected;
  expected.socket_log_id = 731;
  expected.socket_reused = true;
  LoadTimingInfo a(expected), b(expected);
  EXPECT_FALSE(original.GetLoadTimingInfo(false, &a));
  EXPECT_FALSE(rust.GetLoadTimingInfo(false, &b));
  EXPECT_EQ(731u, a.socket_log_id);
  EXPECT_EQ(731u, b.socket_log_id);
  EXPECT_TRUE(a.socket_reused);
  EXPECT_TRUE(b.socket_reused);
  EXPECT_TRUE(a.connect_timing.connect_start.is_null());
  EXPECT_TRUE(b.connect_timing.connect_start.is_null());
}
}  // namespace
}  // namespace net::connection_jobs_const_layout_test
