// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Parent-only timing: same process, 20 deterministic randomized-order pairs.
// Output construction, FFI, copies, consumption and destruction are included.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "net/base/mime_type_rust_test_support.h"

namespace {
struct Case {
  const char* key;
  std::string input;
  unsigned
      api;  // 0=parameters, 1=without parameters, 2=content, 3=alias boundary.
  size_t iterations;
};
volatile uint64_t sink = 0;
uint64_t Consume(std::string_view s) {
  uint64_t result = s.size();
  for (unsigned char c : s) {
    result = result * 33 + c;
  }
  return result;
}
template <bool Rust>
uint64_t Run(const Case& test, size_t iterations) {
  uint64_t total = 0;
  for (size_t i = 0; i < iterations; ++i) {
    std::string mime = "Text/Html", charset = "OLD", boundary = "keep";
    bool had = true;
    if (test.api == 0) {
      base::StringPairs params;
      total += (Rust ? net::ParseMimeType
                     : net::mime_type_test::LegacyParseMimeType)(
          test.input, &mime, &params);
      for (const auto& param : params) {
        total += Consume(param.first) + Consume(param.second);
      }
    } else if (test.api == 1) {
      total += (Rust ? net::ParseMimeTypeWithoutParameter
                     : net::mime_type_test::LegacyWithoutParameter)(
          test.input, &mime, &charset);
    } else {
      (Rust ? net::HttpUtil::ParseContentType
            : net::mime_type_test::LegacyContentType)(
          test.input, &mime, &charset, &had, test.api == 3 ? &mime : &boundary);
      total += had;
    }
    total += Consume(mime) + Consume(charset) + Consume(boundary);
  }
  return total;
}
template <bool Rust>
double Measure(const Case& test) {
  const auto start = std::chrono::steady_clock::now();
  sink = Run<Rust>(test, test.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         test.iterations;
}
}  // namespace

int main() {
  std::string many = "a/b";
  for (unsigned i = 0; i < 32; ++i) {
    many += "; p=\"a;b\\c\"";
  }
  const std::vector<Case> cases = {
      {"parse_simple", "application/json", 0, 100000},
      {"parse_quoted", "text/html; name=\"a;b\\\"c\"; charset=UTF-8", 0,
       100000},
      {"parse_malformed_quote", "a/b; x=\"unterminated\\", 0, 100000},
      {"parse_many", many, 0, 10000},
      {"parse_bytes", std::string("a/\xff; x=\"\0\x80\"", 12), 0, 100000},
      {"strict_simple", "application/json", 1, 100000},
      {"strict_whitespace", "\v\r\n TEXT/Plain\t\f", 1, 100000},
      {"strict_invalid", "text /plain", 1, 100000},
      {"strict_nonascii", "a/\xff", 1, 100000},
      {"strict_multislash", "a/b/c", 1, 100000},
      {"content_simple", "application/json", 2, 100000},
      {"content_charset", "text/html; charset=UTF-8", 2, 100000},
      {"content_boundary", "multipart/form-data; boundary=\" b \\x \"", 2,
       100000},
      {"content_duplicate",
       "text/html; charset=first; charset=last; boundary=one; boundary=two", 2,
       50000},
      {"content_empty_charset", "text/html; charset=\"\"", 2, 100000},
      {"content_exact_wildcard", "*/*", 2, 100000},
      {"content_malformed", "invalid; charset=UTF-8", 2, 100000},
      {"content_same_type", "TEXT/HTML", 2, 100000},
      {"content_changed_type", "TEXT/PLAIN", 2, 100000},
      {"content_alias_boundary", "text/html; boundary=\"TEXT/HTML\"", 3,
       100000},
  };
  std::mt19937 random(20261112);
  for (const auto& test : cases) {
    const auto a = Run<false>(test, test.iterations / 5);
    const auto b = Run<true>(test, test.iterations / 5);
    net::mime_type_test::Require(a == b);
    sink = a + b;
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double cpp_ns, rust_ns;
      if (rust_first) {
        rust_ns = Measure<true>(test);
        cpp_ns = Measure<false>(test);
      } else {
        cpp_ns = Measure<false>(test);
        rust_ns = Measure<true>(test);
      }
      // SAFETY: format, key and boolean text are null-terminated static
      // strings.
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"input_bytes\":%zu,\"iterations\":%zu,"
                      "\"repeat\":%u,\"rust_first\":%s,\"baseline_ns\":%.6f,"
                      "\"rust_ns\":%.6f}\n",
                      test.key, test.input.size(), test.iterations, repeat,
                      rust_first ? "true" : "false", cpp_ns, rust_ns));
      std::fflush(stdout);
    }
  }
}
