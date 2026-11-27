// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_HOST_PORT_RUST_FFI_H_
#define NET_BASE_HOST_PORT_RUST_FFI_H_
#include <cstdint>
#include <string>
#include <string_view>

#include "third_party/rust/cxx/v1/cxx.h"
namespace net::host_port_rust {
struct Authority;
Authority authority(rust::Slice<const uint8_t> input);
int32_t parse_port(rust::Slice<const uint8_t> input,
                   int32_t begin,
                   int32_t len);
bool ipv6(rust::Slice<const uint8_t> input);
// Commits outputs only on success, after all input borrows have ended.
bool ParseHostAndPort(std::string_view input, std::string* host, int* port);
}  // namespace net::host_port_rust
#endif
