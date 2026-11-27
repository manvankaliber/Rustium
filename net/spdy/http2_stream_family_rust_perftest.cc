// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <string_view>
#include "base/compiler_specific.h"
#include "net/spdy/http2_stream_family_rust_test_support.h"

namespace {
constexpr std::array<const char*, 5> kCases = {
    "stream_lifecycle", "response_headers_cycle", "early_hints_and_trailers",
    "flow_window_adjustments", "closed_window_noop"};
volatile uint64_t sink;
template <class Stream>
std::pair<double, uint64_t> Measure(size_t kind,
                                  const net::http2_family_test::Inputs& inputs) {
  constexpr size_t kIterations = 256;
  uint64_t checksum = 0;
  const auto start = std::chrono::steady_clock::now();
  for (size_t i = 0; i != kIterations; ++i) {
    checksum = checksum * 131 +
        net::http2_family_test::RunCase<Stream>(kind, inputs, false);
  }
  const auto end = std::chrono::steady_clock::now();
  sink = checksum;
  return {std::chrono::duration<double, std::nano>(end - start).count() /
              kIterations, checksum};
}
}  // namespace

int main(int argc, char** argv) {
  const bool verify = argc == 2 &&
      UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing = argc == 2 &&
      UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) return 2;
  const net::http2_family_test::Inputs inputs;
  for (size_t kind = 0; kind != kCases.size(); ++kind) {
    CHECK_EQ(net::http2_family_test::RunCase<net::SpdyStreamOriginal>(kind, inputs, true),
             net::http2_family_test::RunCase<net::SpdyStream>(kind, inputs, true));
  }
  if (verify) {
    UNSAFE_BUFFERS(std::puts("Verified five isolated public stream-control paths; no timing"));
    return 0;
  }
  std::mt19937 random(20261113);
  for (size_t kind = 0; kind != kCases.size(); ++kind) {
    for (unsigned repeat = 0; repeat != 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      std::pair<double, uint64_t> baseline, rust;
      if (rust_first) {
        rust = Measure<net::SpdyStream>(kind, inputs);
        baseline = Measure<net::SpdyStreamOriginal>(kind, inputs);
      } else {
        baseline = Measure<net::SpdyStreamOriginal>(kind, inputs);
        rust = Measure<net::SpdyStream>(kind, inputs);
      }
      CHECK_EQ(baseline.second, rust.second);
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,"
          "\"rust_ns\":%.9f,\"iterations\":256,\"rust_first\":%s,"
          "\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
          kCases[kind], repeat, baseline.first, rust.first,
          rust_first ? "true" : "false",
          static_cast<unsigned long long>(baseline.second),
          static_cast<unsigned long long>(rust.second)));
    }
  }
}
