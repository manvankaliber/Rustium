// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_write_queue.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/containers/circular_deque.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/notreached.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "net/spdy/http2_write_queue_rust_ffi.h"
#include "net/spdy/spdy_buffer.h"
#include "net/spdy/spdy_buffer_producer.h"
#include "net/spdy/spdy_stream.h"

namespace net {

static_assert(static_cast<uint8_t>(spdy::SpdyFrameType::RST_STREAM) == 3);
static_assert(static_cast<uint8_t>(spdy::SpdyFrameType::SETTINGS) == 4);
static_assert(static_cast<uint8_t>(spdy::SpdyFrameType::PING) == 6);
static_assert(static_cast<uint8_t>(spdy::SpdyFrameType::GOAWAY) == 7);
static_assert(static_cast<uint8_t>(spdy::SpdyFrameType::WINDOW_UPDATE) == 8);
static_assert(MINIMUM_PRIORITY == 0);

bool IsSpdyFrameTypeWriteCapped(spdy::SpdyFrameType frame_type) {
  return http2_write_queue_rust_capped(static_cast<uint8_t>(frame_type));
}

// Rust owns traversal, matching, priority selection and count transitions.
// These synchronous services only move native ownership/read live WeakPtrs.
// Erase first moves the producer to the deferred vector, so no user callback
// runs inside Rust. The vector uses the original native destruction order.
struct Http2WriteQueueRustAdapter {
  explicit Http2WriteQueueRustAdapter(SpdyWriteQueue& owner) : owner(&owner) {}
  // Stack-scoped service context. Deferred callbacks may delete the owner
  // after Rust returns; the pointer is never read during context destruction.
  RAW_PTR_EXCLUSION SpdyWriteQueue* owner;
  SpdyWriteQueue::PendingWrite pending;
  std::vector<std::unique_ptr<SpdyBufferProducer>> erased;

  static size_t Service(void* context,
                        uint32_t op,
                        size_t p,
                        size_t i,
                        size_t arg) {
    auto& self = *static_cast<Http2WriteQueueRustAdapter*>(context);
    auto& owner = *self.owner;
    switch (op) {
      case 0:
        return owner.queue_[p].size();
      case 1:
        return static_cast<uint8_t>(owner.queue_[p][i].frame_type);
      case 2:
        return reinterpret_cast<size_t>(owner.queue_[p][i].stream.get());
      case 3:
        return owner.queue_[p][i].stream->stream_id();
      case 4:
        owner.queue_[p].push_back(std::move(self.pending));
        return 0;
      case 5:
        self.erased.push_back(std::move(owner.queue_[p][i].frame_producer));
        owner.queue_[p].erase(owner.queue_[p].begin() + i);
        return 0;
      case 6:
        owner.queue_[arg].push_back(std::move(owner.queue_[p][i]));
        owner.queue_[p].erase(owner.queue_[p].begin() + i);
        return 0;
      case 7:
        return static_cast<size_t>(owner.num_queued_capped_frames_);
      case 8:
        owner.num_queued_capped_frames_ = static_cast<int32_t>(arg);
        DCHECK_GE(owner.num_queued_capped_frames_, 0);
        return 0;
      case 9:
        owner.removing_writes_ = arg != 0;
        return 0;
    }
    NOTREACHED();
  }

