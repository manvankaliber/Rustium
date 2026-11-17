// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Adapted from auth_challenge_reference.cc; see the input manifest.
#include "net/http/connection_auth_original_rounds.h"

#include "net/http/auth_challenge_reference.h"
#include "net/http/http_auth_challenge_tokenizer.h"

namespace net::connection_auth_original_rounds {
namespace {
std::string_view Encoded(std::string_view params) {
  size_t n = params.size();
  while (n > 0 && n % 4 != 0 && params[n - 1] == '=') {
    --n;
  }
  return params.substr(0, n);
}
}  // namespace

HttpAuth::AuthorizationResult FirstFields(HttpAuth::Scheme scheme,
                                          std::string_view actual,
                                          std::string_view params) {
  if (actual != HttpAuth::SchemeToString(scheme) || !Encoded(params).empty())
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}
HttpAuth::AuthorizationResult LaterFields(HttpAuth::Scheme scheme,
                                          std::string_view actual,
                                          std::string_view params,
                                          std::string* encoded,
                                          std::string* decoded) {
  if (actual != HttpAuth::SchemeToString(scheme))
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  *encoded = Encoded(params);
  if (encoded->empty())
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  if (!auth_challenge_reference::FrozenDecode(*encoded, decoded))
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}
HttpAuth::AuthorizationResult First(HttpAuth::Scheme scheme,
                                    const HttpAuthChallengeTokenizer& challenge) {
  return FirstFields(scheme, challenge.auth_scheme(), challenge.params());
}
HttpAuth::AuthorizationResult Later(HttpAuth::Scheme scheme,
                                    const HttpAuthChallengeTokenizer& challenge,
                                    std::string* encoded,
                                    std::string* decoded) {
  return LaterFields(scheme, challenge.auth_scheme(), challenge.params(), encoded,
                     decoded);
}
}  // namespace net::connection_auth_original_rounds
