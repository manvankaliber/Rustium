// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_REQUEST_HEADERS_RUST_TEST_SUPPORT_H_
#define NET_HTTP_HTTP_REQUEST_HEADERS_RUST_TEST_SUPPORT_H_
#include <cstdint>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_headers_rust_original.h"
#include "url/gurl.h"

namespace net::http_request_headers_rust_test {
inline uint64_t Random(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

inline void Compare(const HttpRequestHeaders& actual,
                    const HttpRequestHeadersRustOriginal& expected) {
  CHECK_EQ(actual.IsEmpty(), expected.IsEmpty());
  CHECK_EQ(actual.ToString(), expected.ToString());
  const auto& a = actual.GetHeaderVector();
  const auto& b = expected.GetHeaderVector();
  CHECK_EQ(a.size(), b.size());
  HttpRequestHeaders::Iterator ai(actual);
  HttpRequestHeadersRustOriginal::Iterator bi(expected);
  for (size_t i = 0; i < a.size(); ++i) {
    CHECK_EQ(a[i].key, b[i].key);
    CHECK_EQ(a[i].value, b[i].value);
    CHECK(ai.GetNext());
    CHECK(bi.GetNext());
    CHECK_EQ(ai.name(), bi.name());
    CHECK_EQ(ai.value(), bi.value());
    CHECK_EQ(actual.HasHeader(a[i].key), expected.HasHeader(b[i].key));
    CHECK(actual.GetHeader(a[i].key) == expected.GetHeader(b[i].key));
    CHECK(actual.GetHeaderView(a[i].key) == expected.GetHeaderView(b[i].key));
    // Returned views must refer to the actual C++ string, not a Rust copy.
    size_t first = 0;
    while (!base::EqualsCaseInsensitiveASCII(a[first].key, a[i].key)) {
      ++first;
    }
    CHECK_EQ(actual.GetHeaderView(a[i].key)->data(), a[first].value.data());
  }
  CHECK(!ai.GetNext());
  CHECK(!bi.GetNext());
  CHECK(!ai.GetNext());
  CHECK(!bi.GetNext());
  CHECK_EQ(actual.HasHeader("absent"), expected.HasHeader("absent"));
  CHECK(actual.GetHeaderView("absent") == expected.GetHeaderView("absent"));
  CHECK(actual.GetHeader("absent") == expected.GetHeader("absent"));
}

// Every sequence exercises actual exported C++ APIs and destruction; inputs
// include long strings, arbitrary bytes via the test bypass, legal overlaps,
// null/empty views, mixed case, move/copy/swap, self merge and duplicate
// vectors.
inline void Sequence(uint64_t seed, size_t steps = 24) {
  HttpRequestHeaders a, other;
  HttpRequestHeadersRustOriginal b, other_b;
  uint64_t state = seed | 1;
  for (size_t step = 0; step < steps; ++step) {
    const auto random = Random(state);
    const std::string key = "X-" + std::to_string((random >> 8) % 12);
    std::string value((random >> 16) % 100, char('a' + random % 26));
    switch (random % 17) {
      case 0:
        a.SetHeader(key, value);
        b.SetHeader(key, value);
        break;
      case 1:
        a.SetHeader(key, std::string(value));
        b.SetHeader(key, std::string(value));
        break;
      case 2:
        a.SetHeaderIfMissing(key, value);
        b.SetHeaderIfMissing(key, value);
        break;
      case 3:
        a.RemoveHeader(key);
        b.RemoveHeader(key);
        break;
      case 4: {
        const auto line = key + ": \t" + value + "\t ";
        a.AddHeaderFromString(line);
        b.AddHeaderFromString(line);
        break;
      }
      case 5: {
        const auto block =
            " \r\n\v " + key + ": " + value + "\f\r\nX-End: tail\r\n";
        a.AddHeadersFromString(block);
        b.AddHeadersFromString(block);
        break;
      }
      case 6:
        other.SetHeader(key, value);
        other_b.SetHeader(key, value);
        a.MergeFrom(other);
        b.MergeFrom(other_b);
        break;
      case 7:
        a.MergeFrom(a);
        b.MergeFrom(b);
        break;
      case 8: {
        HttpRequestHeaders copy(a);
        HttpRequestHeadersRustOriginal copy_b(b);
        a = copy;
        b = copy_b;
        a = std::move(copy);
        b = std::move(copy_b);
        Compare(copy, copy_b);
        break;
      }
      case 9: {
        HttpRequestHeaders moved(std::move(a));
        HttpRequestHeadersRustOriginal moved_b(std::move(b));
        a.Swap(&moved);
        b.Swap(&moved_b);
        Compare(moved, moved_b);
        break;
      }
      case 10:
        if (!a.IsEmpty()) {
          // A value view of the replaced string, including overlapping
          // subspans.
          const auto& x = a.GetHeaderVector().front();
          const auto& y = b.GetHeaderVector().front();
          const auto av = std::string_view(x.value).substr(x.value.size() / 2);
          const auto bv = std::string_view(y.value).substr(y.value.size() / 2);
          a.SetHeaderWithoutCheckForTesting(x.key, av);
          b.SetHeaderWithoutCheckForTesting(y.key, bv);
        }
        break;
      case 11:
        if (!a.IsEmpty()) {
          const auto& x = a.GetHeaderVector().back();
          const auto& y = b.GetHeaderVector().back();
          a.RemoveHeader(x.key);
          b.RemoveHeader(y.key);
        }
        break;
      case 12: {
        // Mutable backing, embedded NUL/high bytes and name case folding.
        std::string raw;
        for (size_t n = 0; n < (random >> 24) % 20; ++n) {
          raw.push_back(char(Random(state)));
        }
        a.SetHeaderWithoutCheckForTesting(raw, value);
        b.SetHeaderWithoutCheckForTesting(raw, value);
        break;
      }
      case 13:
        a.SetHeaderWithoutCheckForTesting(std::string_view(),
                                          std::string_view());
        b.SetHeaderWithoutCheckForTesting(std::string_view(),
                                          std::string_view());
        break;
      case 14:
        // Avoid public checked merge on arbitrary invalid bypass names.
        a.Clear();
        b.Clear();
        other.Clear();
        other_b.Clear();
        break;
      case 15:
        for (auto mode :
             {NetLogCaptureMode::kHeavilyRedacted, NetLogCaptureMode::kDefault,
              NetLogCaptureMode::kIncludeSensitive,
              NetLogCaptureMode::kEverything}) {
          CHECK(a.NetLogParams("GET / HTTP/1.1\r\n", mode) ==
                b.NetLogParams("GET / HTTP/1.1\r\n", mode));
        }
        break;
      case 16:
        a.SetAcceptEncodingIfMissing(GURL("https://example.test/"),
                                     std::nullopt, random & 32, random & 64);
        b.SetAcceptEncodingIfMissing(GURL("https://example.test/"),
                                     std::nullopt, random & 32, random & 64);
        break;
    }
    // Bypass-only invalid headers must not reach checked self/cross merge.
    Compare(a, b);
    if (random % 17 == 12 || random % 17 == 13) {
      a.Clear();
      b.Clear();
    }
  }
}
}  // namespace net::http_request_headers_rust_test
#endif
