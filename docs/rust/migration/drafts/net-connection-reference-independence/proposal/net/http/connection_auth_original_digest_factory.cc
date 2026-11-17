// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "net/http/connection_auth_original_digest_factory.h"

#include <utility>

#include "net/base/net_errors.h"
#include "net/http/connection_auth_original_policy.h"

namespace net {
ConnectionAuthOriginalDigestFactory::ConnectionAuthOriginalDigestFactory()
    : nonce_generator_(std::make_unique<HttpAuthHandlerDigest::DynamicNonceGenerator>()) {}

int ConnectionAuthOriginalDigestFactory::CreateAuthHandler(
    HttpAuthChallengeTokenizer* challenge, HttpAuth::Target target,
    const SSLInfo& ssl_info, const NetworkAnonymizationKey& key,
    const url::SchemeHostPort& origin, CreateReason, int nonce_count,
    const NetLogWithSource& log, HostResolver*,
    std::unique_ptr<HttpAuthHandler>* handler) {
  auto candidate = std::unique_ptr<HttpAuthHandlerDigest>(
      new HttpAuthHandlerDigest(nonce_count, nonce_generator_.get()));
  if (!ConnectionAuthOriginalHandlerPolicy::InitFromChallenge(
          candidate.get(), challenge, target, ssl_info, key, origin, log))
    return ERR_INVALID_RESPONSE;
  *handler = std::move(candidate);
  return OK;
}
}  // namespace net
