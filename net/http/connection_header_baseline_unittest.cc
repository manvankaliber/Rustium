#include "net/http/connection_auth_original_policy.h"
// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/http/connection_header_original_policy.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/auth.h"
#include "net/base/features.h"
#include "net/base/test_proxy_delegate.h"
#include "net/http/http_auth_controller.h"
#include "net/http/http_request_headers_rust_original.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
using Policy = ConnectionHeaderOriginalPolicy;

void Compare(const HttpRequestHeaders& a,
             const HttpRequestHeaders& b,
             const HttpRequestHeadersRustOriginal& frozen) {
  EXPECT_EQ(a.GetHeaderVector(), b.GetHeaderVector());
  EXPECT_EQ(Policy::ToString(a), b.ToString());
  EXPECT_EQ(Policy::ToString(a), frozen.ToString());
  ASSERT_EQ(a.GetHeaderVector().size(), frozen.GetHeaderVector().size());
  for (size_t i = 0; i < a.GetHeaderVector().size(); ++i) {
    EXPECT_EQ(a.GetHeaderVector()[i].key, frozen.GetHeaderVector()[i].key);
    EXPECT_EQ(a.GetHeaderVector()[i].value, frozen.GetHeaderVector()[i].value);
  }
}

TEST(ConnectionHeaderBaselineTest, CaseOrderAliasSelfMergeAndNativeOwnership) {
  HttpRequestHeaders a, b;
  HttpRequestHeadersRustOriginal frozen;
  for (const auto* key : {"First", "Content-Type", "Last"}) {
    Policy::SetHeader(a, key, "1234567890123456789012345678901234567890");
    b.SetHeader(key, "1234567890123456789012345678901234567890");
    frozen.SetHeader(key, "1234567890123456789012345678901234567890");
  }
  // Existing vector capacity and unmodified long strings must stay in place.
  const auto* storage = a.GetHeaderVector().data();
  const auto* first_value = a.GetHeaderVector()[0].value.data();
  const auto alias = Policy::GetHeaderView(a, "FIRST").value().substr(3, 20);
  const auto alias_b = b.GetHeaderView("FIRST").value().substr(3, 20);
  const auto alias_f = frozen.GetHeaderView("FIRST").value().substr(3, 20);
  Policy::SetHeader(a, "content-type", alias);
  b.SetHeader("content-type", alias_b);
  frozen.SetHeader("content-type", alias_f);
  EXPECT_EQ(storage, a.GetHeaderVector().data());
  EXPECT_EQ(first_value, a.GetHeaderVector()[0].value.data());
  EXPECT_EQ(a.GetHeaderVector()[1].key, "Content-Type");
  Policy::SetHeaderIfMissing(a, "CONTENT-TYPE", "ignored");
  b.SetHeaderIfMissing("CONTENT-TYPE", "ignored");
  frozen.SetHeaderIfMissing("CONTENT-TYPE", "ignored");
  Policy::MergeFrom(a, a);
  b.MergeFrom(b);
  frozen.MergeFrom(frozen);
  Compare(a, b, frozen);
  const HttpRequestHeaders truly_const(a);
  EXPECT_TRUE(Policy::HasHeader(truly_const, "FIRST"));
  EXPECT_EQ(Policy::GetHeader(truly_const, "content-TYPE"),
            b.GetHeader("content-TYPE"));
  EXPECT_FALSE(Policy::GetHeaderView(truly_const, "missing"));
  Policy::RemoveHeader(a, "first");
  b.RemoveHeader("first");
  frozen.RemoveHeader("first");
  Compare(a, b, frozen);

  // The actual native move assignment transfers storage, including capacity;
  // this is the same operation used by a delegate's expected<T, Error> result.
  storage = a.GetHeaderVector().data();
  HttpRequestHeaders moved;
  moved = std::move(a);
  EXPECT_EQ(storage, moved.GetHeaderVector().data());
  Compare(moved, b, frozen);
}

