// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_NET_ERROR_RUST_FFI_H_
#define NET_BASE_NET_ERROR_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
#include <limits>
namespace net::net_error_rust {
inline constexpr int32_t kUnknown = std::numeric_limits<int32_t>::min();
// Token-pasted field names prevent Windows macro expansion in declarations.
struct WindowsConstants {
#define NET_ERROR_RUST_CONSTANT(name) uint32_t constant_##name;
#include "net/base/net_error_rust_win_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
};
struct PosixConstants {
#define NET_ERROR_RUST_CONSTANT(name) int32_t constant_##name;
#include "net/base/net_error_rust_posix_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
};
}  // namespace net::net_error_rust
extern "C" {
bool net_error_rust_defined(int32_t error);
bool net_error_rust_certificate(int32_t error);
bool net_error_rust_client_certificate(int32_t error);
int8_t net_error_rust_hostname(int32_t error);
bool net_error_rust_blocked(int32_t error);
bool net_error_rust_extended(int32_t error, int32_t extended);
int32_t net_error_rust_file(int32_t error);
int32_t net_error_rust_windows(
    uint32_t error,
    const net::net_error_rust::WindowsConstants* constants);
// Windows-native SDK bindings. The callback is non-null, non-throwing, warning
// diagnostics only, and may reenter. Rust returns the final mapped net error.
int32_t net_error_rust_windows_native(uint32_t error,
                                      void (*warn_unknown)(uint32_t));
int32_t net_error_rust_posix(
    int32_t error,
    const net::net_error_rust::PosixConstants* constants);
// Null output queries length. Short output is untouched; no terminator is
// written.
size_t net_error_rust_format(int32_t error,
                             bool full,
                             const uint8_t* quic,
                             size_t quic_len,
                             uint8_t* output,
                             size_t capacity);
}
#endif
