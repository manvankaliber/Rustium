// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_MIME_TYPE_RUST_TEST_SUPPORT_H_
#define NET_BASE_MIME_TYPE_RUST_TEST_SUPPORT_H_
#include <array>
#include <cstdlib>
#include <string>
#include <string_view>

#include "net/base/mime_type_rust_reference.h"
#include "net/http/http_util.h"

namespace net::mime_type_test {
inline void Require(bool result) {
  if (!result) {
    std::abort();
  }
}

// Compare null-output combinations, partial outputs on failure, and all output
// alias partitions. Content-Type parses all bytes before changing any outputs.
inline void Check(std::string_view input) {
  for (unsigned mask = 0; mask < 4; ++mask) {
    std::string a = "sentinel", b = a;
    base::StringPairs pa = {{"old", "value"}}, pb = pa;
    const bool x = LegacyParseMimeType(input, mask & 1 ? &a : nullptr,
                                       mask & 2 ? &pa : nullptr);
    const bool y =
        ParseMimeType(input, mask & 1 ? &b : nullptr, mask & 2 ? &pb : nullptr);
    Require(x == y && a == b && pa == pb);
    std::string sa = "sub", sb = sa;
    const bool u = LegacyWithoutParameter(input, mask & 1 ? &a : nullptr,
                                          mask & 2 ? &sa : nullptr);
    const bool v = ParseMimeTypeWithoutParameter(input, mask & 1 ? &b : nullptr,
                                                 mask & 2 ? &sb : nullptr);
    Require(u == v && a == b && sa == sb);
  }
  std::string a = "old", b = a;
  Require(LegacyWithoutParameter(input, &a, &a) ==
          ParseMimeTypeWithoutParameter(input, &b, &b));
  Require(a == b);
  // Bell partitions of {mime, charset, boundary}, plus null boundary.
  for (const auto ids :
       {std::array{0, 1, 2}, std::array{0, 0, 1}, std::array{0, 1, 0},
        std::array{0, 1, 1}, std::array{0, 0, 0}, std::array{0, 1, -1}}) {
    std::array<std::string, 3> left = {"Text/Html", "OLD", "keep"};
    auto right = left;
    bool ha = input.size() % 2, hb = ha;
    auto apply = [&](std::string_view header) {
      auto* ba = ids[2] < 0 ? nullptr : &left[ids[2]];
      auto* bb = ids[2] < 0 ? nullptr : &right[ids[2]];
      LegacyContentType(header, &left[ids[0]], &left[ids[1]], &ha, ba);
      HttpUtil::ParseContentType(header, &right[ids[0]], &right[ids[1]], &hb,
                                 bb);
      Require(left == right && ha == hb);
    };
    apply(input);
    apply("TEXT/HTML");
    apply("TEXT/plain; charset=\"\"; boundary=\" b \"");
    apply(input);
  }
}
}  // namespace net::mime_type_test
#endif  // NET_BASE_MIME_TYPE_RUST_TEST_SUPPORT_H_
