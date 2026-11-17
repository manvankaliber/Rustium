// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <stddef.h>
#include <stdint.h>

#include "base/compiler_specific.h"
#include "net/http/http_auth_cache_rust_test_support.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer's initialized input remains valid for the call. Empty
  // inputs never construct a range from a potentially null pointer.
  std::vector<uint8_t> bytes;
  if (size) {
    bytes = UNSAFE_BUFFERS(std::vector<uint8_t>(data, data + size));
  }
  net::HttpAuthCacheRustDifferential(bytes);
  return 0;
}
