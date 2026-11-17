// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <new>

#include "base/test/bind.h"
#include "net/spdy/http2_read_queue_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::http2_read_queue_test {
namespace {

// These callbacks only read live members. circular_deque::ClearRetainCapacity
// destroys every element before changing begin_/end_; the original Clear also
// assigns total_size_ only after those destructors. This applies equally when
// Clear is invoked by the queue destructor, before its members are destroyed.
template <class Queue>
void VerifyReadOnlyClearObservations(bool destroy) {
  struct Observation {
    size_t bytes;
    size_t total;
    bool empty;
    bool operator==(const Observation&) const = default;
  };
  std::vector<Observation> observations;
  auto queue = std::make_unique<Queue>();
  auto* raw = queue.get();
  for (size_t length : {2u, 5u, 9u}) {
    auto buffer =
        std::make_unique<BufferFor<Queue>>(std::vector<uint8_t>(length, 42));
    buffer->AddConsumeCallback(base::BindRepeating(
        [](Queue* q, std::vector<Observation>* result, size_t bytes,
           typename BufferFor<Queue>::ConsumeSource source) {
          if (source == BufferFor<Queue>::DISCARD) {
            result->push_back({bytes, q->GetTotalSize(), q->IsEmpty()});
          }
        },
        raw, &observations));
    queue->Enqueue(std::move(buffer));
  }
  std::array<uint8_t, 1> out;
  ASSERT_EQ(queue->Dequeue(out), 1u);
  ASSERT_TRUE(observations.empty());
  ASSERT_EQ(queue->GetTotalSize(), 15u);
  if (destroy) {
    queue.reset();
  } else {
    queue->Clear();
    EXPECT_TRUE(queue->IsEmpty());
    EXPECT_EQ(queue->GetTotalSize(), 0u);
    queue->Clear();
  }
  const std::vector<Observation> expected = {
      {1, 15, false}, {5, 15, false}, {9, 15, false}};
  EXPECT_EQ(observations, expected);
}

}  // namespace

TEST(Http2ReadQueueOriginalCompatibilityTest, ReadOnlyClearObservations) {
  VerifyReadOnlyClearObservations<Http2ReadQueueOriginal>(false);
}

TEST(Http2ReadQueueOriginalCompatibilityTest, ReadOnlyDestructorObservations) {
  VerifyReadOnlyClearObservations<Http2ReadQueueOriginal>(true);
}

TEST(Http2ReadQueueRustCompatibilityTest, ReadOnlyClearObservations) {
  VerifyReadOnlyClearObservations<SpdyReadQueue>(false);
}

TEST(Http2ReadQueueRustCompatibilityTest, ReadOnlyDestructorObservations) {
  VerifyReadOnlyClearObservations<SpdyReadQueue>(true);
}

TEST(Http2ReadQueueRustTest, ClearMutationEndsReadOnlyView) {
  SpdyReadQueue queue;
  std::vector<size_t> totals;
  for (size_t length : {2u, 5u}) {
    auto buffer =
        std::make_unique<SpdyBuffer>(std::vector<uint8_t>(length, 42));
    buffer->AddConsumeCallback(base::BindLambdaForTesting(
        [&, length](size_t, SpdyBuffer::ConsumeSource) {
          totals.push_back(queue.GetTotalSize());
          if (length == 2) {
            queue.Enqueue(
                std::make_unique<SpdyBuffer>(std::vector<uint8_t>(3, 99)));
          }
          totals.push_back(queue.GetTotalSize());
        }));
    queue.Enqueue(std::move(buffer));
  }
  queue.Clear();
  EXPECT_EQ(totals, (std::vector<size_t>{7, 3, 3, 3}));
  EXPECT_EQ(queue.GetTotalSize(), 3u);
  std::array<uint8_t, 3> out;
  EXPECT_EQ(queue.Dequeue(out), 3u);
  EXPECT_EQ(out, (std::array<uint8_t, 3>{99, 99, 99}));
}

TEST(Http2ReadQueueRustTest, NestedClearSnapshotSurvivesOwnerDeletion) {
  auto queue = std::make_unique<SpdyReadQueue>();
  std::vector<int> callbacks;
  auto first = std::make_unique<SpdyBuffer>(std::vector<uint8_t>(2, 1));
  first->AddConsumeCallback(
      base::BindLambdaForTesting([&](size_t, SpdyBuffer::ConsumeSource) {
        callbacks.push_back(1);
        EXPECT_EQ(queue->GetTotalSize(), 5u);
        auto nested = std::make_unique<SpdyBuffer>(std::vector<uint8_t>(7, 2));
        nested->AddConsumeCallback(base::BindLambdaForTesting(
            [&](size_t bytes, SpdyBuffer::ConsumeSource source) {
              EXPECT_EQ(bytes, 7u);
              EXPECT_EQ(source, SpdyBuffer::DISCARD);
              EXPECT_EQ(queue->GetTotalSize(), 7u);
              callbacks.push_back(2);
              queue.reset();
            }));
        queue->Enqueue(std::move(nested));
        queue->Clear();
        EXPECT_FALSE(queue);
        callbacks.push_back(3);
      }));
  auto second = std::make_unique<SpdyBuffer>(std::vector<uint8_t>(3, 3));
  second->AddConsumeCallback(base::BindLambdaForTesting(
      [&](size_t, SpdyBuffer::ConsumeSource) { callbacks.push_back(4); }));
  queue->Enqueue(std::move(first));
  queue->Enqueue(std::move(second));
  queue->Clear();
  EXPECT_FALSE(queue);
  EXPECT_EQ(callbacks, (std::vector<int>{1, 2, 3, 4}));
}

