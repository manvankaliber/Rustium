// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HTTP2_WRITE_QUEUE_RUST_FFI_H_
#define NET_SPDY_HTTP2_WRITE_QUEUE_RUST_FFI_H_

#include <stddef.h>
#include <stdint.h>

// All callbacks are synchronous primitive storage operations. They must not
// invoke producers/user callbacks. The context is stack-owned by the caller.
using Http2WriteQueueRustService =
    size_t (*)(void*, uint32_t, size_t, size_t, size_t);
extern "C" {
uint8_t http2_write_queue_rust_capped(uint8_t);
int32_t http2_write_queue_rust_adjust(int32_t, uint8_t, int32_t);
size_t http2_write_queue_rust_run(void*,
                                  Http2WriteQueueRustService,
                                  uint32_t,
                                  size_t,
                                  size_t,
                                  size_t,
                                  size_t);
}

#endif  // NET_SPDY_HTTP2_WRITE_QUEUE_RUST_FFI_H_
