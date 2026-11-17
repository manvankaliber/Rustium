// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_COOKIES_PARSED_COOKIE_RUST_FFI_H_
#define NET_COOKIES_PARSED_COOKIE_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>
// All borrows are call-scoped. Empty byte ranges may be null. Mutable buffers
// are initialized, exclusive and disjoint. No Rust ownership crosses this ABI.
struct ParsedCookieRustScan {
  size_t next, begin, len, found;
};
struct ParsedCookieRustPair {
  size_t action, next, name_begin, name_len, value_begin, value_len;
  size_t attribute, reason, internal_htab, nameless;
};
struct ParsedCookieRustMutation {
  size_t action, begin, len;
};
struct ParsedCookieRustSameSite {
  int32_t value, metric;
};
static_assert(std::is_standard_layout_v<ParsedCookieRustScan>);
static_assert(std::is_standard_layout_v<ParsedCookieRustPair>);
static_assert(std::is_standard_layout_v<ParsedCookieRustMutation>);
static_assert(std::is_standard_layout_v<ParsedCookieRustSameSite>);
static_assert(sizeof(ParsedCookieRustScan) == 4 * sizeof(size_t));
static_assert(sizeof(ParsedCookieRustPair) == 10 * sizeof(size_t));
static_assert(sizeof(ParsedCookieRustMutation) == 3 * sizeof(size_t));
static_assert(sizeof(ParsedCookieRustSameSite) == 2 * sizeof(int32_t));
extern "C" size_t rustium_cookie_query(size_t, const char*, size_t);
extern "C" ParsedCookieRustScan rustium_cookie_scan(const char*,
                                                    size_t,
                                                    bool,
                                                    bool,
                                                    size_t);
extern "C" ParsedCookieRustPair rustium_cookie_next(const char*,
                                                    size_t,
                                                    size_t,
                                                    bool);
extern "C" size_t rustium_cookie_valid_pair(const char*,
                                            size_t,
                                            const char*,
                                            size_t);
extern "C" ParsedCookieRustMutation rustium_cookie_mutation(size_t,
                                                            const char*,
                                                            size_t,
                                                            const char*,
                                                            size_t,
                                                            const char*,
                                                            size_t,
                                                            bool);
extern "C" void rustium_cookie_lower(char*, size_t);
extern "C" void rustium_cookie_clear(size_t*, size_t);
extern "C" ParsedCookieRustSameSite rustium_cookie_same_site(const char*,
                                                             size_t,
                                                             bool);
extern "C" size_t rustium_cookie_serialization(const char*, size_t, bool, bool);
#endif
