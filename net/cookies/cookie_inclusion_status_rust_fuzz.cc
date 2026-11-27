// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Reproducible differential state fuzz; no timing measurement is performed.
#include <cstdio>

#include "net/cookies/cookie_inclusion_status_rust_test_support.h"
int main() {
  using namespace net::cookie_inclusion_status_rust_test;
  uint64_t random = 0x636f6f6b69657331;
  Rust a;
  Original b;
  for (size_t i = 0; i < 1000000; ++i) {
    const auto bits = Random(random);
    const auto warnings = Random(random);
    const int reason = static_cast<int>(Random(random));
    const auto op = static_cast<unsigned>(Random(random) % 13);
    if (!DeserializeEqual(bits, warnings, reason) ||
        !DeserializeEqual(0, warnings, reason)) {
      return 1;
    }
    Step(a, op, reason, bits);
    Step(b, op, reason, bits);
    if (!Equal(a, b)) {
      std::fprintf(stderr, "differential failure at %zu op=%u seed=%llu\n", i,
                   op, static_cast<unsigned long long>(random));
      return 2;
    }
    if (i % 64 == 0) {
      // Valid high-density deserialized states, as well as API-reachable
      // states.
      uint64_t ex = bits & ((uint64_t{1} << 30) - 1);
      if (ex != (1 << 24)) {
        ex &= ~(uint64_t{1} << 24);
      }
      a = *Components<Rust>(ex, warnings, 0);
      b = *Components<Original>(ex, warnings, 0);
    }
  }
  std::puts(
      "PASS: 1000000 state transitions; 2000000 full-width deserialization "
      "probes; seed=0x636f6f6b69657331; all public "
      "queries/format/copy/equality/exact-testing compared");
}
