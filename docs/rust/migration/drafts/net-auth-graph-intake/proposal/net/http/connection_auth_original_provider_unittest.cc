// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// PREPARED ONLY: no process-wide entrypoint instrumentation is provided here.
#include "net/http/connection_auth_original_policy.h"

#include <memory>
#include <set>
#include <string>

#include "base/functional/callback_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/connection_auth_original_providers.h"
#include "net/net_buildflags.h"
#if BUILDFLAG(USE_KERBEROS)
#include "net/http/connection_auth_original_negotiate_factory.h"
#include "net/http/mock_allow_http_auth_preferences.h"
#if BUILDFLAG(IS_POSIX)
#include "net/http/mock_gssapi_library_posix.h"
#endif
#endif
#if BUILDFLAG(IS_WIN)
#include "net/base/test_completion_callback.h"
#include "net/http/mock_sspi_library_win.h"
#endif
#include "net/ssl/ssl_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace net {
namespace {
using Factory = ConnectionAuthOriginalFactoryPolicy;
using Handler = ConnectionAuthOriginalHandlerPolicy;
class OriginalProviderRouteTest : public testing::Test {
 protected:
  int Create(std::string_view challenge, std::unique_ptr<HttpAuthHandler>* out) {
    return Factory::CreateAuthHandlerFromString(registry.get(), challenge,
        HttpAuth::AUTH_PROXY, SSLInfo(), NetworkAnonymizationKey(), origin,
        NetLogWithSource(), nullptr, out);
  }
  HttpAuth::AuthorizationResult Next(HttpAuthHandler* handler,
                                     std::string_view challenge) {
    auto headers = HttpResponseHeaders::TryToCreate(
        std::string("HTTP/1.1 407 Proxy Authentication Required\r\n") +
        "Proxy-Authenticate: " + std::string(challenge) + "\r\n\r\n");
    EXPECT_TRUE(headers);
    std::string used;
    return Factory::HandleChallengeResponse(handler, *headers,
        HttpAuth::AUTH_PROXY, std::set<HttpAuth::Scheme>(), &used);
  }
  base::test::TaskEnvironment environment;
  std::unique_ptr<ConnectionAuthOriginalRegistry> registry =
      ConnectionAuthOriginalRegistry::Create(nullptr);
  const url::SchemeHostPort origin{GURL("http://proxy.test/")};
};

TEST_F(OriginalProviderRouteTest, DigestUsesRegistryAndOriginalHandlerInit) {
  std::unique_ptr<HttpAuthHandler> handler;
  ASSERT_EQ(OK, Create("Digest realm=\"r\", nonce=\"n\"", &handler));
  ASSERT_TRUE(handler);
  EXPECT_EQ(HttpAuth::AUTH_SCHEME_DIGEST, handler->auth_scheme());
  EXPECT_EQ("r", handler->realm());
}

#if BUILDFLAG(USE_KERBEROS)
class OriginalProviderPreferences : public MockAllowHttpAuthPreferences {
 public:
#if BUILDFLAG(IS_LINUX)
  bool AllowGssapiLibraryLoad() const override { return true; }
#endif
};
TEST_F(OriginalProviderRouteTest, NegotiateFirstRoundUsesNamedProvider) {
  OriginalProviderPreferences preferences;
  auto factory = std::make_unique<ConnectionAuthOriginalNegotiateFactory>(
      HttpAuthMechanismFactory());
  factory->set_http_auth_preferences(&preferences);
#if BUILDFLAG(IS_WIN)
  factory->set_library(base::MakeRefCounted<MockSSPILibrary>(NEGOSSP_NAME));
#elif BUILDFLAG(IS_POSIX)
  factory->set_library(base::MakeRefCounted<test::MockGSSAPILibrary>());
#endif
  registry->RegisterSchemeFactory("negotiate", std::move(factory));
  std::unique_ptr<HttpAuthHandler> bad;
  EXPECT_EQ(ERR_INVALID_RESPONSE, Create("Negotiate Zm9v", &bad));
  EXPECT_FALSE(bad);
  std::unique_ptr<HttpAuthHandler> handler;
  ASSERT_EQ(OK, Create("Negotiate", &handler));
  ASSERT_TRUE(handler);
  EXPECT_EQ(HttpAuth::AUTH_SCHEME_NEGOTIATE, handler->auth_scheme());
  // This is still a first-round provider state. A later round requires a
  // successful mocked OS token exchange and is a separate pending obligation.
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT,
            Next(handler.get(), "Negotiate Zm9v"));
  handler.reset();
  registry->RegisterSchemeFactory("negotiate", nullptr);
}
#endif

#if !BUILDFLAG(IS_WIN)
TEST_F(OriginalProviderRouteTest, PortableNtlmFirstAndLaterStateThroughRegistry) {
  std::unique_ptr<HttpAuthHandler> bad;
  EXPECT_EQ(ERR_INVALID_RESPONSE, Create("NTLM Zm9v", &bad));
  EXPECT_FALSE(bad);
  std::unique_ptr<HttpAuthHandler> handler;
  ASSERT_EQ(OK, Create("NTLM", &handler));
  ASSERT_TRUE(handler);
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://www.endpoint.test:443/");
  AuthCredentials credentials(u"user", u"password");
  std::string output;
  ASSERT_EQ(OK, Handler::GenerateAuthToken(handler.get(), &credentials, &request,
                                           base::DoNothing(), &output));
  EXPECT_FALSE(output.empty());
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "NTLM"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "NTLM ***"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT, Next(handler.get(), "NTLM Zm9v"));
}
#endif
#if BUILDFLAG(IS_WIN)
TEST_F(OriginalProviderRouteTest, SspiNtlmFirstAndLaterStateThroughRegistry) {
  auto factory = std::make_unique<ConnectionAuthOriginalNtlmFactory>();
  factory->set_sspi_library(base::MakeRefCounted<MockSSPILibrary>(NTLMSP_NAME));
  registry->RegisterSchemeFactory("ntlm", std::move(factory));
  std::unique_ptr<HttpAuthHandler> bad;
  EXPECT_EQ(ERR_INVALID_RESPONSE, Create("NTLM Zm9v", &bad));
  EXPECT_FALSE(bad);
  std::unique_ptr<HttpAuthHandler> handler;
  ASSERT_EQ(OK, Create("NTLM", &handler));
  ASSERT_TRUE(handler);
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://www.endpoint.test:443/");
  AuthCredentials credentials(u"user", u"password");
  std::string output;
  TestCompletionCallback callback;
  int rv = Handler::GenerateAuthToken(handler.get(), &credentials, &request,
                                      callback.callback(), &output);
  ASSERT_EQ(ERR_IO_PENDING, rv);
  ASSERT_EQ(OK, callback.GetResult(rv));
  EXPECT_FALSE(output.empty());
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "NTLM"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "NTLM ***"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT, Next(handler.get(), "NTLM Zm9v"));
}
#endif
}  // namespace
}  // namespace net
