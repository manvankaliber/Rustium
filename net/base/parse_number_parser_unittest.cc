// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/parse_number.h"
#include "net/base/parse_number_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

namespace reference = parse_number_internal::test;

template <typename T, typename Actual, typename Expected>
void CompareOne(std::string_view input,
                ParseIntFormat format,
                Actual actual,
                Expected expected) {
  for (bool report_error : {false, true}) {
    T a = 23614;
    T b = a;
    ParseIntError ea = static_cast<ParseIntError>(19);
    ParseIntError eb = ea;
    EXPECT_EQ(actual(input, format, &a, report_error ? &ea : nullptr),
              expected(input, format, &b, report_error ? &eb : nullptr));
    EXPECT_EQ(a, b);
    EXPECT_EQ(ea, eb);
  }
}

void Compare(std::string_view input) {
  for (int format = 0; format < 4; ++format) {
    const auto f = static_cast<ParseIntFormat>(format);
    CompareOne<int32_t>(input, f, ParseInt32, reference::ParseInt32);
    CompareOne<int64_t>(input, f, ParseInt64, reference::ParseInt64);
    if ((format & 1) == 0) {
      CompareOne<uint32_t>(input, f, ParseUint32, reference::ParseUint32);
      CompareOne<uint64_t>(input, f, ParseUint64, reference::ParseUint64);
    }
  }
}

TEST(ParseNumberRustTest, NumericBoundariesAndTrailingGarbage) {
  const char* cases[] = {
      "",
      "-",
      "+",
      "0",
      "-0",
      "00",
      "-00",
      "012",
      "-012",
      "1",
      "-1",
      "2147483647",
      "2147483648",
      "2147483649",
      "-2147483648",
      "-2147483649",
      "4294967295",
      "4294967296",
      "9223372036854775807",
      "9223372036854775808",
      "-9223372036854775808",
      "-9223372036854775809",
      "18446744073709551615",
      "18446744073709551616",
      "-18446744073709551616",
      "999999999999999999999999",
      "999999999999999999999999x",
      "-999999999999999999999999x",
      "-x",
      "--1",
  };
  for (std::string input : cases) {
    SCOPED_TRACE(input);
    Compare(input);
    for (char tail : {' ', '\0', '\n', '\xff'}) {
      auto bad = input;
      bad.push_back(tail);
      Compare(bad);
    }
  }
}

TEST(ParseNumberRustTest, EveryOneAndTwoByteInput) {
  std::array<char, 2> bytes;
  for (unsigned a = 0; a < 256; ++a) {
    bytes[0] = static_cast<char>(a);
    Compare(std::string_view(bytes.data(), 1));
    for (unsigned b = 0; b < 256; ++b) {
      bytes[1] = static_cast<char>(b);
      Compare(std::string_view(bytes.data(), bytes.size()));
    }
  }
}

TEST(ParseNumberRustTest, LongZerosOverflowAndMalformedSuffix) {
  for (size_t length : {16u, 32u, 256u, 4096u}) {
    const std::string zeros(length, '0');
    const std::string nines(length, '9');
    for (const auto& value : {zeros, zeros + "123", "-" + zeros + "123", nines,
                              "-" + nines, nines + "z", "-" + nines + "z"}) {
      Compare(value);
    }
  }
}

TEST(ParseNumberRustTest, EveryDecimalValueAroundTypeLimits) {
  for (uint64_t center : {uint64_t{2147483647}, uint64_t{4294967295},
                          uint64_t{9223372036854775807}}) {
    for (uint64_t value = center - 64; value < center + 64; ++value) {
      const auto digits = base::NumberToString(value);
      Compare(digits);
      Compare("-" + digits);
      Compare("00" + digits);
    }
  }
}

TEST(ParseNumberRustTest, MutatedLongDecimalInputs) {
  for (std::string input :
       {"1234567890", "18446744073709551615", "-9223372036854775808"}) {
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

TEST(ParseNumberRustTest, FailureDoesNotTouchNullOutputStorage) {
  for (auto format :
       {ParseIntFormat::NON_NEGATIVE, ParseIntFormat::STRICT_NON_NEGATIVE}) {
    EXPECT_FALSE(ParseInt32("x", format, nullptr));
    EXPECT_FALSE(ParseInt64("x", format, nullptr));
    EXPECT_FALSE(ParseUint32("x", format, nullptr));
    EXPECT_FALSE(ParseUint64("x", format, nullptr));
  }
}

TEST(ParseNumberRustTest, InputMayAliasOutputStorage) {
  int64_t value = 0;
  // SAFETY: Copy three literal bytes into an eight-byte integer object, whose
  // representation may be accessed as bytes. The borrowed view remains live.
  UNSAFE_BUFFERS(std::memcpy(&value, "123", 3));
  const auto input = UNSAFE_BUFFERS(
      std::string_view(reinterpret_cast<const char*>(&value), 3));
  EXPECT_TRUE(ParseInt64(input, ParseIntFormat::NON_NEGATIVE, &value));
  EXPECT_EQ(value, 123);

  auto error = ParseIntError::FAILED_PARSE;
  // SAFETY: One byte fits in this fixed-underlying-type enum's representation.
  UNSAFE_BUFFERS(std::memcpy(&error, "x", 1));
  const auto bad = UNSAFE_BUFFERS(
      std::string_view(reinterpret_cast<const char*>(&error), 1));
  EXPECT_FALSE(ParseInt64(bad, ParseIntFormat::NON_NEGATIVE, &value, &error));
  EXPECT_EQ(error, ParseIntError::FAILED_PARSE);
  EXPECT_EQ(value, 123);
}

TEST(ParseNumberRustTest, LegacyUnknownSignedFormatBehavior) {
  for (int value : {-1, 4, 256, 2147483647}) {
    const auto format = static_cast<ParseIntFormat>(value);
    for (auto input : {"0", "-0", "001", "-001", "-123", "+1", "x",
                       "999999999999999999999"}) {
      CompareOne<int32_t>(input, format, ParseInt32, reference::ParseInt32);
      CompareOne<int64_t>(input, format, ParseInt64, reference::ParseInt64);
    }
  }
}

TEST(ParseNumberRustTest, UnsignedRejectsSignedFormats) {
  uint32_t small;
  uint64_t large;
  EXPECT_DEATH_IF_SUPPORTED(
      (void)ParseUint32("1", ParseIntFormat::OPTIONALLY_NEGATIVE, &small), "");
  EXPECT_DEATH_IF_SUPPORTED(
      (void)ParseUint64("1", ParseIntFormat::STRICT_OPTIONALLY_NEGATIVE,
                        &large),
      "");
}

}  // namespace
}  // namespace net
