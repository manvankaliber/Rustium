// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_REFERRER_POLICY_DIFFERENTIAL_H_
#define NET_URL_REQUEST_REFERRER_POLICY_DIFFERENTIAL_H_

#include <array>
#include <string>
#include <string_view>

#include "base/check_op.h"
#include "net/url_request/referrer_policy_reference.h"

namespace net::referrer_policy_test {
inline constexpr std::array<std::string_view, 8> kTokens = {
    "no-referrer",
    "no-referrer-when-downgrade",
    "origin",
    "origin-when-cross-origin",
    "same-origin",
    "strict-origin",
    "strict-origin-when-cross-origin",
    "unsafe-url"};

inline void Check(std::string_view input) {
  const auto expected =
      referrer_policy_reference::ReferrerPolicyFromHeader(input);
  CHECK(net::ReferrerPolicyFromHeader(input) == expected);
}

// Both implementations must respect a subview's length, rather than read a
// surrounding byte or require NUL termination.
inline void CheckSlice(std::string_view input) {
  Check(input);
  const std::string surrounded = "x" + std::string(input) + "unsafe-url";
  Check(std::string_view(surrounded).substr(1, input.size()));
}
}  // namespace net::referrer_policy_test

#endif  // NET_URL_REQUEST_REFERRER_POLICY_DIFFERENTIAL_H_
