// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HTTP2_READ_QUEUE_RUST_FFI_H_
#define NET_SPDY_HTTP2_READ_QUEUE_RUST_FFI_H_

#include <stddef.h>
#include <stdint.h>

// Opaque Rust queue. Calls are synchronous and never invoke native callbacks.
// Buffer handles are native addresses, never dereferenced by Rust. Ownership
// transfers on accepted enqueue, complete next, and discard. Partial next
// retains it. Enqueue lazily creates a null owner. Queue handles own a Rust Rc
// reference; the final destroy_next zero result releases it. Snapshots own a
// Box and an Rc. Callers serialize access, complete native copies before any
// callbacks, and never use the owner's state after invoking a callback which
// may delete it.
extern "C" {
size_t http2_read_queue_rust_total(const void* queue);
bool http2_read_queue_rust_enqueue(void** queue,
                                   uintptr_t handle,
                                   size_t remaining);
uintptr_t http2_read_queue_rust_next(void* queue,
                                     size_t limit,
                                     size_t* bytes,
                                     uint8_t* detached);
void* http2_read_queue_rust_take(void* queue);
uintptr_t http2_read_queue_rust_discard(void* queue);
void http2_read_queue_rust_free_snapshot(void* snapshot);
uintptr_t http2_read_queue_rust_destroy_next(void* queue);
}

#endif  // NET_SPDY_HTTP2_READ_QUEUE_RUST_FFI_H_
