// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/dcheck_is_on.h"
#include "base/functional/callback.h"
#include "base/strings/string_view_util.h"
#include "base/test/bind.h"
#include "net/base/io_buffer.h"
#include "net/spdy/spdy_buffer.h"
#include "net/spdy/spdy_buffer_rust_original.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

// Compile-time guards are proposals until this target is built. Matching
// size/alignment alone does not establish complete member-layout equivalence.
static_assert(sizeof(SpdyBuffer) == sizeof(SpdyBufferOriginal));
static_assert(alignof(SpdyBuffer) == alignof(SpdyBufferOriginal));
static_assert(SpdyBuffer::CONSUME == 0 && SpdyBufferOriginal::CONSUME == 0);
static_assert(SpdyBuffer::DISCARD == 1 && SpdyBufferOriginal::DISCARD == 1);

struct Event {
  char callback;
  size_t bytes;
  int source;
  bool operator==(const Event&) const = default;
};

template <class Buffer>
class SpdyBufferRustCompatibilityTest : public ::testing::Test {};

using Implementations = ::testing::Types<SpdyBufferOriginal, SpdyBuffer>;
TYPED_TEST_SUITE(SpdyBufferRustCompatibilityTest, Implementations);

TYPED_TEST(SpdyBufferRustCompatibilityTest, EmptyOwnedFrameIsAllowed) {
  size_t calls = 0;
  TypeParam buffer(std::make_unique<spdy::SpdySerializedFrame>());
  buffer.AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t, typename TypeParam::ConsumeSource) { ++calls; }));
  EXPECT_TRUE(buffer.GetRemaining().empty());
  EXPECT_EQ(buffer.GetRemainingSize(), 0u);
  EXPECT_EQ(calls, 0u);
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, OwnedFrameDoesNotUseRawCopyCap) {
  constexpr size_t kAboveRawCopyCap = 0x01000000;
  auto data = std::make_unique<char[]>(kAboveRawCopyCap);
  TypeParam buffer(std::make_unique<spdy::SpdySerializedFrame>(
      std::move(data), kAboveRawCopyCap));
  EXPECT_EQ(buffer.GetRemainingSize(), kAboveRawCopyCap);
  EXPECT_EQ(buffer.GetRemaining().size(), kAboveRawCopyCap);
  buffer.Consume(kAboveRawCopyCap);
  EXPECT_TRUE(buffer.GetRemaining().empty());
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, RawCopyRejectsEmptyAndOversized) {
  EXPECT_DEATH_IF_SUPPORTED(
      { TypeParam buffer{base::span<const uint8_t>{}}; }, "");
  // The span describes genuinely allocated storage, not an oversized pointer
  // fiction. The constructor must diagnose the cap before reading the bytes.
  const std::vector<uint8_t> above_cap(0x01000000, 7);
  EXPECT_DEATH_IF_SUPPORTED({ TypeParam buffer{base::span(above_cap)}; }, "");
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, OffsetCommittedBeforeCallbacks) {
  constexpr std::array<uint8_t, 6> data = {1, 2, 3, 4, 5, 6};
  std::vector<Event> events;
  size_t expected_remaining = 4;
  TypeParam buffer(data);
  for (char name : {'a', 'b'}) {
    buffer.AddConsumeCallback(base::BindLambdaForTesting(
        [&, name](size_t amount, typename TypeParam::ConsumeSource source) {
          EXPECT_EQ(buffer.GetRemainingSize(), expected_remaining);
          if (expected_remaining != 0) {
            EXPECT_EQ(buffer.GetRemaining().front(), 3u);
          }
          events.push_back({name, amount, static_cast<int>(source)});
        }));
  }
  buffer.Consume(2);
  EXPECT_EQ(events, (std::vector<Event>{{'a', 2, 0}, {'b', 2, 0}}));
  expected_remaining = 0;
  buffer.Consume(4);
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, AddedCallbackStartsNextDispatch) {
  constexpr std::array<uint8_t, 6> data = {1, 2, 3, 4, 5, 6};
  std::vector<Event> events;
  TypeParam buffer(data);
  bool added = false;
  buffer.AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t amount, typename TypeParam::ConsumeSource source) {
        events.push_back({'a', amount, static_cast<int>(source)});
        if (!added) {
          added = true;
          buffer.AddConsumeCallback(base::BindLambdaForTesting(
              [&](size_t bytes, typename TypeParam::ConsumeSource why) {
                events.push_back({'c', bytes, static_cast<int>(why)});
              }));
        }
      }));
  buffer.AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t amount, typename TypeParam::ConsumeSource source) {
        events.push_back({'b', amount, static_cast<int>(source)});
      }));
  buffer.Consume(2);
  EXPECT_EQ(events, (std::vector<Event>{{'a', 2, 0}, {'b', 2, 0}}));
  buffer.Consume(4);
  EXPECT_EQ(events, (std::vector<Event>{{'a', 2, 0}, {'b', 2, 0},
                                       {'a', 4, 0}, {'b', 4, 0}, {'c', 4, 0}}));
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, NestedConsumeUsesLiveOffset) {
  constexpr std::array<uint8_t, 6> data = {1, 2, 3, 4, 5, 6};
  std::vector<Event> events;
  TypeParam buffer(data);
  bool nested = false;
  buffer.AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t amount, typename TypeParam::ConsumeSource source) {
        events.push_back({'a', amount, static_cast<int>(source)});
        if (!nested) {
          nested = true;
          EXPECT_EQ(buffer.GetRemainingSize(), 4u);
          buffer.Consume(1);
          EXPECT_EQ(buffer.GetRemainingSize(), 3u);
        }
      }));
  buffer.AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t amount, typename TypeParam::ConsumeSource source) {
        events.push_back({'b', amount, static_cast<int>(source)});
      }));
  buffer.Consume(2);
  EXPECT_EQ(events, (std::vector<Event>{{'a', 2, 0}, {'a', 1, 0},
                                       {'b', 1, 0}, {'b', 2, 0}}));
  EXPECT_EQ(buffer.GetRemainingSize(), 3u);
  buffer.Consume(3);
}

