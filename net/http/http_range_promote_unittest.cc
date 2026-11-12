// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "base/compiler_specific.h"
#include "base/strings/string_number_conversions.h"
#include "net/http/http_range_ffi.h"
#include "net/http/http_range_promote_reference.h"
#include "net/http/http_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
namespace reference = http_range_promote_internal::test;

void Equal(const HttpByteRange& a, const HttpByteRange& b) {
  EXPECT_EQ(a.first_byte_position(), b.first_byte_position());
  EXPECT_EQ(a.last_byte_position(), b.last_byte_position());
  EXPECT_EQ(a.suffix_length(), b.suffix_length());
}

void Compare(std::string_view input) {
  for (bool prepopulated : {false, true}) {
    std::vector<HttpByteRange> a;
    if (prepopulated) {
      a.push_back(HttpByteRange::Bounded(12, 34));
    }
    auto b = a;
    EXPECT_EQ(HttpUtil::ParseRangeHeader(input, &a),
              reference::ParseRangeHeader(input, &b));
    ASSERT_EQ(a.size(), b.size());
    for (size_t i = 0; i < a.size(); ++i) {
      Equal(a[i], b[i]);
    }
  }
  for (bool whitespace : {false, true}) {
    const auto a = HttpUtil::ParseFetchSingleRange(input, whitespace);
    const auto b = reference::ParseFetchSingleRange(input, whitespace);
    ASSERT_EQ(a.has_value(), b.has_value());
    if (a) {
      Equal(*a, *b);
    }
  }
  constexpr std::array<std::array<size_t, 3>, 5> layouts = {
      {{0, 1, 2}, {0, 0, 2}, {0, 1, 0}, {0, 1, 1}, {0, 0, 0}}};
  for (auto layout : layouts) {
    std::array<int64_t, 3> a = {7, 8, 9};
    auto b = a;
    EXPECT_EQ(HttpUtil::ParseContentRangeHeaderFor206(
                  input, &a[layout[0]], &a[layout[1]], &a[layout[2]]),
              reference::ParseContentRangeHeaderFor206(
                  input, &b[layout[0]], &b[layout[1]], &b[layout[2]]));
    EXPECT_EQ(a, b);
  }
}

TEST(HttpRangePromoteTest, GrammarQuirksAndPartialAppend) {
  for (auto input : {"",
                     "bytes=",
                     "bytes=,,, \t",
                     "bytes=0-1,broken",
                     "bytes=0-1,,5-6,broken",
                     "bytes=+1-+2",
                     "bytes=-+50",
                     "bytes=1--1",
                     "bytes=1--2",
                     "ByTeS = 1 - 2 , -3",
                     "bytes=\"1,2-3\"",
                     "bytes=0-1,\"x\\\",x-3",
                     "bytes=0-1,\"",
                     "bytes=-0",
                     "bytes=10-5",
                     "bytes=-",
                     "bytes=0-1 ",
                     "bytes 9-1/2",
                     "bytes +0-+1/+2",
                     "bytes 0-1/2",
                     "bytes\t0-1/2",
                     " bytes\t 0-1/2 ",
                     "bytes 0-1/*"}) {
    SCOPED_TRACE(input);
    Compare(input);
  }
  std::vector<HttpByteRange> ranges;
  EXPECT_FALSE(HttpUtil::ParseRangeHeader("bytes=1-2, broken", &ranges));
  ASSERT_EQ(ranges.size(), 1u);
  EXPECT_EQ(ranges[0].first_byte_position(), 1);
  EXPECT_TRUE(HttpUtil::ParseRangeHeader("bytes=,,,", &ranges));
  ASSERT_EQ(ranges.size(), 1u);
}

TEST(HttpRangePromoteTest, InvalidListHeaderMayHaveNullOutput) {
  for (std::string_view input : {"", "items=0-1", "bytes", "bytes=x", "bytes=-",
                                 "bytes=1-0", "bytes=0-x"}) {
    EXPECT_FALSE(reference::ParseRangeHeader(input, nullptr));
    EXPECT_FALSE(HttpUtil::ParseRangeHeader(input, nullptr));
  }
}

TEST(HttpRangePromoteTest, NumericBoundariesAndNonAscii) {
  for (auto digits :
       {"0", "-0", "+0", "00", "-1", "+1", "9223372036854775807",
        "9223372036854775808", "-9223372036854775808", "-9223372036854775809",
        "999999999999999999999999", "x", "+-1", "-+1", "\n1", "1\n"}) {
    Compare(std::string("bytes=") + digits + "-100");
    Compare(std::string("bytes=1-") + digits);
    Compare(std::string("bytes ") + digits + "-100/200");
    Compare(std::string("bytes 1-") + digits + "/200");
    Compare(std::string("bytes 1-2/") + digits);
  }
  Compare(std::string("bytes=1-2\0", 11));
  Compare(std::string("bytes 0-1/2\0", 12));
}

