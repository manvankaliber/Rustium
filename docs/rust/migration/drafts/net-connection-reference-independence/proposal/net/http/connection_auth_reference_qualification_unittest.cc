// Prepared only. Requires the isolated instrumented GN configuration.
#include "net/http/connection_auth_original_negotiate_factory.h"
#include "net/http/connection_auth_original_digest_factory.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/http/connection_auth_original_providers.h"
#include "net/http/connection_auth_reference_test_mechanism.h"
#include "net/http/rustium_reference_probe_test_support.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/base64.h"
#include "base/features.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_split.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#if BUILDFLAG(IS_WIN)
#include "base/task/thread_pool/thread_pool_instance.h"
#endif
#include "net/base/net_errors.h"
#include "net/base/auth.h"
#include "net/base/test_completion_callback.h"
#include "net/http/http_auth_cache.h"
#include "net/http/auth_challenge_reference.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/mock_allow_http_auth_preferences.h"
#include "net/log/net_log_entry.h"
#include "net/log/connection_original_log.h"
#include "net/log/net_log_values.h"
#include "net/log/test_net_log.h"
#include "net/net_buildflags.h"
#include "net/ssl/ssl_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"
#if BUILDFLAG(IS_WIN)
#include "net/http/mock_sspi_library_win.h"
#elif BUILDFLAG(IS_POSIX)
#include "net/http/mock_gssapi_library_posix.h"
#endif

