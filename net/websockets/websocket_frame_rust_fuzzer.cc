// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/websockets/websocket_frame_rust_test_support.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer provides size initialized readable bytes for this call.
  auto input = UNSAFE_BUFFERS(base::span(data, size));
  uint64_t seed = size;
  for (uint8_t b : input) {
    seed = seed * 131 + b;
  }
  net::websocket_frame_rust_test::Public(input, seed);
  return 0;
}
