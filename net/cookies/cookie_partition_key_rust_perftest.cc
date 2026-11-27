// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Parent owns the quiet timing window. --verify-only does not read a clock.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <string_view>

#include "net/cookies/cookie_partition_key_rust_test_support.h"

namespace net::cookie_partition_key_test {
namespace {
volatile uint64_t sink = 0;
template <class Types>
std::pair<double, uint64_t> Measure(const Case& c, const Inputs& in, size_t count) {
  const auto start = std::chrono::steady_clock::now();
  const uint64_t checksum = Run<Types>(c, in, count);
  const auto end = std::chrono::steady_clock::now();
  sink = checksum;
  return {std::chrono::duration<double, std::nano>(end - start).count() / count,
          checksum};
}
}  // namespace
}  // namespace net::cookie_partition_key_test

int main(int argc, char** argv) {
  using namespace net::cookie_partition_key_test;
  const auto option = argc == 2 ? UNSAFE_BUFFERS(std::string_view(argv[1]))
                                : std::string_view();
  if (option != "--verify-only" && option != "--timing") {
    UNSAFE_BUFFERS(std::fputs(
        "Use --verify-only; --timing requires the parent-owned quiet window.\n", stderr));
    return 2;
  }
  const Inputs inputs;
  // All independent semantic checks and every paired workload precede ANY
  // timing clock. No case can time successfully after a failed preflight.
  VerifyAllWorkloads(inputs);
  // Opaque Origin identities are process-generated primitives. Archive these
  // exact initialized IDs from stderr with the raw paired report; do not claim
  // they were fixed constants or reconstruct them from file timestamps.
  for (size_t i = 0; i < inputs.origins.size(); ++i) {
    const std::string debug = inputs.origins[i].GetDebugString(true);
    UNSAFE_BUFFERS(std::fprintf(stderr, "fixture_origin[%zu]=%s\n", i, debug.c_str()));
  }
  if (option == "--verify-only") {
    std::puts("Prepared 15 public-API workload checksums match; no clock read.");
    return 0;
  }
  std::mt19937 order(20261113);
  for (const Case& c : kCases) {
    const size_t iterations = c.workload == Workload::kComparison ? 64 : 256;
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (order() & 1) != 0;
      std::pair<double, uint64_t> baseline, rust;
      if (rust_first) {
        rust = Measure<Production>(c, inputs, iterations);
        baseline = Measure<Original>(c, inputs, iterations);
      } else {
        baseline = Measure<Original>(c, inputs, iterations);
        rust = Measure<Production>(c, inputs, iterations);
      }
      CHECK_EQ(baseline.second, rust.second) << c.key;
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"iterations\":%zu,\"rust_first\":%s,"
          "\"baseline_ns\":%.9f,\"rust_ns\":%.9f,"
          "\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
          c.key, repeat, iterations, rust_first ? "true" : "false",
          baseline.first, rust.first,
          static_cast<unsigned long long>(baseline.second),
          static_cast<unsigned long long>(rust.second)));
    }
  }
}