#if BUILDFLAG(USE_KERBEROS)
namespace net {
namespace {
using Factory = ConnectionAuthOriginalFactoryPolicy;
using Handler = ConnectionAuthOriginalHandlerPolicy;
namespace probe = rustium_reference_probe;

class ProbePreferences : public MockAllowHttpAuthPreferences {
 public:
#if BUILDFLAG(IS_LINUX)
  bool AllowGssapiLibraryLoad() const override { return true; }
#endif
};

class AuthReferenceQualificationTest : public testing::Test {
 protected:
  void SetUp() override {
    features.InitAndEnableFeature(base::features::kBase64SimdBase64Encode);
    prefs.set_negotiate_disable_cname_lookup(true);
    registry = ConnectionAuthOriginalRegistry::Create(&prefs);
  }
  void Register(HttpAuthMechanismFactory injection = HttpAuthMechanismFactory()) {
    auto f = std::make_unique<ConnectionAuthOriginalNegotiateFactory>(injection);
    f->set_http_auth_preferences(&prefs);
#if BUILDFLAG(IS_WIN)
    library = base::MakeRefCounted<MockSSPILibrary>(NEGOSSP_NAME);
#elif BUILDFLAG(IS_POSIX)
    library = base::MakeRefCounted<test::MockGSSAPILibrary>();
#endif
    f->set_library(library);
    registry->RegisterSchemeFactory("negotiate", std::move(f));
  }
  int Create(std::string_view value, const NetLogWithSource& log,
             std::unique_ptr<HttpAuthHandler>* out) {
    return Factory::CreateAuthHandlerFromString(registry.get(), value,
        HttpAuth::AUTH_PROXY, SSLInfo(), NetworkAnonymizationKey(), origin,
        log, nullptr, out);
  }
  HttpAuth::AuthorizationResult Next(HttpAuthHandler* h, std::string_view value) {
    auto headers = HttpResponseHeaders::TryToCreate(
        std::string("HTTP/1.1 407 Proxy Authentication Required\r\n") +
        "Proxy-Authenticate: " + std::string(value) + "\r\n\r\n");
    CHECK(headers);
    std::string used;
    return Factory::HandleChallengeResponse(h, *headers, HttpAuth::AUTH_PROXY,
                                             {}, &used);
  }
  void NoObservedRust(const probe::Snapshot& before) {
    environment.RunUntilIdle();
    EXPECT_EQ((probe::Snapshot{0, 0, 0, 0, 0, 0, 0}), probe::Delta(before, probe::Read()));
  }
  base::test::TaskEnvironment environment;
  base::test::ScopedFeatureList features;
  ProbePreferences prefs;
  std::unique_ptr<ConnectionAuthOriginalRegistry> registry;
#if BUILDFLAG(IS_WIN)
  scoped_refptr<MockSSPILibrary> library;
#elif BUILDFLAG(IS_POSIX)
  scoped_refptr<test::MockGSSAPILibrary> library;
#endif
  const url::SchemeHostPort origin{GURL("http://proxy.test/")};
};

// Every probe must respond to a known live entry; six zeros without these
// controls would be inconclusive. Setup occurs before the measured snapshot.
TEST_F(AuthReferenceQualificationTest, EachInstrumentedFamilyHasPositiveControl) {
  HttpAuthCache cache(true);
  HttpRequestHeaders headers;
  auto before = probe::Read();
  HttpAuthChallengeTokenizer tokenizer("Basic realm=x");
  EXPECT_EQ("basic", tokenizer.auth_scheme());
  EXPECT_FALSE(cache.Lookup(origin, HttpAuth::AUTH_PROXY, "r",
                            HttpAuth::AUTH_SCHEME_BASIC, NetworkAnonymizationKey()));
  headers.SetHeader("X-Test", "value");
  EXPECT_TRUE(IsCertificateError(ERR_CERT_DATE_INVALID));
  EXPECT_TRUE(NetLogStringValue("test").is_string());
  EXPECT_TRUE(base::SplitStringOnce("a:b", ':').has_value());
  EXPECT_EQ("Zm9v", base::Base64Encode("foo"));
  auto delta = probe::Delta(before, probe::Read());
  for (uint64_t count : delta) EXPECT_GT(count, 0u);
}

TEST_F(AuthReferenceQualificationTest, FrozenBase64EncodingDoesNotEnterRust) {
  auto before = probe::Read();
  EXPECT_EQ("", auth_challenge_reference::FrozenEncode(""));
  EXPECT_EQ("Zg==", auth_challenge_reference::FrozenEncode("f"));
  EXPECT_EQ("Zm8=", auth_challenge_reference::FrozenEncode("fo"));
  EXPECT_EQ("Zm9v", auth_challenge_reference::FrozenEncode("foo"));
  EXPECT_EQ("AP8=", auth_challenge_reference::FrozenEncode(std::string("\0\xff", 2)));
  NoObservedRust(before);
}

TEST_F(AuthReferenceQualificationTest, DisabledCaptureDoesNotEvaluatePayload) {
  auto before = probe::Read();
  // If the callback is materialized this null input is invalid, as in the
  // established connection baseline test. No logging mode is suppressed.
  ConnectionOriginalNetLogResponseHeaders(NetLogWithSource(),
      NetLogEventType::HTTP_TRANSACTION_READ_TUNNEL_RESPONSE_HEADERS, nullptr);
  NoObservedRust(before);
}

TEST_F(AuthReferenceQualificationTest, BasicWireTokenAvoidsRustBase64) {
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://endpoint.test/");
  AuthCredentials credentials(u"u", u"p");
  std::unique_ptr<HttpAuthHandler> handler;
  auto before = probe::Read();
  ASSERT_EQ(OK, Create("Basic realm=\"r\"", NetLogWithSource(), &handler));
  std::string token;
  ASSERT_EQ(OK, Handler::GenerateAuthToken(handler.get(), &credentials, &request,
                                           base::DoNothing(), &token));
  EXPECT_EQ("Basic dTpw", token);
  handler.reset();
  NoObservedRust(before);
}

TEST_F(AuthReferenceQualificationTest, DigestNativeCaseUsesOriginalHandlerPath) {
  auto factory = std::make_unique<ConnectionAuthOriginalDigestFactory>();
  factory->set_nonce_generator(
      std::make_unique<HttpAuthHandlerDigest::FixedNonceGenerator>("082c875dcb2ca740"));
  registry->RegisterSchemeFactory("digest", std::move(factory));
  HttpRequestInfo request;
  request.method = "GET";
  request.url = GURL("http://endpoint.test/test/drealm1");
  AuthCredentials credentials(u"foo", u"bar");
  std::unique_ptr<HttpAuthHandler> handler;
  auto before = probe::Read();
  ASSERT_EQ(OK, Create("Digest realm=\"DRealm1\", "
      "nonce=\"claGgoRXBAA=7583377687842fdb7b56ba0555d175baa0b800e3\", "
      "qop=\"auth\"", NetLogWithSource(), &handler));
  std::string token;
  ASSERT_EQ(OK, Handler::GenerateAuthToken(handler.get(), &credentials, &request,
                                           base::DoNothing(), &token));
  EXPECT_EQ("Digest username=\"foo\", realm=\"DRealm1\", "
      "nonce=\"claGgoRXBAA=7583377687842fdb7b56ba0555d175baa0b800e3\", "
      "uri=\"/test/drealm1\", "
      "response=\"bcfaa62f1186a31ff1b474a19a17cf57\", "
      "qop=auth, nc=00000001, cnonce=\"082c875dcb2ca740\"", token);
  handler.reset();
  NoObservedRust(before);
}

TEST_F(AuthReferenceQualificationTest, NtlmWireTokenAndLaterRoundAvoidRust) {
#if BUILDFLAG(IS_WIN)
  auto factory = std::make_unique<ConnectionAuthOriginalNtlmFactory>();
  factory->set_sspi_library(base::MakeRefCounted<MockSSPILibrary>(NTLMSP_NAME));
  registry->RegisterSchemeFactory("ntlm", std::move(factory));
#endif
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://endpoint.test/");
  AuthCredentials credentials(u"user", u"password");
  std::unique_ptr<HttpAuthHandler> handler;
  auto before = probe::Read();
  ASSERT_EQ(OK, Create("NTLM", NetLogWithSource(), &handler));
  std::string token;
  TestCompletionCallback callback;
  int rv = Handler::GenerateAuthToken(handler.get(), &credentials, &request,
                                       callback.callback(), &token);
  ASSERT_EQ(OK, callback.GetResult(rv));
  EXPECT_EQ(0u, token.find("NTLM "));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "NTLM ***"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT, Next(handler.get(), "NTLM Zm9v"));
  handler.reset();
#if BUILDFLAG(IS_WIN)
  base::ThreadPoolInstance::Get()->FlushForTesting();
#endif
  NoObservedRust(before);
}

TEST_F(AuthReferenceQualificationTest, DefaultProviderLaterRoundAndMalformed) {
  Register();
#if BUILDFLAG(IS_POSIX)
  test::GssContextMockImpl context("source", "target", 23,
      *CHROME_GSS_SPNEGO_MECH_OID_DESC, 0, 1, 0);
  const test::MockGSSAPILibrary::SecurityContextQuery first(
      "Negotiate", GSS_S_CONTINUE_NEEDED, 0, context, nullptr, "first-token");
  const test::MockGSSAPILibrary::SecurityContextQuery second(
      "Negotiate", GSS_S_COMPLETE, 0, context, "foo", "second-token");
  for (const auto* q : {&first, &second})
    library->ExpectSecurityContext(q->expected_package, q->response_code,
        q->minor_response_code, q->context_info, q->expected_input_token, q->output_token);
#endif
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://endpoint.test/");
  auto before = probe::Read();
  std::unique_ptr<HttpAuthHandler> handler;
  ASSERT_EQ(OK, Create("Negotiate", NetLogWithSource(), &handler));
  TestCompletionCallback callback;
  std::string token;
  int rv = Handler::GenerateAuthToken(handler.get(), nullptr, &request,
                                       callback.callback(), &token);
  ASSERT_EQ(OK, callback.GetResult(rv));
  EXPECT_FALSE(token.empty());
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "Negotiate ***"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "Negotiate"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT, Next(handler.get(), "Negotiate Zm9v"));
  TestCompletionCallback callback2;
  rv = Handler::GenerateAuthToken(handler.get(), nullptr, &request,
                                   callback2.callback(), &token);
  ASSERT_EQ(OK, callback2.GetResult(rv));
  handler.reset();
  NoObservedRust(before);
}

