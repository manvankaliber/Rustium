#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_POLICY_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_POLICY_H_
#include "net/http/http_auth_controller.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/connection_auth_original_cache.h"
namespace net {
class ConnectionAuthOriginalPolicy {
 public:
  using InvalidateHandlerAction = HttpAuthController::InvalidateHandlerAction;
  static constexpr auto INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS = HttpAuthController::INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS;
  static constexpr auto INVALIDATE_HANDLER_AND_DISABLE_SCHEME = HttpAuthController::INVALIDATE_HANDLER_AND_DISABLE_SCHEME;
  static constexpr auto INVALIDATE_HANDLER = HttpAuthController::INVALIDATE_HANDLER;
  using AuthEvent = HttpAuthController::AuthEvent;
  static constexpr auto AUTH_EVENT_START = HttpAuthController::AUTH_EVENT_START;
  static constexpr auto AUTH_EVENT_REJECT = HttpAuthController::AUTH_EVENT_REJECT;
  static constexpr auto AUTH_EVENT_MAX = HttpAuthController::AUTH_EVENT_MAX;
  static void BindToCallingNetLog(HttpAuthController* self,
    const NetLogWithSource& caller_net_log);
  static int MaybeGenerateAuthToken(HttpAuthController* self,
    const HttpRequestInfo* request,
    const SSLInfo& ssl_info,
    CompletionOnceCallback callback,
    const NetLogWithSource& caller_net_log);
  static bool SelectPreemptiveAuth(HttpAuthController* self,
    const NetLogWithSource& caller_net_log);
  static int HandleAuthChallenge(HttpAuthController* self,
    scoped_refptr<HttpResponseHeaders> headers,
    const SSLInfo& ssl_info,
    bool do_not_send_server_auth,
    bool establishing_tunnel,
    const NetLogWithSource& caller_net_log);
  static void ResetAuth(HttpAuthController* self,
    const AuthCredentials& credentials);
  static bool HaveAuthHandler(const HttpAuthController* self);
  static bool HaveAuth(const HttpAuthController* self);
  static bool NeedsHTTP11(const HttpAuthController* self);
  static void InvalidateCurrentHandler(HttpAuthController* self,
    InvalidateHandlerAction action);
  static void InvalidateRejectedAuthFromCache(HttpAuthController* self);
  static void PrepareIdentityForReuse(HttpAuthController* self);
  static bool SelectNextAuthIdentityToTry(HttpAuthController* self);
  static void PopulateAuthChallenge(HttpAuthController* self);
  static int HandleGenerateTokenResult(HttpAuthController* self,
    int result);
  static void OnGenerateAuthTokenDone(HttpAuthController* self,
    int result);
  static void HistogramAuthEvent(HttpAuthController* self,
    AuthEvent auth_event);
  static void TakeAuthInfo(HttpAuthController* self,
    std::optional<AuthChallengeInfo>* other);
  static bool IsAuthSchemeDisabled(const HttpAuthController* self,
    HttpAuth::Scheme scheme);
  static void DisableAuthScheme(HttpAuthController* self,
    HttpAuth::Scheme scheme);
  static void DisableEmbeddedIdentity(HttpAuthController* self);
  static void OnConnectionClosed(HttpAuthController* self);
  static void InvalidateIfServerCertChanged(HttpAuthController* self,
    const X509Certificate* cert);
};
class ConnectionAuthOriginalHandlerPolicy {
 public:
  static bool InitFromChallenge(HttpAuthHandler* self,
    HttpAuthChallengeTokenizer* challenge,
    HttpAuth::Target target,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    const NetLogWithSource& net_log);
  static int GenerateAuthToken(HttpAuthHandler* self,
    const AuthCredentials* credentials,
                                       const HttpRequestInfo* request,
                                       CompletionOnceCallback callback,
                                       std::string* auth_token);
  static void OnGenerateAuthTokenComplete(HttpAuthHandler* self,
    int rv);
  static void FinishGenerateAuthToken(HttpAuthHandler* self,
    int rv);
  static HttpAuth::AuthorizationResult HandleAnotherChallenge(HttpAuthHandler* self,
    HttpAuthChallengeTokenizer* challenge);
};
class ConnectionAuthOriginalRegistry final : public HttpAuthHandlerRegistryFactory {
 public:
  using HttpAuthHandlerRegistryFactory::HttpAuthHandlerRegistryFactory;
  int CreateAuthHandler(
    HttpAuthChallengeTokenizer* challenge,
    HttpAuth::Target target,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    CreateReason reason,
    int digest_nonce_count,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler) override;
  static std::unique_ptr<ConnectionAuthOriginalRegistry>
Create(
    const HttpAuthPreferences* prefs
#if BUILDFLAG(USE_EXTERNAL_GSSAPI)
    ,
    const std::string& gssapi_library_name = ""
#endif
#if BUILDFLAG(USE_KERBEROS)
    ,
    HttpAuthMechanismFactory negotiate_auth_system_factory = HttpAuthMechanismFactory()
#endif
);
 private:
  bool IsSchemeAllowed(const std::string&) const;
  HttpAuthHandlerFactory* GetSchemeFactory(const std::string&) const;
};
class ConnectionAuthOriginalFactoryPolicy {
 public:
  static int CreateAuthHandlerFromString(
    HttpAuthHandlerFactory* self,
    std::string_view challenge,
    HttpAuth::Target target,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler);
  static int CreatePreemptiveAuthHandlerFromString(
    HttpAuthHandlerFactory* self,
    const std::string& challenge,
    HttpAuth::Target target,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    int digest_nonce_count,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler);
  static void ChooseBestChallenge(
    HttpAuthHandlerFactory* http_auth_handler_factory,
    const HttpResponseHeaders& response_headers,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    HttpAuth::Target target,
    const url::SchemeHostPort& scheme_host_port,
    const std::set<HttpAuth::Scheme>& disabled_schemes,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler);
  static HttpAuth::AuthorizationResult HandleChallengeResponse(
    HttpAuthHandler* handler,
    const HttpResponseHeaders& response_headers,
    HttpAuth::Target target,
    const std::set<HttpAuth::Scheme>& disabled_schemes,
    std::string* challenge_used);
  static void InitTokenizer(HttpAuthChallengeTokenizer*, std::string_view);
  static std::string_view Base64Param(const HttpAuthChallengeTokenizer&);
};
}
#endif
