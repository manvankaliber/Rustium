// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/auth_challenge_reference.h"

#include <utility>

#include "base/strings/string_util.h"
#include "net/http/auth_challenge_reference_modp.h"
#include "net/http/auth_challenge_reference_tokenizer.h"
namespace net::auth_challenge_reference {

// Preserve the original public class's out-of-line destructor and iterator
// boundaries as well as its algorithm, including in component benchmarks.
Tokenizer::~Tokenizer() = default;
HttpUtil::NameValuePairsIterator Tokenizer::param_pairs() const {
  return HttpUtil::NameValuePairsIterator(params_, ',');
}

namespace {
bool IsLws(char c) {
  return c == ' ' || c == '\t';
}
// Frozen base::Base64Decode strict path and independently named modp sources.
bool Decode(std::string_view input, std::string* output) {
  std::string buffer(input.size() / 4 * 3 + 2, '\0');
  const size_t n =
      auth_challenge_ref_modp_decode(buffer.data(), input.data(), input.size(),
                                     AuthChallengeRefDecodePolicy::kStrict);
  if (n == AUTH_CHALLENGE_REF_B64_ERROR) {
    return false;
  }
  buffer.resize(n);
  *output = std::move(buffer);
  return true;
}
}  // namespace
// Frozen copies of StringViewTokenizer's original algorithm and the original
// TrimLWS helper. ASCII normalization uses the unchanged original base DLL.
[[clang::noinline]] std::string_view FrozenTrim(std::string_view string) {
  size_t begin_offset = 0;
  size_t end_offset = string.size();
  while (begin_offset < end_offset && IsLws(string[begin_offset])) {
    ++begin_offset;
  }
  while (begin_offset < end_offset && IsLws(string[end_offset - 1])) {
    --end_offset;
  }
  return string.substr(begin_offset, end_offset - begin_offset);
}
Tokenizer::Tokenizer(std::string_view challenge) : challenge_(challenge) {
  frozen::StringViewTokenizer tok(challenge, " \t");
  if (!tok.GetNext()) {
    return;
  }
  scheme_ = base::ToLowerASCII(tok.token_piece());
  params_ = FrozenTrim(std::string_view(tok.token_end(), challenge.end()));
}
std::string_view Tokenizer::base64_param() const {
  size_t n = params_.size();
  while (n > 0 && n % 4 != 0 && params_[n - 1] == '=') {
    --n;
  }
  return params_.substr(0, n);
}
HttpAuth::AuthorizationResult First(HttpAuth::Scheme scheme,
                                    Tokenizer* challenge) {
  if (challenge->auth_scheme() != HttpAuth::SchemeToString(scheme)) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  if (!challenge->base64_param().empty()) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}
HttpAuth::AuthorizationResult Later(HttpAuth::Scheme scheme,
                                    Tokenizer* challenge,
                                    std::string* encoded,
                                    std::string* decoded) {
  if (challenge->auth_scheme() != HttpAuth::SchemeToString(scheme)) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  *encoded = challenge->base64_param();
  if (encoded->empty()) {
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  }
  if (!Decode(*encoded, decoded)) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}
}  // namespace net::auth_challenge_reference
