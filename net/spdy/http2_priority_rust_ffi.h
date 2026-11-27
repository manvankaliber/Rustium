// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HTTP2_PRIORITY_RUST_FFI_H_
#define NET_SPDY_HTTP2_PRIORITY_RUST_FFI_H_

#include <cstddef>
#include <cstdint>

struct Http2PriorityRustState;
struct Http2PriorityRustUpdate {
  uint32_t id;
  uint32_t parent;
  int32_t weight;
  uint8_t exclusive;
  uint8_t has_parent;
};
static_assert(sizeof(Http2PriorityRustUpdate) == 16);
static_assert(offsetof(Http2PriorityRustUpdate, weight) == 8);
static_assert(offsetof(Http2PriorityRustUpdate, has_parent) == 13);

// Opaque unique state, same sequence/threading requirements as the original.
// No callbacks, external references, pointer retention, or native policy calls.
// create leaves output untouched on duplicate; update initializes only count
// elements (0..2). Priorities must be 0..7 except duplicate/missing early
// returns.
extern "C" {
Http2PriorityRustState* http2_priority_rust_new();
void http2_priority_rust_delete(Http2PriorityRustState* state);
Http2PriorityRustState* http2_priority_rust_clone(
    const Http2PriorityRustState* state);
uint8_t http2_priority_rust_create(Http2PriorityRustState* state,
                                   uint32_t id,
                                   uint8_t priority,
                                   Http2PriorityRustUpdate* output);
void http2_priority_rust_destroy(Http2PriorityRustState* state, uint32_t id);
size_t http2_priority_rust_update(Http2PriorityRustState* state,
                                  uint32_t id,
                                  uint8_t priority,
                                  Http2PriorityRustUpdate* output);
}

#endif  // NET_SPDY_HTTP2_PRIORITY_RUST_FFI_H_
