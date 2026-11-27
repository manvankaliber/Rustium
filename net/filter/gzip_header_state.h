// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_FILTER_GZIP_HEADER_STATE_H_
#define NET_FILTER_GZIP_HEADER_STATE_H_

#include <stddef.h>
#include <stdint.h>

namespace net::gzip_header_internal {

// Shared trivial state, matching the Rust repr(C) State. This keeps the parser
// inline, preserves the old GZipHeader size/alignment, and lets each input
// chunk cross the bridge with one state reference instead of separate fields.
struct State {
  int32_t phase = 0;
  uint8_t flags = 0;
  uint16_t extra_length = 0;
};

static_assert(sizeof(State) == 8);
static_assert(alignof(State) == 4);
static_assert(offsetof(State, flags) == 4);
static_assert(offsetof(State, extra_length) == 6);

}  // namespace net::gzip_header_internal

#endif  // NET_FILTER_GZIP_HEADER_STATE_H_
