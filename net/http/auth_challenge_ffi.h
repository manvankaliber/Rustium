// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_AUTH_CHALLENGE_FFI_H_
#define NET_HTTP_AUTH_CHALLENGE_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>
// Input pointers are call-scoped. Writable buffers are initialized and
// disjoint from inputs. Empty buffers may use null pointers.
struct AuthChallengeTokenized {
  size_t scheme_begin;
  size_t scheme_len;
  size_t params_begin;
  size_t params_len;
  size_t has_params;
};
struct AuthChallengeLater {
  size_t encoded_len;
  size_t decision;
};
static_assert(std::is_standard_layout_v<AuthChallengeTokenized>);
static_assert(sizeof(AuthChallengeTokenized) == 5 * sizeof(size_t));
static_assert(alignof(AuthChallengeTokenized) == alignof(size_t));
static_assert(offsetof(AuthChallengeTokenized, scheme_len) == sizeof(size_t));
static_assert(offsetof(AuthChallengeTokenized, params_begin) ==
              2 * sizeof(size_t));
static_assert(offsetof(AuthChallengeTokenized, params_len) ==
              3 * sizeof(size_t));
static_assert(offsetof(AuthChallengeTokenized, has_params) ==
              4 * sizeof(size_t));
static_assert(std::is_standard_layout_v<AuthChallengeLater>);
static_assert(sizeof(AuthChallengeLater) == 2 * sizeof(size_t));
static_assert(offsetof(AuthChallengeLater, decision) == sizeof(size_t));
extern "C" AuthChallengeTokenized rustium_auth_tokenize(const char*,
                                                        size_t,
                                                        char*,
                                                        size_t);
extern "C" void rustium_auth_lower_in_place(char*, size_t);
extern "C" size_t rustium_auth_encoded_len(const char*, size_t);
// params is only read if scheme matches; this preserves mismatch behavior
// even when a prior output assignment invalidated the challenge backing view.
extern "C" int32_t rustium_auth_first(const char*,
                                      size_t,
                                      const char*,
                                      size_t,
                                      const char*,
                                      size_t);
extern "C" AuthChallengeLater rustium_auth_later_decision(const char*,
                                                          size_t,
                                                          const char*,
                                                          size_t,
                                                          const char*,
                                                          size_t);
extern "C" size_t rustium_auth_decode(const char*, size_t, char*, size_t);
#endif
