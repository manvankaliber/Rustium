// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "net/url_request/referrer_policy_reference.h"

namespace {
volatile uint64_t sink = 0;
struct Case {
  const char* key;
  std::string input;
};
double Measure(const Case& item, bool rust, int iterations) {
  uint64_t total = 0;
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i) {
    const auto value =
        rust ? net::ReferrerPolicyFromHeader(item.input)
             : net::referrer_policy_reference::ReferrerPolicyFromHeader(
                   item.input);
    total += value ? 1 + static_cast<int>(*value) : 0;
  }
  sink = total;
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         iterations;
}
}  // namespace

int main() {
  // Input construction and warmup are excluded. Both calls cross a component
  // boundary in component builds. All original parser allocations/destruction,
  // the Rust bridge, and returned optional consumption are inside timing.
  const std::vector<Case> cases = {
      {"referrer-empty", ""},
      {"referrer-origin", "origin"},
      {"referrer-strict", "strict-origin-when-cross-origin"},
      {"referrer-mixed-case", " \tStRiCt-OrIgIn\r\n"},
      {"referrer-unknown", "future-policy"},
      {"referrer-list", "no-referrer, origin, unsafe-url"},
      {"referrer-invalid-suffix", "strict-origin,unknown, ,unknown"},
      {"referrer-long-unknown", std::string(4096, 'x')},
      {"referrer-long-prefix", std::string(4096, 'x') + ",origin"},
      {"referrer-long-suffix", "origin," + std::string(4096, 'x')},
  };
  std::mt19937 random(20261112);
  for (const auto& item : cases) {
    Measure(item, false, 4096);
    Measure(item, true, 4096);
    for (int repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double baseline_ns;
      double rust_ns;
      if (rust_first) {
        rust_ns = Measure(item, true, 20000);
        baseline_ns = Measure(item, false, 20000);
      } else {
        baseline_ns = Measure(item, false, 20000);
        rust_ns = Measure(item, true, 20000);
      }
      // SAFETY: item.key is a NUL-terminated literal in the cases above.
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"repeat\":%d,\"baseline_ns\":%.9f,"
                      "\"rust_ns\":%.9f,\"rust_first\":%s}\n",
                      item.key, repeat, baseline_ns, rust_ns,
                      rust_first ? "true" : "false"));
    }
  }
}
