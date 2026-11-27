// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HTTP2_WRITE_QUEUE_RUST_TEST_SUPPORT_H_
#define NET_SPDY_HTTP2_WRITE_QUEUE_RUST_TEST_SUPPORT_H_

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include "base/check_op.h"
#include "net/log/net_log_with_source.h"
#include "net/spdy/http2_write_queue_rust_original.h"
#include "net/spdy/http2_write_queue_original_stream_access.h"
#include "net/spdy/spdy_buffer_producer.h"
#include "net/spdy/spdy_buffer_producer_rust_original.h"
#include "net/spdy/spdy_stream.h"
#include "net/spdy/spdy_write_queue.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "url/gurl.h"

namespace net::http2_write_queue_test {

// Select only the baseline's original native getter. Keep this call at the
// existing fixture/timed call site; never precompute/cache WeakPtrs. The Rust
// queue side keeps the actual integrated public Stream API and its bridge.
template <class Queue>
base::WeakPtr<SpdyStream> StreamWeakPtrFor(SpdyStream& stream) {
  static_assert(std::is_same_v<Queue, Http2WriteQueueOriginal> ||
                std::is_same_v<Queue, SpdyWriteQueue>);
  if constexpr (std::is_same_v<Queue, Http2WriteQueueOriginal>) {
    return Http2WriteQueueOriginalStreamAccess::GetWeakPtr(stream);
  } else {
    return stream.GetWeakPtr();
  }
}

inline std::unique_ptr<SpdyStream> MakeStream(RequestPriority priority,
                                              uint32_t id) {
  auto stream = std::make_unique<SpdyStream>(
      SPDY_BIDIRECTIONAL_STREAM, base::WeakPtr<SpdySession>(), GURL(), priority,
      0, 0, NetLogWithSource(), TRAFFIC_ANNOTATION_FOR_TESTS, false);
  stream->set_stream_id(id);
  return stream;
}

class Producer final : public SpdyBufferProducer {
 public:
  Producer(int value, std::function<void()> on_destroy = {});
  ~Producer() override;
  std::unique_ptr<SpdyBuffer> ProduceBuffer() override;
  const int value;

 private:
  std::function<void()> on_destroy_;
};

class OriginalProducer final : public SpdyBufferProducerOriginal {
 public:
  OriginalProducer(int value, std::function<void()> on_destroy = {});
  ~OriginalProducer() override;
  std::unique_ptr<SpdyBufferOriginal> ProduceBuffer() override;
  const int value;

