// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_auth_challenge_tokenizer.h"

#include <array>

#include "net/http/auth_challenge_ffi.h"
namespace net {
HttpAuthChallengeTokenizer::HttpAuthChallengeTokenizer(
    std::string_view challenge)
    : challenge_(challenge) {
  Init(challenge);
}
HttpAuthChallengeTokenizer::~HttpAuthChallengeTokenizer() = default;
// The shared name/value iterator remains a C++ metadata/iteration seam.
HttpUtil::NameValuePairsIterator HttpAuthChallengeTokenizer::param_pairs()
    const {
  return HttpUtil::NameValuePairsIterator(params_, /*delimiter=*/',');
}
std::string_view HttpAuthChallengeTokenizer::base64_param() const {
  return params_.substr(
      0, rustium_auth_encoded_len(params_.data(), params_.size()));
}
void HttpAuthChallengeTokenizer::Init(std::string_view challenge) {
  std::array<char, 64> scratch{};
  const auto result = rustium_auth_tokenize(challenge.data(), challenge.size(),
                                            scratch.data(), scratch.size());
  if (result.scheme_len <= scratch.size()) {
    lower_case_scheme_.assign(scratch.data(), result.scheme_len);
  } else {
    lower_case_scheme_.assign(
        challenge.substr(result.scheme_begin, result.scheme_len));
    rustium_auth_lower_in_place(lower_case_scheme_.data(),
                                lower_case_scheme_.size());
  }
  if (result.has_params) {
    params_ = challenge.substr(result.params_begin, result.params_len);
  }
}
}  // namespace net
