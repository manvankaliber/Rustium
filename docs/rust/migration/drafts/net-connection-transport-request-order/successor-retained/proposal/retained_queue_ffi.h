// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_RETAINED_REQUEST_QUEUE_FFI_H_
#define NET_SOCKET_RETAINED_REQUEST_QUEUE_FFI_H_

#include <stddef.h>
#include <stdint.h>

// Director-owned proposal; requires ABI/layout, linkage and ownership review.
// The opaque pointer refers to Rust-owned queue state, never a native Request.
struct RustRequestOrderQueue;
struct RustRequestOrderReply {
  uint64_t token;
  size_t rank;
  uint64_t donor;
  uint32_t action;  // 0 none, 1 unassigned front, 2 donor.
};
// Status: 0 success, 1 missing, 2 invariant/input error, 3 allocation failure,
// 4 token exhaustion. Native adapter must CHECK failures; never convert these
// into a recoverable socket error. A successful zero token represents absence.
extern "C" {
size_t rust_request_queue_reply_layout(uint32_t field);
RustRequestOrderQueue* rust_request_queue_new();
void rust_request_queue_free(RustRequestOrderQueue* queue);
int32_t rust_request_queue_create(RustRequestOrderQueue* queue,
                                  uint8_t priority,
                                  uint8_t respect,
                                  size_t assigned,
                                  size_t unassigned,
                                  RustRequestOrderReply* out);
int32_t rust_request_queue_detach(RustRequestOrderQueue* queue,
                                  uint64_t token,
                                  uint8_t priority,
                                  uint32_t* out);
int32_t rust_request_queue_finish(RustRequestOrderQueue* queue,
                                  uint64_t token,
                                  size_t assigned,
                                  size_t unassigned,
                                  RustRequestOrderReply* out);
int32_t rust_request_queue_remove(RustRequestOrderQueue* queue,
                                  uint64_t token,
                                  uint8_t detached);
int32_t rust_request_queue_hole(const RustRequestOrderQueue* queue,
                                uint64_t token,
                                size_t assigned,
                                size_t unassigned,
                                RustRequestOrderReply* out);
int32_t rust_request_queue_release(const RustRequestOrderQueue* queue,
                                   size_t assigned,
                                   size_t unassigned,
                                   uint64_t* out);
int32_t rust_request_queue_at(const RustRequestOrderQueue* queue,
                              size_t rank,
                              uint64_t* out);
int32_t rust_request_queue_len(const RustRequestOrderQueue* queue, size_t* out);
int32_t rust_request_queue_snapshot(const RustRequestOrderQueue* queue,
                                    uint64_t* out,
                                    size_t capacity,
                                    size_t* written);
}

#endif  // NET_SOCKET_RETAINED_REQUEST_QUEUE_FFI_H_
