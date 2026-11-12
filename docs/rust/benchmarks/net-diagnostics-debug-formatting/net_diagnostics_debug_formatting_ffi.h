// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_BASE_NET_DIAGNOSTICS_DEBUG_FORMATTING_FFI_H_
#define NET_BASE_NET_DIAGNOSTICS_DEBUG_FORMATTING_FFI_H_
#include <cstddef>
#include <cstdint>

extern "C" {
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
