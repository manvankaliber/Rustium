// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_AUTH_CHALLENGE_FFI_H_
#define NET_HTTP_AUTH_CHALLENGE_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>
// Inputs are valid for the call only; output buffers are initialized,
// disjoint and at least input length. Empty input may be null.
struct AuthChallengeTokenized {
  size_t scheme_len;
  size_t params_begin;
  size_t params_len;
  size_t has_params;
};
static_assert(std::is_standard_layout_v<AuthChallengeTokenized>);
static_assert(sizeof(AuthChallengeTokenized) == 4 * sizeof(size_t));
static_assert(alignof(AuthChallengeTokenized) == alignof(size_t));
static_assert(offsetof(AuthChallengeTokenized, params_begin) == sizeof(size_t));
static_assert(offsetof(AuthChallengeTokenized, params_len) ==
              2 * sizeof(size_t));
static_assert(offsetof(AuthChallengeTokenized, has_params) ==
              3 * sizeof(size_t));
extern "C" AuthChallengeTokenized rustium_auth_tokenize(const char*,
                                                        size_t,
                                                        char*);
extern "C" size_t rustium_auth_encoded_len(const char*, size_t);
extern "C" int32_t rustium_auth_first(const char*,
                                      size_t,
                                      const char*,
                                      size_t,
                                      size_t);
extern "C" int32_t rustium_auth_later_decision(const char*,
                                               size_t,
                                               const char*,
                                               size_t,
                                               size_t);
extern "C" size_t rustium_auth_decode(const char*, size_t, char*);
#endif
