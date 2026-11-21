// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include <array>
#include <limits>

#include "net/cookies/cookie_inclusion_status_rust_ffi.h"
#include "net/cookies/cookie_inclusion_status_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::cookie_inclusion_status_rust_test {
TEST(CookieInclusionStatusRustTest, AllReasonPairsAndWarningCombinations) {
  for (int e = 0; e < 30; ++e) {
    for (int e2 = 0; e2 < 30; ++e2) {
      for (unsigned warnings = 0; warnings < 32; ++warnings) {
        auto a = *Components<Rust>(0, warnings | (1 << 12) | (1 << 5), 5);
        auto b = *Components<Original>(0, warnings | (1 << 12) | (1 << 5), 5);
        for (int reason : {e, e2}) {
          a.AddExclusionReason(static_cast<Rust::ExclusionReason>(reason));
          b.AddExclusionReason(static_cast<Original::ExclusionReason>(reason));
          ASSERT_TRUE(Equal(a, b));
        }
        a.RemoveExclusionReason(static_cast<Rust::ExclusionReason>(e2));
        b.RemoveExclusionReason(static_cast<Original::ExclusionReason>(e2));
        ASSERT_TRUE(Equal(a, b));
        a.MaybeClearSameSiteWarning();
        b.MaybeClearSameSiteWarning();
        ASSERT_TRUE(Equal(a, b));
      }
    }
  }
}
TEST(CookieInclusionStatusRustTest, PolicyPartitionsAndEveryTransition) {
  constexpr std::array exclusion_ids = {0, 5, 6, 7, 8, 24};
  constexpr std::array warning_ids = {0, 1, 2, 4, 5, 12};
  for (unsigned e = 0; e < 64; ++e) {
    uint64_t exclusions = 0;
    for (unsigned bit = 0; bit < 6; ++bit) {
      if (e & (1 << bit)) {
        exclusions |= uint64_t{1} << exclusion_ids[bit];
      }
    }
    if (exclusions != (1 << 24)) {
      exclusions &= ~(uint64_t{1} << 24);
    }
    for (unsigned w = 0; w < 64; ++w) {
      uint64_t warnings = 0;
      for (unsigned bit = 0; bit < 6; ++bit) {
        if (w & (1 << bit)) {
          warnings |= uint64_t{1} << warning_ids[bit];
        }
      }
      for (int x : {0, 1, 5, -1, std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::max()}) {
        for (unsigned op = 0; op < 13; ++op) {
          auto a = *Components<Rust>(exclusions, warnings, exclusions ? 0 : x);
          auto b =
              *Components<Original>(exclusions, warnings, exclusions ? 0 : x);
          Step(a, op, x, (uint64_t{1} << 24) | (uint64_t{1} << 7));
          Step(b, op, x, (uint64_t{1} << 24) | (uint64_t{1} << 7));
          ASSERT_TRUE(Equal(a, b)) << e << "," << w << "," << x << "," << op;
        }
      }
    }
  }
}
TEST(CookieInclusionStatusRustTest, FullWarningPowersetAndExtremeBitmasks) {
  for (uint64_t warnings = 0; warnings < (1 << 14); ++warnings) {
    ASSERT_TRUE(DeserializeEqual(0, warnings, static_cast<int>(warnings % 6)));
    ASSERT_TRUE(DeserializeEqual((1 << 24) | (1 << 7), warnings, 0));
    ASSERT_TRUE(DeserializeEqual(1 << 24, warnings, 0));
  }
  for (uint64_t mask :
       {uint64_t{0}, ~uint64_t{0}, uint64_t{1} << 63, uint64_t{1} << 30,
        uint64_t{1} << 14, uint64_t{0xaaaaaaaaaaaaaaaa},
        uint64_t{0x5555555555555555}}) {
    for (int exemption :
         {0, 1, 2, 3, 4, 5, 6, -1, std::numeric_limits<int>::min(),
          std::numeric_limits<int>::max()}) {
      ASSERT_TRUE(DeserializeEqual(mask, mask, exemption));
      ASSERT_TRUE(DeserializeEqual(0, mask, exemption));
    }
  }
}
TEST(CookieInclusionStatusRustTest, WarningOrderAndStreamFormatting) {
  for (int e = 0; e < 30; ++e) {
    for (int w = 0; w < 14; ++w) {
      Rust a;
      Original b;
      a.AddExclusionReason(static_cast<Rust::ExclusionReason>(e));
      b.AddExclusionReason(static_cast<Original::ExclusionReason>(e));
      a.AddWarningReason(static_cast<Rust::WarningReason>(w));
      b.AddWarningReason(static_cast<Original::WarningReason>(w));
      ASSERT_TRUE(Equal(a, b));
      std::ostringstream stream_a, stream_b;
      PrintTo(a, &stream_a);
      PrintTo(b, &stream_b);
      EXPECT_EQ(stream_a.str(), stream_b.str());
    }
  }
}
TEST(CookieInclusionStatusRustTest, NonMatchingComparisons) {
  for (int i = 0; i < 30; ++i) {
    auto a = *Components<Rust>(uint64_t{1} << i, 1, 0);
    auto b = *Components<Original>(uint64_t{1} << i, 1, 0);
    EXPECT_FALSE(a == Rust());
    EXPECT_FALSE(b == Original());
    EXPECT_FALSE(a.HasExactlyExclusionReasonsForTesting({}));
    EXPECT_FALSE(b.HasExactlyExclusionReasonsForTesting({}));
    EXPECT_FALSE(a.HasExactlyWarningReasonsForTesting({}));
    EXPECT_FALSE(b.HasExactlyWarningReasonsForTesting({}));
  }
}
TEST(CookieInclusionStatusRustTest, FatalInvalidAddsAndFactoryDiagnostics) {
  for (int reason : {-1, 30, std::numeric_limits<int>::min(),
                     std::numeric_limits<int>::max()}) {
    EXPECT_DEATH_IF_SUPPORTED(
        {
          Rust a;
          a.AddExclusionReason(static_cast<Rust::ExclusionReason>(reason));
        },
        "Check failed");
    EXPECT_DEATH_IF_SUPPORTED(
        {
          Original a;
          a.AddExclusionReason(static_cast<Original::ExclusionReason>(reason));
        },
        "Check failed");
  }
  for (int reason : {-1, 14, std::numeric_limits<int>::min(),
                     std::numeric_limits<int>::max()}) {
    EXPECT_DEATH_IF_SUPPORTED(
        {
          Rust a;
          a.AddWarningReason(static_cast<Rust::WarningReason>(reason));
        },
        "Check failed");
    EXPECT_DEATH_IF_SUPPORTED(
        {
          Original a;
          a.AddWarningReason(static_cast<Original::WarningReason>(reason));
        },
        "Check failed");
  }
  EXPECT_DEATH_IF_SUPPORTED(
      Rust::MakeFromReasonsForTesting(
          {Rust::ExclusionReason::EXCLUDE_UNKNOWN_ERROR,
           Rust::ExclusionReason::EXCLUDE_THIRD_PARTY_PHASEOUT}),
      "could not be applied");
  EXPECT_DEATH_IF_SUPPORTED(
      Original::MakeFromReasonsForTesting(
          {Original::ExclusionReason::EXCLUDE_UNKNOWN_ERROR,
           Original::ExclusionReason::EXCLUDE_THIRD_PARTY_PHASEOUT}),
      "could not be applied");
  EXPECT_DEATH_IF_SUPPORTED(Rust::MakeFromReasonsForTesting(
                                {Rust::ExclusionReason::EXCLUDE_UNKNOWN_ERROR},
                                {}, Rust::ExemptionReason::kScheme),
                            "could not be applied");
  EXPECT_DEATH_IF_SUPPORTED(
      Original::MakeFromReasonsForTesting(
          {Original::ExclusionReason::EXCLUDE_UNKNOWN_ERROR}, {},
          Original::ExemptionReason::kScheme),
      "could not be applied");
}
TEST(CookieInclusionStatusRustTest, AbiGuardsAndPrivateCoreOperations) {
  CookieInclusionStatusRustState s{(uint64_t{1} << 30) - 1, (1 << 14) - 1, 5,
                                   0};
  auto length = cookie_inclusion_status_rust_debug(s, nullptr, 0);
  EXPECT_GT(length, 0u);
  EXPECT_LT(length, 4096u);
  EXPECT_EQ(length, cookie_inclusion_status_rust_debug(s, nullptr, 9));
  std::array<uint8_t, 4096> full{};
  EXPECT_EQ(length,
            cookie_inclusion_status_rust_debug(s, full.data(), full.size()));
  std::array<uint8_t, 4> short_buffer{};
  EXPECT_EQ(length, cookie_inclusion_status_rust_debug(s, short_buffer.data(),
                                                       short_buffer.size()));
  EXPECT_EQ(short_buffer[0], full[0]);
  auto result = cookie_inclusion_status_rust_apply(s, 9, 0, ~uint64_t{0});
  EXPECT_EQ(result.exclusions, 0u);
  result = cookie_inclusion_status_rust_apply(s, 5, 0, 0);
  EXPECT_EQ(result.exclusions & (1 << 24), 0u);
  EXPECT_EQ(cookie_inclusion_status_rust_apply(s, 99, 0, 0).invalid, 1u);
  EXPECT_FALSE(cookie_inclusion_status_rust_query(s, 99, 0, 0));
  EXPECT_EQ(cookie_inclusion_status_rust_apply(s, 0, -1, 0).invalid, 1u);
  EXPECT_EQ(cookie_inclusion_status_rust_apply(s, 6, 14, 0).invalid, 1u);
  CookieInclusionStatusRustState included{0, (1 << 14) - 1, 0, 0};
  EXPECT_EQ(cookie_inclusion_status_rust_apply(included, 5, 0, 0).warnings,
            included.warnings);
  auto other = s;
  ++other.warnings;
  EXPECT_FALSE(cookie_inclusion_status_rust_same(s, other));
  other = s;
  --other.exemption;
  EXPECT_FALSE(cookie_inclusion_status_rust_same(s, other));
}
}  // namespace net::cookie_inclusion_status_rust_test
