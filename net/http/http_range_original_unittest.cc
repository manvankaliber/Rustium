// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unmodified range tests extracted from the pre-migration HttpUtil test source.
#include <limits>
#include <optional>
#include <vector>

#include "net/http/http_byte_range.h"
#include "net/http/http_util.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
TEST(HttpUtilTest, ParseContentRangeHeader) {
  const struct {
    const char* const content_range_header_spec;
    bool expected_return_value;
    int64_t expected_first_byte_position;
    int64_t expected_last_byte_position;
    int64_t expected_instance_length;
  } tests[] = {
      {"", false, -1, -1, -1},
      {"megabytes 0-10/50", false, -1, -1, -1},
      {"0-10/50", false, -1, -1, -1},
      {"Bytes 0-50/51", true, 0, 50, 51},
      {"bytes 0-50/51", true, 0, 50, 51},
      {"bytes\t0-50/51", false, -1, -1, -1},
      {"    bytes 0-50/51", true, 0, 50, 51},
      {"    bytes    0    -   50  \t / \t51", true, 0, 50, 51},
      {"bytes 0\t-\t50\t/\t51\t", true, 0, 50, 51},
      {"  \tbytes\t\t\t 0\t-\t50\t/\t51\t", true, 0, 50, 51},
      {"\t   bytes \t  0    -   50   /   5   1", false, -1, -1, -1},
      {"\t   bytes \t  0    -   5 0   /   51", false, -1, -1, -1},
      {"bytes 50-0/51", false, -1, -1, -1},
      {"bytes * /*", false, -1, -1, -1},
      {"bytes *   /    *   ", false, -1, -1, -1},
      {"bytes 0-50/*", false, -1, -1, -1},
      {"bytes 0-50  /    * ", false, -1, -1, -1},
      {"bytes 0-10000000000/10000000001", true, 0, 10000000000ll,
       10000000001ll},
      {"bytes 0-10000000000/10000000000", false, -1, -1, -1},
      // 64 bit wraparound.
      {"bytes 0 - 9223372036854775807 / 100", false, -1, -1, -1},
      // 64 bit wraparound.
      {"bytes 0 - 100 / -9223372036854775808", false, -1, -1, -1},
      {"bytes */50", false, -1, -1, -1},
      {"bytes 0-50/10", false, -1, -1, -1},
      {"bytes 40-50/45", false, -1, -1, -1},
      {"bytes 0-50/-10", false, -1, -1, -1},
      {"bytes 0-0/1", true, 0, 0, 1},
      {"bytes 0-40000000000000000000/40000000000000000001", false, -1, -1, -1},
      {"bytes 1-/100", false, -1, -1, -1},
      {"bytes -/100", false, -1, -1, -1},
      {"bytes -1/100", false, -1, -1, -1},
      {"bytes 0-1233/*", false, -1, -1, -1},
      {"bytes -123 - -1/100", false, -1, -1, -1},
  };

  for (const auto& test : tests) {
    int64_t first_byte_position, last_byte_position, instance_length;
    EXPECT_EQ(test.expected_return_value,
              HttpUtil::ParseContentRangeHeaderFor206(
                  test.content_range_header_spec, &first_byte_position,
                  &last_byte_position, &instance_length))
        << test.content_range_header_spec;
    EXPECT_EQ(test.expected_first_byte_position, first_byte_position)
        << test.content_range_header_spec;
    EXPECT_EQ(test.expected_last_byte_position, last_byte_position)
        << test.content_range_header_spec;
    EXPECT_EQ(test.expected_instance_length, instance_length)
        << test.content_range_header_spec;
  }
}

TEST(HttpUtilTest, ParseRangeHeader) {
  std::vector<HttpByteRange> ranges;
  ASSERT_TRUE(HttpUtil::ParseRangeHeader("bytes=0-99", &ranges));
  ASSERT_EQ(1u, ranges.size());
  EXPECT_EQ(0, ranges[0].first_byte_position());
  EXPECT_EQ(99, ranges[0].last_byte_position());

  ranges.clear();
  ASSERT_TRUE(HttpUtil::ParseRangeHeader("bytes=100-", &ranges));
  ASSERT_EQ(1u, ranges.size());
  EXPECT_EQ(100, ranges[0].first_byte_position());
  EXPECT_FALSE(ranges[0].HasLastBytePosition());

  ranges.clear();
  ASSERT_TRUE(HttpUtil::ParseRangeHeader("bytes=-50", &ranges));
  ASSERT_EQ(1u, ranges.size());
  EXPECT_TRUE(ranges[0].IsSuffixByteRange());
  EXPECT_EQ(50, ranges[0].suffix_length());

  ranges.clear();
  EXPECT_FALSE(HttpUtil::ParseRangeHeader("bytes", &ranges));
  EXPECT_FALSE(HttpUtil::ParseRangeHeader("bytes=100", &ranges));
}