TEST(HttpRangePromoteTest, EveryTruncation) {
  for (std::string input :
       {"bytes=0-100,200-300", "bytes 0-100/200", "bytes = \t5 - 10",
        "bytes=0-999999999999999999999", "bytes=0-1,\"2,3-4\""}) {
    for (size_t length = 0; length <= input.size(); ++length) {
      Compare(std::string_view(input).substr(0, length));
    }
  }
}

TEST(HttpRangePromoteTest, EveryOctetAtEachGrammarPosition) {
  for (std::string input :
       {"bytes=0-100,200-300", "bytes 0-100/200", "bytes = 5 - 10"}) {
    for (size_t index = 0; index < input.size(); ++index) {
      const char previous = input[index];
      for (unsigned byte = 0; byte < 256; ++byte) {
        input[index] = static_cast<char>(byte);
        Compare(input);
      }
      input[index] = previous;
    }
  }
}

TEST(HttpRangePromoteTest, EveryOneAndTwoByteInput) {
  std::array<char, 2> bytes;
  for (unsigned a = 0; a < 256; ++a) {
    bytes[0] = static_cast<char>(a);
    Compare(std::string_view(bytes.data(), 1));
    for (unsigned b = 0; b < 256; ++b) {
      bytes[1] = static_cast<char>(b);
      Compare(std::string_view(bytes.data(), 2));
    }
  }
}

TEST(HttpRangePromoteTest, ManyRangesAndLongFields) {
  std::string ranges = "bytes=";
  for (unsigned i = 0; i < 128; ++i) {
    if (i) {
      ranges += ',';
    }
    ranges += base::NumberToString(i) + "-" + base::NumberToString(i + 1);
  }
  Compare(ranges);
  Compare(ranges + ",broken");
  const std::string zeros(4096, '0'), nines(4096, '9');
  Compare("bytes=" + zeros + "1-" + zeros + "2");
  Compare("bytes=" + nines + "-");
  Compare("bytes 0-" + nines + "/" + nines);
}

TEST(HttpRangePromoteTest, ScalarAliasingRetainsSequentialValidation) {
  int64_t shared, length;
  EXPECT_TRUE(HttpUtil::ParseContentRangeHeaderFor206("bytes 9-1/2", &shared,
                                                      &shared, &length));
  EXPECT_EQ(shared, 1);
  EXPECT_EQ(length, 2);
  int64_t first, last;
  EXPECT_TRUE(HttpUtil::ParseContentRangeHeaderFor206("bytes 0-1/2", &first,
                                                      &last, &first));
  EXPECT_EQ(first, 2);
  EXPECT_EQ(last, 1);
  EXPECT_FALSE(HttpUtil::ParseContentRangeHeaderFor206("bytes 0-1/2", &first,
                                                       &last, &last));
  EXPECT_EQ(first, -1);
  EXPECT_EQ(last, -1);
}

TEST(HttpRangePromoteTest, IntermediateWriteCanRestoreOverlappingInput) {
  std::array<int64_t, 8> actual{}, expected{};
  constexpr std::string_view header = "bytes 49-50/10001";
  static_assert(header.size() == 17);
  // SAFETY: The full literal fits the live 64-byte scalar array. Object bytes
  // may be accessed as char, including the first byte of actual[2].
  UNSAFE_BUFFERS(std::memcpy(actual.data(), header.data(), header.size()));
  expected = actual;
  const auto a = UNSAFE_BUFFERS(std::string_view(
      reinterpret_cast<const char*>(actual.data()), header.size()));
  const auto b = UNSAFE_BUFFERS(std::string_view(
      reinterpret_cast<const char*>(expected.data()), header.size()));
  EXPECT_TRUE(reference::ParseContentRangeHeaderFor206(
      b, &expected[2], &expected[3], &expected[4]));
  EXPECT_TRUE(HttpUtil::ParseContentRangeHeaderFor206(a, &actual[2], &actual[3],
                                                      &actual[4]));
  EXPECT_EQ(actual, expected);
  EXPECT_EQ(actual[2], 49);
  EXPECT_EQ(actual[3], 50);
  EXPECT_EQ(actual[4], 10001);
}

