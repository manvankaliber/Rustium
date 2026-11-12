// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_auth_multi_round_parse.h"

#include <limits>
#include <string_view>
#include <utility>

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
  const auto params = challenge->params();
  return static_cast<HttpAuth::AuthorizationResult>(
      rustium_auth_first(actual.data(), actual.size(), expected.data(),
                         expected.size(), params.data(), params.size()));
}
HttpAuth::AuthorizationResult ParseLaterRoundChallenge(
    HttpAuth::Scheme scheme,
    HttpAuthChallengeTokenizer* challenge,
    std::string* encoded_token,
    std::string* decoded_token) {
  const auto expected = HttpAuth::SchemeToString(scheme);
  const auto& actual = challenge->auth_scheme();
  const auto params = challenge->params();
  const auto result = rustium_auth_later_decision(
      actual.data(), actual.size(), expected.data(), expected.size(),
      params.data(), params.size());
  if (result.decision == HttpAuth::AUTHORIZATION_RESULT_INVALID) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  // Commit in the original order. The input can be owned by encoded_token,
  // and both output pointers may name the same string.
  *encoded_token = params.substr(0, result.encoded_len);
  if (result.decision == HttpAuth::AUTHORIZATION_RESULT_REJECT) {
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  }
  // The decoder writes only into a private, initialized buffer. Its partial
  // contents are discarded on failure, leaving decoded_token untouched.
  std::string decoded(encoded_token->size() / 4 * 3, '\0');
  const size_t n =
      rustium_auth_decode(encoded_token->data(), encoded_token->size(),
                          decoded.data(), decoded.size());
  if (n == std::numeric_limits<size_t>::max()) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  decoded.resize(n);
  *decoded_token = std::move(decoded);
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}
}  // namespace net
