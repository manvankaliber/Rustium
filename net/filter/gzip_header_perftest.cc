// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Paired production-API benchmark. Output is JSON Lines; use the accompanying
// report script for medians and paired bootstrap confidence intervals.
#include "net/filter/gzip_header.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "net/filter/gzip_header_test_support.h"

namespace {
using Legacy = net::gzip_header_internal::test::LegacyHeader;
static_assert(sizeof(net::GZipHeader) == sizeof(Legacy));
static_assert(alignof(net::GZipHeader) == alignof(Legacy));

struct Case {
  const char* name;
  std::vector<uint8_t> input;
  size_t chunk;
  size_t iterations;
};

std::vector<uint8_t> Header(uint8_t flags = 0, size_t length = 16) {
  std::vector<uint8_t> bytes = {0x1f, 0x8b, 8, flags, 0, 0, 0, 0, 0, 255};
  if (flags & 4) {
    bytes.push_back(static_cast<uint8_t>(length));
    bytes.push_back(static_cast<uint8_t>(length >> 8));
    bytes.insert(bytes.end(), length, 'x');
  }
  for (unsigned bit : {8u, 16u}) {
    if (flags & bit) {
      bytes.insert(bytes.end(), length, 'n');
      bytes.push_back(0);
    }
  }
  if (flags & 2) {
    bytes.insert(bytes.end(), {0, 0});
  }
  return bytes;
}

template <typename Parser>
uint64_t Run(const Case& test, size_t iterations) {
  uint64_t sum = 0;
  for (size_t repeat = 0; repeat < iterations; ++repeat) {
    if (test.chunk == 0) {
      sum += Parser::HasGZipHeader(test.input);
    } else {
      Parser parser;
      size_t end = 0;
      for (size_t pos = 0; pos < test.input.size();) {
        const size_t count = std::min(test.chunk, test.input.size() - pos);
        const auto status =
            parser.ReadMore(base::span(test.input).subspan(pos, count), end);
        sum += static_cast<unsigned>(status) + end;
        pos += count;
        if (status != Parser::INCOMPLETE_HEADER) {
          break;
        }
      }
    }
  }
  return sum;
}

volatile uint64_t sink = 0;

template <typename Parser>
double Measure(const Case& test) {
  const auto start = std::chrono::steady_clock::now();
  const auto checksum = Run<Parser>(test, test.iterations);
  const auto finish = std::chrono::steady_clock::now();
  sink = checksum;
  return std::chrono::duration<double, std::nano>(finish - start).count() /
         static_cast<double>(test.iterations);
}
}  // namespace

int main() {
  auto payload = Header();
  payload.resize(65536, 'p');
  std::vector<Case> cases = {
      {"fixed", Header(), 0, 500000},
      {"fixed_with_payload", payload, 0, 500000},
      {"not_gzip", std::vector<uint8_t>(64, 5), 0, 500000},
      {"short_rejection", {0x1f, 0}, 0, 500000},
      {"bad_method", {0x1f, 0x8b, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 500000},
      {"all_options", Header(30), 0, 200000},
      {"empty_extra", Header(4, 0), 0, 500000},
      {"long_name", Header(8, 4096), 0, 20000},
      {"long_comment", Header(16, 4096), 0, 20000},
      {"large_extra", Header(4, 16384), 0, 200000},
      {"fixed_one_byte", Header(), 1, 100000},
      {"options_one_byte", Header(30), 1, 50000},
      {"options_two_bytes", Header(30), 2, 100000},
      {"options_eight_bytes", Header(30), 8, 200000},
      {"options_64_bytes", Header(30), 64, 200000},
  };
  std::mt19937 random(20261112);
  for (const auto& test : cases) {
    const auto baseline_warmup = Run<Legacy>(test, test.iterations / 5);
    const auto rust_warmup = Run<net::GZipHeader>(test, test.iterations / 5);
    if (baseline_warmup != rust_warmup) {
      // SAFETY: The diagnostic is a static null-terminated literal.
      UNSAFE_BUFFERS(
          std::fputs("Benchmark reader results disagree.\n", stderr));
      return 1;
    }
    sink = baseline_warmup + rust_warmup;
    for (unsigned repetition = 0; repetition < 20; ++repetition) {
      double baseline;
      double rust;
      const bool rust_first = (random() & 1) != 0;
      if (rust_first) {
        rust = Measure<net::GZipHeader>(test);
        baseline = Measure<Legacy>(test);
      } else {
        baseline = Measure<Legacy>(test);
        rust = Measure<net::GZipHeader>(test);
      }
      // SAFETY: The format and every Case::name are static null-terminated
      // literals. The remaining string argument is also a literal.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"chunk_bytes\":%zu,\"input_bytes\":%zu,"
          "\"iterations\":%zu,\"repeat\":%u,\"rust_first\":%s,"
          "\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          test.name, test.chunk, test.input.size(), test.iterations, repetition,
          rust_first ? "true" : "false", baseline, rust));
      std::fflush(stdout);
    }
  }
}