TEST(HttpRangePromoteTest, InputOutputOverlapAcrossAlignedPositions) {
  for (size_t begin = 0; begin < 16; ++begin) {
    for (size_t slot = 0; slot < 12; ++slot) {
      for (unsigned layout = 0; layout < 5; ++layout) {
        std::array<int64_t, 32> a{}, b{};
        constexpr std::string_view header = "bytes +49-50/10001";
        // SAFETY: start+header length stays inside the live scalar-array
        // storage.
        UNSAFE_BUFFERS(std::memcpy(reinterpret_cast<char*>(a.data()) + begin,
                                   header.data(), header.size()));
        b = a;
        const auto av = UNSAFE_BUFFERS(std::string_view(
            reinterpret_cast<const char*>(a.data()) + begin, header.size()));
        const auto bv = UNSAFE_BUFFERS(std::string_view(
            reinterpret_cast<const char*>(b.data()) + begin, header.size()));
        const size_t end = layout == 1 || layout == 4 ? slot : 20;
        const size_t length = layout == 2 || layout == 4 ? slot
                              : layout == 3              ? end
                                                         : 24;
        EXPECT_EQ(HttpUtil::ParseContentRangeHeaderFor206(av, &a[slot], &a[end],
                                                          &a[length]),
                  reference::ParseContentRangeHeaderFor206(
                      bv, &b[slot], &b[end], &b[length]));
        EXPECT_EQ(a, b);
      }
    }
  }
}

TEST(HttpRangePromoteTest, MalformedFieldsWithOverlappingOutputs) {
  for (std::string_view header :
       {"bytes -1/2", "bytes +-1/2", "bytes +x-1/2", "bytes 0-/2",
        "bytes 0-+/2", "bytes 0-+x/2"}) {
    std::array<int64_t, 8> a{}, b{};
    const size_t begin = 17 - header.size();
    // SAFETY: This positions only the final input byte inside a[2], within
    // the live 64-byte scalar array. The leading grammar remains available.
    UNSAFE_BUFFERS(std::memcpy(reinterpret_cast<char*>(a.data()) + begin,
                               header.data(), header.size()));
    b = a;
    const auto av = UNSAFE_BUFFERS(std::string_view(
        reinterpret_cast<const char*>(a.data()) + begin, header.size()));
    const auto bv = UNSAFE_BUFFERS(std::string_view(
        reinterpret_cast<const char*>(b.data()) + begin, header.size()));
    EXPECT_FALSE(
        reference::ParseContentRangeHeaderFor206(bv, &b[2], &b[3], &b[4]));
    EXPECT_FALSE(
        HttpUtil::ParseContentRangeHeaderFor206(av, &a[2], &a[3], &a[4]));
    EXPECT_EQ(a, b);
  }
}

TEST(HttpRangePromoteTest, OwnedListBridgeHandlesEmptyAndPartialResults) {
  using namespace http_range_internal;
  EXPECT_EQ(rustium_http_range_list(nullptr, 0, nullptr), 0);
  constexpr std::string_view empty_list = "bytes=,,,";
  EXPECT_EQ(rustium_http_range_list(
                reinterpret_cast<const uint8_t*>(empty_list.data()),
                empty_list.size(), nullptr),
            kListSyntaxValid);
  constexpr std::string_view prefix = "bytes=1-2,broken";
  RangesBuffer buffer;
  EXPECT_EQ(
      rustium_http_range_list(reinterpret_cast<const uint8_t*>(prefix.data()),
                              prefix.size(), &buffer),
      kListHasEntries);
  const ScopedRangeList owned(buffer);
  ASSERT_EQ(owned.values().size(), 1u);
  EXPECT_EQ(owned.values()[0].first, 1);
  EXPECT_EQ(owned.values()[0].last, 2);
}

TEST(HttpRangePromoteTest, FixedBridgeHandlesNullEmptyAndAliasedStorage) {
  using http_range_internal::ParsedRange;
  EXPECT_FALSE(http_range_internal::rustium_http_range_fetch(nullptr, 0, false,
                                                             nullptr));
  ParsedRange result{7, 8, 9};
  constexpr std::string_view header = "bytes=1-2";
  // SAFETY: The nine-byte header fits the 24-byte live integer record. Its
  // complete input borrow must end before the parsed record overwrites it.
  UNSAFE_BUFFERS(std::memcpy(&result, header.data(), header.size()));
  EXPECT_TRUE(http_range_internal::rustium_http_range_fetch(
      reinterpret_cast<const uint8_t*>(&result), header.size(), false,
      &result));
  EXPECT_EQ(result.first, 1);
  EXPECT_EQ(result.last, 2);
  EXPECT_EQ(result.suffix, -1);
  int64_t first, last, length;
  EXPECT_FALSE(http_range_internal::rustium_http_range_content(
      nullptr, 0, &first, &last, &length));
  EXPECT_EQ(first, -1);
  EXPECT_EQ(last, -1);
  EXPECT_EQ(length, -1);
}
}  // namespace
}  // namespace net
