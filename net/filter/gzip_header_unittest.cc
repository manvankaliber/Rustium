// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/filter/gzip_header.h"

#include <algorithm>
#include <array>
#include <vector>

#include "base/containers/span.h"
#include "base/containers/span_rust.h"
#include "net/filter/gzip_header_ffi.rs.h"
#include "net/filter/gzip_header_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

using gzip_header_internal::test::LegacyHeader;

static_assert(sizeof(GZipHeader) == sizeof(LegacyHeader));
static_assert(alignof(GZipHeader) == alignof(LegacyHeader));

std::vector<uint8_t> Header(uint8_t flags = 0,
                            uint16_t extra = 3,
                            size_t name = 5,
                            size_t comment = 7) {
  std::vector<uint8_t> bytes = {0x1f, 0x8b, 8, flags, 0, 1, 2, 3, 4, 255};
  if (flags & 4) {
    bytes.push_back(static_cast<uint8_t>(extra));
    bytes.push_back(static_cast<uint8_t>(extra >> 8));
    bytes.insert(bytes.end(), extra, 0x45);
  }
  if (flags & 8) {
    bytes.insert(bytes.end(), name, 0x4e);
    bytes.push_back(0);
  }
  if (flags & 16) {
    bytes.insert(bytes.end(), comment, 0x43);
    bytes.push_back(0);
  }
  if (flags & 2) {
    bytes.push_back(0xa5);
    bytes.push_back(0x5a);
  }
  return bytes;
}

void Compare(base::span<const uint8_t> input, size_t chunk_size) {
  ASSERT_GT(chunk_size, 0u);
  GZipHeader actual;
  LegacyHeader expected;
  gzip_header_internal::State state;
  auto step = [&](base::span<const uint8_t> chunk) {
    size_t actual_end = 0xcafe;
    size_t expected_end = actual_end;
    size_t bridge_end = actual_end;
    const auto reference = expected.ReadMore(chunk, expected_end);
    EXPECT_EQ(static_cast<int>(actual.ReadMore(chunk, actual_end)), reference);
    EXPECT_EQ(gzip_header_internal::read_more(base::SpanToRustSlice(chunk),
                                              state, bridge_end),
              reference);
    EXPECT_EQ(actual_end, expected_end);
    EXPECT_EQ(bridge_end, expected_end);
    EXPECT_EQ(state.phase, expected.phase());
    EXPECT_EQ(state.flags, expected.flags());
    EXPECT_EQ(state.extra_length, expected.extra_length());
  };
  step({});
  for (size_t pos = 0; pos < input.size();) {
    const size_t count = std::min(chunk_size, input.size() - pos);
    step(input.subspan(pos, count));
    step({});
    pos += count;
  }
  step({});
  const std::array<uint8_t, 3> trailing = {0x42, 0, 0xff};
  step(trailing);
}

TEST(GZipHeaderRustTest, OrdinaryHeaderAndPayload) {
  auto data = Header();
  data.insert(data.end(), {0x42, 0x43, 0x44});
  GZipHeader parser;
  size_t end = 0;
  EXPECT_EQ(parser.ReadMore(data, end), GZipHeader::COMPLETE_HEADER);
  EXPECT_EQ(end, 10u);
  EXPECT_TRUE(GZipHeader::HasGZipHeader(data));
  Compare(data, data.size());
}

TEST(GZipHeaderRustTest, CompletionInitializesOutputStorage) {
  for (size_t chunk : {1u, 10u}) {
    GZipHeader parser;
    const auto data = Header();
    size_t end;
    for (size_t pos = 0; pos < data.size(); pos += chunk) {
      const auto status =
          parser.ReadMore(base::span(data).subspan(pos, chunk), end);
      EXPECT_EQ(status, pos + chunk == data.size()
                            ? GZipHeader::COMPLETE_HEADER
                            : GZipHeader::INCOMPLETE_HEADER);
      if (status == GZipHeader::COMPLETE_HEADER) {
        EXPECT_EQ(end, chunk);
      }
    }
  }
}

TEST(GZipHeaderRustTest, InputCanAliasOutputStorage) {
  std::array<size_t, 4> storage = {};
  const auto data = Header();
  auto bytes = base::as_writable_byte_span(storage);
  std::copy(data.begin(), data.end(), bytes.begin());
  GZipHeader parser;
  EXPECT_EQ(parser.ReadMore(base::as_byte_span(storage).first(data.size()),
                            storage[0]),
            GZipHeader::COMPLETE_HEADER);
  EXPECT_EQ(storage[0], data.size());
}

TEST(GZipHeaderRustTest, EveryFlagCombination) {
  for (unsigned flags = 0; flags <= 255; ++flags) {
    const auto data = Header(static_cast<uint8_t>(flags));
    EXPECT_TRUE(GZipHeader::HasGZipHeader(data));
    Compare(data, data.size());
    Compare(data, 1);
  }
}

TEST(GZipHeaderRustTest, EveryTruncation) {
  for (unsigned flags = 0; flags < 32; ++flags) {
    const auto data = Header(static_cast<uint8_t>(flags));
    for (size_t size = 0; size <= data.size(); ++size) {
      const auto prefix = base::span(data).first(size);
      EXPECT_EQ(GZipHeader::HasGZipHeader(prefix),
                LegacyHeader::HasGZipHeader(prefix));
      Compare(prefix, 3);
    }
  }
}

