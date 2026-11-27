// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <thread>
#include <tuple>
#include <utility>

#include "net/spdy/http2_write_queue_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::http2_write_queue_test {
namespace {

TEST(Http2WriteQueueRustTest, MillionPublicApiOperations) {
  // 4096 separately drained sequences, 256 generated operations per sequence.
  // Each generated operation calls both actual public implementations; priority
  // changes additionally pop and restore, exercising the intermediate ordering.
  for (uint64_t i = 0; i < 4096; ++i) {
    Differential(0x9182a5fe17d430b1ULL + i, 256);
  }
}

TEST(Http2WriteQueueRustTest, EveryFrameAndCapCount) {
  Http2WriteQueueOriginal old;
  SpdyWriteQueue fresh;
  for (int f = 0; f < 256; ++f) {
    auto frame = static_cast<spdy::SpdyFrameType>(f);
    EXPECT_EQ(Http2OriginalIsCapped(frame), IsSpdyFrameTypeWriteCapped(frame));
    Push(old, HIGHEST, f, f);
    Push(fresh, HIGHEST, f, f);
    EXPECT_EQ(old.num_queued_capped_frames(), fresh.num_queued_capped_frames());
  }
  EXPECT_EQ(5, fresh.num_queued_capped_frames());
  for (int f = 0; f < 256; ++f) {
    OriginalOutput a;
    Output b;
    ASSERT_TRUE(a.Pop(old));
    ASSERT_TRUE(b.Pop(fresh));
    EXPECT_EQ(f, b.value());
    EXPECT_EQ(a.frame, b.frame);
    EXPECT_EQ(old.num_queued_capped_frames(), fresh.num_queued_capped_frames());
  }
  EXPECT_EQ(0, fresh.num_queued_capped_frames());
}

template <class Queue>
std::vector<int> Reentrancy(int operation) {
  std::vector<int> observations;
  auto stream = MakeStream(MEDIUM, 5);
  Queue q;
  for (int i = 0; i < 6; ++i) {
    Push(q, MEDIUM, i + 3, i, StreamWeakPtrFor<Queue>(*stream), [&, i] {
      observations.push_back(i);
      observations.push_back(q.num_queued_capped_frames());
      observations.push_back(q.IsEmpty());
      // Each deferred destruction mutates the same queue after the previous
      // mutation completes. Nested clear/removal must not see removing=true.
      if (i == 0) {
        Push(q, HIGHEST, 6, 100);
        q.Clear();
      }
      Push(q, LOW, 3, i + 200);
    });
  }
  if (operation == 0) {
    q.Clear();
  }
  if (operation == 1) {
    q.RemovePendingWritesForStream(stream.get());
  }
  if (operation == 2) {
    q.RemovePendingWritesForStreamsAfter(4);
  }
  for (;;) {
    OutputFor<Queue> out;
    if (!out.Pop(q)) {
      break;
    }
    observations.push_back(out.value());
  }
  observations.push_back(q.num_queued_capped_frames());
  return observations;
}

TEST(Http2WriteQueueRustTest, DeferredDestructionOrderAndNestedCallbacks) {
  for (int op = 0; op < 3; ++op) {
    EXPECT_EQ(Reentrancy<Http2WriteQueueOriginal>(op),
              Reentrancy<SpdyWriteQueue>(op));
  }
}

template <class Queue>
std::vector<int> PartialOutput() {
  std::vector<int> result;
  auto stream = MakeStream(MEDIUM, 8);
  Queue q;
  OutputFor<Queue> out;
  out.stream = StreamWeakPtrFor<Queue>(*stream);
  out.producer = std::make_unique<ProducerFor<Queue>>(99, [&] {
    // frame has changed, but stream and annotation have not been assigned yet.
    result.push_back(static_cast<int>(out.frame));
    result.push_back(out.stream.get() == stream.get());
    result.push_back(q.IsEmpty());
    result.push_back(q.num_queued_capped_frames());
    // Caller aliases frame_type through its output producer callback.
    out.frame = spdy::SpdyFrameType::DATA;
    Push(q, HIGHEST, 6, 123);
  });
  // Empty failure leaves every output intact and does not destroy producer.
  CHECK(!out.Pop(q));
  CHECK_EQ(out.value(), 99);
  Push(q, LOW, 3, 7);
  CHECK(out.Pop(q));
  result.push_back(static_cast<int>(out.frame));
  result.push_back(out.stream.get() == nullptr);
  result.push_back(out.value());
  result.push_back(q.num_queued_capped_frames());
  CHECK(out.Pop(q));
  result.push_back(out.value());
  result.push_back(q.num_queued_capped_frames());
  q.Clear();
  return result;
}

TEST(Http2WriteQueueRustTest, OutputAliasingAndPartialMutation) {
  EXPECT_EQ(PartialOutput<Http2WriteQueueOriginal>(),
            PartialOutput<SpdyWriteQueue>());
}

template <class Queue>
std::vector<int> ExpiredAndDynamicId() {
  std::vector<int> result;
  auto stream = MakeStream(MEDIUM, 1);
  Queue q;
  Push(q, MEDIUM, 3, 1, StreamWeakPtrFor<Queue>(*stream), [&] { result.push_back(1); });
  stream->set_stream_id(
      100);  // IDs must be read live, not captured at enqueue.
  q.RemovePendingWritesForStreamsAfter(1);
  CHECK(q.IsEmpty());
  Push(q, MEDIUM, 4, 2, StreamWeakPtrFor<Queue>(*stream), [&] { result.push_back(2); });
  stream.reset();
  q.RemovePendingWritesForStreamsAfter(
      0);  // Expired WeakPtr is not a live stream.
  result.push_back(q.IsEmpty());
  result.push_back(q.num_queued_capped_frames());
  q.Clear();
  return result;
}

TEST(Http2WriteQueueRustTest, WeakLifetimeAndLiveStreamIds) {
  EXPECT_EQ(ExpiredAndDynamicId<Http2WriteQueueOriginal>(),
            ExpiredAndDynamicId<SpdyWriteQueue>());
}

template <class Queue>
std::vector<int> DeleteOwner(int operation) {
  std::vector<int> destroyed;
  auto stream = MakeStream(MEDIUM, 2);
  auto q = std::make_unique<Queue>();
  Push(*q, MEDIUM, 3, 1, StreamWeakPtrFor<Queue>(*stream), [&] {
    destroyed.push_back(1);
    q.reset();
  });
  Push(*q, MEDIUM, 4, 2, StreamWeakPtrFor<Queue>(*stream), [&] { destroyed.push_back(2); });
  if (operation == 0) {
    q->Clear();
  }
  if (operation == 1) {
    q->RemovePendingWritesForStream(stream.get());
  }
  if (operation == 2) {
    q->RemovePendingWritesForStreamsAfter(1);
  }
  CHECK(!q);
  return destroyed;
}

TEST(Http2WriteQueueRustTest, DeferredCallbackMayDeleteOwner) {
  for (int op = 0; op < 3; ++op) {
    EXPECT_EQ(DeleteOwner<Http2WriteQueueOriginal>(op),
              DeleteOwner<SpdyWriteQueue>(op));
  }
}

template <class Queue>
std::vector<int> Destruction() {
  std::vector<int> result;
  {
    Queue q;
    for (int p = MAXIMUM_PRIORITY; p >= MINIMUM_PRIORITY; --p) {
      for (int j = 0; j < 4; ++j) {
        int id = p * 4 + j;
        Push(q, static_cast<RequestPriority>(p), 3, id, {}, [&, id] {
          result.push_back(id);
          result.push_back(q.IsEmpty());
          result.push_back(q.num_queued_capped_frames());
        });
      }
    }
  }
  return result;
}

TEST(Http2WriteQueueRustTest, DestructorPriorityFifoAndObservableState) {
  EXPECT_EQ(Destruction<Http2WriteQueueOriginal>(),
            Destruction<SpdyWriteQueue>());
}

TEST(Http2WriteQueueRustTest, RepeatedRingGrowthAndStablePriorityMoves) {
  for (uint64_t seed = 1; seed <= 8; ++seed) {
    Differential(seed, 8192);
  }
}

template <class Queue>
std::vector<int> LargeQueue() {
  std::vector<int> result;
  std::vector<std::unique_ptr<SpdyStream>> streams;
  for (int p = 0; p < NUM_PRIORITIES; ++p) {
    streams.push_back(MakeStream(static_cast<RequestPriority>(p), p * 2));
  }
  Queue q;
  auto push = [&](int first, int last) {
    for (int i = first; i < last; ++i) {
      const int p = i % NUM_PRIORITIES;
      Push(q, static_cast<RequestPriority>(p), i % 11, i,
           (i % 3) ? StreamWeakPtrFor<Queue>(*streams[p]) : base::WeakPtr<SpdyStream>(),
           [&, i] { result.push_back(-i - 1); });
    }
  };
  push(0, 4096);
  for (int i = 0; i < 1024; ++i) {
    OutputFor<Queue> out;
    CHECK(out.Pop(q));
    result.push_back(out.value());
  }
  push(4096, 6144);
  q.ChangePriorityOfWritesForStream(streams[MEDIUM].get(), MEDIUM, HIGHEST);
  q.RemovePendingWritesForStream(streams[LOW].get());
  q.RemovePendingWritesForStreamsAfter(7);
  result.push_back(q.num_queued_capped_frames());
  for (;;) {
    OutputFor<Queue> out;
    if (!out.Pop(q)) {
      break;
    }
    result.push_back(out.value());
    result.push_back(static_cast<int>(out.frame));
  }
  result.push_back(q.num_queued_capped_frames());
  return result;
}

TEST(Http2WriteQueueRustTest, ThousandsOfNodesWrapAndMiddleErase) {
  EXPECT_EQ(LargeQueue<Http2WriteQueueOriginal>(),
            LargeQueue<SpdyWriteQueue>());
}

TEST(Http2WriteQueueRustTest, IndependentQueuesOnConcurrentSequences) {
  // Each queue and its WeakPtrs stay on one sequence, as in the original API.
  std::vector<std::thread> threads;
  for (uint64_t seed = 1; seed <= 4; ++seed) {
    threads.emplace_back([seed] { Differential(seed * 3919, 4096); });
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

TEST(Http2WriteQueueRustTest, OutputProducerMayDeleteQueue) {
  // The original accessed freed queue state after output destruction; do not
  // execute that undefined path in the frozen reference. Native lifetime
  // protection must complete outputs without touching the deleted owner.
  auto q = std::make_unique<SpdyWriteQueue>();
  auto stream = MakeStream(MEDIUM, 5);
  Output out;
  out.producer = std::make_unique<Producer>(99, [&] { q.reset(); });
  Push(*q, MEDIUM, 3, 7, stream->GetWeakPtr());
  Push(*q, LOW, 4, 8);
  EXPECT_TRUE(out.Pop(*q));
  EXPECT_FALSE(q);
  EXPECT_EQ(7, out.value());
  EXPECT_EQ(spdy::SpdyFrameType::RST_STREAM, out.frame);
  EXPECT_EQ(stream.get(), out.stream.get());
  EXPECT_EQ(MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
            out.annotation);
}

}  // namespace
}  // namespace net::http2_write_queue_test
