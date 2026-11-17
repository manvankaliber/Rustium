// Named reference-only NTLM handler, copied from hash-bound native sources.
#include "net/http/connection_auth_original_ntlm_handler.h"
#include <utility>
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/cert/x509_util.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_auth_scheme.h"
#include "net/ssl/ssl_info.h"
#include "url/scheme_host_port.h"
namespace net {
bool ConnectionAuthOriginalNtlmHandler::Init(
    HttpAuthChallengeTokenizer* tok,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key) {
  auth_scheme_ = HttpAuth::AUTH_SCHEME_NTLM;
  score_ = 3;
  properties_ = ENCRYPTS_IDENTITY | IS_CONNECTION_BASED;

  if (ssl_info.is_valid())
    x509_util::GetTLSServerEndPointChannelBinding(*ssl_info.cert,
                                                  &channel_bindings_);

  return ParseChallenge(tok) == HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalNtlmHandler::HandleAnotherChallengeImpl(
    HttpAuthChallengeTokenizer* challenge) {
  return ParseChallenge(challenge);
}

// static
std::string ConnectionAuthOriginalNtlmHandler::CreateSPN(
    const url::SchemeHostPort& scheme_host_port) {
  // The service principal name of the destination server.  See
  // http://msdn.microsoft.com/en-us/library/ms677949%28VS.85%29.aspx
  std::string target("HTTP/");
  target.append(GetHostAndOptionalPort(scheme_host_port));
  return target;
}


#if defined(NTLM_SSPI)
ConnectionAuthOriginalNtlmHandler::ConnectionAuthOriginalNtlmHandler(
    scoped_refptr<SSPILibrary> sspi_library,
    const HttpAuthPreferences* http_auth_preferences)
    : mechanism_(std::move(sspi_library), HttpAuth::AUTH_SCHEME_NTLM),
      http_auth_preferences_(http_auth_preferences) {}

int ConnectionAuthOriginalNtlmHandler::GenerateAuthTokenImpl(
    const AuthCredentials* credentials,
    const HttpRequestInfo* request,
    CompletionOnceCallback callback,
    std::string* auth_token) {
  return mechanism_.GenerateAuthToken(credentials, CreateSPN(scheme_host_port_),
                                      channel_bindings_, auth_token, net_log(),
                                      std::move(callback));
}

ConnectionAuthOriginalNtlmHandler::~ConnectionAuthOriginalNtlmHandler() = default;

// Require identity on first pass instead of second.
bool ConnectionAuthOriginalNtlmHandler::NeedsIdentity() {
  return mechanism_.NeedsIdentity();
}

bool ConnectionAuthOriginalNtlmHandler::AllowsDefaultCredentials() {
  if (target_ == HttpAuth::AUTH_PROXY)
    return true;
  if (!http_auth_preferences_)
    return false;
  return http_auth_preferences_->CanUseDefaultCredentials(scheme_host_port_);
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalNtlmHandler::ParseChallenge(
    HttpAuthChallengeTokenizer* tok) {
  return mechanism_.ParseChallenge(tok);
}

#else
ConnectionAuthOriginalNtlmHandler::ConnectionAuthOriginalNtlmHandler(
    const HttpAuthPreferences* http_auth_preferences)
    : mechanism_(http_auth_preferences) {}

bool ConnectionAuthOriginalNtlmHandler::NeedsIdentity() {
  return mechanism_.NeedsIdentity();
}

bool ConnectionAuthOriginalNtlmHandler::AllowsDefaultCredentials() {
  // Default credentials are not supported in the portable implementation of
  // NTLM, but are supported in the SSPI implementation.
  return false;
}

int ConnectionAuthOriginalNtlmHandler::GenerateAuthTokenImpl(
    const AuthCredentials* credentials,
    const HttpRequestInfo* request,
    CompletionOnceCallback callback,
    std::string* auth_token) {
  return mechanism_.GenerateAuthToken(credentials, CreateSPN(scheme_host_port_),
                                      channel_bindings_, auth_token, net_log(),
                                      std::move(callback));
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalNtlmHandler::ParseChallenge(
    HttpAuthChallengeTokenizer* tok) {
  return mechanism_.ParseChallenge(tok);
}

ConnectionAuthOriginalNtlmHandler::~ConnectionAuthOriginalNtlmHandler() = default;

#endif
}  // namespace net
