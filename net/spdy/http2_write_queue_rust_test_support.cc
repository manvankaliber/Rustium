// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/spdy/http2_write_queue_rust_test_support.h"

#include "net/spdy/spdy_buffer.h"
#include "net/spdy/spdy_buffer_rust_original.h"

namespace net::http2_write_queue_test {
Producer::Producer(int value, std::function<void()> on_destroy)
    : value(value), on_destroy_(std::move(on_destroy)) {}
Producer::~Producer() {
  if (on_destroy_) {
    on_destroy_();
  }
}
std::unique_ptr<SpdyBuffer> Producer::ProduceBuffer() {
  return nullptr;
}
OriginalProducer::OriginalProducer(int value, std::function<void()> on_destroy)
    : value(value), on_destroy_(std::move(on_destroy)) {}
OriginalProducer::~OriginalProducer() {
  if (on_destroy_) {
    on_destroy_();
  }
}
std::unique_ptr<SpdyBufferOriginal> OriginalProducer::ProduceBuffer() {
  return nullptr;
}
}  // namespace net::http2_write_queue_test
