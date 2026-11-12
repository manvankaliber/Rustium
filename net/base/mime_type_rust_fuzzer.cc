// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <cstdint>

#include "base/compiler_specific.h"
#include "net/base/mime_type_rust_test_support.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer provides size readable bytes for this callback's
  // lifetime.
  net::mime_type_test::Check(UNSAFE_BUFFERS(
      std::string_view(reinterpret_cast<const char*>(data), size)));
  return 0;
}
