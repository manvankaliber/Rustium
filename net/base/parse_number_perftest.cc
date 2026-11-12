// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/parse_number.h"

#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <type_traits>
#include <vector>

#include "base/compiler_specific.h"
#include "net/base/parse_number_test_support.h"

namespace {
enum class Kind { I32, I64, U32, U64 };
struct Case {
  const char* name;
  std::string input;
  Kind kind;
  net::ParseIntFormat format;
  bool report_error = true;
  size_t iterations = 500000;
};

template <typename T, bool Rust>
bool Parse(std::string_view input,
           net::ParseIntFormat format,
           T* output,
           net::ParseIntError* error) {
  namespace old = net::parse_number_internal::test;
  if constexpr (std::is_same_v<T, int32_t>) {
    if constexpr (Rust) {
      return net::ParseInt32(input, format, output, error);
    } else {
      return old::ParseInt32(input, format, output, error);
    }
  } else if constexpr (std::is_same_v<T, int64_t>) {
    if constexpr (Rust) {
      return net::ParseInt64(input, format, output, error);
    } else {
      return old::ParseInt64(input, format, output, error);
    }
  } else if constexpr (std::is_same_v<T, uint32_t>) {
    if constexpr (Rust) {
      return net::ParseUint32(input, format, output, error);
    } else {
      return old::ParseUint32(input, format, output, error);
    }
  } else {
    if constexpr (Rust) {
      return net::ParseUint64(input, format, output, error);
    } else {
      return old::ParseUint64(input, format, output, error);
    }
  }
}

template <typename T, bool Rust>
uint64_t RunTyped(const Case& test, size_t iterations) {
  uint64_t sum = 0;
  for (size_t repeat = 0; repeat < iterations; ++repeat) {
    T value = 23614;
    auto error = static_cast<net::ParseIntError>(19);
    const bool ok = Parse<T, Rust>(test.input, test.format, &value,
                                   test.report_error ? &error : nullptr);
    sum += static_cast<uint64_t>(value) + static_cast<unsigned>(error) + ok;
  }
  return sum;
}

template <bool Rust>
uint64_t Run(const Case& test, size_t iterations) {
  switch (test.kind) {
    case Kind::I32:
      return RunTyped<int32_t, Rust>(test, iterations);
    case Kind::I64:
      return RunTyped<int64_t, Rust>(test, iterations);
    case Kind::U32:
      return RunTyped<uint32_t, Rust>(test, iterations);
    case Kind::U64:
      return RunTyped<uint64_t, Rust>(test, iterations);
  }
}

volatile uint64_t sink = 0;
template <bool Rust>
double Measure(const Case& test) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = Run<Rust>(test, test.iterations);
  const auto end = std::chrono::steady_clock::now();
  sink = result;
  return std::chrono::duration<double, std::nano>(end - start).count() /
         test.iterations;
}
}  // namespace

int main() {
  using F = net::ParseIntFormat;
  const std::vector<Case> cases = {
      {"i32_short", "123", Kind::I32, F::NON_NEGATIVE},
      {"i32_negative", "-123", Kind::I32, F::OPTIONALLY_NEGATIVE},
      {"i32_strict", "123456", Kind::I32, F::STRICT_NON_NEGATIVE},
      {"i32_max", "2147483647", Kind::I32, F::NON_NEGATIVE},
      {"i32_min", "-2147483648", Kind::I32, F::STRICT_OPTIONALLY_NEGATIVE},
      {"i64_short", "123", Kind::I64, F::NON_NEGATIVE},
      {"i64_19digits", "1234567890123456789", Kind::I64, F::NON_NEGATIVE},
      {"i64_negative", "-1234567890123", Kind::I64, F::OPTIONALLY_NEGATIVE},
      {"i64_max", "9223372036854775807", Kind::I64, F::NON_NEGATIVE},
      {"i64_min", "-9223372036854775808", Kind::I64,
       F::STRICT_OPTIONALLY_NEGATIVE},
      {"u32_short", "123", Kind::U32, F::NON_NEGATIVE},
      {"u32_strict", "123456", Kind::U32, F::STRICT_NON_NEGATIVE},
      {"u32_max", "4294967295", Kind::U32, F::NON_NEGATIVE},
      {"u64_short", "123", Kind::U64, F::NON_NEGATIVE},
      {"u64_max", "18446744073709551615", Kind::U64, F::NON_NEGATIVE},
      {"empty", "", Kind::I32, F::NON_NEGATIVE},
      {"plus", "+123", Kind::I32, F::NON_NEGATIVE},
      {"leading_ws", " 123", Kind::I64, F::NON_NEGATIVE},
      {"strict_zero", "0123", Kind::U32, F::STRICT_NON_NEGATIVE},
      {"embedded_nul", std::string("123\0", 4), Kind::U64, F::NON_NEGATIVE},
      {"i32_overflow", "2147483648", Kind::I32, F::NON_NEGATIVE},
      {"i32_underflow", "-2147483649", Kind::I32, F::OPTIONALLY_NEGATIVE},
      {"u32_overflow", "4294967296", Kind::U32, F::NON_NEGATIVE},
      {"i64_overflow", "9223372036854775808", Kind::I64, F::NON_NEGATIVE},
      {"u64_overflow", "18446744073709551616", Kind::U64, F::NON_NEGATIVE},
      {"overflow_junk", "999999999999999999999x", Kind::U64, F::NON_NEGATIVE},
      {"negative_junk", "-999999999999999999999x", Kind::I64,
       F::OPTIONALLY_NEGATIVE},
      {"long_leading_zeros", std::string(256, '0') + "123", Kind::U32,
       F::NON_NEGATIVE, true, 10000},
      {"long_overflow_error", std::string(4096, '9'), Kind::U64,
       F::NON_NEGATIVE, true, 10000},
      {"long_overflow_no_error", std::string(4096, '9'), Kind::U64,
       F::NON_NEGATIVE, false},
      {"i32_short_no_error", "123", Kind::I32, F::NON_NEGATIVE, false},
      {"i64_short_no_error", "123", Kind::I64, F::NON_NEGATIVE, false},
      {"u32_short_no_error", "123", Kind::U32, F::NON_NEGATIVE, false},
      {"u64_short_no_error", "123", Kind::U64, F::NON_NEGATIVE, false},
      {"invalid_no_error", "123x", Kind::U32, F::NON_NEGATIVE, false},
  };
  std::mt19937 random(20261112);
  for (const auto& test : cases) {
    const auto baseline = Run<false>(test, test.iterations / 5);
    const auto rust = Run<true>(test, test.iterations / 5);
    if (baseline != rust) {
      // SAFETY: The diagnostic is a static null-terminated literal.
      UNSAFE_BUFFERS(
          std::fputs("Benchmark implementations disagree.\n", stderr));
      return 1;
    }
    sink = baseline + rust;
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      double b, n;
      const bool rust_first = (random() & 1) != 0;
      if (rust_first) {
        n = Measure<true>(test);
        b = Measure<false>(test);
      } else {
        b = Measure<false>(test);
        n = Measure<true>(test);
      }
      // SAFETY: Format, case names and boolean strings are static literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.6f,\"rust_ns\":%."
          "6f,\"iterations\":%zu,\"input_bytes\":%zu,\"format\":%d,\"kind\":%d,"
          "\"report_error\":%s,\"rust_first\":%s}\n",
          test.name, repeat, b, n, test.iterations, test.input.size(),
          static_cast<int>(test.format), static_cast<int>(test.kind),
          test.report_error ? "true" : "false", rust_first ? "true" : "false"));
      std::fflush(stdout);
    }
  }
}
