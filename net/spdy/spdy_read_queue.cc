// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_read_queue.h"

#include <utility>
#include <vector>

#include "base/check_op.h"
#include "net/spdy/http2_read_queue_rust_ffi.h"
#include "net/spdy/spdy_buffer.h"

namespace net {
namespace {

// Detached state has no relation to the possibly deleted queue owner. Every
// Rust borrow ends before a native destructor can invoke a consume callback.
void DiscardDetached(void* state) {
  if (!state) {
    return;
  }
  while (uintptr_t handle = http2_read_queue_rust_discard(state)) {
    std::unique_ptr<SpdyBuffer> buffer(reinterpret_cast<SpdyBuffer*>(handle));
  }
  http2_read_queue_rust_free_snapshot(state);
}

}  // namespace

SpdyReadQueue::SpdyReadQueue() : state_(nullptr) {}

SpdyReadQueue::~SpdyReadQueue() {
  // Keep the original read-only view while members are alive. Rust rejects
  // mutations after begin_destroy, and each guard ends before native callbacks.
  // Only the local state handle is used after the first buffer destructor.
  void* state = state_;
  if (!state) {
    return;
  }
  while (uintptr_t handle = http2_read_queue_rust_destroy_next(state)) {
    std::unique_ptr<SpdyBuffer> buffer(reinterpret_cast<SpdyBuffer*>(handle));
  }
}

bool SpdyReadQueue::IsEmpty() const {
  return GetTotalSize() == 0;
}

size_t SpdyReadQueue::GetTotalSize() const {
  return state_ ? http2_read_queue_rust_total(state_) : 0;
}

void SpdyReadQueue::Enqueue(std::unique_ptr<SpdyBuffer> buffer) {
  DCHECK_GT(buffer->GetRemainingSize(), 0u);
  const size_t remaining = buffer->GetRemainingSize();
  if (http2_read_queue_rust_enqueue(
          &state_, reinterpret_cast<uintptr_t>(buffer.get()), remaining)) {
    buffer.release();
  }
  // During destruction reject ownership by discarding the incoming buffer.
  // Its callback may delete the owner; no subsequent member access is allowed.
}

size_t SpdyReadQueue::Dequeue(base::span<uint8_t> out) {
  DCHECK_GT(out.size(), 0u);
  size_t bytes_copied = 0;
  std::vector<std::unique_ptr<SpdyBuffer>> to_discard;
  SpdyBuffer* partial = nullptr;
  size_t to_consume = 0;
  if (state_) {
    while (!out.empty()) {
      size_t bytes;
      uint8_t detached;
      uintptr_t handle =
          http2_read_queue_rust_next(state_, out.size(), &bytes, &detached);
      if (!handle) {
        break;
      }
      auto* buffer = reinterpret_cast<SpdyBuffer*>(handle);
      out.take_first(bytes).copy_from(buffer->GetRemaining().first(bytes));
      bytes_copied += bytes;
      if (detached) {
        to_discard.emplace_back(buffer);
      } else {
        partial = buffer;
        to_consume = bytes;
        break;
      }
    }
  }
  // Preserve the existing callback order: partial CONSUME first, then complete
  // buffer DISCARD callbacks. Accounting is final before either can reenter.
  if (partial) {
    partial->Consume(to_consume);  // May delete this and/or the partial buffer.
  }
  // Only locals are used beyond the first callback. Their destructors may
  // delete this as well; the detached buffers remain independently owned.
  return bytes_copied;
}

void SpdyReadQueue::Clear() {
  if (!state_) {
    return;
  }
  // Detach storage but preserve the original pre-clear read-only observations.
  // A reentrant mutation switches to the live queue; newly enqueued data
  // survives. The snapshot owns a Rust state reference: cleanup never touches
  // this, even if a callback deletes the owner or reuses its address for
  // another queue.
  void* detached = http2_read_queue_rust_take(state_);
  DiscardDetached(detached);  // May delete this. No member access follows.
}

}  // namespace net
