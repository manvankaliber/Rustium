// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_AUTH_CHALLENGE_REFERENCE_H_
#define NET_HTTP_AUTH_CHALLENGE_REFERENCE_H_
#include <string>
#include <string_view>

#include "base/component_export.h"
#include "net/http/http_auth.h"
#include "net/http/http_util.h"
namespace net::auth_challenge_reference {
// Frozen original algorithm. It never calls the production tokenizer or Rust.
class COMPONENT_EXPORT(AUTH_CHALLENGE_REFERENCE) Tokenizer {
 public:
  explicit Tokenizer(std::string_view challenge);
  ~Tokenizer();
  std::string_view challenge_text() const { return challenge_; }
  const std::string& auth_scheme() const { return scheme_; }
  std::string_view params() const { return params_; }
  std::string_view base64_param() const;
  HttpUtil::NameValuePairsIterator param_pairs() const;

 private:
  std::string_view challenge_;
  std::string_view params_;
  std::string scheme_;
};
COMPONENT_EXPORT(AUTH_CHALLENGE_REFERENCE)
HttpAuth::AuthorizationResult First(HttpAuth::Scheme scheme,
                                    Tokenizer* challenge);
COMPONENT_EXPORT(AUTH_CHALLENGE_REFERENCE)
HttpAuth::AuthorizationResult Later(HttpAuth::Scheme scheme,
                                    Tokenizer* challenge,
                                    std::string* encoded,
                                    std::string* decoded);
}  // namespace net::auth_challenge_reference
#endif
