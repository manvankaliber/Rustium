// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HTTP2_READ_QUEUE_RUST_TEST_SUPPORT_H_
#define NET_SPDY_HTTP2_READ_QUEUE_RUST_TEST_SUPPORT_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <type_traits>
#include <vector>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "net/spdy/http2_read_queue_rust_original.h"
#include "net/spdy/spdy_buffer.h"
#include "net/spdy/spdy_buffer_rust_original.h"
#include "net/spdy/spdy_read_queue.h"

namespace net::http2_read_queue_test {

// Independent helper ownership for the C++ queue baseline. No conversion
// through the migrated public buffer API occurs in reference fixtures.
template <class Queue>
using BufferFor = std::conditional_t<
    std::is_same_v<Queue, Http2ReadQueueOriginal>, SpdyBufferOriginal, SpdyBuffer>;
static_assert(SpdyBufferOriginal::CONSUME == 0 && SpdyBuffer::CONSUME == 0);
static_assert(SpdyBufferOriginal::DISCARD == 1 && SpdyBuffer::DISCARD == 1);

struct Event {
  uint64_t id;
  size_t bytes;
  SpdyBuffer::ConsumeSource source;
  bool operator==(const Event&) const = default;
};

template <class Queue>
void Push(Queue& queue,
          uint64_t id,
          const std::vector<uint8_t>& data,
          std::vector<Event>& events) {
  auto buffer = std::make_unique<BufferFor<Queue>>(base::span(data));
  buffer->AddConsumeCallback(base::BindRepeating(
      [](std::vector<Event>* target, uint64_t token, size_t bytes,
         typename BufferFor<Queue>::ConsumeSource source) {
        target->push_back({token, bytes, static_cast<SpdyBuffer::ConsumeSource>(source)});
      },
      &events, id));
  queue.Enqueue(std::move(buffer));
}

// Includes constructors/destructors, FIFO bytes, output canaries, all size
// queries, callback byte/source/order traces, partial reads and repeated clear.
// Queue-specific safety extensions with original UB are tested separately.
inline void Differential(uint64_t seed, size_t transitions) {
  std::mt19937_64 random(seed);
  std::vector<Event> actual_events, expected_events;
  auto actual = std::make_unique<SpdyReadQueue>();
  auto expected = std::make_unique<Http2ReadQueueOriginal>();
  for (size_t i = 0; i < transitions; ++i) {
    unsigned action = random() % 16;
    if (action < 7) {
      std::vector<uint8_t> data(1 + random() % 257);
      for (auto& byte : data) {
        byte = static_cast<uint8_t>(random());
      }
      Push(*actual, i, data, actual_events);
      Push(*expected, i, data, expected_events);
    } else if (action < 14) {
      size_t size = 1 + random() % 1024;
      std::vector<uint8_t> a(size + 32, 0xa5), b(a);
      const size_t before = expected->GetTotalSize();
      size_t copied = actual->Dequeue(base::span(a).subspan(16u, size));
      CHECK_EQ(copied, expected->Dequeue(base::span(b).subspan(16u, size)));
      CHECK_EQ(copied, std::min(before, size));
      CHECK(a == b);
      CHECK(std::all_of(a.begin(), a.begin() + 16,
                        [](uint8_t c) { return c == 0xa5; }));
      CHECK(std::all_of(a.begin() + 16 + copied, a.end(),
                        [](uint8_t c) { return c == 0xa5; }));
    } else if (action == 14) {
      actual->Clear();
      expected->Clear();
    } else {
      actual = std::make_unique<SpdyReadQueue>();
      expected = std::make_unique<Http2ReadQueueOriginal>();
    }
    CHECK_EQ(actual->IsEmpty(), expected->IsEmpty());
    CHECK_EQ(actual->GetTotalSize(), expected->GetTotalSize());
    CHECK(actual_events == expected_events);
    actual_events.clear();
    expected_events.clear();
  }
  actual.reset();
  expected.reset();
  CHECK(actual_events == expected_events);
}

// Semantic assertions are independent of the other implementation's answer.
template <class Queue>
std::vector<Event> VerifyFixture() {
  std::vector<Event> events;
  {
    Queue queue;
    CHECK(queue.IsEmpty());
    CHECK_EQ(queue.GetTotalSize(), 0u);
    std::array<uint8_t, 5> out = {99, 99, 99, 99, 99};
    CHECK_EQ(queue.Dequeue(out), 0u);
    CHECK_EQ(out[0], 99u);
    Push(queue, 1, {1, 2}, events);
    Push(queue, 2, {3, 4, 5, 6}, events);
    CHECK_EQ(queue.GetTotalSize(), 6u);
    CHECK_EQ(queue.Dequeue(out), 5u);
    CHECK((out == std::array<uint8_t, 5>{1, 2, 3, 4, 5}));
    CHECK_EQ(queue.GetTotalSize(), 1u);
    CHECK_EQ(events.size(), 2u);
    // The partial buffer is consumed before detached full buffers are dropped.
    CHECK((events[0] == Event{2, 3, SpdyBuffer::CONSUME}));
    CHECK((events[1] == Event{1, 2, SpdyBuffer::DISCARD}));
    queue.Clear();
    queue.Clear();
    CHECK(queue.IsEmpty());
    CHECK((events.back() == Event{2, 1, SpdyBuffer::DISCARD}));
    Push(queue, 3, {7, 8, 9}, events);
  }
  CHECK((events.back() == Event{3, 3, SpdyBuffer::DISCARD}));
  return events;
}

}  // namespace net::http2_read_queue_test
#endif  // NET_SPDY_HTTP2_READ_QUEUE_RUST_TEST_SUPPORT_H_
