// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/test/gtest_util.h"
#include "net/base/load_flags_to_string.h"
#include "net/base/net_diagnostics_debug_formatting_ffi.h"
#include "net/base/net_diagnostics_load_flags_reference.h"
#include "net/base/net_diagnostics_privacy_mode_reference.h"
#include "net/base/privacy_mode.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

constexpr uint32_t kDefinedLoadFlagMask = (1u << 20) - 1;

void ExpectFlagApiMatchesFrozenCpp(int flags) {
  EXPECT_EQ(net_diagnostics_load_flags_reference::LoadFlagsToString(flags),
            LoadFlagsToString(flags))
      << "flags=" << flags;
}

TEST(NetDiagnosticsDebugFormattingTest, ExhaustiveDefinedLoadFlagCombinations) {
  for (uint32_t flags = 0; flags <= kDefinedLoadFlagMask; ++flags) {
    ExpectFlagApiMatchesFrozenCpp(static_cast<int>(flags));
  }
}

TEST(NetDiagnosticsDebugFormattingTest, MillionBoundedPublicApiDifferentials) {
  uint32_t state = 0x4d595df4u;
  for (int i = 0; i < 1'000'000; ++i) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    ExpectFlagApiMatchesFrozenCpp(
        static_cast<int>(state & kDefinedLoadFlagMask));
  }
}

TEST(NetDiagnosticsDebugFormattingTest, InvalidFlagsKeepCheckDiagnostics) {
  EXPECT_CHECK_DEATH_WITH(LoadFlagsToString(-1), "Invalid load flags: -1");
  EXPECT_CHECK_DEATH_WITH(LoadFlagsToString(1 << 30),
                          "Invalid load flags: 1073741824");
  EXPECT_CHECK_DEATH_WITH(LoadFlagsToString(std::numeric_limits<int>::min()),
                          "Invalid load flags: -2147483648");
}

TEST(NetDiagnosticsDebugFormattingTest,
     PrivacyLabelsMatchFrozenCppForEveryMode) {
  for (int mode = PRIVACY_MODE_DISABLED;
       mode <= PRIVACY_MODE_ENABLED_PARTITIONED_STATE_ALLOWED; ++mode) {
    const auto privacy_mode = static_cast<PrivacyMode>(mode);
    EXPECT_STREQ(
        net_diagnostics_privacy_mode_reference::PrivacyModeToDebugString(mode),
        PrivacyModeToDebugString(privacy_mode));
  }
}

TEST(NetDiagnosticsDebugFormattingTest, InvalidPrivacyModesKeepNotReached) {
  EXPECT_NOTREACHED_DEATH(
      PrivacyModeToDebugString(static_cast<PrivacyMode>(-1)));
  EXPECT_NOTREACHED_DEATH(
      PrivacyModeToDebugString(static_cast<PrivacyMode>(4)));
}

TEST(NetDiagnosticsDebugFormattingTest, RustFfiOwnershipAndStaticLifetime) {
  for (int i = 0; i < 100'000; ++i) {
    const int flags = static_cast<int>(
        (static_cast<uint32_t>(i) * 2654435761u) & kDefinedLoadFlagMask);
    size_t length = std::numeric_limits<size_t>::max();
    uint8_t* bytes = net_diagnostics_load_flags_format(flags, &length);
    ASSERT_NE(bytes, nullptr);
    const std::string expected =
        net_diagnostics_load_flags_reference::LoadFlagsToString(flags);
    // SAFETY: This view is bounded by the exact live allocation returned from
    // the Rust formatter and is copied before the matching Rust free call.
    const std::string actual = UNSAFE_BUFFERS(
        std::string(reinterpret_cast<const char*>(bytes), length));
    EXPECT_EQ(expected, actual);
    net_diagnostics_load_flags_free(bytes, length);
  }

  size_t unchanged_length = 0xfeed;
  EXPECT_EQ(net_diagnostics_load_flags_format(1 << 30, &unchanged_length),
            nullptr);
  EXPECT_EQ(unchanged_length, 0xfeedu);
  EXPECT_EQ(net_diagnostics_load_flags_format(0, nullptr), nullptr);

  for (int mode = 0; mode <= PRIVACY_MODE_ENABLED_PARTITIONED_STATE_ALLOWED;
       ++mode) {
    const char* first = net_diagnostics_privacy_mode(mode);
    const char* second = net_diagnostics_privacy_mode(mode);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, second);
    EXPECT_STREQ(
        first,
        net_diagnostics_privacy_mode_reference::PrivacyModeToDebugString(mode));
  }
  EXPECT_EQ(net_diagnostics_privacy_mode(-1), nullptr);
  EXPECT_EQ(net_diagnostics_privacy_mode(4), nullptr);
}

TEST(NetDiagnosticsDebugFormattingTest,
     DirectWriterCapacityAndFailureContract) {
  const std::string expected =
      net_diagnostics_load_flags_reference::LoadFlagsToString(3);
  const size_t length = net_diagnostics_load_flags_length(3);
  ASSERT_EQ(length, expected.size());
  ASSERT_GT(length, 0u);
  std::array<uint8_t, 128> output;
  output.fill(0xa5);
  const auto before = output;
  EXPECT_FALSE(net_diagnostics_load_flags_write(3, output.data(), length - 1));
  EXPECT_EQ(output, before);
  EXPECT_FALSE(
      net_diagnostics_load_flags_write(-1, output.data(), output.size()));
  EXPECT_EQ(output, before);
  ASSERT_TRUE(
      net_diagnostics_load_flags_write(3, output.data(), output.size()));
  // SAFETY: Only the measured prefix of this live initialized array is copied.
  const std::string actual = UNSAFE_BUFFERS(
      std::string(reinterpret_cast<const char*>(output.data()), length));
  EXPECT_EQ(actual, expected);
  for (size_t index = length; index < output.size(); ++index) {
    EXPECT_EQ(output[index], 0xa5);
  }
  EXPECT_FALSE(net_diagnostics_load_flags_write(3, nullptr, 1));
  EXPECT_FALSE(net_diagnostics_load_flags_write(
      3, nullptr, std::numeric_limits<size_t>::max()));
}

}  // namespace
}  // namespace net
