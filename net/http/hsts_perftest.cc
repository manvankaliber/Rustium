// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Paired C++ API benchmark: construction of caller outputs + complete HSTS
// parse + returned policy consumption. Inputs are preloaded, warm memory.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "net/http/hsts_test_support.h"
#include "net/http/http_security_headers.h"

namespace {
struct Case {
  const char* name;
  std::string input;
  size_t iterations;
};

volatile uint64_t sink = 0;

template <auto Parse>
uint64_t Run(const Case& test, size_t iterations) {
  uint64_t sum = 0;
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    auto age = base::Seconds(777);
    bool subdomains = false;
    sum += Parse(test.input, &age, &subdomains);
    sum += age.InSeconds() + subdomains;
  }
  return sum;
}

template <auto Parse>
double Measure(const Case& test) {
  const auto start = std::chrono::steady_clock::now();
  sink = Run<Parse>(test, test.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         static_cast<double>(test.iterations);
}
}  // namespace

int main() {
  constexpr auto baseline = net::hsts_internal::test::LegacyParseHSTSHeader;
  constexpr auto rust = net::ParseHSTSHeader;
  std::string many = "max-age=31536000";
  for (size_t i = 0; i < 32; ++i) {
    many += "; x=token";
  }
  const std::vector<Case> cases = {
      {"max_age", "max-age=31536000", 200000},
      {"subdomains", "max-age=31536000; includeSubDomains", 200000},
      {"quoted_age", "max-age=\"31536000\"; includeSubDomains", 200000},
      {"whitespace", " \t; Max-AgE \t=\t 31536000 \t; includeSubDomains \t;;",
       200000},
      {"unknown", "x=token; max-age=31536000; includeSubDomains; y=\"a;b\"",
       100000},
      {"empty", "", 500000},
      {"missing_age", "includeSubDomains", 500000},
      {"duplicate", "max-age=1;max-age=2", 200000},
      {"bad_digit", "max-age=31536000x;includeSubDomains", 200000},
      {"bad_quote", "max-age=\"31536000\\\"", 200000},
      {"overflow", "max-age=99999999999999999999999999999999999999", 100000},
      {"overflow_bad", "max-age=99999999999999999999999999999999999999x",
       100000},
      {"escaped_age", "max-age=\"\\3\\1\\5\\3\\6\\0\\0\\0\";includeSubDomains",
       100000},
      {"long_quoted", "max-age=1;x=\"" + std::string(2048, 'x') + "\"", 10000},
      {"many_directives", many, 20000},
  };
  std::mt19937 random(20261112);
  for (const auto& test : cases) {
    const auto a = Run<baseline>(test, test.iterations / 5);
    const auto b = Run<rust>(test, test.iterations / 5);
    if (a != b) {
      // SAFETY: This diagnostic is a static null-terminated literal.
      UNSAFE_BUFFERS(std::fputs("HSTS benchmark readers disagree.\n", stderr));
      return 1;
    }
    sink = a + b;
    for (unsigned repetition = 0; repetition < 20; ++repetition) {
      double cpp_ns;
      double rust_ns;
      const bool rust_first = (random() & 1) != 0;
      if (rust_first) {
        rust_ns = Measure<rust>(test);
        cpp_ns = Measure<baseline>(test);
      } else {
        cpp_ns = Measure<baseline>(test);
        rust_ns = Measure<rust>(test);
      }
      // SAFETY: The format, case name, and boolean string are null-terminated
      // literals. All remaining arguments are primitive numbers.
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"input_bytes\":%zu,\"iterations\":%zu,"
                      "\"repeat\":%u,\"rust_first\":%s,\"baseline_ns\":%.6f,"
                      "\"rust_ns\":%.6f}\n",
                      test.name, test.input.size(), test.iterations, repetition,
                      rust_first ? "true" : "false", cpp_ns, rust_ns));
      std::fflush(stdout);
    }
  }
}
