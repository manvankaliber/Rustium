// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/compiler_specific.h"
#include "net/log/net_log_values_rust_differential.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: The fuzz engine supplies initialized bytes for this call; null is
  // allowed for an empty input. Only local read-only views are constructed.
  const std::string_view input =
      size == 0 ? std::string_view()
                : UNSAFE_BUFFERS(std::string_view(
                      reinterpret_cast<const char*>(data), size));
  uint64_t bits = 0;
  for (unsigned char byte : input.substr(0, 8)) {
    bits = (bits << 8) | byte;
  }
  net::net_log_values_rust_test::Check(input, bits);
  return 0;
}