// Invalid Fetch single-range header values should fail parsing, including
// malformed syntax, unsupported formats, missing digits, reverse ranges,
// and out-of-range numeric values.
TEST(HttpUtilTest, ParseFetchSingleRangeRejectsInvalid) {
  static constexpr const char* kCases[] = {
      "",
      "byte=0-",
      "bytes",
      "bytes\t \t",
      "bytes=0-5,15-",
      "bytes=0-5, 15-",
      "bytes=0-5,",
      "bytes=-",
      "bytes=x-5",
      "bytes=5",
      "bytes=5-x",
      "bytes=x",
      "5-",
      "Bytes=0-5",
      "bytes=-5-10",
      // Trailing whitespace after the range is not allowed by the spec.
      "bytes=5-10 ",
      // Multiple ranges are not a single range.
      "bytes=0-10,15-20",
      // Extra '=' or '-' delimiters are rejected.
      "bytes==0-5",
      "bytes=0--5",
      "bytes=0-5-",
      "bytes=-0-5",
  };
  for (const char* value : kCases) {
    SCOPED_TRACE(value);
    // Invalid Fetch range forms must be rejected.
    EXPECT_FALSE(
        HttpUtil::ParseFetchSingleRange(value, /*allow_whitespace=*/true)
            .has_value());
  }
}

// Verifies that syntactically valid but semantically invalid ranges parse
// successfully. Callers are responsible for checking IsValid().
TEST(HttpUtilTest, ParseFetchSingleRangeParsesSemanticInvalid) {
  {
    // "bytes=-0" is a zero-length suffix range -- syntactically valid, but
    // IsValid() returns false because suffix_length must be > 0.
    SCOPED_TRACE("bytes=-0");
    std::optional<HttpByteRange> range =
        HttpUtil::ParseFetchSingleRange("bytes=-0", /*allow_whitespace=*/true);
    ASSERT_TRUE(range.has_value());
    EXPECT_TRUE(range->IsSuffixByteRange());
    EXPECT_EQ(0, range->suffix_length());
    EXPECT_FALSE(range->IsValid());
  }
  {
    // "bytes=10-5" has start > end -- syntactically valid, but IsValid()
    // returns false.
    SCOPED_TRACE("bytes=10-5");
    std::optional<HttpByteRange> range =
        HttpUtil::ParseFetchSingleRange("bytes=10-5",
                                        /*allow_whitespace=*/true);
    ASSERT_TRUE(range.has_value());
    EXPECT_FALSE(range->IsSuffixByteRange());
    EXPECT_EQ(10, range->first_byte_position());
    EXPECT_EQ(5, range->last_byte_position());
    EXPECT_FALSE(range->IsValid());
  }
  {
    // Overflow saturates to int64_t max, making start > end.
    SCOPED_TRACE("bytes=999999999999999999999-5");
    std::optional<HttpByteRange> range =
        HttpUtil::ParseFetchSingleRange("bytes=999999999999999999999-5",
                                        /*allow_whitespace=*/true);
    ASSERT_TRUE(range.has_value());
    EXPECT_FALSE(range->IsValid());
  }
}

// Verifies that valid bounded byte ranges ("bytes=first-last") are parsed
// correctly, including leading-zero variants.
TEST(HttpUtilTest, ParseFetchSingleRangeAcceptsNormal) {
  struct {
    const char* value;
    int64_t first;
    int64_t last;
  } kCases[] = {
      {"bytes=5-10", 5, 10},
      {"bytes=00005-00010", 5, 10},
      {"bytes=0-0", 0, 0},
  };
  for (const auto& tc : kCases) {
    SCOPED_TRACE(tc.value);
    std::optional<HttpByteRange> range =
        HttpUtil::ParseFetchSingleRange(tc.value, /*allow_whitespace=*/true);
    ASSERT_TRUE(range.has_value());
    EXPECT_FALSE(range->IsSuffixByteRange());
    EXPECT_EQ(tc.first, range->first_byte_position());
    EXPECT_EQ(tc.last, range->last_byte_position());
  }
}

// Verifies that suffix byte ranges ("bytes=-N") are parsed as suffix
// ranges, with and without allowed HTTP whitespace.
TEST(HttpUtilTest, ParseFetchSingleRangeAcceptsSuffix) {
  static constexpr const char* kCases[] = {
      "bytes=-5",
      // Whitespace variant: Fetch allows HTTP tab/space around '-'.
      "bytes=-\t 5",
  };
  for (const char* value : kCases) {
    SCOPED_TRACE(value);
    std::optional<HttpByteRange> range =
        HttpUtil::ParseFetchSingleRange(value, /*allow_whitespace=*/true);
    ASSERT_TRUE(range.has_value());
    EXPECT_TRUE(range->IsSuffixByteRange());
    EXPECT_EQ(5, range->suffix_length());
  }
}

