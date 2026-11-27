// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>

#include "base/compiler_specific.h"
#include "net/http/http_byte_range_rust_test_support.h"
namespace {
using Old = net::http_byte_range_rust_test::Original;
struct Case {
  const char* name;
  unsigned operation;
  int64_t first, last, suffix, size;
};
constexpr auto cases = std::to_array<Case>({
    {"construct_default", 0, -1, -1, -1, 0},
    {"construct_bounded", 1, 17, 300, -1, 1000},
    {"construct_open", 2, 17, -1, -1, 1000},
    {"construct_suffix", 3, -1, -1, 100, 1000},
    {"predicates_empty", 4, -1, -1, -1, 0},
    {"predicates_bounded", 4, 17, 300, -1, 1000},
    {"predicates_contradictory", 4, 10, 0, 100, 1000},
    {"predicates_negative_suffix", 4, 0, 1, -2, 0},
    {"bounds_empty", 5, -1, -1, -1, 1000},
    {"bounds_zero", 5, -1, -1, -1, 0},
    {"bounds_bounded", 5, 17, 300, -1, 1000},
    {"bounds_open", 5, 17, -1, -1, 1000},
    {"bounds_suffix", 5, -1, -1, 100, 1000},
    {"bounds_suffix_larger", 5, -1, -1, 2000, 1000},
    {"bounds_invalid", 5, 10, 0, -1, 1000},
    {"bounds_unsatisfiable", 5, 1000, -1, -1, 1000},
    {"bounds_negative_size", 5, 17, 300, -1, -1},
    {"bounds_negative_suffix", 5, 0, 1, -2, 0},
    {"bounds_max", 5, INT64_MAX - 1, INT64_MAX, -1, INT64_MAX},
    {"bounds_repeat_after_setters", 6, 10, 0, -1, 1000},
    {"header_bounded", 7, 17, 300, -1, 0},
    {"header_open", 7, 17, -1, -1, 0},
    {"header_suffix", 7, -1, -1, 100, 0},
    {"header_max", 7, INT64_MAX - 1, INT64_MAX, -1, 0},
    {"header_min_suffix", 7, 0, 0, INT64_MIN, 0},
});
template <class R>
R Construct(const Case& c) {
  switch (c.operation) {
    case 1:
      return R::Bounded(c.first, c.last);
    case 2:
      return R::RightUnbounded(c.first);
    case 3:
      return R::Suffix(c.suffix);
    default:
      return R();
  }
}
template <class R>
uint64_t Run(const Case& c, size_t n) {
  uint64_t sum = 0;
  for (size_t i = 0; i < n; ++i) {
    auto r = Construct<R>(c);
    if (c.operation >= 4) {
      r.set_first_byte_position(c.first);
      r.set_last_byte_position(c.last);
      r.set_suffix_length(c.suffix);
      switch (c.operation) {
        case 4:
          sum += r.IsSuffixByteRange() + 2 * r.HasFirstBytePosition() +
                 4 * r.HasLastBytePosition() + 8 * r.IsValid();
          break;
        case 5:
          sum += r.ComputeBounds(c.size);
          break;
        case 6:
          sum += r.ComputeBounds(c.size);
          r.set_first_byte_position(0);
          r.set_last_byte_position(-1);
          r.set_suffix_length(-1);
          sum += r.ComputeBounds(c.size);
          sum += r.ComputeBounds(0);
          break;
        case 7: {
          const auto header = r.GetHeaderValue();
          sum += header.size();
          for (unsigned char ch : header) {
            sum += ch;
          }
          break;
        }
      }
    }
    sum += static_cast<uint64_t>(r.first_byte_position()) +
           static_cast<uint64_t>(r.last_byte_position()) +
           static_cast<uint64_t>(r.suffix_length());
  }
  return sum;
}
volatile uint64_t sink = 0;
template <class R>
double Measure(const Case& c) {
  constexpr size_t iterations = 200000;
  const auto start = std::chrono::steady_clock::now();
  const auto value = Run<R>(c, iterations);
  const auto end = std::chrono::steady_clock::now();
  sink = value;
  return std::chrono::duration<double, std::nano>(end - start).count() /
         iterations;
}
}  // namespace
int main(int argc, char** argv) {
  for (const auto& c : cases) {
    CHECK_EQ(Run<Old>(c, 32), Run<net::HttpByteRange>(c, 32));
  }
  if (argc == 2 &&
      UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only") {
    std::printf("Verified all %zu frozen byte-range workloads; no timing.\n",
                std::size(cases));
    return 0;
  }
  // Parent coordinates the quiet window. A fixed seed randomizes both case
  // order and which implementation runs first for every one of 20 pairs.
  std::mt19937 rng(0x17b9e3);
  std::array<size_t, std::size(cases)> order;
  for (size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
  }
  for (unsigned pair = 0; pair < 20; ++pair) {
    std::shuffle(order.begin(), order.end(), rng);
    for (auto i : order) {
      const bool rust_first = (rng() & 1) != 0;
      double cpp, rust;
      if (rust_first) {
        rust = Measure<net::HttpByteRange>(cases[i]);
        cpp = Measure<Old>(cases[i]);
      } else {
        cpp = Measure<Old>(cases[i]);
        rust = Measure<net::HttpByteRange>(cases[i]);
      }
      // SAFETY: case names and boolean strings are static NUL-terminated
      // literals; numeric arguments match the fixed format.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"pair\":%u,\"cpp_ns\":%.9f,\"rust_ns\":%.9f,"
          "\"rust_first\":%s,\"iterations\":200000}\n",
          cases[i].name, pair, cpp, rust, rust_first ? "true" : "false"));
    }
  }
}
