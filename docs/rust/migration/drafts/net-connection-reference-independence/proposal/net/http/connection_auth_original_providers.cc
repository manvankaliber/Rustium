// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Prepared source: provider token generation/logging still needs closure.
#include "net/http/connection_auth_original_providers.h"

#include <utility>

#include "net/base/net_errors.h"
#include "net/cert/x509_util.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/http/connection_auth_original_rounds.h"
#include "net/ssl/ssl_info.h"

namespace net {
namespace rounds = connection_auth_original_rounds;
#if BUILDFLAG(IS_WIN)
HttpAuth::AuthorizationResult ConnectionAuthOriginalProviders::Parse(
    HttpAuthSSPI* self, HttpAuthChallengeTokenizer* tok) {
  if (!self->has_security_context_)
    return rounds::First(self->scheme_, *tok);
  std::string encoded;
  return rounds::Later(self->scheme_, *tok, &encoded,
                       &self->decoded_server_auth_token_);
}
#else
HttpAuth::AuthorizationResult ConnectionAuthOriginalProviders::Parse(
    HttpAuthNtlmMechanism* self, HttpAuthChallengeTokenizer* tok) {
  if (!self->first_token_sent_)
    return rounds::First(HttpAuth::AUTH_SCHEME_NTLM, *tok);
  self->challenge_token_.clear();
  std::string encoded;
  return rounds::Later(HttpAuth::AUTH_SCHEME_NTLM, *tok, &encoded,
                       &self->challenge_token_);
}
#endif
#if BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_POSIX)
HttpAuth::AuthorizationResult ConnectionAuthOriginalProviders::Parse(
    HttpAuthGSSAPI* self, HttpAuthChallengeTokenizer* tok) {
  if (self->scoped_sec_context_.get() == GSS_C_NO_CONTEXT)
    return rounds::First(HttpAuth::AUTH_SCHEME_NEGOTIATE, *tok);
  std::string encoded;
  return rounds::Later(HttpAuth::AUTH_SCHEME_NEGOTIATE, *tok, &encoded,
                       &self->decoded_server_auth_token_);
}
#endif

#if BUILDFLAG(IS_WIN)
ConnectionAuthOriginalNtlmHandler::ConnectionAuthOriginalNtlmHandler(
    scoped_refptr<SSPILibrary> library, const HttpAuthPreferences* preferences)
    : HttpAuthHandlerNTLM(std::move(library), preferences) {}
#else
ConnectionAuthOriginalNtlmHandler::ConnectionAuthOriginalNtlmHandler(
    const HttpAuthPreferences* preferences) : HttpAuthHandlerNTLM(preferences) {}
#endif
bool ConnectionAuthOriginalNtlmHandler::Init(
    HttpAuthChallengeTokenizer* tok, const SSLInfo& ssl_info,
    const NetworkAnonymizationKey&) {
  auth_scheme_ = HttpAuth::AUTH_SCHEME_NTLM;
  score_ = 3;
  properties_ = ENCRYPTS_IDENTITY | IS_CONNECTION_BASED;
  if (ssl_info.is_valid())
    x509_util::GetTLSServerEndPointChannelBinding(*ssl_info.cert, &channel_bindings_);
  return ConnectionAuthOriginalProviders::Parse(&mechanism_, tok) ==
         HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}
HttpAuth::AuthorizationResult ConnectionAuthOriginalNtlmHandler::HandleAnotherChallengeImpl(
    HttpAuthChallengeTokenizer* tok) {
  return ConnectionAuthOriginalProviders::Parse(&mechanism_, tok);
}
int ConnectionAuthOriginalNtlmFactory::CreateAuthHandler(
    HttpAuthChallengeTokenizer* challenge, HttpAuth::Target target,
    const SSLInfo& ssl_info, const NetworkAnonymizationKey& key,
    const url::SchemeHostPort& origin, CreateReason reason, int,
    const NetLogWithSource& log, HostResolver*,
    std::unique_ptr<HttpAuthHandler>* handler) {
  if (reason == CREATE_PREEMPTIVE)
    return ERR_UNSUPPORTED_AUTH_SCHEME;
#if BUILDFLAG(IS_WIN)
  auto candidate = std::make_unique<ConnectionAuthOriginalNtlmHandler>(library_, http_auth_preferences());
#else
  auto candidate = std::make_unique<ConnectionAuthOriginalNtlmHandler>(http_auth_preferences());
#endif
  if (!ConnectionAuthOriginalHandlerPolicy::InitFromChallenge(
          candidate.get(), challenge, target, ssl_info, key, origin, log))
    return ERR_INVALID_RESPONSE;
  *handler = std::move(candidate);
  return OK;
}
#if BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_WIN)
HttpAuth::AuthorizationResult ConnectionAuthOriginalSSPI::ParseChallenge(
    HttpAuthChallengeTokenizer* tok) {
  return ConnectionAuthOriginalProviders::Parse(this, tok);
}
#elif BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_POSIX)
HttpAuth::AuthorizationResult ConnectionAuthOriginalGSSAPI::ParseChallenge(
    HttpAuthChallengeTokenizer* tok) {
  return ConnectionAuthOriginalProviders::Parse(this, tok);
}
#endif
}  // namespace net
