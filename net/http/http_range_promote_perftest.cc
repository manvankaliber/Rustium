// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "net/http/http_range_promote_reference.h"
#include "net/http/http_util.h"

namespace {
enum class Kind { Ranges, Fetch, Content, InputAlias };
struct Case {
  const char* name;
  std::string input;
  Kind kind;
  bool option = false;
  unsigned aliases = 0;
  size_t iterations = 200000;
};
uint64_t Value(const net::HttpByteRange& range) {
  return static_cast<uint64_t>(range.first_byte_position()) +
         static_cast<uint64_t>(range.last_byte_position()) +
         static_cast<uint64_t>(range.suffix_length());
}
template <bool Rust>
uint64_t Run(const Case& test, size_t iterations) {
  namespace old = net::http_range_promote_internal::test;
  uint64_t sum = 0;
  for (size_t repeat = 0; repeat < iterations; ++repeat) {
    switch (test.kind) {
      case Kind::Ranges: {
        std::vector<net::HttpByteRange> ranges;
        if (test.option) {
          ranges.push_back(net::HttpByteRange::Bounded(12, 34));
        }
        const bool ok =
            Rust ? net::HttpUtil::ParseRangeHeader(test.input, &ranges)
                 : old::ParseRangeHeader(test.input, &ranges);
        sum += ok + ranges.size();
        for (const auto& r : ranges) {
          sum += Value(r);
        }
        break;
      }
      case Kind::Fetch: {
        const auto range =
            Rust ? net::HttpUtil::ParseFetchSingleRange(test.input, test.option)
                 : old::ParseFetchSingleRange(test.input, test.option);
        sum += range.has_value();
        if (range) {
          sum += Value(*range);
        }
        break;
      }
      case Kind::Content: {
        constexpr std::array<std::array<size_t, 3>, 5> layouts = {
            {{0, 1, 2}, {0, 0, 2}, {0, 1, 0}, {0, 1, 1}, {0, 0, 0}}};
        const auto index = layouts[test.aliases];
        std::array<int64_t, 3> values = {7, 8, 9};
        const bool ok = Rust ? net::HttpUtil::ParseContentRangeHeaderFor206(
                                   test.input, &values[index[0]],
                                   &values[index[1]], &values[index[2]])
                             : old::ParseContentRangeHeaderFor206(
                                   test.input, &values[index[0]],
                                   &values[index[1]], &values[index[2]]);
        sum += ok;
        for (int64_t value : values) {
          sum += static_cast<uint64_t>(value);
        }
        break;
      }
      case Kind::InputAlias: {
        std::array<int64_t, 8> values{};
        constexpr std::string_view input = "bytes 49-50/10001";
        // SAFETY: The complete17-byte literal fits in this live64-byte array.
        UNSAFE_BUFFERS(std::memcpy(values.data(), input.data(), input.size()));
        const auto view = UNSAFE_BUFFERS(std::string_view(
            reinterpret_cast<const char*>(values.data()), input.size()));
        const bool ok = Rust ? net::HttpUtil::ParseContentRangeHeaderFor206(
                                   view, &values[2], &values[3], &values[4])
                             : old::ParseContentRangeHeaderFor206(
                                   view, &values[2], &values[3], &values[4]);
        sum += ok;
        for (int64_t value : values) {
          sum += static_cast<uint64_t>(value);
        }
        break;
      }
    }
  }
  return sum;
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
  std::string many = "bytes=";
  for (unsigned i = 0; i < 32; ++i) {
    if (i) {
      many += ',';
    }
    many += std::to_string(i * 100) + "-" + std::to_string(i * 100 + 99);
  }
  const std::string zeros(128, '0');
  const std::vector<Case> cases = {
      {"range_single", "bytes=0-999", Kind::Ranges},
      {"range_open", "bytes=1024-", Kind::Ranges},
      {"range_suffix", "bytes=-4096", Kind::Ranges},
      {"range_multi4", "bytes=0-99,200-299,400-499,600-699", Kind::Ranges,
       false, 0, 100000},
      {"range_multi32", many, Kind::Ranges, false, 0, 10000},
      {"range_empty", "", Kind::Ranges},
      {"range_wrong_unit", "words=1-2", Kind::Ranges},
      {"range_partial_failure", "bytes=1-2,broken", Kind::Ranges},
      {"range_prepop_empty", "bytes=", Kind::Ranges, true},
      {"range_plus_ows", " ByTeS = +01 - +99 , -3 ", Kind::Ranges},
      {"fetch_bound", "bytes=0-1023", Kind::Fetch},
      {"fetch_open", "bytes=1024-", Kind::Fetch},
      {"fetch_suffix", "bytes=-4096", Kind::Fetch},
      {"fetch_space", "bytes\t = 0\t - 1023", Kind::Fetch, true},
      {"fetch_huge", "bytes=000000000000000000001-99999999999999999999999",
       Kind::Fetch},
      {"fetch_empty", "", Kind::Fetch},
      {"fetch_wrong_case", "Bytes=0-1", Kind::Fetch},
      {"fetch_multi", "bytes=0-1,4-5", Kind::Fetch},
      {"fetch_descending", "bytes=10-5", Kind::Fetch},
      {"fetch_trailing", "bytes=0-1 ", Kind::Fetch},
      {"content_small", "bytes 0-1023/4096", Kind::Content},
      {"content_large", "bytes 1000000000-1000001023/9000000000",
       Kind::Content},
      {"content_ws", " \tBytes \t0\t-\t1023\t/\t4096\t", Kind::Content},
      {"content_plus", "bytes +0-+1023/+4096", Kind::Content},
      {"content_long_zeros",
       "bytes " + zeros + "0-" + zeros + "1/" + zeros + "2", Kind::Content,
       false, 0, 10000},
      {"content_empty", "", Kind::Content},
      {"content_wrong_unit", "items 0-1/2", Kind::Content},
      {"content_descending", "bytes 9-1/2", Kind::Content},
      {"content_overflow", "bytes 0-9223372036854775808/9223372036854775809",
       Kind::Content},
      {"content_small_instance", "bytes 0-50/5", Kind::Content},
      {"content_first_last_alias", "bytes 9-1/2", Kind::Content, false, 1},
      {"content_first_length_alias", "bytes 0-1/2", Kind::Content, false, 2},
      {"content_last_length_alias", "bytes 0-1/2", Kind::Content, false, 3},
      {"content_all_alias", "bytes 0-1/2", Kind::Content, false, 4},
      {"content_input_restore", "bytes 49-50/10001", Kind::InputAlias},
  };
  std::mt19937 random(20261112);
  for (const auto& test : cases) {
    const auto b = Run<false>(test, test.iterations / 5),
               n = Run<true>(test, test.iterations / 5);
    if (b != n) {
      UNSAFE_BUFFERS(
          std::fputs("Benchmark implementations disagree.\n", stderr));
      return 1;
    }
    sink = b + n;
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      double baseline, rust;
      const bool rust_first = (random() & 1) != 0;
      if (rust_first) {
        rust = Measure<true>(test);
        baseline = Measure<false>(test);
      } else {
        baseline = Measure<false>(test);
        rust = Measure<true>(test);
      }
      // SAFETY: Format, case names and boolean strings are static literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.6f,\"rust_ns\":%."
          "6f,\"iterations\":%zu,\"input_bytes\":%zu,\"kind\":%d,\"option\":%s,"
          "\"aliases\":%u,\"rust_first\":%s}\n",
          test.name, repeat, baseline, rust, test.iterations, test.input.size(),
          static_cast<int>(test.kind), test.option ? "true" : "false",
          test.aliases, rust_first ? "true" : "false"));
      std::fflush(stdout);
    }
  }
}
