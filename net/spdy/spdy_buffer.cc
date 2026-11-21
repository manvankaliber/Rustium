// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_buffer.h"

#include <cstring>
#include <optional>
#include <utility>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "net/base/io_buffer.h"
#include "net/spdy/spdy_buffer_rust_ffi.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"

namespace net {

namespace {

// Retained for Chromium's original fatal diagnostic expression. Rust owns the
// raw-copy size decision; constructing from an existing frame has no such cap.
const size_t kMaxSpdyFrameSize = 0x00ffffff;

// Native allocation/copy/ownership primitive. The Rust decision has already
// accepted data.size(); no C++ size policy is repeated in this primitive.
std::unique_ptr<spdy::SpdySerializedFrame> MakeSpdySerializedFrame(
    base::span<const uint8_t> data) {
  auto frame_data = std::make_unique<char[]>(data.size());
  // SAFETY: frame_data and data have the same size. Quiche takes ownership of
  // the character allocation, which must retain that allocation's type.
  UNSAFE_BUFFERS(std::memcpy(frame_data.get(), data.data(), data.size()));
  return std::make_unique<spdy::SpdySerializedFrame>(std::move(frame_data),
                                                     data.size());
}

static_assert(SpdyBuffer::CONSUME == 0);
static_assert(SpdyBuffer::DISCARD == 1);

}  // namespace

// Retains a shared frame and a fixed view independently of the SpdyBuffer's
// cursor or lifetime. Native IOBuffer/refcounting remain ownership primitives.
class SpdyBuffer::SharedFrameIOBuffer : public IOBuffer {
 public:
  SharedFrameIOBuffer(const scoped_refptr<SharedFrame>& shared_frame,
                      size_t offset)
      : IOBuffer(base::span(*shared_frame->data)
                     .last(spdy_buffer_rust_remaining(shared_frame->data->size(),
                                                       offset))),
        shared_frame_(shared_frame) {}

  SharedFrameIOBuffer(const SharedFrameIOBuffer&) = delete;
  SharedFrameIOBuffer& operator=(const SharedFrameIOBuffer&) = delete;

 private:
  ~SharedFrameIOBuffer() override {
    // Clear the native view before the last shared frame can be released.
    ClearSpan();
  }

  const scoped_refptr<SharedFrame> shared_frame_;
};

SpdyBuffer::SpdyBuffer(std::unique_ptr<spdy::SpdySerializedFrame> frame)
    : shared_frame_(base::MakeRefCounted<SharedFrame>(std::move(frame))),
      offset_(spdy_buffer_rust_initial_offset()) {}

// The given data may not be strictly a SPDY frame; the frame is its container.
SpdyBuffer::SpdyBuffer(base::span<const uint8_t> data)
    : shared_frame_(base::MakeRefCounted<SharedFrame>()),
      offset_(spdy_buffer_rust_initial_offset()) {
  switch (spdy_buffer_rust_copy_action(data.size())) {
    case SpdyBufferRustCopyAction::kCopy:
      break;
    case SpdyBufferRustCopyAction::kEmpty:
      CHECK_GT(data.size(), 0u);
      NOTREACHED();
    case SpdyBufferRustCopyAction::kTooLarge:
      CHECK_LE(data.size(), kMaxSpdyFrameSize);
      NOTREACHED();
  }
  shared_frame_->data = MakeSpdySerializedFrame(data);
}

SpdyBuffer::~SpdyBuffer() {
  // Rust chooses whether/what to discard. The primitive reuses the ordinary
  // native diagnostics and commit/snapshot path rather than duplicating it.
  spdy_buffer_rust_discard(
      shared_frame_->data->size(), offset_, this,
      [](void* context, size_t amount, uint32_t source) {
        static_cast<SpdyBuffer*>(context)->ConsumeHelper(
            amount, static_cast<ConsumeSource>(source));
      });
}

base::span<const uint8_t> SpdyBuffer::GetRemaining() const {
  std::string_view frame_view(*shared_frame_->data);
  return base::as_byte_span(frame_view).last(
      spdy_buffer_rust_remaining(frame_view.size(), offset_));
}

size_t SpdyBuffer::GetRemainingSize() const {
  return spdy_buffer_rust_remaining(shared_frame_->data->size(), offset_);
}

void SpdyBuffer::AddConsumeCallback(const ConsumeCallback& consume_callback) {
  // Native callback ownership primitive: append in registration order.
  consume_callbacks_.push_back(consume_callback);
}

void SpdyBuffer::Consume(size_t consume_size) {
  ConsumeHelper(consume_size, CONSUME);
}

scoped_refptr<IOBuffer> SpdyBuffer::GetIOBufferForRemainingData() {
  return base::MakeRefCounted<SharedFrameIOBuffer>(shared_frame_, offset_);
}

void SpdyBuffer::ConsumeHelper(size_t consume_size,
                               ConsumeSource consume_source) {
  // Preserve Chromium's exact precondition diagnostic order/expressions.
  DCHECK_GE(consume_size, 1u);
  DCHECK_LE(consume_size, GetRemainingSize());

  struct Dispatch {
    raw_ptr<SpdyBuffer> owner;
    std::optional<std::vector<ConsumeCallback>> callbacks;
  } dispatch{this, std::nullopt};

  spdy_buffer_rust_consume(
      shared_frame_->data->size(), offset_, consume_size,
      static_cast<uint32_t>(consume_source), &dispatch,
      [](void* context, size_t next_offset) -> size_t {
        auto& state = *static_cast<Dispatch*>(context);
        // The native field must be committed before callback ownership is
        // copied. Only this primitive accesses the owner during dispatch.
        state.owner->offset_ = next_offset;
        state.callbacks.emplace(state.owner->consume_callbacks_);
        // Callbacks may delete the owner; retain only the local snapshot.
        state.owner = nullptr;
        return state.callbacks->size();
      },
      [](void* context, size_t index, size_t amount, uint32_t source) {
        auto& state = *static_cast<Dispatch*>(context);
        base::span(*state.callbacks)[index].Run(
            amount, static_cast<ConsumeSource>(source));
      });
  // Rust iterates the fixed local snapshot. A callback may reenter or delete
  // this buffer; neither Rust nor this function revisits the owner afterward.
  // Native snapshot destruction keeps captures alive through the full loop.
}

}  // namespace net
