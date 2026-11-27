// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/dcheck_is_on.h"
#include "net/http/http_byte_range_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
TEST(HttpByteRangeRustTest, ExhaustiveEdgeStateSequences) {
  using namespace http_byte_range_rust_test;
  constexpr std::array<int64_t, 9> values = {
      INT64_MIN, INT64_MIN + 1, -2, -1, 0, 1, 2, INT64_MAX - 1, INT64_MAX};
  uint64_t excluded = 0;
  for (auto f : values) {
    for (auto l : values) {
      for (auto s : values) {
        for (auto size : values) {
          HttpByteRange a;
          Original b;
          a.set_first_byte_position(f);
          b.set_first_byte_position(f);
          a.set_last_byte_position(l);
          b.set_last_byte_position(l);
          a.set_suffix_length(s);
          b.set_suffix_length(s);
          Equal(a, b);
          if (UndefinedCompute(b, size)) {
            ++excluded;
            continue;
          }
          EXPECT_EQ(a.ComputeBounds(size), b.ComputeBounds(size));
          Equal(a, b);
          a.set_first_byte_position(0);
          b.set_first_byte_position(0);
          a.set_last_byte_position(-1);
          b.set_last_byte_position(-1);
          a.set_suffix_length(-1);
          b.set_suffix_length(-1);
          EXPECT_EQ(a.ComputeBounds(10), b.ComputeBounds(10));
          Equal(a, b);
          EXPECT_EQ(a.ComputeBounds(0), b.ComputeBounds(0));
          Equal(a, b);
        }
      }
    }
  }
  EXPECT_GT(excluded, 0u);
}
TEST(HttpByteRangeRustTest, FailedAttemptLatchesAndNegativeSizeDoesNot) {
  HttpByteRange r;
  EXPECT_FALSE(r.ComputeBounds(-1));
  r.set_first_byte_position(-2);
  EXPECT_FALSE(r.ComputeBounds(0));
  EXPECT_TRUE(http_byte_range_rust_test::Computed(r));
  r.set_first_byte_position(0);
  EXPECT_FALSE(r.ComputeBounds(10));
  r = HttpByteRange();
  EXPECT_TRUE(r.ComputeBounds(0));
  EXPECT_EQ(0, r.first_byte_position());
  EXPECT_EQ(-1, r.last_byte_position());
}
TEST(HttpByteRangeRustTest, NegativeNonSentinelSuffixAndContradictions) {
  auto r = HttpByteRange::Bounded(0, 1);
  r.set_suffix_length(-2);
  EXPECT_TRUE(r.IsValid());
  EXPECT_TRUE(r.IsSuffixByteRange());
  EXPECT_EQ("bytes=--2", r.GetHeaderValue());
  EXPECT_TRUE(r.ComputeBounds(0));
  EXPECT_EQ(2, r.first_byte_position());
  EXPECT_EQ(-1, r.last_byte_position());
  auto min = HttpByteRange::Bounded(0, 0);
  min.set_suffix_length(INT64_MIN);
  EXPECT_EQ("bytes=--9223372036854775808", min.GetHeaderValue());
}
TEST(HttpByteRangeRustTest, InvalidHeaderDiagnostic) {
  HttpByteRange r;
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(r.GetHeaderValue(), "IsValid");
#else
  GTEST_SKIP() << "DCHECK is disabled";
#endif
}
TEST(HttpByteRangeRustTest, GuidedStateDifferential) {
  const auto counts =
      http_byte_range_rust_test::Exercise(0x728aef691ff12537ULL, 10000);
  EXPECT_EQ(10000u, counts.states);
}
}  // namespace net