// The public header discourages owner deletion. The frozen implementation
// explicitly accommodates it with a local callback snapshot; preserve that
// existing implementation behavior without broadening unsupported contracts.
TYPED_TEST(SpdyBufferRustCompatibilityTest, CallbackSnapshotOutlivesOwner) {
  constexpr std::array<uint8_t, 3> data = {1, 2, 3};
  std::vector<Event> events;
  auto buffer = std::make_unique<TypeParam>(data);
  auto capture = std::make_shared<int>(1);
  std::weak_ptr<int> weak = capture;
  buffer->AddConsumeCallback(base::BindLambdaForTesting(
      [&, keep = capture](size_t amount,
                          typename TypeParam::ConsumeSource source) {
        EXPECT_EQ(*keep, 1);
        events.push_back({'a', amount, static_cast<int>(source)});
        buffer.reset();
        EXPECT_FALSE(weak.expired());
      }));
  buffer->AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t amount, typename TypeParam::ConsumeSource source) {
        EXPECT_FALSE(buffer);
        EXPECT_FALSE(weak.expired());
        events.push_back({'b', amount, static_cast<int>(source)});
      }));
  capture.reset();
  buffer->Consume(3);
  EXPECT_FALSE(buffer);
  EXPECT_TRUE(weak.expired());
  EXPECT_EQ(events, (std::vector<Event>{{'a', 3, 0}, {'b', 3, 0}}));
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, PartialDeletionDiscardsBeforeResume) {
  constexpr std::array<uint8_t, 6> data = {1, 2, 3, 4, 5, 6};
  std::vector<Event> events;
  auto buffer = std::make_unique<TypeParam>(data);
  buffer->AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t amount, typename TypeParam::ConsumeSource source) {
        events.push_back({'a', amount, static_cast<int>(source)});
        if (source == TypeParam::CONSUME) {
          buffer.reset();
        }
      }));
  buffer->AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t amount, typename TypeParam::ConsumeSource source) {
        events.push_back({'b', amount, static_cast<int>(source)});
      }));
  buffer->Consume(2);
  EXPECT_FALSE(buffer);
  EXPECT_EQ(events, (std::vector<Event>{{'a', 2, 0}, {'a', 4, 1},
                                       {'b', 4, 1}, {'b', 2, 0}}));
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, IOViewsKeepTheirOwnFixedOffsets) {
  const std::string data = "abcdef";
  auto buffer = std::make_unique<TypeParam>(base::as_byte_span(data));
  auto full = buffer->GetIOBufferForRemainingData();
  buffer->Consume(2);
  auto tail = buffer->GetIOBufferForRemainingData();
  buffer->Consume(4);
  auto empty = buffer->GetIOBufferForRemainingData();
  EXPECT_TRUE(empty->span().empty());
  buffer.reset();
  EXPECT_EQ(base::as_string_view(full->span()), "abcdef");
  EXPECT_EQ(base::as_string_view(tail->span()), "cdef");
  tail->span().front() = 'Z';
  EXPECT_EQ(base::as_string_view(full->span()), "abZdef");
  full.reset();
  EXPECT_EQ(base::as_string_view(tail->span()), "Zdef");
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, FullConsumptionSuppressesDiscard) {
  constexpr std::array<uint8_t, 1> data = {42};
  size_t calls = 0;
  {
    TypeParam buffer(data);
    buffer.Consume(1);
    buffer.AddConsumeCallback(base::BindLambdaForTesting(
        [&](size_t, typename TypeParam::ConsumeSource) { ++calls; }));
  }
  EXPECT_EQ(calls, 0u);
}

TYPED_TEST(SpdyBufferRustCompatibilityTest, InvalidConsumeRetainsDebugContract) {
#if DCHECK_IS_ON()
  constexpr std::array<uint8_t, 2> data = {1, 2};
  EXPECT_DEATH_IF_SUPPORTED(
      {
        TypeParam buffer(data);
        buffer.Consume(0);
      }, "");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        TypeParam buffer(data);
        buffer.Consume(3);
      }, "");
#else
  GTEST_SKIP() << "Invalid consume arguments are debug-only preconditions.";
#endif
}

}  // namespace
}  // namespace net
