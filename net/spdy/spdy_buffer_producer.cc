// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_buffer_producer.h"
#include "net/spdy/http2_family_rust_ffi.h"

#include <utility>
#include <optional>
#include "base/notreached.h"

#include "base/check.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "net/spdy/spdy_buffer.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"

namespace net {

SpdyBufferProducer::SpdyBufferProducer() = default;

SpdyBufferProducer::~SpdyBufferProducer() = default;

SimpleBufferProducer::SimpleBufferProducer(std::unique_ptr<SpdyBuffer> buffer)
    : buffer_(std::move(buffer)) {}

SimpleBufferProducer::~SimpleBufferProducer() = default;

std::unique_ptr<SpdyBuffer> SimpleBufferProducer::ProduceBuffer() {
  // Proposed family boundary: Rust owns method control flow.
  std::unique_ptr<SpdyBuffer> result;
  Http2FamilyRun(3, 0, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        DCHECK(buffer_);
        return 0;
      }
      case 1: {  // Move
        result = std::move(buffer_);
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return result;
}

}  // namespace net
