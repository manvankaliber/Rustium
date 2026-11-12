// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Parent-only quiet-window timing. This file prepares measurements; it does
// not execute during the implementation agent's validation.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <string_view>

#include "net/http/auth_challenge_test_support.h"
namespace {
struct Case {
  const char* key;
  std::string_view input;
  size_t iterations;
};
volatile uint64_t sink = 0;
uint64_t Consume(std::string_view s) {
  uint64_t n = s.size();
  for (unsigned char c : s) {
    n = n * 33 + c;
  }
  return n;
}
template <bool Rust>
uint64_t Run(const Case& c, size_t n) {
  uint64_t result = 0;
  for (size_t i = 0; i < n; ++i) {
    std::string encoded = "old encoded", decoded = "old decoded";
    if constexpr (Rust) {
      net::HttpAuthChallengeTokenizer t(c.input);
      result += Consume(t.auth_scheme()) + Consume(t.params()) +
                Consume(t.base64_param());
      auto pairs = t.param_pairs();
      while (pairs.GetNext()) {
        result += Consume(pairs.name()) + Consume(pairs.value());
      }
      result += pairs.valid();
      result += net::ParseFirstRoundChallenge(
          net::HttpAuth::AUTH_SCHEME_NEGOTIATE, &t);
      result += net::ParseLaterRoundChallenge(
          net::HttpAuth::AUTH_SCHEME_NEGOTIATE, &t, &encoded, &decoded);
    } else {
      net::auth_challenge_reference::Tokenizer t(c.input);
      result += Consume(t.auth_scheme()) + Consume(t.params()) +
                Consume(t.base64_param());
      auto pairs = t.param_pairs();
      while (pairs.GetNext()) {
        result += Consume(pairs.name()) + Consume(pairs.value());
      }
      result += pairs.valid();
      result += net::auth_challenge_reference::First(
          net::HttpAuth::AUTH_SCHEME_NEGOTIATE, &t);
      result += net::auth_challenge_reference::Later(
          net::HttpAuth::AUTH_SCHEME_NEGOTIATE, &t, &encoded, &decoded);
    }
    result += Consume(encoded) + Consume(decoded);
  }
  return result;
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
int main() {
  const Case cases[] = {
      {"auth_empty", "Negotiate", 100000},
      {"auth_valid", "Negotiate Zm9vYmFy", 100000},
      {"auth_padding", "Negotiate SGVsbG8sIFdvcmxkCg===", 100000},
      {"auth_invalid", "Negotiate Zm9v***=", 100000},
      {"auth_mismatch", "NTLM Zm9vYmFy", 100000},
      {"auth_params", "Negotiate realm=foo, qop=auth", 100000}};
  std::mt19937 random(20261112);
  for (const auto& c : cases) {
    net::auth_challenge_test::Require(Run<false>(c, 4096) ==
                                      Run<true>(c, 4096));
    for (unsigned i = 0; i < 20; ++i) {
      bool first = (random() & 1) != 0;
      double cpp, rust;
      if (first) {
        rust = Measure<true>(c);
        cpp = Measure<false>(c);
      } else {
        cpp = Measure<false>(c);
        rust = Measure<true>(c);
      }
      std::printf(
          "{\"case\":\"%s\",\"input_bytes\":%zu,\"iterations\":%zu,\"repeat\":%"
          "u,\"rust_first\":%s,\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          c.key, c.input.size(), c.iterations, i, first ? "true" : "false", cpp,
          rust);
      std::fflush(stdout);
    }
  }
}