#if BUILDFLAG(IS_WIN)
TEST_F(AuthReferenceQualificationTest, DefaultSspiCancellationDropsReply) {
  Register();
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://endpoint.test/");
  std::unique_ptr<HttpAuthHandler> handler;
  auto before = probe::Read();
  ASSERT_EQ(OK, Create("Negotiate", NetLogWithSource(), &handler));
  bool called = false;
  std::string token = "unchanged";
  int rv = Handler::GenerateAuthToken(handler.get(), nullptr, &request,
      base::BindOnce([](bool* called, int) { *called = true; },
                     base::Unretained(&called)), &token);
  ASSERT_EQ(ERR_IO_PENDING, rv);
  handler.reset();
  base::ThreadPoolInstance::Get()->FlushForTesting();
  environment.RunUntilIdle();
  EXPECT_FALSE(called);
  EXPECT_EQ("unchanged", token);
  NoObservedRust(before);
}

TEST_F(AuthReferenceQualificationTest, DefaultSspiCallbackMayDeleteHandler) {
  Register();
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://endpoint.test/");
  std::unique_ptr<HttpAuthHandler> handler;
  auto before = probe::Read();
  ASSERT_EQ(OK, Create("Negotiate", NetLogWithSource(), &handler));
  unsigned callbacks = 0;
  std::string token;
  int rv = Handler::GenerateAuthToken(handler.get(), nullptr, &request,
      base::BindOnce([](unsigned* n, std::unique_ptr<HttpAuthHandler>* owner, int result) {
        EXPECT_EQ(OK, result);
        ++*n;
        owner->reset();
      }, base::Unretained(&callbacks), base::Unretained(&handler)), &token);
  ASSERT_EQ(ERR_IO_PENDING, rv);
  base::ThreadPoolInstance::Get()->FlushForTesting();
  environment.RunUntilIdle();
  EXPECT_FALSE(handler);
  EXPECT_EQ(1u, callbacks);
  EXPECT_FALSE(token.empty());
  NoObservedRust(before);
}
#endif

