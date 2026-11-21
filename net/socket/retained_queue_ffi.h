// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_RETAINED_REQUEST_QUEUE_FFI_H_
#define NET_SOCKET_RETAINED_REQUEST_QUEUE_FFI_H_

#include <stddef.h>
#include <stdint.h>

#include "net/base/net_export.h"

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
int32_t rust_request_queue_release(RustRequestOrderQueue* queue,
                                   size_t assigned,
                                   size_t unassigned,
                                   uint8_t added_preconnect,
                                   uint64_t* out);
int32_t rust_request_queue_preconnect_claims(
    const RustRequestOrderQueue* queue,
    size_t* out);
int32_t rust_request_queue_claim_preconnect(RustRequestOrderQueue* queue,
                                            uint8_t* out);
int32_t rust_request_queue_trim_preconnect_claims(RustRequestOrderQueue* queue,
                                                 size_t job_count);
int32_t rust_request_queue_at(const RustRequestOrderQueue* queue,
                              size_t rank,
                              uint64_t* out);
int32_t rust_request_queue_priority(const RustRequestOrderQueue* queue,
                                   uint64_t token,
                                   uint8_t* out);
int32_t rust_request_queue_len(const RustRequestOrderQueue* queue, size_t* out);
// `out` must point to capacity initialized, aligned u64 values; storage must
// not overlap queue or written. Null is permitted only at zero capacity.
int32_t rust_request_queue_snapshot(const RustRequestOrderQueue* queue,
                                    uint64_t* out,
                                    size_t capacity,
                                    size_t* written);
// Same initialized/aligned/nonalias contract; copies at most capacity tokens.
int32_t rust_request_queue_snapshot_prefix(const RustRequestOrderQueue* queue,
                                           uint64_t* out,
                                           size_t capacity,
                                           size_t* written);
}

namespace net {

NET_EXPORT_PRIVATE size_t RetainedQueueReplyLayoutForTesting(uint32_t field);

}  // namespace net

#endif  // NET_SOCKET_RETAINED_REQUEST_QUEUE_FFI_H_
