// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_buffer_producer_rust_original.h"

#include <utility>

#include "base/check.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "net/spdy/spdy_buffer_rust_original.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"

namespace net {

SpdyBufferProducerOriginal::SpdyBufferProducerOriginal() = default;

SpdyBufferProducerOriginal::~SpdyBufferProducerOriginal() = default;

SimpleBufferProducerOriginal::SimpleBufferProducerOriginal(std::unique_ptr<SpdyBufferOriginal> buffer)
    : buffer_(std::move(buffer)) {}

SimpleBufferProducerOriginal::~SimpleBufferProducerOriginal() = default;

std::unique_ptr<SpdyBufferOriginal> SimpleBufferProducerOriginal::ProduceBuffer() {
  DCHECK(buffer_);
  return std::move(buffer_);
}

}  // namespace net