TEST_F(AuthReferenceQualificationTest, InjectedReferenceCancellationAndReentrantDelete) {
  for (bool cancel : {false, true}) {
    ConnectionAuthReferenceTestState state;
    Register(base::BindRepeating(&MakeConnectionAuthReferenceTestMechanism,
                                 base::Unretained(&state)));
    std::unique_ptr<HttpAuthHandler> handler;
    HttpRequestInfo request;
    request.method = "CONNECT";
    request.url = GURL("https://endpoint.test/");
    auto before = probe::Read();
    ASSERT_EQ(OK, Create("Negotiate", NetLogWithSource(), &handler));
    unsigned callbacks = 0;
    std::string token;
    int rv = Handler::GenerateAuthToken(handler.get(), nullptr, &request,
        base::BindOnce([](unsigned* count, std::unique_ptr<HttpAuthHandler>* owner,
                          int result) {
          EXPECT_EQ(OK, result);
          ++*count;
          owner->reset();
        }, base::Unretained(&callbacks), base::Unretained(&handler)), &token);
    ASSERT_EQ(ERR_IO_PENDING, rv);
    if (cancel) handler.reset();
    environment.RunUntilIdle();
    EXPECT_FALSE(handler);
    EXPECT_EQ(1u, state.created);
    EXPECT_EQ(1u, state.destroyed);
    EXPECT_EQ(1u, state.posted);
    EXPECT_EQ(cancel ? 0u : 1u, state.completed);
    EXPECT_EQ(cancel ? 0u : 1u, callbacks);
    NoObservedRust(before);
    registry->RegisterSchemeFactory("negotiate", nullptr);
  }
}

TEST_F(AuthReferenceQualificationTest, InjectedReferenceLaterRoundAndMalformed) {
  ConnectionAuthReferenceTestState state;
  Register(base::BindRepeating(&MakeConnectionAuthReferenceTestMechanism,
                               base::Unretained(&state)));
  HttpRequestInfo request;
  request.method = "CONNECT";
  request.url = GURL("https://endpoint.test/");
  std::unique_ptr<HttpAuthHandler> handler;
  auto before = probe::Read();
  ASSERT_EQ(OK, Create("Negotiate", NetLogWithSource(), &handler));
  std::string token;
  TestCompletionCallback first;
  int rv = Handler::GenerateAuthToken(handler.get(), nullptr, &request,
                                       first.callback(), &token);
  ASSERT_EQ(ERR_IO_PENDING, rv);
  ASSERT_EQ(OK, first.GetResult(rv));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "Negotiate ***"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT, Next(handler.get(), "Negotiate"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT, Next(handler.get(), "Negotiate Zm9v"));
  TestCompletionCallback second;
  rv = Handler::GenerateAuthToken(handler.get(), nullptr, &request,
                                   second.callback(), &token);
  ASSERT_EQ(ERR_IO_PENDING, rv);
  ASSERT_EQ(OK, second.GetResult(rv));
  EXPECT_EQ(2u, state.completed);
  handler.reset();
  NoObservedRust(before);
  EXPECT_EQ(1u, state.destroyed);
  registry->RegisterSchemeFactory("negotiate", nullptr);
}

