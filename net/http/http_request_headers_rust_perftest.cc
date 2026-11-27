// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Standalone deterministic paired benchmark. Run only in a coordinated quiet
// window. Both implementations execute the full public API, including
// ownership, allocation, FFI, external URL/NetLog services and destruction in
// timed scopes.
#include <array>
#include <chrono>
#include <cstdio>
#include <ratio>
#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_headers_rust_original.h"
#include "net/http/http_request_headers_rust_test_support.h"
#include "url/gurl.h"

namespace {
struct Inputs {
  std::array<std::string, 32> keys;
  std::array<std::string, 32> values;
  std::string block;
  GURL url{"https://example.test/"};
  Inputs() {
    for (size_t i = 0; i < keys.size(); ++i) {
      keys[i] = "X-Header-" + std::to_string(i);
      values[i] = std::string(i * 7, char('a' + i % 26));
      block += keys[i] + ": \t" + values[i] + " \t\r\n";
    }
  }
};
template <class Headers>
uint64_t Work(unsigned workload, const Inputs& in, unsigned repetitions) {
  uint64_t checksum = 0;
  for (unsigned rep = 0; rep < repetitions; ++rep) {
    Headers headers;
    if (workload == 0) {
      for (size_t i = 0; i < in.keys.size(); ++i) {
        headers.SetHeader(in.keys[i], in.values[i]);
      }
      Headers copy(headers);
      for (size_t i = 0; i < in.keys.size(); ++i) {
        copy.SetHeaderIfMissing(in.keys[i], "ignored");
        copy.SetHeader(in.keys[i],
                       std::string(in.values[(i + 1) % in.values.size()]));
      }
      headers.MergeFrom(copy);
      headers.MergeFrom(headers);
      Headers moved(std::move(copy));
      copy = headers;
      moved = std::move(copy);
      moved.Swap(&headers);
      for (const auto& key : in.keys) {
        moved.RemoveHeader(key);
      }
      checksum += headers.GetHeaderVector().size() + moved.IsEmpty();
      headers.SetHeaderWithoutCheckForTesting("Bypass", "value");
      checksum += headers.GetHeaderVector().size();
      headers.Clear();
    } else if (workload == 1) {
      headers.AddHeadersFromString(in.block);
      headers.AddHeaderFromString("x-header-0: \t replacement \t");
      checksum += headers.ToString().size();
    } else if (workload == 2) {
      for (size_t i = 0; i < in.keys.size(); ++i) {
        headers.SetHeader(in.keys[i], in.values[i]);
      }
      for (const auto& key : in.keys) {
        checksum += headers.HasHeader(key);
        checksum += headers.GetHeaderView(key)->size();
        checksum += headers.GetHeader(key)->size();
      }
      checksum += headers.HasHeader("missing");
      typename Headers::Iterator iter(headers);
      while (iter.GetNext()) {
        checksum += iter.name().size() + iter.value().size();
      }
    } else if (workload == 3) {
      headers.AddHeadersFromString(in.block);
      headers.SetHeader("Cookie", "secret");
      checksum += headers.ToString().size();
      checksum +=
          headers
              .NetLogParams("GET / HTTP/1.1", net::NetLogCaptureMode::kDefault)
              .size();
    } else {
      for (unsigned flags = 0; flags < 16; ++flags) {
        headers.Clear();
        std::optional<base::flat_set<net::SourceStreamType>> supported;
        if (flags & 1) {
          supported.emplace(base::flat_set<net::SourceStreamType>{
              net::SourceStreamType::kGzip, net::SourceStreamType::kBrotli});
        }
        if (flags & 2) {
          headers.SetHeader("Range", "bytes=0-1");
        }
        headers.SetAcceptEncodingIfMissing(in.url, supported, flags & 4,
                                           flags & 8);
        headers.SetAcceptEncodingIfMissing(in.url, std::nullopt, true, true);
        checksum += headers.ToString().size();
      }
    }
  }
  return checksum;
}
constexpr std::array kCases = {"lifecycle_mutations", "parse_block",
                               "lookup_iteration", "serialize_netlog",
                               "accept_encoding"};
}  // namespace

int main(int argc, char** argv) {
  const Inputs inputs;
  if (argc > 1 && std::string_view(argv[1]) == "--validate-only") {
    for (unsigned workload = 0; workload < std::size(kCases); ++workload) {
      CHECK_EQ(Work<net::HttpRequestHeaders>(workload, inputs, 2),
               Work<net::HttpRequestHeadersRustOriginal>(workload, inputs, 2));
    }
    std::puts(
        "PASS all five complete benchmark workloads; no timing performed");
    return 0;
  }
  uint64_t rng = 0x20261113;
  for (unsigned workload = 0; workload < std::size(kCases); ++workload) {
    // Identical warmup and correctness check outside recorded samples.
    CHECK_EQ(Work<net::HttpRequestHeaders>(workload, inputs, 8),
             Work<net::HttpRequestHeadersRustOriginal>(workload, inputs, 8));
    for (unsigned pair = 0; pair < 20; ++pair) {
      const bool rust_first =
          net::http_request_headers_rust_test::Random(rng) & 1;
      std::array<double, 2> times{};
      std::array<uint64_t, 2> checksums{};
      constexpr unsigned kIterations = 2000;
      for (unsigned turn = 0; turn < 2; ++turn) {
        const bool rust = rust_first != (turn != 0);
        const auto begin = std::chrono::steady_clock::now();
        const auto checksum =
            rust ? Work<net::HttpRequestHeaders>(workload, inputs, kIterations)
                 : Work<net::HttpRequestHeadersRustOriginal>(workload, inputs,
                                                             kIterations);
        const auto end = std::chrono::steady_clock::now();
        const double ns =
            std::chrono::duration<double, std::nano>(end - begin).count() /
            kIterations;
        times[rust ? 1 : 0] = ns;
        checksums[rust ? 1 : 0] = checksum;
      }
      CHECK_EQ(checksums[0], checksums[1]);
      // SAFETY: case names and boolean strings are static NUL-terminated
      // literals; numeric arguments match the fixed format.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"rust_first\":%s,\"iterations\":%u,\"checksum\":%llu}\n",
          kCases[workload], pair, times[0], times[1],
          rust_first ? "true" : "false", kIterations,
          static_cast<unsigned long long>(checksums[0])));
    }
  }
}
