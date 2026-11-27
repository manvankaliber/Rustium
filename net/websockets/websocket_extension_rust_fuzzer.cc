// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <cstddef>
#include <cstdint>

#include "base/compiler_specific.h"
#include "net/websockets/websocket_extension_rust_test_support.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies size readable initialized bytes.
  auto input = size ? UNSAFE_BUFFERS(std::string_view(
                          reinterpret_cast<const char*>(data), size))
                    : std::string_view();
  net::websocket_extension_rust_test::Check(input);
  return 0;
}
