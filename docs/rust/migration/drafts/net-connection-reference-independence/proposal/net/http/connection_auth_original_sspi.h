// Named reference-only SSPI mechanism, adapted from hash-bound original.
#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_SSPI_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_SSPI_H_
#include "build/build_config.h"
#if BUILDFLAG(IS_WIN)
#include "net/http/http_auth_sspi_win.h"
namespace net {
class ConnectionAuthOriginalSSPI : public HttpAuthMechanism {
 public:
  ConnectionAuthOriginalSSPI(scoped_refptr<SSPILibrary> sspi_library,
               HttpAuth::Scheme scheme);
  ~ConnectionAuthOriginalSSPI() override;

  // HttpAuthMechanism implementation:
  bool Init(const NetLogWithSource& net_log) override;
  bool NeedsIdentity() const override;
  bool AllowsExplicitCredentials() const override;
  HttpAuth::AuthorizationResult ParseChallenge(
      HttpAuthChallengeTokenizer* tok) override;
  int GenerateAuthToken(const AuthCredentials* credentials,
                        const std::string& spn,
                        const std::string& channel_bindings,
                        std::string* auth_token,
                        const NetLogWithSource& net_log,
                        CompletionOnceCallback callback) override;
  void SetDelegation(HttpAuth::DelegationType delegation_type) override;

 private:
  class SspiContext;

  struct TokenResult {
    int net_error = OK;
    std::string auth_token;
    bool has_security_context = false;
  };

  void OnTokenGenerated(TokenResult result);

  // Created on first use; most handlers never generate a token.
  base::SequenceBound<SspiContext> context_;
  scoped_refptr<SSPILibrary> library_;
  HttpAuth::Scheme scheme_;
  std::string decoded_server_auth_token_;
  HttpAuth::DelegationType delegation_type_;

  // Mirrors the validity of the CtxtHandle owned by `context_`, which cannot
  // be inspected off the blocking sequence.
  bool has_security_context_ = false;

  // Valid only while a GenerateAuthToken() call is in flight.
  raw_ptr<std::string> auth_token_ = nullptr;
  CompletionOnceCallback callback_;

  base::WeakPtrFactory<ConnectionAuthOriginalSSPI> weak_factory_{this};
};


}  // namespace net
#endif
#endif
