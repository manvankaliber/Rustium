// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "net/http/http_request_headers_rust_ffi.h"
#include "net/http/http_request_headers_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::http_request_headers_rust_test {
TEST(HttpRequestHeadersRust, Sequences) {
  for (uint64_t i = 0; i < 2000; ++i) {
    Sequence(i + 1);
  }
}

TEST(HttpRequestHeadersRust, AllBytesAndMalformedPartialMutation) {
  // Intercept DFATAL only in this test to observe original release continuation
  // and the exact mutation prefix. CHECK-invalid setters are tested by death
  // tests.
  logging::ScopedLogAssertHandler handler(base::BindRepeating(
      [](const char*, int, std::string_view, std::string_view) {}));
  logging::SetLogMessageHandler(
      [](int, const char*, int, size_t, const std::string&) { return true; });
  for (unsigned c = 0; c < 256; ++c) {
    const std::string byte(1, static_cast<char>(c));
    for (const auto& line :
         {byte + ": ok", "X: " + byte, "X" + byte + ": v", std::string(),
          std::string(":"), std::string("colonless"), std::string("X : bad")}) {
      HttpRequestHeaders a;
      HttpRequestHeadersRustOriginal b;
      a.SetHeader("Before", "first");
      b.SetHeader("Before", "first");
      a.AddHeaderFromString(line);
      b.AddHeaderFromString(line);
      Compare(a, b);
      const auto block = "Good: prefix\r\n" + line + "\r\nAfter: suffix\r\n";
      a.AddHeadersFromString(block);
      b.AddHeadersFromString(block);
      Compare(a, b);
    }
  }
  EXPECT_TRUE(rustium_request_headers_validate("\r\n", 2, 2));
  EXPECT_EQ(0u, rustium_request_headers_crlf("\r\n", 2));
  logging::SetLogMessageHandler(nullptr);
}

TEST(HttpRequestHeadersRust, EncodingCartesianProduct) {
  for (const auto* url : {"https://example.test/", "http://example.test/",
                          "http://localhost/", "http://127.0.0.1/",
                          "http://[::1]/", "wss://example.test/", "invalid"}) {
    for (unsigned mask = 0; mask < 32; ++mask) {
      std::optional<base::flat_set<SourceStreamType>> types;
      if (mask < 16) {
        types.emplace();
        if (mask & 1) {
          types->insert(SourceStreamType::kGzip);
        }
        if (mask & 2) {
          types->insert(SourceStreamType::kDeflate);
        }
        if (mask & 4) {
          types->insert(SourceStreamType::kBrotli);
        }
        if (mask & 8) {
          types->insert(SourceStreamType::kZstd);
        }
        types->insert(SourceStreamType::kUnknown);
      }
      for (unsigned flags = 0; flags < 16; ++flags) {
        HttpRequestHeaders a;
        HttpRequestHeadersRustOriginal b;
        if (flags & 1) {
          a.SetHeader("rANGE", "bytes=0-1");
          b.SetHeader("rANGE", "bytes=0-1");
        }
        if (flags & 2) {
          a.SetHeader("aCCEPT-eNCODING", "custom");
          b.SetHeader("aCCEPT-eNCODING", "custom");
        }
        a.SetAcceptEncodingIfMissing(GURL(url), types, flags & 4, flags & 8);
        b.SetAcceptEncodingIfMissing(GURL(url), types, flags & 4, flags & 8);
        Compare(a, b);
      }
    }
  }
}

TEST(HttpRequestHeadersRust, DuplicatesAliasesAndLifetimes) {
  HttpRequestHeaders a;
  HttpRequestHeadersRustOriginal b;
  // Const access exposes the owned vector; modifying through const_cast is
  // defined for these non-const objects, and manufactures supported duplicates.
  auto& av = const_cast<HttpRequestHeaders::HeaderVector&>(a.GetHeaderVector());
  auto& bv = const_cast<HttpRequestHeadersRustOriginal::HeaderVector&>(
      b.GetHeaderVector());
  av.emplace_back("X", "first");
  av.emplace_back("x", "second");
  bv.emplace_back("X", "first");
  bv.emplace_back("x", "second");
  a.SetHeader("x", "replace");
  b.SetHeader("x", "replace");
  EXPECT_EQ(av[1].value, "second");
  Compare(a, b);
  a.RemoveHeader(av.front().key);
  b.RemoveHeader(bv.front().key);
  Compare(a, b);
  a.SetHeaderIfMissing(av.front().key, av.front().value);
  b.SetHeaderIfMissing(bv.front().key, bv.front().value);
  Compare(a, b);
  a.SetHeader("Raw", "New: value");
  b.SetHeader("Raw", "New: value");
  a.AddHeaderFromString(*a.GetHeaderView("Raw"));
  b.AddHeaderFromString(*b.GetHeaderView("Raw"));
  Compare(a, b);
  const auto* ap = a.GetHeaderView("x")->data();
  const auto* bp = b.GetHeaderView("x")->data();
  a.SetHeaderIfMissing("X", "ignored");
  b.SetHeaderIfMissing("X", "ignored");
  EXPECT_EQ(ap, a.GetHeaderView("X")->data());
  EXPECT_EQ(bp, b.GetHeaderView("X")->data());
  a.Swap(&a);
  b.Swap(&b);
  auto* aa = &a;
  auto* bb = &b;
  a = *aa;
  b = *bb;
  Compare(a, b);
  HttpRequestHeaders::HeaderKeyValuePair empty;
  EXPECT_TRUE(empty.key.empty());
  HttpRequestHeaders::HeaderKeyValuePair literal("X", "y");
  HttpRequestHeaders::HeaderKeyValuePair copy("X", std::string_view("y"));
  EXPECT_EQ(literal, copy);
}

