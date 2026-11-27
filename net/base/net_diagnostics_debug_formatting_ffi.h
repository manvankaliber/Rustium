// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_BASE_NET_DIAGNOSTICS_DEBUG_FORMATTING_FFI_H_
#define NET_BASE_NET_DIAGNOSTICS_DEBUG_FORMATTING_FFI_H_
#include <cstddef>
#include <cstdint>

extern "C" {
// Required bytes or SIZE_MAX for invalid flags. The writer borrows initialized
// caller-owned storage synchronously, never retains it, and allocates nothing.
// Invalid flags/insufficient capacity leave output untouched. These lengths
// exclude the trailing NUL owned by a C++ std::string.
size_t net_diagnostics_load_flags_length(int32_t flags);
bool net_diagnostics_load_flags_write(int32_t flags,
                                      uint8_t* output,
                                      size_t len);

// `out_len` must be non-null and writable for one size_t. On invalid flags,
// returns nullptr and leaves `out_len` unchanged. On success, caller owns the
// returned pointer/length pair and must pass it exactly once to the matching
// free function below. The buffer must not be retained after freeing it.
uint8_t* net_diagnostics_load_flags_format(int32_t flags, size_t* out_len);
void net_diagnostics_load_flags_free(uint8_t* ptr, size_t len);

// Returns a static NUL-terminated string with process lifetime; invalid values
// return nullptr.
const char* net_diagnostics_privacy_mode(int32_t mode);
}
#endif
