// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_ROUNDS_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_ROUNDS_H_

#include <string>
#include <string_view>

#include "net/http/http_auth.h"

namespace net {
class HttpAuthChallengeTokenizer;
namespace connection_auth_original_rounds {
// The fields must have been initialized by the original tokenizer policy.
// Input views can alias either output; output pointers can alias each other.
HttpAuth::AuthorizationResult First(HttpAuth::Scheme scheme,
                                    const HttpAuthChallengeTokenizer& challenge);
HttpAuth::AuthorizationResult Later(HttpAuth::Scheme scheme,
                                    const HttpAuthChallengeTokenizer& challenge,
                                    std::string* encoded,
                                    std::string* decoded);
// Exposes the same original algorithm for source-level verification without
// invoking the production tokenizer in the reference half of a test.
HttpAuth::AuthorizationResult FirstFields(HttpAuth::Scheme scheme,
                                          std::string_view actual,
                                          std::string_view params);
HttpAuth::AuthorizationResult LaterFields(HttpAuth::Scheme scheme,
                                          std::string_view actual,
                                          std::string_view params,
                                          std::string* encoded,
                                          std::string* decoded);
}  // namespace connection_auth_original_rounds
}  // namespace net
#endif  // NET_HTTP_CONNECTION_AUTH_ORIGINAL_ROUNDS_H_