TEST(ConnectionHeaderBaselineTest, RvaluePreservesOriginalAllocationTransfer) {
  HttpRequestHeaders a, b;
  HttpRequestHeadersRustOriginal frozen;
  std::string large(8192, 'x');
  const auto* allocation = large.data();
  Policy::SetHeader(a, "Big", std::move(large));
  b.SetHeader("Big", std::string(8192, 'x'));
  frozen.SetHeader("Big", std::string(8192, 'x'));
  EXPECT_EQ(allocation, a.GetHeaderVector()[0].value.data());
  Compare(a, b, frozen);
}

TEST(ConnectionHeaderBaselineTest, MalformedLineKeepsOriginalMutationPrefix) {
  logging::ScopedLogAssertHandler ignore_dfatal(base::BindRepeating(
      [](const char*, int, std::string_view, std::string_view) {}));
  for (const auto* malformed : {"colonless", ": missing", "Bad Key: v"}) {
    HttpRequestHeaders a, b;
    HttpRequestHeadersRustOriginal frozen;
    const std::string block = std::string("Before: first\r\n") + malformed +
                              "\r\nAfter: second\r\n";
    Policy::AddHeadersFromString(a, block);
    b.AddHeadersFromString(block);
    frozen.AddHeadersFromString(block);
    Compare(a, b, frozen);
    EXPECT_EQ(Policy::GetHeader(a, "before"), "first");
    EXPECT_EQ(Policy::GetHeader(a, "after"), "second");
  }
}

TEST(ConnectionHeaderBaselineTest, HeaderChecksRejectInvalidInputs) {
  HttpRequestHeaders a;
  EXPECT_CHECK_DEATH(Policy::SetHeader(a, "Bad Key", "v"));
  EXPECT_CHECK_DEATH(Policy::SetHeader(a, "X", std::string_view("a\0b", 3)));
  EXPECT_CHECK_DEATH(Policy::SetHeaderIfMissing(a, "X", "a\rb"));
  EXPECT_CHECK_DEATH(Policy::SetHeaderIfMissing(a, "X", "a\nb"));
}

TEST(ConnectionHeaderBaselineTest, SerializationAndSensitiveLoggingMatch) {
  HttpRequestHeaders a, b;
  HttpRequestHeadersRustOriginal frozen;
  for (const auto* line : {"User-Agent: abc", "Proxy-Authorization: Basic c2VjcmV0",
                           "X-Empty:", "Cookie: secret", "X-Byte: \xff"}) {
    Policy::AddHeaderFromString(a, line);
    b.AddHeaderFromString(line);
    frozen.AddHeaderFromString(line);
  }
  Compare(a, b, frozen);
  for (auto mode : {NetLogCaptureMode::kDefault,
                    NetLogCaptureMode::kIncludeSensitive}) {
    EXPECT_EQ(Policy::NetLogParams(a, "CONNECT endpoint:443 HTTP/1.1\r\n", mode),
              b.NetLogParams("CONNECT endpoint:443 HTTP/1.1\r\n", mode));
  }
}

using namespace connection_jobs_job_workloads;

