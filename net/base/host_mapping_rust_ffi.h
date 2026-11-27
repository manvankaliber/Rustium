// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_HOST_MAPPING_RUST_FFI_H_
#define NET_BASE_HOST_MAPPING_RUST_FFI_H_
#include <stdint.h>

#include "third_party/rust/cxx/v1/cxx.h"
namespace net::host_mapping_rust {
struct Authority;
Authority authority(rust::Slice<const uint8_t> bytes);
int32_t parse_port(rust::Slice<const uint8_t> bytes,
                   int32_t begin,
                   int32_t len);
bool ipv6(rust::Slice<const uint8_t> bytes);
void report_format_diagnostics(rust::Slice<const uint8_t> diagnostic,
                               bool has_null,
                               bool bad_brackets);
}  // namespace net::host_mapping_rust
#endif
