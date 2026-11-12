// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <random>
#include <string>
#include <string_view>

#include "base/containers/span_rust.h"
#include "net/http/hsts_ffi.rs.h"
#include "net/http/hsts_http_util_reference.h"
#include "net/http/hsts_test_support.h"
#include "net/http/http_security_headers.h"
#include "net/http/http_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

static_assert(sizeof(hsts_internal::test::LegacyHttpUtil::ValuesIterator) ==
              sizeof(HttpUtil::ValuesIterator));
static_assert(
    sizeof(hsts_internal::test::LegacyHttpUtil::NameValuePairsIterator) ==
    sizeof(HttpUtil::NameValuePairsIterator));

void Compare(std::string_view input) {
  for (bool initial_subdomains : {false, true}) {
    auto actual_age = base::Seconds(-12345);
    auto expected_age = actual_age;
    bool actual_subdomains = initial_subdomains;
    bool expected_subdomains = initial_subdomains;
    const bool expected = hsts_internal::test::LegacyParseHSTSHeader(
        input, &expected_age, &expected_subdomains);
    EXPECT_EQ(ParseHSTSHeader(input, &actual_age, &actual_subdomains),
              expected);
    EXPECT_EQ(actual_age, expected_age);
    EXPECT_EQ(actual_subdomains, expected_subdomains);
  }
}

TEST(HstsRustTest, QuotedValuesAndEscapes) {
  for (std::string_view value : {
           "max-age=\"123\"",
           "max-age=\"\\1\\2\\3\"",
           "max-age=\"00000000000000000000000000000000000000\"",
           "max-age=\"\\0\\0\\0\"",
           "max-age=\"\"",
           "max-age=\"\\\"\"",
           "max-age=\"123\\\"",
           "max-age=\"123",
           "max-age=123\"",
           "max-age=1; x=\"a;b; c\"",
           "max-age=1; x=\"a\\\";b\"",
           "max-age=1; x=\"a\\\\b\"",
           "max-age=1; x=\"a\"b\"",
           "max-age=1; x=\"a\"\"b\"",
           "max-age=1; x=\"\"",
           "max-age=1; x=\"a\\\"",
           "max-age=1; x=\"a\"tail",
           "max-age=1; x=\"\"; x=\"\"",
           "max-age=1; includeSubDomains=\"\"",
           "max-age=1; includeSubDomains=",
           "max-age=1; includeSubDomains=''",
       }) {
    SCOPED_TRACE(value);
    Compare(value);
  }
}

TEST(HstsRustTest, EveryByteInNamesAndValues) {
  for (unsigned byte = 0; byte <= 255; ++byte) {
    for (std::string_view prefix :
         {"max-age=1; x", "max-age=1; x=", "max-age=1; x=\""}) {
      std::string value(prefix);
      value.push_back(static_cast<char>(byte));
      if (prefix.ends_with('"')) {
        value.push_back('"');
      }
      Compare(value);
    }
  }
}

TEST(HstsRustTest, EveryByteAsWhitespace) {
  for (unsigned byte = 0; byte <= 255; ++byte) {
    const char c = static_cast<char>(byte);
    Compare(std::string(1, c) + "max-age=1" + c);
    Compare("max-age" + std::string(1, c) + "=" + c + "1" + c + ";" + c +
            "includeSubDomains" + c);
  }
}

TEST(HstsRustTest, ClampingAndMalformedOverflow) {
  for (std::string_view value : {
           "0",
           "0001",
           "31535999",
           "31536000",
           "31536001",
           "4294967295",
           "4294967296",
           "18446744073709551615",
           "18446744073709551616",
           "9999999999999999999999999999999999999999999999",
           "9999999999999999999999999999999999999999999999x",
           "9999999999999999999999999999999999999999999999.0",
           "+1",
           "-1",
           "1.0",
           "",
           " 1",
           "1 ",
           "1 0",
       }) {
    Compare("max-age=" + std::string(value));
    Compare("max-age=\"" + std::string(value) + "\"");
    std::string escaped;
    for (char c : value) {
      escaped.push_back('\\');
      escaped.push_back(c);
    }
    Compare("max-age=\"" + escaped + "\"");
  }
}

TEST(HstsRustTest, DuplicatesAndEmptyDirectives) {
  for (std::string_view value : {
           "",
           " ",
           ";;;; \t;;",
           ";;max-age=0;;;includeSubDomains;;",
           "max-age=1; MAX-AGE=1",
           "max-age=0;max-age=1",
           "max-age=1; includeSubDomains; includesubdomains",
           "max-age=1;x=1;x=2",
           "max-age=1;x;x",
           "max-age=1;=x",
           "max-age=1; =x",
           "max-age=1;\"x\"=x",
           "max-age=1;\"x",
           "max-age=1;foo\"bar=baz",
           "max-age=1;foo=bar\";x=y",
           "includeSubDomains;max-age=1",
           "max-age",
           "max-age; x=1",
       }) {
    Compare(value);
  }
}

TEST(HstsRustTest, LongInputAndEmbeddedNul) {
  Compare("max-age=" + std::string(65536, '0') + "1;includeSubDomains");
  Compare("max-age=" + std::string(65536, '9'));
  Compare("max-age=\"" + std::string(65536, '9') + "\\0\"");
  Compare("max-age=1;x=\"" + std::string(65536, 'x') + "\"");
  for (const std::string& value :
       {std::string("max-age=1\0", 10), std::string("max-age=1;x=\"a\0b\"", 17),
        std::string("max-age=\"1\0\"", 12)}) {
    Compare(value);
  }
}

TEST(HstsRustTest, EverySingleByteMutation) {
  for (std::string input : {"max-age=31536000;includeSubDomains",
                            "x=\"quoted;value\";max-age=\"\\1\\2\"",
                            "max-age=4294967296;x=token"}) {
    for (size_t index = 0; index < input.size(); ++index) {
      const char original = input[index];
      for (unsigned byte = 0; byte <= 255; ++byte) {
        input[index] = static_cast<char>(byte);
        Compare(input);
      }
      input[index] = original;
    }
  }
}

TEST(HstsRustTest, DeterministicGrammarMutations) {
  std::mt19937 random(20261112);
  constexpr std::string_view alphabet =
      "max-age=1234567890;includeSubDomains xX!@#'\"\\\t\r\n";
  for (size_t repetition = 0; repetition < 10000; ++repetition) {
    std::string input = repetition % 2 ? "max-age=1;" : "";
    const size_t size = random() % 128;
    for (size_t i = 0; i < size; ++i) {
      input.push_back(alphabet[random() % alphabet.size()]);
    }
    Compare(input);
  }
}

TEST(HstsRustTest, BridgeLimitAndScalarLayout) {
  for (uint32_t limit : {0u, 1u, kMaxHSTSAgeSecs, UINT32_MAX}) {
    const std::string value = "max-age=4294967295;includeSubDomains";
    EXPECT_EQ(hsts_internal::parse_hsts(
                  base::SpanToRustSlice(base::as_byte_span(value)), limit),
              (static_cast<uint64_t>(limit) << 2) | 3u);
  }
  const std::string value = "max-age=0";
  EXPECT_EQ(hsts_internal::parse_hsts(
                base::SpanToRustSlice(base::as_byte_span(value)), 0),
            1u);
}

}  // namespace
}  // namespace net
