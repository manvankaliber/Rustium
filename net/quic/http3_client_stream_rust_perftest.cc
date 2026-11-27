// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdio>
#include <string_view>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/test/test_timeouts.h"
#include "net/quic/http3_client_stream_rust_reference.h"
#include "net/quic/http3_client_stream_rust_test_support.h"

namespace {
using Rust = net::QuicChromiumClientStream;
using Original = net::http3_reference::QuicChromiumClientStream;
using net::http3_rust_test::Fixture;
constexpr std::array kCases = {"headers-body-1", "headers-body-16",
                               "headers-body-64", "headers-body-256",
                               "headers-body-1024"};
constexpr std::array<uint32_t, 5> kOperations = {1, 16, 64, 256, 1024};
template <class Stream>
std::vector<uint64_t> Run(
    size_t which,
    uint32_t iterations,
    net::http3_rust_test::ActionCounts* counts = nullptr) {
  Fixture<Stream> fixture(quic::AllSupportedVersions().front());
  std::vector<uint64_t> result;
  uint64_t seed = 0x39019d00f11e;
  for (uint32_t i = 0; i < iterations; ++i) {
    auto trace = fixture.Scenario(
        net::http3_rust_test::Next(seed) ^ (0x9e3779b97f4a7c15ULL * (i + 1)),
        kOperations[which], counts);
    result.insert(result.end(), trace.begin(), trace.end());
  }
  return result;
}
struct Measurement {
  std::vector<uint64_t> result;
  double ns;
};
template <class Stream>
Measurement Measure(size_t which, uint32_t iterations) {
  // Explicit timing mode only. Includes fixture construction/destruction,
  // all public calls, copies, callback/task work and transcript allocation.
  // Fixture costs are amortized across iterations, never subtracted.
  auto start = std::chrono::steady_clock::now();
  auto result = Run<Stream>(which, iterations);
  auto end = std::chrono::steady_clock::now();
  return {std::move(result),
          std::chrono::duration<double, std::nano>(end - start).count() /
              iterations};
}
}  // namespace

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);
  TestTimeouts::Initialize();
  testing::InitGoogleMock(&argc, argv);
  bool timing = argc == 2 && std::string_view(argv[1]) == "--benchmark";
  bool fuzz = argc == 2 && std::string_view(argv[1]) == "--fuzz";
  if (argc > 1 && !timing && !fuzz &&
      (argc != 2 || std::string_view(argv[1]) != "--verify-only")) {
    std::fprintf(stderr,
                 "Use --verify-only (default), --fuzz or --benchmark\n");
    return 2;
  }
  if (fuzz) {
    // 4096 independent lifecycles, each with 256 deterministic randomized
    // public-API actions: 1,048,576 inputs per implementation plus setup.
    net::http3_rust_test::ActionCounts rust_counts{}, cpp_counts{};
    auto rust = Run<Rust>(3, 4096, &rust_counts);
    auto cpp = Run<Original>(3, 4096, &cpp_counts);
    CHECK(rust == cpp);
    CHECK(rust_counts == cpp_counts);
    for (size_t action = 0; action < rust_counts.size(); ++action) {
      std::printf(
          "{\"action\":%zu,\"selections_per_implementation\":%llu,\"timing\":"
          "false}\n",
          action, static_cast<unsigned long long>(rust_counts[action]));
    }
    std::printf(
        "{\"verified\":true,\"timing\":false,\"inputs_per_implementation\":"
        "1048576,\"lifecycles\":4096,\"trace_values\":%zu}\n",
        rust.size());
    return 0;
  }
  uint64_t seed = 20261113;
  for (size_t which = 0; which < std::size(kCases); ++which) {
    CHECK(Run<Rust>(which, 32) == Run<Original>(which, 32));
    if (!timing) {
      // SAFETY: case names are static NUL-terminated literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"verified\":true,\"timing\":false,\"iterations\":"
          "32}\n",
          kCases[which]));
      continue;
    }
    for (size_t pair = 0; pair < 20; ++pair) {
      bool cpp_first = net::http3_rust_test::Next(seed) & 1;
      Measurement cpp, rust;
      if (cpp_first) {
        cpp = Measure<Original>(which, 128);
        rust = Measure<Rust>(which, 128);
      } else {
        rust = Measure<Rust>(which, 128);
        cpp = Measure<Original>(which, 128);
      }
      CHECK(cpp.result == rust.result);
      // SAFETY: case names and boolean strings are static NUL-terminated
      // literals; numeric arguments match the fixed format.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"pair\":%zu,\"cpp_ns\":%.9f,\"rust_ns\":%.9f,"
          "\"iterations\":128,\"cpp_first\":%s}\n",
          kCases[which], pair, cpp.ns, rust.ns, cpp_first ? "true" : "false"));
    }
  }
  return 0;
}
