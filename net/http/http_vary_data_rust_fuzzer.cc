// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_vary_data_rust_test_support.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies an initialized input range valid for this call.
  net::http_vary_data_rust_test::Check(UNSAFE_BUFFERS(base::span(data, size)));
  return 0;
}
