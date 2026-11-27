// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <random>

#include "net/spdy/alps_decoder_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::alps_rust_test {
TEST(AlpsRustTest, BoundariesFeaturesFlagsAndTruncations) {
  const auto valid =
      Frame(0x89, 0, 0, Pair("https://example.test", "sec-ch-ua"));
  const auto settings =
      Frame(4, 0, 0, Setting(0xffff, 1) + Setting(0, 3) + Setting(0xffff, 99));
  const std::array<std::string, 12> inputs = {
      "",
      valid,
      settings,
      valid + settings,
      Frame(0x89, 0, 0,
            Pair(std::string("a\0\xff", 3), std::string("\x80\0z", 3))),
      Frame(0x89, 0, 0, Pair("", "") + "x") + valid,
      Frame(4, 1, 0, "") + valid,
      Frame(0, 1, 1, "x") + Frame(4, 1, 0, ""),
      Frame(4, 1, 0, "x"),
      Frame(4, 0, 1, Setting(1, 1)),
      Frame(0x89, 1, 1, "x") + Frame(4, 1, 0, ""),
      Frame(0x2a, 255, 8, "\0\x80")};
  for (bool enabled : {false, true}) {
    for (const auto& input : inputs) {
      Compare(input, enabled, input.size() + 1);
      for (size_t length = 0; length <= input.size(); ++length) {
        Compare(std::string_view(input).substr(0, length), enabled, length + 1);
        Compare(input, enabled, length);
      }
    }
    for (unsigned type = 0; type < 256; ++type) {
      for (unsigned flags = 0; flags < 256; ++flags) {
        Compare(Frame(type, flags, type & 1, ""), enabled, 10);
      }
    }
  }
}
TEST(AlpsRustTest, OriginalSignedCharLengthsAndLargePayload) {
  static_assert(static_cast<char>(0xff) < 0,
                "This platform validates signed-char baseline");
  for (size_t length :
       {0u, 1u, 127u, 128u, 255u, 256u, 32767u, 32768u, 65535u}) {
    auto input =
        Frame(0x89, 0, 0,
              Pair(std::string(length, '\xff'), std::string(length, '\x80')));
    // The real QUICHE default maximum frame size also participates in this API.
    Compare(input, true, input.size() + 1);
    Compare(input, true, 9 + std::min(length, input.size() - 9));
  }
  Compare(
      Frame(0x89, 0, 0, Pair(std::string(8000, 'x'), std::string(8000, 'y'))),
      true, 16010);
}
TEST(AlpsRustTest, ExpectedOutputsDiagnosticsAndInputAlias) {
  base::test::ScopedFeatureList feature;
  feature.InitAndEnableFeature(features::kAlpsParsing);
  AlpsDecoder decoder;
  base::HistogramTester histogram;
  EXPECT_EQ(AlpsDecoder::Error::kNoError,
            decoder.Decode(Frame(4, 0, 0, Setting(7, 2) + Setting(7, 9))));
  EXPECT_EQ(decoder.GetSettings().at(7), 9u);
  EXPECT_EQ(1, decoder.settings_frame_count());
  auto frame = Frame(0x89, 0, 0, Pair("", "") + "x");
  EXPECT_EQ(AlpsDecoder::Error::kNoError, decoder.Decode(frame));
  ASSERT_EQ(1u, decoder.GetAcceptCh().size());
  EXPECT_TRUE(decoder.GetAcceptCh()[0].origin.empty());
  histogram.ExpectBucketCount(kHistogram, 7, 1);
  // An empty span aliases previously returned empty storage without reading it.
  EXPECT_EQ(AlpsDecoder::Error::kNoError,
            decoder.Decode(decoder.GetAcceptCh()[0].origin));
  histogram.ExpectBucketCount(kHistogram, 7, 2);
  EXPECT_EQ(AlpsDecoder::Error::kNoError,
            decoder.Decode(Frame(0x89, 0, 0, Pair("z", "v"))));
  histogram.ExpectBucketCount(kHistogram, 7, 3);
  // A long value containing a SETTINGS frame remains readable after output
  // observation and repeated Decode. No extension mutation aliases its bytes.
  std::string nested = Frame(4, 0, 0, Setting(44, 55));
  nested += Frame(42, 0, 0, std::string(50, 'z'));
  AlpsDecoder alias;
  ASSERT_EQ(AlpsDecoder::Error::kNoError,
            alias.Decode(Frame(0x89, 0, 0, Pair("a", nested))));
  EXPECT_EQ(AlpsDecoder::Error::kNoError,
            alias.Decode(alias.GetAcceptCh()[0].value));
  EXPECT_EQ(55u, alias.GetSettings().at(44));
}
TEST(AlpsRustTest, MillionDeterministicWholeApiInputs) {
  std::mt19937 random(0xa1b5dec0);
  for (size_t iteration = 0; iteration < 1000000; ++iteration) {
    std::string bytes(1 + random() % 257, '\0');
    for (char& byte : bytes) {
      byte = static_cast<char>(random());
    }
    Fuzz(bytes);
  }
}
}  // namespace net::alps_rust_test
