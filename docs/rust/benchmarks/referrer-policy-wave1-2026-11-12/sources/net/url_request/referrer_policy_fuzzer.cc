// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/compiler_specific.h"
#include "net/url_request/referrer_policy_differential.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: The fuzz engine owns `size` initialized bytes for this callback.
  // Handle an empty input before constructing a pointer-and-length view.
  const auto input = size == 0
                         ? std::string_view()
                         : UNSAFE_BUFFERS(std::string_view(
                               reinterpret_cast<const char*>(data), size));
  net::referrer_policy_test::CheckSlice(input);
  return 0;
}
