// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Full public calls, representation policy, FFI, Value/dictionary construction,
// all output consumption and destruction are timed. Input setup, warmup, JSON
// reporting and randomized pair ordering are outside the timing boundary.
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <random>
#include <ratio>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "net/log/net_log_values.h"
#include "net/log/net_log_values_rust_reference.h"

namespace {
enum class Api {
  kSigned,
  kUnsigned,
  kUint32,
  kString,
  kBinaryPointer,
  kBinarySpan,
  kParamInt,
  kParamInt64,
  kParamBool,
  kParamString
};
struct Case {
  const char* key;
  Api api;
  std::string input;
  uint64_t number = 0;
};
volatile uint64_t sink = 0;

uint64_t Consume(const base::Value& value) {
  uint64_t sum = static_cast<uint64_t>(value.type());
  if (value.is_string()) {
    for (unsigned char byte : value.GetString()) {
      sum += byte;
    }
    return sum + value.GetString().size();
  }
  if (value.is_int()) {
    return sum + static_cast<uint64_t>(value.GetInt());
  }
  if (value.is_double()) {
    return sum + std::bit_cast<uint64_t>(value.GetDouble());
  }
  if (value.is_bool()) {
    return sum + value.GetBool();
  }
  for (const auto item : value.GetDict()) {
    for (unsigned char byte : item.first) {
      sum += byte;
    }
    sum += Consume(item.second);
  }
  return sum;
}

// Keep both public implementations behind the same out-of-loop selector. In
// configured builds Rust calls use net.dll's actual exported API. The frozen
// original reference is linked separately; native scratch EXEs have local
// production adapters and are correctness validation only.
template <bool kRust>
base::Value Invoke(const Case& c) {
  const auto signed_value = std::bit_cast<int64_t>(c.number);
  constexpr std::string_view key = "net_log_value";
#define CALL(name, ...)           \
  (kRust ? net::name(__VA_ARGS__) \
         : net::net_log_values_rust_reference::name(__VA_ARGS__))
  switch (c.api) {
    case Api::kSigned:
      return CALL(NetLogNumberValue, signed_value);
    case Api::kUnsigned:
      return CALL(NetLogNumberValue, c.number);
    case Api::kUint32:
      return CALL(NetLogNumberValue, static_cast<uint32_t>(c.number));
    case Api::kString:
      return CALL(NetLogStringValue, c.input);
    case Api::kBinaryPointer:
      return CALL(NetLogBinaryValue, c.input.empty() ? nullptr : c.input.data(),
                  c.input.size());
    case Api::kBinarySpan:
      return CALL(NetLogBinaryValue, base::as_byte_span(c.input));
    case Api::kParamInt:
      return base::Value(
          CALL(NetLogParamsWithInt, key, static_cast<int32_t>(c.number)));
    case Api::kParamInt64:
      return base::Value(CALL(NetLogParamsWithInt64, key, signed_value));
    case Api::kParamBool:
      return base::Value(CALL(NetLogParamsWithBool, key, c.number != 0));
    case Api::kParamString:
      return base::Value(CALL(NetLogParamsWithString, key, c.input));
  }
#undef CALL
  std::abort();
}

template <bool kRust>
double Measure(const Case& c, int iterations) {
  uint64_t sum = 0;
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i) {
    sum += Consume(Invoke<kRust>(c));
  }
  sink = sum;
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         iterations;
}
}  // namespace

int main(int argc, char** argv) {
  const bool verify_only =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  if (argc != 1 && !verify_only) {
    // SAFETY: the diagnostic is a static NUL-terminated literal.
    UNSAFE_BUFFERS(
        std::fputs("Use --verify-only or no arguments for timing\n", stderr));
    return 2;
  }
  const Case cases[] = {
      {"signed-int", Api::kSigned, {}, uint64_t(-2147483648LL)},
      {"signed-double", Api::kSigned, {}, uint64_t(-9007199254740991LL)},
      {"signed-decimal",
       Api::kSigned,
       {},
       uint64_t(std::numeric_limits<int64_t>::min())},
      {"unsigned-int", Api::kUnsigned, {}, 2147483647},
      {"unsigned-double", Api::kUnsigned, {}, 9007199254740991},
      {"unsigned-decimal",
       Api::kUnsigned,
       {},
       std::numeric_limits<uint64_t>::max()},
      {"uint32-int", Api::kUint32, {}, 42},
      {"uint32-double", Api::kUint32, {}, std::numeric_limits<uint32_t>::max()},
      {"string-empty", Api::kString, {}},
      {"string-ascii", Api::kString,
       "ASCII % diagnostic string with ordinary spaces"},
      {"string-escaped", Api::kString,
       std::string("UTF-8 \xE2\x98\x83 100%\0", 15)},
      {"string-escaped-4k", Api::kString, std::string(4096, '\xFF')},
      {"binary-empty-pointer", Api::kBinaryPointer, {}},
      {"binary-padded-pointer", Api::kBinaryPointer,
       std::string("\0\xF3\xF8\xFF", 4)},
      {"binary-span-1k", Api::kBinarySpan, std::string(1024, '\xA5')},
      {"params-int", Api::kParamInt, {}, 42},
      {"params-int64",
       Api::kParamInt64,
       {},
       uint64_t(std::numeric_limits<int64_t>::min())},
      {"params-bool", Api::kParamBool, {}, 1},
      {"params-string", Api::kParamString, "UTF-8 \xE2\x98\x83 100%"}};
  std::mt19937 random(2026111302);
  for (const auto& c : cases) {
    CHECK(Invoke<true>(c) == Invoke<false>(c));
    if (verify_only) {
      continue;
    }
    Measure<false>(c, 256);
    Measure<true>(c, 256);
    for (int repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double baseline, rust;
      if (rust_first) {
        rust = Measure<true>(c, 20000);
        baseline = Measure<false>(c, 20000);
      } else {
        baseline = Measure<false>(c, 20000);
        rust = Measure<true>(c, 20000);
      }
      // SAFETY: keys are NUL-terminated string literals defined above.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%d,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"rust_first\":%s}\n",
          c.key, repeat, baseline, rust, rust_first ? "true" : "false"));
    }
  }
  if (verify_only) {
    std::printf("Validated all %zu paired workloads without timing\n",
                std::size(cases));
  }
}
