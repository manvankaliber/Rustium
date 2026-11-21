// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <cstdio>

#include "net/http/http_byte_range_rust_test_support.h"
// Deterministic, guided full-API state fuzz. No timing performed.
int main() {
  const auto counts =
      net::http_byte_range_rust_test::Exercise(0x3e719b8fa2c04d65ULL, 1000000);
  std::printf(
      "{\"states\":%llu,\"operations\":%llu,\"undefined_compute_excluded\":%"
      "llu,\"seed\":\"0x3e719b8fa2c04d65\",\"steps_per_state\":16}\n",
      static_cast<unsigned long long>(counts.states),
      static_cast<unsigned long long>(counts.operations),
      static_cast<unsigned long long>(counts.undefined_compute_excluded));
}
