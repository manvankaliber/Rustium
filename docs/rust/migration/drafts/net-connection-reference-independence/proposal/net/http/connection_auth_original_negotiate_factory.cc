// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Prepared source; the injected custom mechanism is not independently audited.
#include "net/http/connection_auth_original_negotiate_factory.h"
#if BUILDFLAG(USE_KERBEROS)
#include <utility>
#include "net/base/net_errors.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/http/connection_auth_original_negotiate_handler.h"
#include "net/http/connection_auth_original_providers.h"
#include "net/http/http_auth_preferences.h"

namespace net {
int ConnectionAuthOriginalNegotiateFactory::CreateAuthHandler(
    HttpAuthChallengeTokenizer* challenge, HttpAuth::Target target,
    const SSLInfo& ssl_info, const NetworkAnonymizationKey& key,
    const url::SchemeHostPort& origin, CreateReason reason, int,
    const NetLogWithSource& log, HostResolver* resolver,
    std::unique_ptr<HttpAuthHandler>* handler) {
#if BUILDFLAG(IS_WIN)
  if (is_unsupported_ || reason == CREATE_PREEMPTIVE)
    return ERR_UNSUPPORTED_AUTH_SCHEME;
#elif BUILDFLAG(IS_POSIX)
  if (is_unsupported_)
    return ERR_UNSUPPORTED_AUTH_SCHEME;
#if BUILDFLAG(IS_LINUX)
  if (!http_auth_preferences() || !http_auth_preferences()->AllowGssapiLibraryLoad())
    return ERR_UNSUPPORTED_AUTH_SCHEME;
#endif
  if (!library_->Init(log)) {
    is_unsupported_ = true;
    return ERR_UNSUPPORTED_AUTH_SCHEME;
  }
#endif
  std::unique_ptr<HttpAuthMechanism> mechanism;
  if (mechanism_factory_) {
    mechanism = mechanism_factory_.Run(http_auth_preferences());
  } else {
#if BUILDFLAG(IS_WIN)
    mechanism = std::make_unique<ConnectionAuthOriginalSSPI>(
        library_, HttpAuth::AUTH_SCHEME_NEGOTIATE);
#elif BUILDFLAG(IS_POSIX)
    mechanism = std::make_unique<ConnectionAuthOriginalGSSAPI>(
        library_, CHROME_GSS_SPNEGO_MECH_OID_DESC);
#endif
  }
  auto candidate = std::make_unique<ConnectionAuthOriginalNegotiateHandler>(
      std::move(mechanism), http_auth_preferences(), resolver);
  if (!ConnectionAuthOriginalHandlerPolicy::InitFromChallenge(
          candidate.get(), challenge, target, ssl_info, key, origin, log))
    return ERR_INVALID_RESPONSE;
  *handler = std::move(candidate);
  return OK;
}
}  // namespace net
#endif
