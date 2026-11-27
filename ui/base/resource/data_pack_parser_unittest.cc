// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <limits>
#include <span>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/rust/cxx/v1/cxx.h"
#include "ui/base/resource/data_pack_parser_ffi.rs.h"
#include "ui/base/resource/data_pack_parser_test_support.h"

namespace ui::data_pack_internal {
namespace {

using test::MakePack;
using test::Write16;
using test::Write32;

ParseResult Parse(std::span<const uint8_t> data) {
  return parse_data_pack({data.data(), data.size()});
}

void ExpectEquivalent(std::span<const uint8_t> data) {
  const auto expected = test::ParseLegacy(data);
  const auto actual = Parse(data);
  ASSERT_EQ(actual.status, expected.status);
  EXPECT_EQ(actual.resource_count, expected.resource_count);
  EXPECT_EQ(actual.alias_count, expected.alias_count);
  EXPECT_EQ(actual.resource_table_offset, expected.resource_table_offset);
  EXPECT_EQ(actual.alias_table_offset, expected.alias_table_offset);
  EXPECT_EQ(actual.encoding, expected.encoding);
}

TEST(DataPackParserTest, ValidVersionsAndEncodings) {
  for (uint32_t version : {4, 5}) {
    for (uint8_t encoding : {0, 1, 2}) {
      for (uint16_t count : {0, 1, 300}) {
        auto data = MakePack(version, count, 0, encoding);
        const auto result = Parse(data);
        ASSERT_EQ(result.status, ParseStatus::Ok);
        EXPECT_EQ(result.resource_count, count);
        EXPECT_EQ(result.encoding, encoding);
        EXPECT_EQ(result.resource_table_offset, version == 4 ? 9u : 12u);
        EXPECT_EQ(result.alias_table_offset,
                  result.resource_table_offset + (size_t{count} + 1) * 6);
        ExpectEquivalent(data);
      }
    }
  }
}

TEST(DataPackParserTest, AliasesAndMaximumV5Counts) {
  for (uint16_t count : {1, 300, 65535}) {
    auto data = MakePack(5, count, 65535);
    const auto result = Parse(data);
    ASSERT_EQ(result.status, ParseStatus::Ok);
    EXPECT_EQ(result.alias_count, 65535u);
    ExpectEquivalent(data);
  }
}

TEST(DataPackParserTest, EveryTruncation) {
  for (uint32_t version : {4, 5}) {
    auto data = MakePack(version, 3, version == 5 ? 2 : 0);
    for (size_t length = 0; length < data.size(); ++length) {
      SCOPED_TRACE(length);
      auto truncated = std::span(data).first(length);
      EXPECT_NE(Parse(truncated).status, ParseStatus::Ok);
      ExpectEquivalent(truncated);
    }
  }
}

TEST(DataPackParserTest, ErrorCategoriesAndPrecedence) {
  auto data = MakePack(5, 2, 1);
  const auto original = data;
  auto expect = [&](ParseStatus status) {
    EXPECT_EQ(Parse(data).status, status);
    ExpectEquivalent(data);
    data = original;
  };
  Write32(data, 0, 0);
  expect(ParseStatus::IncompleteHeader);
  Write32(data, 0, 6);
  data[4] = 255;
  expect(ParseStatus::BadVersion);
  data[4] = 3;
  Write16(data, 8, 65535);
  expect(ParseStatus::BadEncoding);
  Write16(data, 8, 65535);
  expect(ParseStatus::TooShort);
  Write32(data, 12 + 2, static_cast<uint32_t>(data.size() + 1));
  expect(ParseStatus::BoundsExceeded);
  Write32(data, 12 + 6 + 2, 0);
  Write32(data, 12 + 12 + 2, static_cast<uint32_t>(data.size() + 1));
  expect(ParseStatus::BoundsExceeded);  // Bounds take priority over ordering.
  Write32(data, 12 + 6 + 2, 0);
  Write16(data, 12 + 18 + 2, 2);
  expect(ParseStatus::OrderingViolation);  // Ordering precedes bad aliases.
  Write16(data, 12 + 18 + 2, 2);
  expect(ParseStatus::AliasTableCorrupt);
  data = MakePack(5, 0, 1);
  EXPECT_EQ(Parse(data).status, ParseStatus::AliasTableCorrupt);
  ExpectEquivalent(data);
}

TEST(DataPackParserTest, HugeV4CountAndOffset) {
  auto data = MakePack(4, 1);
  Write32(data, 4, std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(Parse(data).status, ParseStatus::TooShort);
  ExpectEquivalent(data);
  data = MakePack(4, 1);
  Write32(data, 9 + 2, std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(Parse(data).status, ParseStatus::BoundsExceeded);
  ExpectEquivalent(data);
}

TEST(DataPackParserTest, PreservePermissiveFormatRules) {
  auto data = MakePack(5, 2, 1);
  data[5] = data[6] = data[7] = 255;  // Unused header padding.
  Write16(data, 12, 40);
  Write16(data, 18, 40);  // Duplicate IDs are not a validation error.
  for (size_t at : {14, 20, 26}) {
    // Equal offsets, including header overlap, are valid.
    Write32(data, at, 0);
  }
  data.push_back(255);  // Trailing bytes are allowed.
  EXPECT_EQ(Parse(data).status, ParseStatus::Ok);
  ExpectEquivalent(data);
}

TEST(DataPackParserTest, UnalignedInput) {
  auto data = MakePack(4, 300);
  data.insert(data.begin(), 0);
  auto unaligned = std::span(data).subspan(1);
  EXPECT_EQ(Parse(unaligned).status, ParseStatus::Ok);
  ExpectEquivalent(unaligned);
}

TEST(DataPackParserTest, DifferentialSingleByteMutations) {
  for (uint32_t version : {4, 5}) {
    const auto original = MakePack(version, 3, version == 5 ? 2 : 0);
    auto data = original;
    for (size_t at = 0; at < data.size(); ++at) {
      for (unsigned value = 0; value < 256; ++value) {
        data[at] = static_cast<uint8_t>(value);
        ExpectEquivalent(data);
      }
      data[at] = original[at];
    }
  }
}

LookupResult Lookup(std::span<const uint8_t> data,
                    const ParseResult& layout,
                    uint16_t id) {
  return lookup_data_pack_resource({data.data(), data.size()},
                                   layout.resource_count, layout.alias_count,
                                   layout.resource_table_offset, id);
}

bool Has(std::span<const uint8_t> data,
         const ParseResult& layout,
         uint16_t id) {
  return has_data_pack_resource({data.data(), data.size()},
                                layout.resource_count, layout.alias_count,
                                layout.resource_table_offset, id);
}

void ExpectLookupEquivalent(std::span<const uint8_t> data,
                            const ParseResult& layout,
                            uint16_t id) {
  const auto expected = test::LookupLegacy(data, layout, id);
  const auto actual = Lookup(data, layout, id);
  EXPECT_EQ(actual.status, expected.status);
  EXPECT_EQ(actual.offset, expected.offset);
  EXPECT_EQ(actual.length, expected.length);
  EXPECT_EQ(Has(data, layout, id),
            test::LookupLegacy(data, layout, id, false).status ==
                LookupStatus::Found);
}

TEST(DataPackLookupTest, HitsMissesAndAliases) {
  for (uint32_t version : {4, 5}) {
    for (uint16_t count : {0, 1, 2, 3, 16, 1024}) {
      auto data = MakePack(version, count, version == 5 && count ? 10 : 0);
      const auto layout = Parse(data);
      ASSERT_EQ(layout.status, ParseStatus::Ok);
      for (uint16_t id = 0; id < count + 12; ++id) {
        ExpectLookupEquivalent(data, layout, id);
      }
      for (uint16_t id = 1; id <= count; ++id) {
        const auto result = Lookup(data, layout, id);
        EXPECT_EQ(result.status, LookupStatus::Found);
        EXPECT_EQ(result.length, 1u);
        EXPECT_EQ(result.offset,
                  layout.alias_table_offset + layout.alias_count * 4 + id - 1);
      }
      EXPECT_EQ(Lookup(data, layout, 65535).status, LookupStatus::NotFound);
    }
  }
}

TEST(DataPackLookupTest, AliasTargetsAndResourcePrecedence) {
  auto data = MakePack(5, 3, 3);
  const auto layout = Parse(data);
  for (uint16_t i = 0; i < 3; ++i) {
    Write16(data, layout.alias_table_offset + i * 4 + 2, i);
    EXPECT_EQ(Lookup(data, layout, 4 + i).offset,
              Lookup(data, layout, 1 + i).offset);
    ExpectLookupEquivalent(data, layout, 4 + i);
  }
  Write16(data, layout.alias_table_offset, 1);
  Write16(data, layout.alias_table_offset + 2, 2);
  // A real resource wins over an alias with the same ID.
  EXPECT_EQ(Lookup(data, layout, 1).offset, layout.alias_table_offset + 12);
  ExpectLookupEquivalent(data, layout, 1);
}

TEST(DataPackLookupTest, DuplicateAndUnsortedIdsUseLibcProbeOrder) {
  for (uint16_t count = 2; count < 12; ++count) {
    auto data = MakePack(5, count, count);
    const auto layout = Parse(data);
    for (size_t i = 0; i < count; ++i) {
      Write16(data, 12 + i * 6, 7);
      Write16(data, layout.alias_table_offset + i * 4, 90);
      Write16(data, layout.alias_table_offset + i * 4 + 2, i);
    }
    ExpectLookupEquivalent(data, layout, 7);
    ExpectLookupEquivalent(data, layout, 90);
    for (size_t i = 0; i < count; ++i) {
      Write16(data, 12 + i * 6, static_cast<uint16_t>((i * 5) % count));
    }
    for (uint16_t id = 0; id < count; ++id) {
      ExpectLookupEquivalent(data, layout, id);
    }
  }
}

TEST(DataPackLookupTest, EmptyRangesAndExtremeIds) {
  auto data = MakePack(5, 2, 1);
  const auto layout = Parse(data);
  Write16(data, 12, 0);
  Write16(data, 18, 65535);
  Write32(data, 20, static_cast<uint32_t>(data.size()));
  for (uint16_t id : {0, 1, 3, 65535}) {
    ExpectLookupEquivalent(data, layout, id);
  }
  EXPECT_EQ(Lookup(data, layout, 65535).status, LookupStatus::Found);
  EXPECT_EQ(Lookup(data, layout, 65535).length, 0u);
  // The sentinel's ID is never searched.
  Write16(data, 24, 42);
  EXPECT_EQ(Lookup(data, layout, 42).status, LookupStatus::NotFound);
}

TEST(DataPackLookupTest, MaximumResourceCountAndAliasTarget) {
  for (uint32_t version : {4, 5}) {
    auto data = MakePack(version, 65535, version == 5 ? 1 : 0);
    const auto layout = Parse(data);
    ASSERT_EQ(layout.status, ParseStatus::Ok);
    for (uint16_t id : {0, 1, 32768, 65535}) {
      ExpectLookupEquivalent(data, layout, id);
    }
    EXPECT_EQ(Lookup(data, layout, 65535).status, LookupStatus::Found);
    if (version == 5) {
      // The alias ID wraps to zero, which is absent from the real entries.
      Write16(data, layout.alias_table_offset + 2, 65534);
      EXPECT_EQ(Lookup(data, layout, 0).offset,
                Lookup(data, layout, 65535).offset);
      ExpectLookupEquivalent(data, layout, 0);
      Write16(data, layout.alias_table_offset + 2, 65535);
      EXPECT_EQ(Lookup(data, layout, 0).status, LookupStatus::CorruptIndex);
      ExpectLookupEquivalent(data, layout, 0);
    }
  }
}

TEST(DataPackLookupTest, CachedLayoutAndPostLoadCorruption) {
  auto data = MakePack(5, 2, 1);
  const auto original = data;
  const auto layout = Parse(data);
  // Layout is cached; overwriting the header must not reconfigure lookups.
  for (size_t i = 0; i < 12; ++i) {
    data[i] = 255;
  }
  EXPECT_EQ(Lookup(data, layout, 1).status, LookupStatus::Found);
  ExpectLookupEquivalent(data, layout, 1);
  data = original;
  Write32(data, 20, static_cast<uint32_t>(data.size() + 1));
  EXPECT_TRUE(Has(data, layout, 1));
  EXPECT_EQ(Lookup(data, layout, 1).status, LookupStatus::BoundsExceeded);
  ExpectLookupEquivalent(data, layout, 1);
  data = original;
  Write32(data, 20, 0);
  EXPECT_TRUE(Has(data, layout, 1));
  EXPECT_EQ(Lookup(data, layout, 1).status, LookupStatus::OrderingViolation);
  ExpectLookupEquivalent(data, layout, 1);
  data = original;
  Write16(data, layout.alias_table_offset + 2, 2);
  EXPECT_FALSE(Has(data, layout, 3));
  EXPECT_EQ(Lookup(data, layout, 3).status, LookupStatus::CorruptIndex);
  ExpectLookupEquivalent(data, layout, 3);
}

TEST(DataPackLookupTest, TruncationAndArbitraryMetadataAreChecked) {
  const auto data = MakePack(5, 3, 2);
  auto layout = Parse(data);
  for (size_t size = 0; size <= data.size(); ++size) {
    for (uint16_t id : {0, 1, 3, 4, 5, 65535}) {
      ExpectLookupEquivalent(std::span(data).first(size), layout, id);
    }
  }
  layout.resource_count = std::numeric_limits<size_t>::max();
  ExpectLookupEquivalent(data, layout, 1);
  EXPECT_EQ(Lookup(data, layout, 1).status, LookupStatus::CorruptIndex);
  layout.resource_count = std::numeric_limits<size_t>::max() / 6 + 1;
  ExpectLookupEquivalent(data, layout, 1);
  layout = Parse(data);
  layout.alias_count = std::numeric_limits<size_t>::max();
  ExpectLookupEquivalent(data, layout, 1);
  layout.alias_count = std::numeric_limits<size_t>::max() / 4;
  ExpectLookupEquivalent(data, layout, 1);
  layout = Parse(data);
  layout.resource_table_offset = std::numeric_limits<size_t>::max();
  ExpectLookupEquivalent(data, layout, 1);
}

TEST(DataPackLookupTest, DifferentialPostLoadByteMutations) {
  for (uint32_t version : {4, 5}) {
    auto data = MakePack(version, 3, version == 5 ? 2 : 0);
    const auto original = data;
    const auto layout = Parse(data);
    for (size_t at = 0; at < data.size(); ++at) {
      for (unsigned value = 0; value < 256; ++value) {
        data[at] = static_cast<uint8_t>(value);
        for (uint16_t id : {0, 1, 3, 4, 5, 65535}) {
          ExpectLookupEquivalent(data, layout, id);
        }
      }
      data[at] = original[at];
    }
  }
}

}  // namespace
}  // namespace ui::data_pack_internal