// Verifies that open-ended byte ranges ("bytes=N-") are parsed as
// right-unbounded ranges, with and without allowed HTTP whitespace.
TEST(HttpUtilTest, ParseFetchSingleRangeAcceptsOpenEnded) {
  static constexpr const char* kCases[] = {
      "bytes=5-",
      // Whitespace variants: Fetch allows HTTP tab/space around '=' and '-'.
      "bytes= 5 - ",
      "bytes \t =\t 5-",
  };
  for (const char* value : kCases) {
    SCOPED_TRACE(value);
    std::optional<HttpByteRange> range =
        HttpUtil::ParseFetchSingleRange(value, /*allow_whitespace=*/true);
    ASSERT_TRUE(range.has_value());
    EXPECT_FALSE(range->IsSuffixByteRange());
    EXPECT_EQ(5, range->first_byte_position());
    EXPECT_FALSE(range->HasLastBytePosition());
  }
}

// Verifies that optional HTTP tab/space around '=' and '-' is accepted
// only when `allow_whitespace` is true, and rejected otherwise.
TEST(HttpUtilTest, ParseFetchSingleRangeAcceptsWhitespaceWhenAllowed) {
  static constexpr const char* kCases[] = {
      "bytes =5-10", "bytes= 5-10",    "bytes=5 -10",
      "bytes=5- 10", "bytes=5\t-\t10",
  };
  for (const char* value : kCases) {
    SCOPED_TRACE(value);
    // Spec-allowed whitespace is accepted only when enabled.
    std::optional<HttpByteRange> range =
        HttpUtil::ParseFetchSingleRange(value, /*allow_whitespace=*/true);
    ASSERT_TRUE(range.has_value());
    EXPECT_EQ(5, range->first_byte_position());
    EXPECT_EQ(10, range->last_byte_position());
    EXPECT_FALSE(
        HttpUtil::ParseFetchSingleRange(value, /*allow_whitespace=*/false)
            .has_value());
  }
}

// Verifies boundary values: a zero-start open-ended range and a
// single-byte suffix range both parse successfully.
TEST(HttpUtilTest, ParseFetchSingleRangeAcceptsBoundaryValues) {
  // "bytes=0-": open-ended range starting at byte 0.
  std::optional<HttpByteRange> open_ended =
      HttpUtil::ParseFetchSingleRange("bytes=0-", /*allow_whitespace=*/true);
  ASSERT_TRUE(open_ended.has_value());
  EXPECT_FALSE(open_ended->IsSuffixByteRange());
  EXPECT_EQ(0, open_ended->first_byte_position());
  EXPECT_FALSE(open_ended->HasLastBytePosition());

  // "bytes=-1": suffix range of the last single byte.
  std::optional<HttpByteRange> suffix =
      HttpUtil::ParseFetchSingleRange("bytes=-1", /*allow_whitespace=*/true);
  ASSERT_TRUE(suffix.has_value());
  EXPECT_TRUE(suffix->IsSuffixByteRange());
  EXPECT_EQ(1, suffix->suffix_length());
}

// Verifies that valid ranges without whitespace still parse in strict
// mode (`allow_whitespace=false`).
TEST(HttpUtilTest, ParseFetchSingleRangeAcceptsValidRangesInStrictMode) {
  static constexpr const char* kCases[] = {
      "bytes=5-10",
      "bytes=5-",
      "bytes=-5",
  };
  for (const char* value : kCases) {
    SCOPED_TRACE(value);
    EXPECT_TRUE(
        HttpUtil::ParseFetchSingleRange(value, /*allow_whitespace=*/false)
            .has_value());
  }
}

// Verifies that overflowing range values saturate to int64_t max rather than
// being rejected
TEST(HttpUtilTest, ParseFetchSingleRangeSaturatesOverflow) {
  constexpr int64_t kMax = std::numeric_limits<int64_t>::max();

  // Overflowing end with a valid start: bounded range [5, max].
  std::optional<HttpByteRange> bounded = HttpUtil::ParseFetchSingleRange(
      "bytes=5-999999999999999999999", /*allow_whitespace=*/true);
  ASSERT_TRUE(bounded.has_value());
  EXPECT_FALSE(bounded->IsSuffixByteRange());
  EXPECT_EQ(5, bounded->first_byte_position());
  EXPECT_EQ(kMax, bounded->last_byte_position());

  // Overflowing start, open-ended: right-unbounded range [max, ).
  std::optional<HttpByteRange> open_ended = HttpUtil::ParseFetchSingleRange(
      "bytes=999999999999999999999-", /*allow_whitespace=*/true);
  ASSERT_TRUE(open_ended.has_value());
  EXPECT_FALSE(open_ended->IsSuffixByteRange());
  EXPECT_EQ(kMax, open_ended->first_byte_position());
  EXPECT_FALSE(open_ended->HasLastBytePosition());

  // Overflowing suffix: suffix range of int64_t max length.
  std::optional<HttpByteRange> suffix = HttpUtil::ParseFetchSingleRange(
      "bytes=-999999999999999999999", /*allow_whitespace=*/true);
  ASSERT_TRUE(suffix.has_value());
  EXPECT_TRUE(suffix->IsSuffixByteRange());
  EXPECT_EQ(kMax, suffix->suffix_length());
}

}  // namespace net
