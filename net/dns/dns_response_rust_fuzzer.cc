// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/dns/dns_response_rust_test_support.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies this initialized live input for the call.
  auto bytes = UNSAFE_BUFFERS(base::span(data, size));
  net::dns_response_rust_test::Differential(bytes);
  if (!bytes.empty()) {
    net::dns_response_rust_test::Serialize(bytes.front());
  }
  return 0;
}