TEST(Http2ReadQueueRustTest, ClearCleanupNeverTouchesReusedOwnerAddress) {
  alignas(SpdyReadQueue) std::byte storage[sizeof(SpdyReadQueue)];
  auto* queue = new (storage) SpdyReadQueue;
  int callbacks = 0;
  auto buffer = std::make_unique<SpdyBuffer>(std::vector<uint8_t>(2, 1));
  buffer->AddConsumeCallback(
      base::BindLambdaForTesting([&](size_t, SpdyBuffer::ConsumeSource) {
        EXPECT_EQ(queue->GetTotalSize(), 2u);
        queue->~SpdyReadQueue();
        queue = new (storage) SpdyReadQueue;
        queue->Enqueue(
            std::make_unique<SpdyBuffer>(std::vector<uint8_t>(7, 99)));
        ++callbacks;
      }));
  queue->Enqueue(std::move(buffer));
  queue->Clear();
  EXPECT_EQ(callbacks, 1);
  EXPECT_EQ(queue->GetTotalSize(), 7u);
  std::array<uint8_t, 7> out;
  EXPECT_EQ(queue->Dequeue(out), 7u);
  EXPECT_EQ(out, (std::array<uint8_t, 7>{99, 99, 99, 99, 99, 99, 99}));
  queue->~SpdyReadQueue();
}

TEST(Http2ReadQueueRustTest, SemanticFixtures) {
  EXPECT_EQ(VerifyFixture<SpdyReadQueue>(),
            VerifyFixture<Http2ReadQueueOriginal>());
}

TEST(Http2ReadQueueRustTest, UnusedQueueQueriesAndClearBeforeFirstEnqueue) {
  static_assert(sizeof(SpdyReadQueue) == sizeof(void*));
  SpdyReadQueue queue;
  std::array<uint8_t, 3> out = {11, 22, 33};
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_TRUE(queue.IsEmpty());
    EXPECT_EQ(queue.GetTotalSize(), 0u);
    EXPECT_EQ(queue.Dequeue(out), 0u);
    queue.Clear();
  }
  EXPECT_EQ(out, (std::array<uint8_t, 3>{11, 22, 33}));
  queue.Enqueue(std::make_unique<SpdyBuffer>(std::vector<uint8_t>{4, 5, 6}));
  EXPECT_EQ(queue.GetTotalSize(), 3u);
  EXPECT_EQ(queue.Dequeue(out), 3u);
  EXPECT_EQ(out, (std::array<uint8_t, 3>{4, 5, 6}));
  EXPECT_TRUE(queue.IsEmpty());
}

TEST(Http2ReadQueueRustTest, DrainedQueueRetainsStorageAcrossEnqueueCycles) {
  SpdyReadQueue queue;
  std::vector<int> callbacks;
  for (int value : {1, 2, 3}) {
    auto buffer = std::make_unique<SpdyBuffer>(
        std::vector<uint8_t>(1, static_cast<uint8_t>(value)));
    buffer->AddConsumeCallback(base::BindLambdaForTesting(
        [&, value](size_t bytes, SpdyBuffer::ConsumeSource source) {
          EXPECT_EQ(bytes, 1u);
          EXPECT_EQ(source, SpdyBuffer::DISCARD);
          EXPECT_TRUE(queue.IsEmpty());
          EXPECT_EQ(queue.GetTotalSize(), 0u);
          callbacks.push_back(value);
        }));
    queue.Enqueue(std::move(buffer));
    std::array<uint8_t, 1> out;
    EXPECT_EQ(queue.Dequeue(out), 1u);
    EXPECT_EQ(out.front(), value);
  }
  EXPECT_EQ(callbacks, (std::vector<int>{1, 2, 3}));
}

TEST(Http2ReadQueueRustTest, MillionStatefulTransitions) {
  Differential(20261113, 1000000);
}

TEST(Http2ReadQueueRustTest, FrameSizeAndReadBoundaries) {
  for (size_t size :
       {size_t{1}, size_t{2}, size_t{16}, size_t{255}, size_t{256},
        size_t{4095}, size_t{4096}, size_t{65535}, size_t{0x00ffffff}}) {
    std::vector<Event> actual_events, expected_events;
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
      data[i] = static_cast<uint8_t>(i * 31);
    }
    {
      SpdyReadQueue actual;
      Http2ReadQueueOriginal expected;
      Push(actual, 1, data, actual_events);
      Push(expected, 1, data, expected_events);
      for (size_t request : {size_t{1}, size - 1, size, size + 1}) {
        if (request == 0) {
          continue;  // Zero-capacity output is outside the original API.
        }
        std::vector<uint8_t> a(request), b(request);
        CHECK_EQ(actual.Dequeue(a), expected.Dequeue(b));
        CHECK(a == b);
        CHECK_EQ(actual.GetTotalSize(), expected.GetTotalSize());
      }
    }
    EXPECT_EQ(actual_events, expected_events);
  }
}
}  // namespace net::http2_read_queue_test
