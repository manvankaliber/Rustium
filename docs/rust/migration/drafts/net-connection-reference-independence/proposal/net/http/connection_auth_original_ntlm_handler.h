// Named reference-only NTLM handler; retains platform-specific original state.
#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_NTLM_HANDLER_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_NTLM_HANDLER_H_
#include "net/http/http_auth_handler_ntlm.h"
#if defined(NTLM_SSPI)
#include "net/http/connection_auth_original_sspi.h"
#else
#include "net/http/connection_auth_original_ntlm_mechanism.h"
#endif
namespace net {
class ConnectionAuthOriginalNtlmHandler : public HttpAuthHandler {
 public:
#if defined(NTLM_PORTABLE)
  explicit ConnectionAuthOriginalNtlmHandler(
      const HttpAuthPreferences* http_auth_preferences);
#endif
#if defined(NTLM_SSPI)
  ConnectionAuthOriginalNtlmHandler(scoped_refptr<SSPILibrary> sspi_library,
                      const HttpAuthPreferences* http_auth_preferences);
#endif

  ConnectionAuthOriginalNtlmHandler(const ConnectionAuthOriginalNtlmHandler&) = delete;
  ConnectionAuthOriginalNtlmHandler& operator=(const ConnectionAuthOriginalNtlmHandler&) = delete;

  ~ConnectionAuthOriginalNtlmHandler() override;

  // HttpAuthHandler
  bool NeedsIdentity() override;
  bool AllowsDefaultCredentials() override;

 protected:
  // HttpAuthHandler
  bool Init(HttpAuthChallengeTokenizer* tok,
            const SSLInfo& ssl_info,
            const NetworkAnonymizationKey& network_anonymization_key) override;
  int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                            const HttpRequestInfo* request,
                            CompletionOnceCallback callback,
                            std::string* auth_token) override;
  HttpAuth::AuthorizationResult HandleAnotherChallengeImpl(
      HttpAuthChallengeTokenizer* challenge) override;

 private:
  // Parse the challenge, saving the results into this instance.
  HttpAuth::AuthorizationResult ParseChallenge(HttpAuthChallengeTokenizer* tok);

  // Create an NTLM SPN to identify the |scheme_host_port| server.
  static std::string CreateSPN(const url::SchemeHostPort& scheme_host_port);

#if defined(NTLM_SSPI)
  ConnectionAuthOriginalSSPI mechanism_;
  raw_ptr<const HttpAuthPreferences> http_auth_preferences_;
#elif defined(NTLM_PORTABLE)
  ConnectionAuthOriginalNtlmMechanism mechanism_;
#endif

  std::string channel_bindings_;
};
}  // namespace net
#endif
