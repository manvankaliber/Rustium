// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_auth_multi_round_parse.h"

#include <limits>
#include <string_view>

#include "net/http/auth_challenge_ffi.h"
#include "net/http/http_auth_challenge_tokenizer.h"
namespace net {
static_assert(HttpAuth::AUTHORIZATION_RESULT_ACCEPT == 0);
static_assert(HttpAuth::AUTHORIZATION_RESULT_REJECT == 1);
static_assert(HttpAuth::AUTHORIZATION_RESULT_INVALID == 3);
HttpAuth::AuthorizationResult ParseFirstRoundChallenge(
    HttpAuth::Scheme scheme,
    HttpAuthChallengeTokenizer* challenge) {
  const auto expected = HttpAuth::SchemeToString(scheme);
  const auto& actual = challenge->auth_scheme();
  if (rustium_auth_first(actual.data(), actual.size(), expected.data(),
                         expected.size(),
                         0) == HttpAuth::AUTHORIZATION_RESULT_INVALID) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  const auto encoded = challenge->base64_param();
  return static_cast<HttpAuth::AuthorizationResult>(
      rustium_auth_first(actual.data(), actual.size(), expected.data(),
                         expected.size(), encoded.size()));
}
HttpAuth::AuthorizationResult ParseLaterRoundChallenge(
    HttpAuth::Scheme scheme,
    HttpAuthChallengeTokenizer* challenge,
    std::string* encoded_token,
    std::string* decoded_token) {
  const auto expected = HttpAuth::SchemeToString(scheme);
  const auto& actual = challenge->auth_scheme();
  if (rustium_auth_later_decision(actual.data(), actual.size(), expected.data(),
                                  expected.size(), 0) ==
      HttpAuth::AUTHORIZATION_RESULT_INVALID) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  const auto encoded = challenge->base64_param();
  const int decision =
      rustium_auth_later_decision(actual.data(), actual.size(), expected.data(),
                                  expected.size(), encoded.size());
  if (decision == HttpAuth::AUTHORIZATION_RESULT_INVALID) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  // Commit in the original order. In particular encoded_token can own the
  // challenge bytes and can be the same object as decoded_token.
  *encoded_token = encoded;
  if (decision == HttpAuth::AUTHORIZATION_RESULT_REJECT) {
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  }
  std::string decoded(encoded_token->size(), '\0');
  const size_t n = rustium_auth_decode(encoded_token->data(),
                                       encoded_token->size(), decoded.data());
  if (n == std::numeric_limits<size_t>::max()) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  decoded.resize(n);
  *decoded_token = std::move(decoded);
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}
}  // namespace net
