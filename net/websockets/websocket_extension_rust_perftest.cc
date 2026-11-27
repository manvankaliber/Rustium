// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Prepared for parent-owned coordinated quiet-window execution only.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>
#include <type_traits>

#include "base/compiler_specific.h"
#include "net/websockets/websocket_extension_rust_test_support.h"
namespace {
struct Case {
  const char* key;
  std::string_view input;
  size_t operation;  // 0 parse, 1 serialization, 2 equivalence, 3 equality
  size_t iterations;
};
volatile size_t sink = 0;
template <bool Rust>
size_t Run(const Case& c, size_t count) {
  using E = std::conditional_t<
      Rust, net::WebSocketExtension,
      net::websocket_extension_rust_reference::WebSocketExtension>;
  auto parse = [](std::string_view input) {
    if constexpr (Rust) {
      return net::ParseWebSocketExtensions(input);
    } else {
      return net::websocket_extension_rust_reference::ParseWebSocketExtensions(
          input);
    }
  };
  // Fixed fixtures are constructed once per sample inside the timer and
  // amortized over all iterations. Parse
  // timings include construction, FFI, allocation, getters and destruction.
  auto fixtures = parse(c.input);
  E other("x");
  other.Add(typename E::Parameter("b", "2"));
  other.Add(typename E::Parameter("a", "1"));
  other.Add(typename E::Parameter("a", "3"));
  size_t total = 0;
  for (size_t i = 0; i < count; ++i) {
    if (c.operation == 0) {
      auto result = parse(c.input);
      total += result.size();
      for (const auto& e : result) {
        total += e.name().size() + e.parameters().size();
        for (const auto& p : e.parameters()) {
          total += p.name().size() + p.value().size();
        }
      }
    } else if (c.operation == 1) {
      auto text = fixtures.front().ToString();
      total += text.size();
      if (!text.empty()) {
        total += static_cast<unsigned char>(text.back());
      }
    } else if (c.operation == 2) {
      total += fixtures.front().Equivalent(other);
    } else {
      total +=
          fixtures.front().parameters().front() == other.parameters().front();
    }
  }
  return total;
}
template <bool Rust>
double Measure(const Case& c) {
  const auto start = std::chrono::steady_clock::now();
  sink = Run<Rust>(c, c.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         c.iterations;
}
}  // namespace
int main(int argc, char**) {
  const Case cases[] = {
      {"ws-simple", "permessage-deflate", 0, 100000},
      {"ws-parameters",
       "permessage-deflate; client_max_window_bits=15; "
       "server_no_context_takeover",
       0, 100000},
      {"ws-escaped", "a; p=\"one\\t\\wo\"; p=three, b; q=\"f\\our\"", 0,
       100000},
      {"ws-invalid-tail", "a; p=one; q=two, b; p=\"bad\\", 0, 100000},
      {"ws-invalid-byte", "a; p=\"bad\x7f\"", 0, 100000},
      {"ws-serialize", "x; a=1; b=2; a=3", 1, 100000},
      {"ws-serialize-long",
       "permessage-deflate; client_max_window_bits=15; "
       "server_no_context_takeover; client_no_context_takeover",
       1, 100000},
      {"ws-equivalent", "x; a=1; b=2; a=3", 2, 100000},
      {"ws-value-order", "x; a=3; b=2; a=1", 2, 100000},
      {"ws-equality", "x; b=2", 3, 100000},
  };
  std::mt19937 random(20261112);
  for (const auto& c : cases) {
    net::websocket_extension_rust_test::Check(c.input);
    net::websocket_extension_rust_test::Require(Run<false>(c, 4096) ==
                                                Run<true>(c, 4096));
    // Any argument requests fixture validation only, without reading a clock.
    if (argc > 1) {
      continue;
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool first = (random() & 1) != 0;
      double cpp, rust;
      if (first) {
        rust = Measure<true>(c);
        cpp = Measure<false>(c);
      } else {
        cpp = Measure<false>(c);
        rust = Measure<true>(c);
      }
      // SAFETY: key is a fixed NUL-terminated literal above.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"input_bytes\":%zu,\"iterations\":%zu,\"repeat\":%"
          "u,\"rust_first\":%s,\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          c.key, c.input.size(), c.iterations, repeat, first ? "true" : "false",
          cpp, rust));
      std::fflush(stdout);
    }
  }
}