TEST(HttpRequestHeadersRust, CheckedInvalidInputsDeathTest) {
  EXPECT_DEATH_IF_SUPPORTED(
      {
        HttpRequestHeaders a;
        a.AddHeaderFromString("X: a\r\nz");
      },
      "contains CRLF");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        HttpRequestHeadersRustOriginal a;
        a.AddHeaderFromString("X: a\r\nz");
      },
      "contains CRLF");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        HttpRequestHeaders a;
        a.SetHeader("", "v");
      },
      "IsValidHeaderName");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        HttpRequestHeaders a;
        a.SetHeader("X", "bad\r");
      },
      "has invalid value");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        HttpRequestHeaders a;
        a.SetHeaderIfMissing("bad name", "v");
      },
      "IsValidHeaderName");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        HttpRequestHeaders a;
        a.SetHeader("X", "v");
        a.SetHeaderIfMissing("X", "bad\n");
      },
      "IsValidHeaderValue");
}

TEST(HttpRequestHeadersRust, StorageAliasingMatrix) {
  for (unsigned scenario = 0; scenario < 9; ++scenario) {
    HttpRequestHeaders a;
    HttpRequestHeadersRustOriginal b;
    a.SetHeader("X", std::string(180, 'a'));
    b.SetHeader("X", std::string(180, 'a'));
    a.SetHeader("Y", "X");
    b.SetHeader("Y", "X");
    auto& av =
        const_cast<HttpRequestHeaders::HeaderVector&>(a.GetHeaderVector());
    auto& bv = const_cast<HttpRequestHeadersRustOriginal::HeaderVector&>(
        b.GetHeaderVector());
    switch (scenario) {
      case 0:
        a.SetHeader(av[0].key, av[0].value);
        b.SetHeader(bv[0].key, bv[0].value);
        break;
      case 1:
        a.SetHeader(av[0].key, std::move(av[0].value));
        b.SetHeader(bv[0].key, std::move(bv[0].value));
        break;
      case 2:
        // Name points into an existing value; replacement invalidates it only
        // after Rust's read-only plan returns.
        a.SetHeader(av[1].value, av[0].value);
        b.SetHeader(bv[1].value, bv[0].value);
        break;
      case 3:
        a.SetHeader("New", std::string_view(av[0].value).substr(17, 80));
        b.SetHeader("New", std::string_view(bv[0].value).substr(17, 80));
        break;
      case 4:
        a.SetHeaderIfMissing("New", av[0].value);
        b.SetHeaderIfMissing("New", bv[0].value);
        break;
      case 5:
        a.SetHeader(av[1].value, std::move(av[1].value));
        b.SetHeader(bv[1].value, std::move(bv[1].value));
        break;
      case 6:
        a.SetHeader("X", "X: self-backed raw line");
        b.SetHeader("X", "X: self-backed raw line");
        a.AddHeaderFromString(*a.GetHeaderView("X"));
        b.AddHeaderFromString(*b.GetHeaderView("X"));
        break;
      case 7:
        a.MergeFrom(a);
        b.MergeFrom(b);
        break;
      case 8:
        a.SetHeaderWithoutCheckForTesting(av[0].key, std::string_view());
        b.SetHeaderWithoutCheckForTesting(bv[0].key, std::string_view());
        break;
    }
    Compare(a, b);
  }
}

TEST(HttpRequestHeadersRust, NullEmptyAbiAndLayout) {
  HttpRequestHeaders a;
  HttpRequestHeadersRustOriginal b;
  EXPECT_EQ(sizeof(a), sizeof(b));
  EXPECT_EQ(alignof(HttpRequestHeaders),
            alignof(HttpRequestHeadersRustOriginal));
  EXPECT_EQ(sizeof(HttpRequestHeaders::Iterator),
            sizeof(HttpRequestHeadersRustOriginal::Iterator));
  EXPECT_EQ(sizeof(HttpRequestHeaders::HeaderKeyValuePair),
            sizeof(HttpRequestHeadersRustOriginal::HeaderKeyValuePair));
  a.AddHeadersFromString(std::string_view());
  b.AddHeadersFromString(std::string_view());
  a.RemoveHeader(std::string_view());
  b.RemoveHeader(std::string_view());
  EXPECT_FALSE(a.GetHeader(std::string_view()));
  EXPECT_FALSE(a.GetHeaderView(std::string_view()));
  EXPECT_FALSE(rustium_request_headers_validate(nullptr, 0, 0));
  EXPECT_TRUE(rustium_request_headers_validate(nullptr, 0, 1));
  EXPECT_FALSE(rustium_request_headers_validate(nullptr, 0, 2));
  EXPECT_EQ(1u, rustium_request_headers_parse(nullptr, 0).error);
  Compare(a, b);
}
}  // namespace net::http_request_headers_rust_test