TEST_F(AuthReferenceQualificationTest, CapturePayloadOrderMatchesProduction) {
  for (auto mode : {NetLogCaptureMode::kDefault,
                    NetLogCaptureMode::kIncludeSensitive,
                    NetLogCaptureMode::kEverything}) {
    auto capture = [&](bool original) -> std::vector<NetLogEntry> {
      ConnectionAuthReferenceTestState state;
      auto make_mechanism = base::BindRepeating(
          &MakeConnectionAuthReferenceTestMechanism, base::Unretained(&state));
      std::unique_ptr<HttpAuthHandlerRegistryFactory> production;
      if (original) {
        Register(make_mechanism);
      } else {
        production = HttpAuthHandlerRegistryFactory::Create(&prefs);
        auto f = std::make_unique<HttpAuthHandlerNegotiate::Factory>(make_mechanism);
        f->set_http_auth_preferences(&prefs);
#if BUILDFLAG(IS_WIN)
        f->set_library(base::MakeRefCounted<MockSSPILibrary>(NEGOSSP_NAME));
#elif BUILDFLAG(IS_POSIX)
        f->set_library(base::MakeRefCounted<test::MockGSSAPILibrary>());
#endif
        production->RegisterSchemeFactory("negotiate", std::move(f));
      }
      RecordingNetLogObserver observer(mode);
      auto log = NetLogWithSource::Make(NetLog::Get(), NetLogSourceType::HTTP_AUTH_CONTROLLER);
      HttpRequestInfo request;
      request.method = "CONNECT";
      request.url = GURL("https://endpoint.test/");
      std::unique_ptr<HttpAuthHandler> handler;
      auto before = probe::Read();  // Exclude factory/observer setup.
      int rv = original ? Create("Negotiate", log, &handler)
          : production->CreateAuthHandlerFromString("Negotiate", HttpAuth::AUTH_PROXY,
              SSLInfo(), NetworkAnonymizationKey(), origin, log, nullptr, &handler);
      EXPECT_EQ(OK, rv);
      if (!handler) return {};
      std::string token;
      TestCompletionCallback callback;
      rv = original ? Handler::GenerateAuthToken(handler.get(), nullptr, &request,
                                                 callback.callback(), &token)
                    : handler->GenerateAuthToken(nullptr, &request, callback.callback(), &token);
      EXPECT_EQ(ERR_IO_PENDING, rv);
      EXPECT_EQ(OK, callback.GetResult(rv));
      EXPECT_EQ("Negotiate cmVmZXJlbmNl", token);
      handler.reset();
      environment.RunUntilIdle();  // Include async work and materialized log callbacks.
      EXPECT_EQ(1u, state.created);
      EXPECT_EQ(1u, state.posted);
      EXPECT_EQ(1u, state.completed);
      EXPECT_EQ(1u, state.destroyed);
      auto delta = probe::Delta(before, probe::Read());
      if (original) {
        EXPECT_EQ((probe::Snapshot{0, 0, 0, 0, 0, 0, 0}), delta);
        registry->RegisterSchemeFactory("negotiate", nullptr);
      } else {
        EXPECT_GT(delta[0], 0u);  // Native tokenizer is the live auth control.
        EXPECT_GT(delta[4], 0u);  // Native create-result log is the live value control.
      }
      return observer.GetEntriesForSource(log.source());
    };
    auto actual = capture(true);
    auto expected = capture(false);
    ASSERT_FALSE(actual.empty());
    ASSERT_EQ(expected.size(), actual.size());
    bool saw_creation = false;
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_EQ(expected[i].type, actual[i].type);
      EXPECT_EQ(expected[i].phase, actual[i].phase);
      EXPECT_EQ(expected[i].params, actual[i].params);
      if (actual[i].type == NetLogEventType::AUTH_HANDLER_CREATE_RESULT) {
        saw_creation = true;
        EXPECT_EQ(mode != NetLogCaptureMode::kDefault,
                  actual[i].params.FindString("challenge") != nullptr);
      }
    }
    EXPECT_TRUE(saw_creation);
  }
}

