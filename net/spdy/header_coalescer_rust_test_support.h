// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HEADER_COALESCER_RUST_TEST_SUPPORT_H_
#define NET_SPDY_HEADER_COALESCER_RUST_TEST_SUPPORT_H_
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "net/log/net_log.h"
#include "net/spdy/header_coalescer.h"
#include "net/spdy/header_coalescer_rust_original.h"

namespace net::header_coalescer_rust_test {
using Headers = std::vector<std::pair<std::string, std::string>>;
inline Headers Copy(const quiche::HttpHeaderBlock& block) {
  Headers out;
  for (const auto& [key, value] : block) {
    out.emplace_back(key, value);
  }
  return out;
}
inline void Differential(std::string_view input) {
  // No observer, clocks, network or unbounded allocation. Structured choices
  // keep valid multi-header paths alive instead of stopping at random noise.
  if (input.empty()) {
    return;
  }
  auto byte = [&]() -> uint8_t {
    if (input.empty()) {
      return 0;
    }
    const uint8_t value = input.front();
    input.remove_prefix(1);
    return value;
  };
  const uint32_t limit = (byte() & 1) ? 4096 : byte();
  const NetLogWithSource log;
  HeaderCoalescer rust(limit, log);
  HeaderCoalescerRustOriginal cpp(limit, log);
  const std::string_view names[] = {":status", ":method",    "a", "cookie",
                                    "a",       "set-cookie", "",  ":",
                                    "A",       "A?"};
  for (unsigned step = 0; !input.empty() && step < 16; ++step) {
    const auto kind = byte();
    std::string_view key;
    if (kind < 240) {
      key = names[kind % std::size(names)];
    } else {
      key = input.substr(0, byte() % 17);
      input.remove_prefix(key.size());
    }
    const auto value = input.substr(0, byte() % 33);
    input.remove_prefix(value.size());
    rust.OnHeaderBlockStart();
    cpp.OnHeaderBlockStart();
    rust.OnHeader(key, value);
    cpp.OnHeader(key, value);
    CHECK(rust.error_seen() == cpp.error_seen());
    rust.OnHeaderBlockEnd(kind, step);
    cpp.OnHeaderBlockEnd(kind, step);
  }
  CHECK(Copy(rust.release_headers()) == Copy(cpp.release_headers()));
  // Appends after release are permitted by the original OnHeader. They must
  // preserve its existing error latch. Destructors exercise the moved state.
  rust.OnHeader("post-release", "ok");
  cpp.OnHeader("post-release", "ok");
  CHECK(rust.error_seen() == cpp.error_seen());
}
}  // namespace net::header_coalescer_rust_test
#endif  // NET_SPDY_HEADER_COALESCER_RUST_TEST_SUPPORT_H_
