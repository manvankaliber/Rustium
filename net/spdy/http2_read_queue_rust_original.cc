// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/http2_read_queue_rust_original.h"

#include <algorithm>
#include <utility>

#include "base/check_op.h"
#include "net/spdy/spdy_buffer_rust_original.h"

namespace net {

Http2ReadQueueOriginal::Http2ReadQueueOriginal() = default;

Http2ReadQueueOriginal::~Http2ReadQueueOriginal() {
  Clear();
}

bool Http2ReadQueueOriginal::IsEmpty() const {
  DCHECK_EQ(queue_.empty(), total_size_ == 0);
  return queue_.empty();
}

size_t Http2ReadQueueOriginal::GetTotalSize() const {
  return total_size_;
}

void Http2ReadQueueOriginal::Enqueue(std::unique_ptr<SpdyBufferOriginal> buffer) {
  DCHECK_GT(buffer->GetRemainingSize(), 0u);
  total_size_ += buffer->GetRemainingSize();
  queue_.push_back(std::move(buffer));
}

size_t Http2ReadQueueOriginal::Dequeue(base::span<uint8_t> out) {
  DCHECK_GT(out.size(), 0u);
  size_t bytes_copied = 0;
  // The destructor of SpdyBufferOriginal and SpdyBufferOriginal::Consume() run ConsumeCallbacks
  // that can re-enter arbitrary network-stack code (session WINDOW_UPDATE ->
  // DoDrainSession) and synchronously destroy the SpdyHttpStream that owns
  // `this`. Defer firing them until after we are done touching member state.
  std::vector<std::unique_ptr<SpdyBufferOriginal>> to_discard;
  size_t to_consume = 0;
  while (!queue_.empty() && !out.empty()) {
    SpdyBufferOriginal* buffer = queue_.front().get();
    size_t bytes_to_copy = std::min(out.size(), buffer->GetRemainingSize());
    out.take_first(bytes_to_copy)
        .copy_from(buffer->GetRemaining().first(bytes_to_copy));
    bytes_copied += bytes_to_copy;
    if (bytes_to_copy == buffer->GetRemainingSize()) {
      to_discard.push_back(std::move(queue_.front()));
      queue_.pop_front();
    } else {
      to_consume = bytes_to_copy;
      break;
    }
  }
  total_size_ -= bytes_copied;
  if (to_consume) {
    // May delete `this`.
    queue_.front()->Consume(to_consume);
  }
  // `to_discard` destruction may delete `this`; do not touch members past here.
  return bytes_copied;
}

void Http2ReadQueueOriginal::Clear() {
  queue_.clear();
  total_size_ = 0;
}

}  // namespace net
