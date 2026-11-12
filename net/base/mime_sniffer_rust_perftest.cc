// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Whole-public-API paired benchmark. Both wrappers are compiled out of line
// into this executable, avoiding asymmetric net.dll import overhead.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_sniffer_reference.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace {
enum class Operation {
  kSniff,
  kLocal,
  kBinary,
  kPolicy,
  kHintAlias,
  kContentAlias,
  kJointAlias
};
struct Case {
  const char* name;
  Operation operation;
  std::string input;
  GURL url;
  std::string hint;
  size_t iterations = 100000;
  bool nosniff = false;
  size_t alias_offset = 0;
};

volatile uint64_t sink = 0;

template <bool Legacy>
ALWAYS_INLINE bool Execute(const Case& test,
                           const net::HttpResponseHeaders* headers,
                           std::string& result) {
  bool success = false;
  switch (test.operation) {
    case Operation::kSniff:
      if constexpr (Legacy) {
        success = net::mime_sniffer_test::SniffMimeType(
            test.input, test.url, test.hint,
            net::ForceSniffFileUrlsForHtml::kDisabled, &result);
      } else {
        success = net::SniffMimeType(test.input, test.url, test.hint,
                                     net::ForceSniffFileUrlsForHtml::kDisabled,
                                     &result);
      }
      break;
    case Operation::kLocal:
      if constexpr (Legacy) {
        success = net::mime_sniffer_test::SniffMimeTypeFromLocalData(test.input,
                                                                     &result);
      } else {
        success = net::SniffMimeTypeFromLocalData(test.input, &result);
      }
      break;
    case Operation::kBinary:
      if constexpr (Legacy) {
        success = net::mime_sniffer_test::LooksLikeBinary(test.input);
      } else {
        success = net::LooksLikeBinary(test.input);
      }
      break;
    case Operation::kPolicy:
      if constexpr (Legacy) {
        success = net::mime_sniffer_test::ShouldSniffMimeType(test.url, headers,
                                                              test.hint);
      } else {
        success = net::ShouldSniffMimeType(test.url, headers, test.hint);
      }
      break;
    case Operation::kHintAlias:
    case Operation::kContentAlias:
    case Operation::kJointAlias: {
      const bool hint_only = test.operation == Operation::kHintAlias;
      result.assign(hint_only ? test.hint : test.input);
      // Content views stay valid across every legacy write. Include the
      // identical reset/reservation cost for both readers in the timing.
      if (!hint_only) {
        result.reserve(test.input.size() + 128);
      }
      const auto view =
          hint_only ? std::string_view(test.input)
                    : std::string_view(result).substr(test.alias_offset);
      const auto& hint =
          test.operation == Operation::kContentAlias ? test.hint : result;
      if constexpr (Legacy) {
        success = net::mime_sniffer_test::SniffMimeType(
            view, test.url, hint, net::ForceSniffFileUrlsForHtml::kDisabled,
            &result);
      } else {
        success = net::SniffMimeType(view, test.url, hint,
                                     net::ForceSniffFileUrlsForHtml::kDisabled,
                                     &result);
      }
      break;
    }
  }

  return success;
}

template <bool Legacy>
uint64_t Run(const Case& test,
             const net::HttpResponseHeaders* headers,
             size_t iterations) {
  uint64_t sum = 0;
  for (size_t i = 0; i < iterations; ++i) {
    std::string result;
    sum += Execute<Legacy>(test, headers, result);
    // Read the post-call size, not the pre-call empty string.
    sum += result.size();
  }
  return sum;
}

template <bool Legacy>
double Measure(const Case& test, const net::HttpResponseHeaders* headers) {
  const auto start = std::chrono::steady_clock::now();
  const auto checksum = Run<Legacy>(test, headers, test.iterations);
  const auto finish = std::chrono::steady_clock::now();
  sink = checksum;
  return std::chrono::duration<double, std::nano>(finish - start).count() /
         test.iterations;
}
}  // namespace

