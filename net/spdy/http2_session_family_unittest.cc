// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/spdy/http2_session_family_factory.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::http2_family_test {
namespace {

template <class Pool>
typename FamilyTypes<Pool>::Key MakeKey() {
  if constexpr (std::is_same_v<Pool, SpdySessionPool>) {
    return SpdySessionKey(HostPortPair("example.test", 443), PRIVACY_MODE_DISABLED,
                          ProxyChain::Direct(), SessionUsage::kDestination,
                          SocketTag(), NetworkAnonymizationKey(),
                          SecureDnsPolicy::kAllow, false,
                          handles::kInvalidNetworkHandle);
  } else {
    return SpdySessionKeyOriginal(
        proxy_value_rust_reference::HostPortPair("example.test", 443),
        PRIVACY_MODE_DISABLED, proxy_value_rust_reference::ProxyChain::Direct(),
        SessionUsage::kDestination, SocketTag(), NetworkAnonymizationKey(),
        SecureDnsPolicy::kAllow, false, handles::kInvalidNetworkHandle);
  }
}

template <class Pool>
class RequestDelegate final : public Pool::SpdySessionRequest::Delegate {
 public:
  using Session = typename FamilyTypes<Pool>::Session;
  void OnSpdySessionAvailable(base::WeakPtr<Session> session) override {
    ++calls;
    last = session;
  }
  size_t calls = 0;
  base::WeakPtr<Session> last;
};

template <class Pool>
class Http2SessionFamilyTest : public TestWithTaskEnvironment {};
using Pools = ::testing::Types<SpdySessionPoolOriginal, SpdySessionPool>;
TYPED_TEST_SUITE(Http2SessionFamilyTest, Pools);

TYPED_TEST(Http2SessionFamilyTest, BlockingCancellationResumesAfterRemoval) {
  Http2SessionFamilyFactory<TypeParam> factory({});
  using Request = typename TypeParam::SpdySessionRequest;
  const auto key = MakeKey<TypeParam>();
  RequestDelegate<TypeParam> first_delegate, second_delegate, third_delegate;
  std::unique_ptr<Request> first, second, third;
  bool first_blocking = false, second_blocking = true, third_blocking = false;
  size_t deferred_calls = 0;
  EXPECT_FALSE(factory.pool()->RequestSession(
      key, true, false, NetLogWithSource(), {}, &first_delegate, &first,
      &first_blocking));
  EXPECT_TRUE(first_blocking);
  EXPECT_FALSE(factory.pool()->RequestSession(
      key, true, false, NetLogWithSource(),
      base::BindLambdaForTesting([&] {
        ++deferred_calls;
        // The previous blocking bit and deferred list must be cleared before
        // this callback creates another request for the same key.
        EXPECT_FALSE(factory.pool()->RequestSession(
            key, true, false, NetLogWithSource(), {}, &third_delegate, &third,
            &third_blocking));
      }),
      &second_delegate, &second, &second_blocking));
  EXPECT_FALSE(second_blocking);
  first.reset();
  EXPECT_EQ(deferred_calls, 0u);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(deferred_calls, 1u);
  EXPECT_TRUE(third_blocking);
  EXPECT_TRUE(third);
  EXPECT_EQ(first_delegate.calls + second_delegate.calls + third_delegate.calls,
            0u);
  third.reset();
  second.reset();
  base::RunLoop().RunUntilIdle();
}

TYPED_TEST(Http2SessionFamilyTest, NonblockingCancelKeepsDeferredNotification) {
  Http2SessionFamilyFactory<TypeParam> factory({});
  using Request = typename TypeParam::SpdySessionRequest;
  const auto key = MakeKey<TypeParam>();
  RequestDelegate<TypeParam> a, b;
  std::unique_ptr<Request> first, second;
  bool blocking = false;
  size_t calls = 0;
  factory.pool()->RequestSession(key, true, false, NetLogWithSource(), {}, &a,
                                 &first, &blocking);
  ASSERT_TRUE(blocking);
  factory.pool()->RequestSession(
      key, true, false, NetLogWithSource(),
      base::BindLambdaForTesting([&] { ++calls; }), &b, &second, &blocking);
  ASSERT_FALSE(blocking);
  second.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(calls, 0u);
  first.reset();
  base::RunLoop().RunUntilIdle();
  // The original API has no unregister operation for these callbacks. Callers
  // normally bind WeakPtrs, but an ordinary callback remains queued here.
  EXPECT_EQ(calls, 1u);
}

TYPED_TEST(Http2SessionFamilyTest, NoPendingResolutionDoesNotCreateAlias) {
  Http2SessionFamilyFactory<TypeParam> factory({});
  const auto key = MakeKey<TypeParam>();
  HostResolverEndpointResult endpoint;
  endpoint.metadata.supported_protocol_alpns = {"h3"};
  const std::vector<HostResolverEndpointResult> endpoints = {endpoint};
  EXPECT_EQ(factory.pool()->OnHostResolutionComplete(key, true, endpoints, {}),
            OnHostResolutionCallbackResult::kContinue);
  const std::vector<ServiceEndpoint> service_endpoints;
  EXPECT_EQ(factory.pool()->OnHostResolutionComplete(
                key, false, service_endpoints, {}),
            OnHostResolutionCallbackResult::kContinue);
  EXPECT_TRUE(factory.pool()->available_sessions_for_testing().empty());
  EXPECT_FALSE(factory.pool()->HasAvailableSession(key, true, false));
  EXPECT_FALSE(factory.pool()->HasAvailableSession(key, false, true));
}

TYPED_TEST(Http2SessionFamilyTest, CloseAndConfigurationOnEmptyPool) {
  Http2SessionFamilyFactory<TypeParam> factory({});
  factory.pool()->CloseCurrentSessions(ERR_ABORTED);
  factory.pool()->CloseCurrentIdleSessions("empty pool");
  factory.pool()->MakeCurrentSessionsGoingAway(ERR_NETWORK_CHANGED);
  factory.pool()->CloseAllSessions();
  factory.pool()->OnSSLConfigChanged(
      SSLClientContext::SSLConfigChangeType::kSSLConfigChanged);
  factory.pool()->OnSSLConfigChanged(
      SSLClientContext::SSLConfigChangeType::kCertDatabaseChanged);
  factory.pool()->OnSSLConfigChanged(
      SSLClientContext::SSLConfigChangeType::kCertVerifierChanged);
  EXPECT_TRUE(factory.pool()->available_sessions_for_testing().empty());
}

}  // namespace
}  // namespace net::http2_family_test