class ConnectionHeaderJobsBaselineTest : public testing::Test {
 protected:
  ConnectionHeaderJobsBaselineTest() {
    features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
  }
  base::test::ScopedFeatureList features;
  base::test::TaskEnvironment environment{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

template<class Types> struct AuthCalls : ConnectionAuthOriginalPolicy {};
template<> struct AuthCalls<Production> {
  template<class... A> static auto HandleAuthChallenge(HttpAuthController* c,A&&... a) { return c->HandleAuthChallenge(std::forward<A>(a)...); }
  template<class... A> static auto ResetAuth(HttpAuthController* c,A&&... a) { return c->ResetAuth(std::forward<A>(a)...); }
  template<class... A> static auto MaybeGenerateAuthToken(HttpAuthController* c,A&&... a) { return c->MaybeGenerateAuthToken(std::forward<A>(a)...); }
  static bool HaveAuth(HttpAuthController* c) { return c->HaveAuth(); }
};
template <class Types>
std::string MaterializeToken(base::test::TaskEnvironment& environment) {
  Fixture<Types> fixture(Case::kHttpTunnel, environment);
  auto auth = base::MakeRefCounted<HttpAuthController>(
      HttpAuth::AUTH_PROXY, GURL("http://proxy.test/"), NetworkAnonymizationKey(),
      fixture.session->http_auth_cache(), fixture.session->http_auth_handler_factory(),
      fixture.deps.host_resolver.get());
  auto challenge = HttpResponseHeaders::TryToCreate(
      "HTTP/1.1 407 Proxy Authentication Required\r\n"
      "Proxy-Authenticate: Basic realm=\"r\"\r\nContent-Length: 0\r\n\r\n");
  EXPECT_EQ(OK, AuthCalls<Types>::HandleAuthChallenge(auth.get(), challenge, SSLInfo(), false, true,
                                        NetLogWithSource()));
  AuthCalls<Types>::ResetAuth(auth.get(), AuthCredentials(u"u", u"p"));
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://www.endpoint.test:443/");
  EXPECT_EQ(OK, AuthCalls<Types>::MaybeGenerateAuthToken(auth.get(), &request, SSLInfo(),
                                            base::DoNothing(), NetLogWithSource()));
  EXPECT_TRUE(AuthCalls<Types>::HaveAuth(auth.get()));
  HttpRequestHeaders headers;
  if constexpr (std::is_same_v<Types, Original>) {
    Policy::AddAuthorizationHeader(*auth, &headers);
  } else {
    auth->AddAuthorizationHeader(&headers);
  }
  EXPECT_EQ(Policy::GetHeader(headers, "proxy-authorization"), "Basic dTpw");
  HttpRequestHeaders second;
  if constexpr (std::is_same_v<Types, Original>) {
    Policy::AddAuthorizationHeader(*auth, &second);
  } else {
    auth->AddAuthorizationHeader(&second);
  }
  EXPECT_TRUE(second.IsEmpty());  // token consumed exactly once
  return Policy::ToString(headers);
}

TEST_F(ConnectionHeaderJobsBaselineTest, AuthTokenIsCopiedAndConsumedExactlyOnce) {
  EXPECT_EQ(MaterializeToken<Original>(environment),
            MaterializeToken<Production>(environment));
}

// No new timed cases. These are ordinary probes for the native delegate type
// contract, weak callback cancellation, partial I/O and failure propagation.
template <class Types>
class DelayedHeaders final : public TestProxyDelegate {
 public:
  base::expected<HttpRequestHeaders, Error> OnBeforeTunnelRequest(
      const ProxyChain&, size_t, OnBeforeTunnelRequestCallback callback) override {
    ++calls;
    pending = std::move(callback);
    return base::unexpected(ERR_IO_PENDING);
  }
  void Complete(bool error) {
    ASSERT_TRUE(pending);
    if (error) {
      std::move(pending).Run(base::unexpected(ERR_TUNNEL_CONNECTION_FAILED));
      return;
    }
    HttpRequestHeaders result;
    if constexpr (std::is_same_v<Types, Original>) {
      Policy::SetHeader(result, "X-Delegate", "borrowed-native-storage");
    } else {
      result.SetHeader("X-Delegate", "borrowed-native-storage");
    }
    std::move(pending).Run(std::move(result));
  }
  unsigned calls = 0;
  OnBeforeTunnelRequestCallback pending;
};

struct DelegateResult {
  int immediate, result;
  unsigned completions, calls;
  bool consumed;
  bool operator==(const DelegateResult&) const = default;
};

template <class Types>
DelegateResult DelayedTunnel(base::test::TaskEnvironment& environment,
                             bool fail, bool cancel, bool h2) {
  // These strings/frame objects outlive borrowed MockRead/Write bytes.
  const std::string request =
      "CONNECT www.endpoint.test:443 HTTP/1.1\r\n"
      "Host: www.endpoint.test:443\r\nProxy-Connection: keep-alive\r\n"
      "User-Agent: test-ua\r\nX-Delegate: borrowed-native-storage\r\n\r\n";
  SpdyTestUtil wire;
  // The delegate header is merged before the user agent is added.
  const std::string_view fields[] = {"x-delegate", "borrowed-native-storage",
                                    "user-agent", "test-ua"};
  auto connect = wire.ConstructSpdyConnect(fields, 1, DEFAULT_PRIORITY,
                                          HostPortPair("www.endpoint.test", 443));
  auto reset = wire.ConstructSpdyRstStream(1, spdy::ERROR_CODE_CANCEL);
  DelayedHeaders<Types> proxy;
  Fixture<Types> fixture(h2 ? Case::kHttp2Pending : Case::kHttpTunnel, environment);
  fixture.common->proxy_delegate = &proxy;
  auto script = std::make_unique<Script>();
  if (!fail && !cancel) {
    if (h2) {
      // The H2 session starts reading before its CONNECT write completes.
      script->reads.emplace_back(SYNCHRONOUS, ERR_IO_PENDING, 0);
      script->writes.push_back(CreateMockWrite(connect, 1));
      script->writes.push_back(CreateMockWrite(reset, 2));
    } else {
      // Split request write and response read across separate async completions.
      script->writes.emplace_back(ASYNC, 0, std::string_view(request).substr(0, 12));
      script->writes.emplace_back(ASYNC, 1, std::string_view(request).substr(12));
      script->reads.emplace_back(ASYNC, 2, "HTTP/1.1 200 Connection");
      script->reads.emplace_back(ASYNC, 3, " Established\r\n\r\n");
    }
  }
  script->data = std::make_unique<SequencedSocketData>(script->reads, script->writes);
  script->data->set_connect_data(MockConnect(SYNCHRONOUS, OK));
  fixture.deps.socket_factory->AddSocketDataProvider(script->data.get());
  if (h2) {
    script->ssl = std::make_unique<SSLSocketDataProvider>(SYNCHRONOUS, OK);
    script->ssl->next_proto = NextProto::kProtoHTTP2;
    fixture.deps.socket_factory->AddSSLSocketDataProvider(script->ssl.get());
  }
  fixture.scripts.push_back(std::move(script));
  Delegate<Types> delegate;
  auto job = std::make_unique<typename Types::Proxy>(DEFAULT_PRIORITY, SocketTag(),
      &*fixture.common, fixture.proxy_params, &delegate, nullptr);
  const int immediate = job->Connect();
  EXPECT_EQ(immediate, ERR_IO_PENDING);
  environment.RunUntilIdle();
  EXPECT_EQ(proxy.calls, 1u);
  EXPECT_FALSE(delegate.result);
  if (cancel) job.reset();
  proxy.Complete(fail);  // canceled weak callback is safe after owner destruction
  environment.RunUntilIdle();
  if (cancel || h2) EXPECT_FALSE(delegate.result);
  else EXPECT_EQ(delegate.result, fail ? ERR_TUNNEL_CONNECTION_FAILED : OK);
  job.reset();
  Types::CloseHttp2(*fixture.session, fixture.http2_context);
  environment.RunUntilIdle();
  const bool consumed = fixture.scripts[0]->data->AllWriteDataConsumed();
  EXPECT_TRUE(consumed);
  EXPECT_EQ(delegate.completions, (cancel || h2) ? 0u : 1u);
  return {immediate, delegate.result.value_or(ERR_IO_PENDING),
          delegate.completions, proxy.calls, consumed};
}

TEST_F(ConnectionHeaderJobsBaselineTest, DelegateHeadersAndPartialHttp11Io) {
  EXPECT_EQ(DelayedTunnel<Original>(environment, false, false, false),
            DelayedTunnel<Production>(environment, false, false, false));
}
TEST_F(ConnectionHeaderJobsBaselineTest, DelegateHeadersReachOriginalH2Conversion) {
  EXPECT_EQ(DelayedTunnel<Original>(environment, false, false, true),
            DelayedTunnel<Production>(environment, false, false, true));
}
TEST_F(ConnectionHeaderJobsBaselineTest, DelegateFailureCompletesExactlyOnce) {
  EXPECT_EQ(DelayedTunnel<Original>(environment, true, false, false),
            DelayedTunnel<Production>(environment, true, false, false));
}
TEST_F(ConnectionHeaderJobsBaselineTest, CanceledDelegateSuccessAndFailureCannotResumeOwner) {
  for (bool fail : {false, true}) {
    EXPECT_EQ(DelayedTunnel<Original>(environment, fail, true, false),
              DelayedTunnel<Production>(environment, fail, true, false));
  }
}
}  // namespace
}  // namespace net
