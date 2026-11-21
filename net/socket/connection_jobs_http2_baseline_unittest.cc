// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"

#include <type_traits>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/raw_ref.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/http/http_response_headers.h"
#include "net/spdy/spdy_stream.h"
#include "net/spdy/spdy_stream_family_original.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::connection_jobs_job_workloads {
namespace {

static_assert(std::is_same_v<
    decltype(CommonConnectJobParamsRustOriginal::spdy_session_pool),
    raw_ptr<SpdySessionPoolOriginal>>);
static_assert(!std::is_convertible_v<SpdySessionPool*, SpdySessionPoolOriginal*>);
static_assert(!std::is_convertible_v<SpdyStream*, SpdyStreamOriginal*>);

struct CallbackResult {
  int immediate = ERR_FAILED;
  int completion = ERR_FAILED;
  unsigned completions = 0;
  unsigned auth_challenges = 0;
  int response_code = 0;
  bool consumed_writes = false;
  bool operator==(const CallbackResult&) const = default;
};

template <class Types>
class DeletingDelegate final : public Types::Base::Delegate {
 public:
  DeletingDelegate(std::unique_ptr<typename Types::Base>& job,
                   CallbackResult& result)
      : job_(job), result_(result) {}
  void OnConnectJobComplete(int rv, typename Types::Base*) override {
    result_->completion = rv;
    ++result_->completions;
    job_->reset();  // Callback may synchronously destroy the complete owner chain.
  }
  void OnNeedsProxyAuth(const HttpResponseInfo& response, HttpAuthController*,
                       base::OnceClosure restart,
                       typename Types::Base*) override {
    result_->response_code = response.headers->response_code();
    ++result_->auth_challenges;
    restart_ = std::move(restart);
    job_->reset();
  }
  void RunStaleRestart() {
    if (restart_) std::move(restart_).Run();
  }
 private:
  raw_ref<std::unique_ptr<typename Types::Base>> job_;
  raw_ref<CallbackResult> result_;
  base::OnceClosure restart_;
};

// Ordinary untimed probe. Immutable reply storage precedes the fixture so the
// native socket data cannot outlive its borrowed wire bytes.
template <class Types>
CallbackResult ReplyAndDelete(base::test::TaskEnvironment& environment,
                              bool auth_challenge) {
  SpdyTestUtil wire;
  const std::string_view challenge[] = {"proxy-authenticate", "Basic realm=\"r\""};
  auto reply = wire.ConstructSpdyReplyError(
      auth_challenge ? "407" : "502",
      auth_challenge ? base::span<const std::string_view>(challenge)
                     : base::span<const std::string_view>(), 1);
  Fixture<Types> fixture(Case::kHttp2Pending, environment);
  auto script = std::make_unique<Script>();
  const std::string_view headers[] = {"user-agent", "test-ua"};
  script->h2_request.emplace(wire.ConstructSpdyConnect(
      headers, 1, DEFAULT_PRIORITY, HostPortPair("www.endpoint.test", 443)));
  script->h2_reset.emplace(wire.ConstructSpdyRstStream(1, spdy::ERROR_CODE_CANCEL));
  script->writes.push_back(CreateMockWrite(*script->h2_request, 0));
  script->reads.push_back(CreateMockRead(reply, 1));
  script->reads.emplace_back(SYNCHRONOUS, ERR_IO_PENDING, 2);
  script->writes.push_back(CreateMockWrite(*script->h2_reset, 3));
  script->data = std::make_unique<SequencedSocketData>(script->reads, script->writes);
  script->data->set_connect_data(MockConnect(SYNCHRONOUS, OK));
  script->ssl = std::make_unique<SSLSocketDataProvider>(SYNCHRONOUS, OK);
  script->ssl->next_proto = NextProto::kProtoHTTP2;
  fixture.deps.socket_factory->AddSocketDataProvider(script->data.get());
  fixture.deps.socket_factory->AddSSLSocketDataProvider(script->ssl.get());
  fixture.scripts.push_back(std::move(script));

  CallbackResult result;
  std::unique_ptr<typename Types::Base> job;
  DeletingDelegate<Types> delegate(job, result);
  job = std::make_unique<typename Types::Proxy>(
      DEFAULT_PRIORITY, SocketTag(), &*fixture.common, fixture.proxy_params,
      &delegate, nullptr);
  result.immediate = job->Connect();
  EXPECT_EQ(result.immediate, ERR_IO_PENDING);
  environment.RunUntilIdle();
  EXPECT_FALSE(job);
  delegate.RunStaleRestart();  // Original job's weak callback must stay canceled.
  environment.RunUntilIdle();
  result.consumed_writes = fixture.scripts[0]->data->AllWriteDataConsumed();
  EXPECT_TRUE(result.consumed_writes);
  Types::CloseHttp2(*fixture.session, fixture.http2_context);
  environment.RunUntilIdle();
  EXPECT_EQ(result.completions, auth_challenge ? 0u : 1u);
  EXPECT_EQ(result.auth_challenges, auth_challenge ? 1u : 0u);
  if (auth_challenge) EXPECT_EQ(result.response_code, 407);
  else EXPECT_EQ(result.completion, ERR_TUNNEL_CONNECTION_FAILED);
  return result;
}

template <class Types>
unsigned ClosePoolWhilePending(base::test::TaskEnvironment& environment) {
  Fixture<Types> fixture(Case::kHttp2Pending, environment);
  fixture.Prepare(1);
  Delegate<Types> delegate;
  auto job = std::make_unique<typename Types::Proxy>(
      DEFAULT_PRIORITY, SocketTag(), &*fixture.common, fixture.proxy_params,
      &delegate, nullptr);
  EXPECT_EQ(job->Connect(), ERR_IO_PENDING);
  environment.RunUntilIdle();
  EXPECT_FALSE(delegate.result);
  Types::CloseHttp2(*fixture.session, fixture.http2_context);
  environment.RunUntilIdle();
  EXPECT_EQ(delegate.result, ERR_ABORTED);
  EXPECT_EQ(delegate.completions, 1u);
  job.reset();
  environment.RunUntilIdle();
  EXPECT_EQ(delegate.completions, 1u);
  return delegate.completions;
}

class ConnectionJobsHttp2BaselineTest : public testing::Test {
 protected:
  ConnectionJobsHttp2BaselineTest() {
    features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
  }
  base::test::ScopedFeatureList features;
  base::test::TaskEnvironment environment{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(ConnectionJobsHttp2BaselineTest, TypedPoolUsesNativeProvidersOnly) {
  Fixture<Original> fixture(Case::kHttp2Pending, environment);
  EXPECT_EQ(fixture.common->spdy_session_pool.get(), fixture.http2_context->pool());
  EXPECT_EQ(fixture.common->ssl_client_context.get(), fixture.session->ssl_client_context());
  EXPECT_EQ(fixture.common->client_socket_factory.get(), fixture.deps.socket_factory.get());
  EXPECT_TRUE(fixture.common->spdy_session_pool->available_sessions_for_testing().empty());
  const auto ipv6 = ConnectionJobsUpperRustOriginalStorage::FrozenHost(
      ConnectionJobsUpperRustOriginalStorage::FromSchemeHostPort(
          url::SchemeHostPort(GURL("https://[2001:db8::1]:8443/"))));
  EXPECT_EQ(ipv6.host(), "2001:db8::1");
  EXPECT_EQ(ipv6.port(), 8443);
  const auto direct = ConnectionJobsUpperRustOriginalStorage::FrozenChain(
      ProxyChain::WithOpaqueData({}, 7));
  EXPECT_TRUE(direct.is_direct());
  EXPECT_EQ(direct.opaque_data(), 7);
}

TEST_F(ConnectionJobsHttp2BaselineTest, PendingCancellationWritesResetWithoutLateCompletion) {
  uint64_t baseline = 0;
  {
    Fixture<Original> fixture(Case::kHttp2Pending, environment);
    fixture.Prepare(3);
    baseline = fixture.Run(3, true);
    for (const auto& script : fixture.scripts)
      EXPECT_TRUE(script->data->AllWriteDataConsumed());
    EXPECT_TRUE(fixture.http2_context->pool()->available_sessions_for_testing().empty());
  }
  {
    Fixture<Production> fixture(Case::kHttp2Pending, environment);
    fixture.Prepare(3);
    EXPECT_EQ(baseline, fixture.Run(3, true));
    for (const auto& script : fixture.scripts)
      EXPECT_TRUE(script->data->AllWriteDataConsumed());
  }
}

TEST_F(ConnectionJobsHttp2BaselineTest, SessionCloseCompletesPendingJobExactlyOnce) {
  EXPECT_EQ(ClosePoolWhilePending<Original>(environment),
            ClosePoolWhilePending<Production>(environment));
}

TEST_F(ConnectionJobsHttp2BaselineTest, FailedTunnelCallbackCanDeleteJob) {
  EXPECT_EQ(ReplyAndDelete<Original>(environment, false),
            ReplyAndDelete<Production>(environment, false));
}

TEST_F(ConnectionJobsHttp2BaselineTest, AuthCallbackDeletesJobAndStaleRestartIsCanceled) {
  EXPECT_EQ(ReplyAndDelete<Original>(environment, true),
            ReplyAndDelete<Production>(environment, true));
}

TEST_F(ConnectionJobsHttp2BaselineTest, HttpsHttp11TunnelStillUsesOriginalEmptyPoolLookup) {
  uint64_t baseline;
  {
    Fixture<Original> fixture(Case::kHttpsTunnel, environment);
    fixture.Prepare(2);
    baseline = fixture.Run(2, true);
    EXPECT_TRUE(fixture.http2_context->pool()->available_sessions_for_testing().empty());
  }
  {
    Fixture<Production> fixture(Case::kHttpsTunnel, environment);
    fixture.Prepare(2);
    EXPECT_EQ(baseline, fixture.Run(2, true));
  }
}

}  // namespace
}  // namespace net::connection_jobs_job_workloads
