// Copyright 2026 The Rustium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/strings/string_split.h"

#include <cstdint>
#include <string_view>
#include <vector>

#include "base/strings/string_split_rust.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
TEST(StringSplitRustBridgeTest, PartialViewsRetainNullAndBorrowedEmptyIdentity) {
  constexpr std::string_view input = ":value,key:,broken,tail:v";
  StringViewPairs output = {{"stale", "stale"}};
  EXPECT_FALSE(SplitStringIntoKeyValueViewPairsUsingSubstr(input, ':', ",", &output));
  ASSERT_EQ(output.size(), 4u);
  EXPECT_TRUE(output[0].first.empty());
  EXPECT_EQ(output[0].first.data(), input.data());
  EXPECT_EQ(output[0].second, "value");
  EXPECT_EQ(output[1].first, "key");
  EXPECT_EQ(output[1].second.data(), nullptr);
  EXPECT_EQ(output[2].first.data(), nullptr);
  EXPECT_EQ(output[2].second.data(), nullptr);
  EXPECT_EQ(output[3].first, "tail");
  EXPECT_EQ(output[3].second, "v");
  EXPECT_EQ(output[3].first.data(), input.data() + 19);
}
TEST(StringSplitRustBridgeTest, NullEmptyAndEmbeddedNulInputs) {
  EXPECT_FALSE(SplitStringOnce(std::string_view(), ':').has_value());
  constexpr char data[] = {'a', '\0', 'b', ':', 'c'};
  auto pair = SplitStringOnce(std::string_view(data, sizeof(data)), ':');
  ASSERT_TRUE(pair.has_value());
  EXPECT_EQ(pair->first, std::string_view(data, 3));
  EXPECT_EQ(pair->second, "c");
  EXPECT_EQ(pair->first.data(), data);
}
TEST(StringSplitRustBridgeTest, TypedUtf16WireUsesCodeUnitsAndOffsets) {
  namespace wire = internal::rust_string_split;
  // A typed buffer preserves lone surrogates and NUL without aliasing char16_t.
  const std::vector<uint16_t> input = {0xd800, ',', 0xdc00, ',', 0};
  const std::vector<uint16_t> delimiters = {','};
  auto ranges = wire::split_utf16(
      rust::Slice<const uint16_t>(input.data(), input.size()),
      rust::Slice<const uint16_t>(delimiters.data(), delimiters.size()),
      false, false, false);
  ASSERT_EQ(ranges.size(), 3u);
  EXPECT_EQ(ranges[0].offset, 0u); EXPECT_EQ(ranges[0].length, 1u);
  EXPECT_EQ(ranges[1].offset, 2u); EXPECT_EQ(ranges[1].length, 1u);
  EXPECT_EQ(ranges[2].offset, 4u); EXPECT_EQ(ranges[2].length, 1u);
  auto empty = wire::split_utf16(rust::Slice<const uint16_t>(nullptr, 0),
                                 rust::Slice<const uint16_t>(nullptr, 0),
                                 true, true, true);
  ASSERT_EQ(empty.size(), 1u);
  EXPECT_EQ(empty[0].offset, 0u); EXPECT_EQ(empty[0].length, 0u);
  EXPECT_FALSE(empty[0].default_empty);
}
}  // namespace base