int main() {
  const GURL web("https://example.test/file");
  const std::string plain(1024, 'A');
  std::string early_binary = plain;
  early_binary[0] = 0;
  std::string late_binary = plain;
  late_binary.back() = 0;
  const auto sniff = Operation::kSniff, local = Operation::kLocal,
             binary = Operation::kBinary, policy = Operation::kPolicy;
  std::vector<Case> cases = {
      {"unknown_text", sniff, plain, web, ""},
      {"plain_text", sniff, plain, web, "text/plain"},
      {"html", sniff, " \r\n<!DOCTYPE html><title>hi</title>", web, ""},
      {"png", sniff, std::string("\x89PNG\r\n\x1a\n", 8), web, ""},
      {"jpeg", sniff, std::string("\xff\xd8\xff", 3), web, ""},
      {"office_cfb", sniff, std::string("\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1", 8),
       GURL("https://example.test/a.doc"), "application/octet-stream"},
      {"office_ooxml", sniff, std::string("PK\3\4\0\0\0\0", 8),
       GURL("https://example.test/a.docx"), "application/octet-stream"},
      {"office_invalid", sniff, plain, web, "application/msword"},
      {"crx", sniff, std::string("Cr24\3\0\0\0", 8),
       GURL("https://example.test/a.crx"), "application/octet-stream"},
      {"xml_atom", sniff, "<?xml version='1.0'?> <feed>", web,
       "application/xml"},
      {"local_gif", local, "GIF89a"},
      {"local_mp4", local, "abcdftypmp42"},
      {"local_wav", local, "RIFF1234WAVEfmt "},
      {"local_raw", local, std::string("II\x2a\0\x10\0\0\0CR", 10)},
      {"local_miss", local, plain},
      {"binary_text_1k", binary, plain},
      {"binary_text_256k", binary, std::string(262144, 'A'), GURL(), "", 500},
      {"binary_early", binary, early_binary},
      {"binary_late", binary, late_binary},
      {"binary_empty", binary, ""},
      {"policy_plain", policy, "", web, "text/plain"},
      {"policy_known", policy, "", web, "image/png"},
      {"policy_unknown", policy, "", web, "unknown/unknown"},
      {"policy_blocked_url", policy, "", GURL("data:text/plain,hi"),
       "text/plain"},
      {"policy_nosniff", policy, "", web, "text/plain", 100000, true},
      {"alias_hint_plain", Operation::kHintAlias, plain, web, "text/plain"},
      {"alias_hint_binary", Operation::kHintAlias,
       std::string("\x89PNG\r\n\x1a\n", 8), web, "text/plain"},
      {"alias_content_small", Operation::kContentAlias, "<html><p>x", web, ""},
      {"alias_content_1k", Operation::kContentAlias, plain, web, "text/plain"},
      {"alias_joint_subview", Operation::kJointAlias, "abc<?xml", web, "",
       100000, false, 3},
  };
  const auto nosniff = net::HttpResponseHeaders::Builder({1, 1}, "200 OK")
                           .AddHeader("x-content-type-options", "nosniff")
                           .Build();
  std::mt19937 order(20261112);
  for (const auto& test : cases) {
    const auto* headers = test.nosniff ? nosniff.get() : nullptr;
    std::string baseline_result, rust_result;
    const bool baseline_status = Execute<true>(test, headers, baseline_result);
    const bool rust_status = Execute<false>(test, headers, rust_result);
    if (baseline_status != rust_status || baseline_result != rust_result) {
      // SAFETY: fixed literal; test-only diagnostic output.
      UNSAFE_TODO(std::fputs("MIME benchmark result mismatch\n", stderr));
      return 1;
    }
    const auto warmup = std::max(size_t{1}, test.iterations / 5);
    if (Run<true>(test, headers, warmup) != Run<false>(test, headers, warmup)) {
      // SAFETY: fixed literal; this is test-only diagnostic output.
      UNSAFE_TODO(std::fputs("MIME benchmark checksum mismatch\n", stderr));
      return 1;
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      double baseline_ns, rust_ns;
      const bool rust_first = (order() & 1) != 0;
      if (rust_first) {
        rust_ns = Measure<false>(test, headers);
        baseline_ns = Measure<true>(test, headers);
      } else {
        baseline_ns = Measure<true>(test, headers);
        rust_ns = Measure<false>(test, headers);
      }
      // SAFETY: format and case names are fixed literals, with matching types.
      UNSAFE_TODO(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"input_bytes\":%zu,\"rust_first\":%s}\n",
          test.name, repeat, baseline_ns, rust_ns, test.iterations,
          test.input.size(), rust_first ? "true" : "false"));
    }
  }
  return 0;
}