TEST_F(AuthReferenceQualificationTest, DefaultProviderCapturedContextMatchesProduction) {
  for (auto mode : {NetLogCaptureMode::kDefault, NetLogCaptureMode::kIncludeSensitive,
                    NetLogCaptureMode::kEverything}) {
    auto capture = [&](bool original) -> std::vector<NetLogEntry> {
      std::unique_ptr<HttpAuthHandlerRegistryFactory> production;
#if BUILDFLAG(IS_WIN)
      scoped_refptr<MockSSPILibrary> mock;
#else
      scoped_refptr<test::MockGSSAPILibrary> mock;
#endif
      if (original) {
        Register();
        mock = library;
      } else {
        production = HttpAuthHandlerRegistryFactory::Create(&prefs);
        auto f = std::make_unique<HttpAuthHandlerNegotiate::Factory>(HttpAuthMechanismFactory());
        f->set_http_auth_preferences(&prefs);
#if BUILDFLAG(IS_WIN)
        mock = base::MakeRefCounted<MockSSPILibrary>(NEGOSSP_NAME);
#else
        mock = base::MakeRefCounted<test::MockGSSAPILibrary>();
#endif
        f->set_library(mock);
        production->RegisterSchemeFactory("negotiate", std::move(f));
      }
#if BUILDFLAG(IS_POSIX)
      test::GssContextMockImpl context("source", "target", 23,
          *CHROME_GSS_SPNEGO_MECH_OID_DESC, 0, 1, 0);
      const test::MockGSSAPILibrary::SecurityContextQuery q(
          "Negotiate", GSS_S_CONTINUE_NEEDED, 0, context, nullptr, "first-token");
      mock->ExpectSecurityContext(q.expected_package, q.response_code,
          q.minor_response_code, q.context_info, q.expected_input_token, q.output_token);
#endif
      RecordingNetLogObserver observer(mode);
      auto log = NetLogWithSource::Make(NetLog::Get(), NetLogSourceType::HTTP_AUTH_CONTROLLER);
      HttpRequestInfo request;
      request.method = "CONNECT";
      request.url = GURL("https://endpoint.test/");
      std::unique_ptr<HttpAuthHandler> handler;
      auto before = probe::Read();
      int rv = original ? Create("Negotiate", log, &handler)
          : production->CreateAuthHandlerFromString("Negotiate", HttpAuth::AUTH_PROXY,
              SSLInfo(), NetworkAnonymizationKey(), origin, log, nullptr, &handler);
      EXPECT_EQ(OK, rv);
      if (!handler) return {};
      TestCompletionCallback callback;
      std::string token;
      rv = original ? Handler::GenerateAuthToken(handler.get(), nullptr, &request,
                                                 callback.callback(), &token)
                    : handler->GenerateAuthToken(nullptr, &request, callback.callback(), &token);
      EXPECT_EQ(OK, callback.GetResult(rv));
      EXPECT_FALSE(token.empty());
      handler.reset();
#if BUILDFLAG(IS_WIN)
      base::ThreadPoolInstance::Get()->FlushForTesting();
#endif
      environment.RunUntilIdle();
      auto delta = probe::Delta(before, probe::Read());
      if (original) {
        EXPECT_EQ((probe::Snapshot{0, 0, 0, 0, 0, 0, 0}), delta);
        registry->RegisterSchemeFactory("negotiate", nullptr);
      } else {
        EXPECT_GT(delta[0], 0u);
        EXPECT_GT(delta[4], 0u);
        EXPECT_GT(delta[6], 0u);
      }
      return observer.GetEntriesForSource(log.source());
    };
    auto actual = capture(true);
    auto expected = capture(false);
    ASSERT_FALSE(actual.empty());
    ASSERT_EQ(expected.size(), actual.size());
    bool saw_context = false;
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_EQ(expected[i].type, actual[i].type);
      EXPECT_EQ(expected[i].phase, actual[i].phase);
      EXPECT_EQ(expected[i].params, actual[i].params);
      if (actual[i].type == NetLogEventType::AUTH_LIBRARY_INIT_SEC_CTX)
        saw_context = true;
    }
    EXPECT_TRUE(saw_context);
  }
}
}  // namespace
}  // namespace net
#endif  // BUILDFLAG(USE_KERBEROS)