 private:
  std::function<void()> on_destroy_;
};

template <class Queue>
using ProducerFor = std::conditional_t<
    std::is_same_v<Queue, Http2WriteQueueOriginal>, OriginalProducer, Producer>;
template <class Queue>
using ProducerBaseFor = std::conditional_t<
    std::is_same_v<Queue, Http2WriteQueueOriginal>, SpdyBufferProducerOriginal,
    SpdyBufferProducer>;

template <class Queue>
void Push(Queue& queue,
          RequestPriority priority,
          uint8_t frame,
          int id,
          base::WeakPtr<SpdyStream> stream = {},
          std::function<void()> on_destroy = {}) {
  queue.Enqueue(priority, static_cast<spdy::SpdyFrameType>(frame),
                std::make_unique<ProducerFor<Queue>>(id, std::move(on_destroy)), stream,
                TRAFFIC_ANNOTATION_FOR_TESTS);
}

template <class Queue>
struct OutputFor {
  spdy::SpdyFrameType frame = spdy::SpdyFrameType::ACCEPT_CH;
  std::unique_ptr<ProducerBaseFor<Queue>> producer;
  base::WeakPtr<SpdyStream> stream;
  MutableNetworkTrafficAnnotationTag annotation;
  bool Pop(Queue& queue) {
    return queue.Dequeue(&frame, &producer, &stream, &annotation);
  }
  int value() const {
    return producer ? static_cast<ProducerFor<Queue>*>(producer.get())->value : -1;
  }
};

using Output = OutputFor<SpdyWriteQueue>;
using OriginalOutput = OutputFor<Http2WriteQueueOriginal>;

// Runs the real frozen public C++ API and the real production Rust-backed API.
// Streams are shared deterministic services; each queue owns distinct
// producers. No independently implemented model predicts the queue policy.
inline void Differential(uint64_t seed, size_t operations) {
  std::vector<std::unique_ptr<SpdyStream>> streams;
  for (int p = 0; p < NUM_PRIORITIES; ++p) {
    streams.push_back(MakeStream(static_cast<RequestPriority>(p), p * 2));
  }
  std::vector<int> old_destroyed, new_destroyed;
  Http2WriteQueueOriginal old;
  SpdyWriteQueue fresh;
  int next = 0;
  auto random = [&] {
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    return seed;
  };
  for (size_t i = 0; i < operations; ++i) {
    const uint64_t bits = random();
    const size_t p = (bits >> 8) % NUM_PRIORITIES;
    const auto priority = static_cast<RequestPriority>(p);
    switch (bits % 10) {
      case 0:
      case 1:
      case 2:
      case 3: {
        auto original_weak = (bits & 128)
                                 ? StreamWeakPtrFor<Http2WriteQueueOriginal>(*streams[p])
                                 : base::WeakPtr<SpdyStream>();
        auto production_weak = (bits & 128)
                                 ? StreamWeakPtrFor<SpdyWriteQueue>(*streams[p])
                                 : base::WeakPtr<SpdyStream>();
        const int id = next++;
        const uint8_t frame = (bits & 64) ? (bits >> 32) % 11 : bits >> 32;
        Push(old, priority, frame, id, original_weak,
             [&, id] { old_destroyed.push_back(id); });
        Push(fresh, priority, frame, id, production_weak,
             [&, id] { new_destroyed.push_back(id); });
        break;
      }
      case 4:
      case 5: {
        OriginalOutput a;
        Output b;
        a.producer = std::make_unique<OriginalProducer>(-99);
        b.producer = std::make_unique<Producer>(-99);
        CHECK_EQ(a.Pop(old), b.Pop(fresh));
        CHECK_EQ(a.frame, b.frame);
        CHECK_EQ(a.value(), b.value());
        CHECK_EQ(a.stream.get(), b.stream.get());
        CHECK(a.annotation == b.annotation);
        break;
      }
      case 6:
        old.RemovePendingWritesForStream(streams[p].get());
        fresh.RemovePendingWritesForStream(streams[p].get());
        break;
      case 7:
        old.RemovePendingWritesForStreamsAfter(bits >> 40);
        fresh.RemovePendingWritesForStreamsAfter(bits >> 40);
        break;
      case 8: {
        const auto to = static_cast<RequestPriority>(
            (p + 1 + (bits >> 16) % 5) % NUM_PRIORITIES);
        old.ChangePriorityOfWritesForStream(streams[p].get(), priority, to);
        fresh.ChangePriorityOfWritesForStream(streams[p].get(), priority, to);
        OriginalOutput a;
        Output b;
        CHECK_EQ(a.Pop(old), b.Pop(fresh));
        CHECK_EQ(a.frame, b.frame);
        CHECK_EQ(a.value(), b.value());
        CHECK_EQ(a.stream.get(), b.stream.get());
        CHECK(a.annotation == b.annotation);
        old.ChangePriorityOfWritesForStream(streams[p].get(), to, priority);
        fresh.ChangePriorityOfWritesForStream(streams[p].get(), to, priority);
        break;
      }
      case 9:
        old.Clear();
        fresh.Clear();
        break;
    }
    CHECK_EQ(old.IsEmpty(), fresh.IsEmpty());
    CHECK_EQ(old.num_queued_capped_frames(), fresh.num_queued_capped_frames());
    CHECK(old_destroyed == new_destroyed);
  }
  for (;;) {
    OriginalOutput a;
        Output b;
    bool more = a.Pop(old);
    CHECK_EQ(more, b.Pop(fresh));
    CHECK_EQ(a.value(), b.value());
    CHECK_EQ(a.frame, b.frame);
    CHECK_EQ(a.stream.get(), b.stream.get());
    CHECK(a.annotation == b.annotation);
    if (!more) {
      break;
    }
  }
  CHECK(old_destroyed == new_destroyed);
}

}  // namespace net::http2_write_queue_test
#endif  // NET_SPDY_HTTP2_WRITE_QUEUE_RUST_TEST_SUPPORT_H_