TEST(GZipHeaderRustTest, EverySplitPoint) {
  for (unsigned flags = 0; flags < 32; ++flags) {
    const auto data = Header(static_cast<uint8_t>(flags));
    for (size_t split = 0; split <= data.size(); ++split) {
      GZipHeader actual;
      LegacyHeader expected;
      size_t actual_end = 999;
      size_t expected_end = actual_end;
      for (auto part :
           {base::span(data).first(split), base::span(data).subspan(split)}) {
        EXPECT_EQ(static_cast<int>(actual.ReadMore(part, actual_end)),
                  expected.ReadMore(part, expected_end));
        EXPECT_EQ(actual_end, expected_end);
      }
    }
  }
}

TEST(GZipHeaderRustTest, FragmentedOptionalFields) {
  const auto data = Header(30, 31, 37, 41);
  for (size_t size : {1, 2, 3, 4, 7, 10, 64}) {
    Compare(data, size);
  }
}

TEST(GZipHeaderRustTest, ZeroLengthExtraAtChunkBoundary) {
  const auto data = Header(4, 0);
  GZipHeader parser;
  size_t end = 123;
  EXPECT_EQ(parser.ReadMore(data, end), GZipHeader::COMPLETE_HEADER);
  EXPECT_EQ(end, 12u);
  Compare(data, 1);
  Compare(data, 12);
}

TEST(GZipHeaderRustTest, MaximumExtraLength) {
  const auto data = Header(30, 65535, 1, 1);
  Compare(data, 1);
  Compare(data, 257);
  Compare(data, data.size());
}

TEST(GZipHeaderRustTest, EmptyNameAndComment) {
  const auto data = Header(24, 0, 0, 0);
  Compare(data, 1);
  EXPECT_TRUE(GZipHeader::HasGZipHeader(data));
}

TEST(GZipHeaderRustTest, LongNameAndComment) {
  const auto data = Header(24, 0, 4096, 8192);
  Compare(data, 1024);
  Compare(data, data.size());
}

TEST(GZipHeaderRustTest, ReservedFlagsAndTextAreAccepted) {
  const auto data = Header(0xe1);
  EXPECT_TRUE(GZipHeader::HasGZipHeader(data));
  Compare(data, 2);
}

TEST(GZipHeaderRustTest, HeaderCrcRemainsOpaque) {
  auto data = Header(2);
  data[data.size() - 2] ^= 0xff;
  data.back() ^= 0xff;
  EXPECT_TRUE(GZipHeader::HasGZipHeader(data));
  Compare(data, 1);
}

TEST(GZipHeaderRustTest, InvalidMagicAndCompressionMethod) {
  for (size_t pos = 0; pos < 3; ++pos) {
    for (unsigned value = 0; value <= 255; ++value) {
      auto data = Header();
      data[pos] = static_cast<uint8_t>(value);
      EXPECT_EQ(GZipHeader::HasGZipHeader(data),
                LegacyHeader::HasGZipHeader(data));
      Compare(data, 1);
      Compare(data, data.size());
    }
  }
}

TEST(GZipHeaderRustTest, ResetAfterPartialInvalidAndComplete) {
  for (size_t prefix : {0, 1, 3, 10, 12}) {
    for (bool corrupt : {false, true}) {
      auto data = Header(30);
      if (corrupt) {
        data[0] = 0;
      }
      GZipHeader parser;
      size_t end = 12;
      parser.ReadMore(base::span(data).first(prefix), end);
      parser.Reset();
      EXPECT_EQ(parser.ReadMore(Header(), end), GZipHeader::COMPLETE_HEADER);
      EXPECT_EQ(end, 10u);
    }
  }
}

TEST(GZipHeaderRustTest, IncompleteAndInvalidLeaveOutputUnchanged) {
  for (const auto& data :
       {std::vector<uint8_t>{}, std::vector<uint8_t>{0x1f},
        std::vector<uint8_t>{0x1f, 0x8b}, std::vector<uint8_t>{0, 0, 0, 0}}) {
    GZipHeader parser;
    size_t end = 0x123456;
    EXPECT_NE(parser.ReadMore(data, end), GZipHeader::COMPLETE_HEADER);
    EXPECT_EQ(end, 0x123456u);
  }
}

TEST(GZipHeaderRustTest, CompletionAndEmptyCalls) {
  Compare(Header(), 10);
  Compare(Header(30), 3);
  GZipHeader parser;
  size_t end = 12;
  ASSERT_EQ(parser.ReadMore(Header(), end), GZipHeader::COMPLETE_HEADER);
  EXPECT_EQ(parser.ReadMore({}, end), GZipHeader::COMPLETE_HEADER);
  EXPECT_EQ(end, 0u);
}

TEST(GZipHeaderRustTest, DifferentialSingleByteMutations) {
  auto data = Header(30, 2, 2, 2);
  for (size_t pos = 0; pos < data.size(); ++pos) {
    const auto original = data[pos];
    for (unsigned value = 0; value <= 255; ++value) {
      data[pos] = static_cast<uint8_t>(value);
      Compare(data, 3);
    }
    data[pos] = original;
  }
}

TEST(GZipHeaderRustTest, InvalidBridgeStateIsRejected) {
  for (int32_t phase : {-1, 18, 2147483647}) {
    gzip_header_internal::State state{phase, 0, 0};
    size_t end = 9;
    const std::array<uint8_t, 1> data = {0};
    EXPECT_EQ(gzip_header_internal::read_more(base::SpanToRustSlice(data),
                                              state, end),
              GZipHeader::INVALID_HEADER);
    EXPECT_EQ(end, 9u);
  }
}

}  // namespace
}  // namespace net
