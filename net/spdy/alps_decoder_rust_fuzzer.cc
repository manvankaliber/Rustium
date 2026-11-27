// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/spdy/alps_decoder_rust_test_support.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // libFuzzer owns this initialized byte span for the synchronous callback.
  net::alps_rust_test::Fuzz(UNSAFE_BUFFERS(
      std::string_view(reinterpret_cast<const char*>(data), size)));
  return 0;
}
