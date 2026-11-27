// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "net/spdy/http2_read_queue_rust_original.h"
#include "net/spdy/spdy_buffer.h"
#include "net/spdy/spdy_read_queue.h"

namespace {

uint8_t Byte(base::span<const uint8_t>& input) {
  return input.empty() ? 0 : input.take_first<1>()[0];
}

struct Event {
  size_t buffer;
  size_t callback;
  size_t bytes;
  net::SpdyBuffer::ConsumeSource source;
  size_t total;
  bool empty;
  bool operator==(const Event&) const = default;
};

template <class Queue>
void Enqueue(Queue& queue,
             size_t id,
             base::span<const uint8_t> payload,
             size_t consumed,
             size_t callbacks,
             std::vector<Event>& events) {
  auto buffer = std::make_unique<net::SpdyBuffer>(payload);
  if (consumed != 0) {
    buffer->Consume(consumed);
  }
  for (size_t i = 0; i < callbacks; ++i) {
    // The queue object remains alive through its destructor's Clear call.
    // Read only; never mutate a buffer/container while it is being destroyed.
    buffer->AddConsumeCallback(base::BindRepeating(
        [](Queue* owner, std::vector<Event>* trace, size_t token, size_t index,
           size_t bytes, net::SpdyBuffer::ConsumeSource source) {
          const size_t total = owner->GetTotalSize();
          const bool empty = owner->IsEmpty();
          CHECK_EQ(empty, total == 0);
          trace->push_back({token, index, bytes, source, total, empty});
        },
        &queue, &events, id, i));
  }
  queue.Enqueue(std::move(buffer));
}

void Fuzz(base::span<const uint8_t> input) {
  // Traces outlive both queues, including callbacks fired during destruction.
  std::vector<Event> actual_events;
  std::vector<Event> expected_events;
  auto actual = std::make_unique<net::SpdyReadQueue>();
  auto expected = std::make_unique<net::Http2ReadQueueOriginal>();
  std::deque<uint8_t> fifo;
  for (size_t step = 0; step < 256 && !input.empty(); ++step) {
    switch (Byte(input) % 8) {
      case 0:
      case 1:
      case 2: {
        const size_t length = 1u + Byte(input);
        // Never enqueue an empty buffer, including after pre-consumption.
        const size_t consumed = Byte(input) % length;
        const size_t callbacks = Byte(input) % 4;
        std::vector<uint8_t> payload(length, 0);
        const size_t available = std::min(length, input.size());
        base::span(payload).first(available).copy_from(
            input.take_first(available));
        Enqueue(*actual, step, payload, consumed, callbacks, actual_events);
        Enqueue(*expected, step, payload, consumed, callbacks, expected_events);
        for (uint8_t byte : base::span(payload).subspan(consumed)) {
          fifo.push_back(byte);
        }
        break;
      }
      case 3:
      case 4: {
        const size_t low = Byte(input);
        const size_t request = 1u + (low + 256u * Byte(input)) % 1024;
        std::vector<uint8_t> a(request + 32, 0xa5);
        std::vector<uint8_t> b(a);
        const size_t want = std::min(request, fifo.size());
        const size_t copied =
            actual->Dequeue(base::span(a).subspan(16u, request));
        CHECK_EQ(copied,
                 expected->Dequeue(base::span(b).subspan(16u, request)));
        CHECK_EQ(copied, want);
        CHECK(a == b);
        CHECK(std::ranges::all_of(base::span(a).first<16>(),
                                  [](uint8_t c) { return c == 0xa5; }));
        CHECK(std::ranges::all_of(base::span(a).subspan(16u + copied),
                                  [](uint8_t c) { return c == 0xa5; }));
        for (uint8_t byte : base::span(a).subspan(16u, copied)) {
          CHECK_EQ(byte, fifo.front());
          fifo.pop_front();
        }
        break;
      }
      case 5:
        actual->Clear();
        expected->Clear();
        actual->Clear();
        expected->Clear();
        fifo.clear();
        break;
      case 6:
        actual.reset();
        expected.reset();
        actual = std::make_unique<net::SpdyReadQueue>();
        expected = std::make_unique<net::Http2ReadQueueOriginal>();
        fifo.clear();
        break;
      case 7:
        break;  // Queries below also execute after every mutation.
    }
    CHECK_EQ(actual->GetTotalSize(), fifo.size());
    CHECK_EQ(expected->GetTotalSize(), fifo.size());
    CHECK_EQ(actual->IsEmpty(), fifo.empty());
    CHECK_EQ(expected->IsEmpty(), fifo.empty());
    CHECK(actual_events == expected_events);
    actual_events.clear();
    expected_events.clear();
  }
  actual.reset();
  expected.reset();
  CHECK(actual_events == expected_events);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies size initialized readable bytes for this call.
  // All parsing/copying afterward uses checked spans; empty input needs no
  // data.
  const auto input = size == 0 ? base::span<const uint8_t>()
                               : UNSAFE_BUFFERS(base::span(data, size));
  Fuzz(input.first(std::min(input.size(), size_t{8192})));
  return 0;
}
