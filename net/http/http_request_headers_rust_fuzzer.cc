// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <cstdio>

#include "net/http/http_request_headers_rust_test_support.h"
int main() {
  for (uint64_t i = 0; i < 41667; ++i) {
    net::http_request_headers_rust_test::Sequence(0x20261113u +
                                                  i * 0x9e3779b97f4a7c15ull);
  }
  std::puts(
      "PASS 41667 deterministic whole-public-API sequences, 1000008 "
      "differential state inputs");
}