  size_t Run(uint32_t op,
             size_t a = 0,
             size_t b = 0,
             SpdyStream* stream = nullptr) {
    return http2_write_queue_rust_run(this, Service, op, NUM_PRIORITIES, a, b,
                                      reinterpret_cast<size_t>(stream));
  }
};

SpdyWriteQueue::PendingWrite::PendingWrite() = default;

SpdyWriteQueue::PendingWrite::PendingWrite(
    spdy::SpdyFrameType frame_type,
    std::unique_ptr<SpdyBufferProducer> frame_producer,
    const base::WeakPtr<SpdyStream>& stream,
    const MutableNetworkTrafficAnnotationTag& traffic_annotation)
    : frame_type(frame_type),
      frame_producer(std::move(frame_producer)),
      stream(stream),
      traffic_annotation(traffic_annotation),
      has_stream(stream.get() != nullptr) {}

SpdyWriteQueue::PendingWrite::~PendingWrite() = default;

SpdyWriteQueue::PendingWrite::PendingWrite(PendingWrite&& other) = default;
SpdyWriteQueue::PendingWrite& SpdyWriteQueue::PendingWrite::operator=(
    PendingWrite&& other) = default;

SpdyWriteQueue::SpdyWriteQueue() = default;

SpdyWriteQueue::~SpdyWriteQueue() {
  weak_factory_.InvalidateWeakPtrs();
  DCHECK_GE(num_queued_capped_frames_, 0);
  Clear();
}

bool SpdyWriteQueue::IsEmpty() const {
  // This operation only reads services; no const state is modified.
  Http2WriteQueueRustAdapter adapter(const_cast<SpdyWriteQueue&>(*this));
  return adapter.Run(0);
}

void SpdyWriteQueue::Enqueue(
    RequestPriority priority,
    spdy::SpdyFrameType frame_type,
    std::unique_ptr<SpdyBufferProducer> frame_producer,
    const base::WeakPtr<SpdyStream>& stream,
    const NetworkTrafficAnnotationTag& traffic_annotation) {
  CHECK(!removing_writes_);
  CHECK_GE(priority, MINIMUM_PRIORITY);
  CHECK_LE(priority, MAXIMUM_PRIORITY);
  if (stream.get()) {
    DCHECK_EQ(stream->priority(), priority);
  }
  Http2WriteQueueRustAdapter adapter(*this);
  adapter.pending =
      PendingWrite(frame_type, std::move(frame_producer), stream,
                   MutableNetworkTrafficAnnotationTag(traffic_annotation));
  adapter.Run(1, priority, static_cast<uint8_t>(frame_type));
}

bool SpdyWriteQueue::Dequeue(
    spdy::SpdyFrameType* frame_type,
    std::unique_ptr<SpdyBufferProducer>* frame_producer,
    base::WeakPtr<SpdyStream>* stream,
    MutableNetworkTrafficAnnotationTag* traffic_annotation) {
  CHECK(!removing_writes_);
  Http2WriteQueueRustAdapter adapter(*this);
  const size_t selected = adapter.Run(2);
  if (!selected) {
    return false;
  }
  auto& queue = queue_[selected - 1];
  PendingWrite pending_write = std::move(queue.front());
  queue.pop_front();
  const auto weak_this = weak_factory_.GetWeakPtr();
  // Preserve assignment order and partial output visibility to destruction of
  // the previous output producer. Rust has returned before that callback runs.
  *frame_type = pending_write.frame_type;
  *frame_producer = std::move(pending_write.frame_producer);
  *stream = pending_write.stream;
  *traffic_annotation = pending_write.traffic_annotation;
  // All remaining outputs are local to this call. A deleted queue has no
  // remaining count to update or stream-lifetime invariant to diagnose.
  if (!weak_this) {
    return true;
  }
  if (pending_write.has_stream) {
    DCHECK(stream->get());
  }
  // Re-read the caller's frame_type: output-producer destruction may reenter
  // and overwrite it. This intentionally matches the original alias behavior.
  num_queued_capped_frames_ = http2_write_queue_rust_adjust(
      num_queued_capped_frames_, static_cast<uint8_t>(*frame_type), -1);
  DCHECK_GE(num_queued_capped_frames_, 0);
  return true;
}

void SpdyWriteQueue::RemovePendingWritesForStream(SpdyStream* stream) {
  CHECK(!removing_writes_);
  RequestPriority priority = stream->priority();
  CHECK_GE(priority, MINIMUM_PRIORITY);
  CHECK_LE(priority, MAXIMUM_PRIORITY);
  Http2WriteQueueRustAdapter adapter(*this);
#if DCHECK_IS_ON()
  DCHECK(!adapter.Run(7, priority, 0, stream));
#endif
  adapter.Run(3, priority, 0, stream);
}

void SpdyWriteQueue::RemovePendingWritesForStreamsAfter(
    spdy::SpdyStreamId last_good_stream_id) {
  CHECK(!removing_writes_);
  Http2WriteQueueRustAdapter adapter(*this);
  adapter.Run(4, last_good_stream_id);
}

void SpdyWriteQueue::ChangePriorityOfWritesForStream(
    SpdyStream* stream,
    RequestPriority old_priority,
    RequestPriority new_priority) {
  CHECK(!removing_writes_);
  DCHECK(stream);
  Http2WriteQueueRustAdapter adapter(*this);
#if DCHECK_IS_ON()
  DCHECK(!adapter.Run(7, old_priority, 0, stream));
#endif
  adapter.Run(5, old_priority, new_priority, stream);
}

void SpdyWriteQueue::Clear() {
  CHECK(!removing_writes_);
  Http2WriteQueueRustAdapter adapter(*this);
  adapter.Run(6);
  // No owner access follows deferred destruction. A callback may mutate or
  // destroy the queue, as with the original local deferred-producer vector.
}

}  // namespace net
