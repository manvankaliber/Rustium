// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_LOG_NET_LOG_VALUES_RUST_FFI_H_
#define NET_LOG_NET_LOG_VALUES_RUST_FFI_H_

#include <stddef.h>
#include <stdint.h>

// Internal ABI. The sink constructs a base::Value and copies borrowed text
// synchronously. Data and destination never alias; no pointer is retained.
extern "C" {
using NetLogValuesRustSink =
    void (*)(void*, uint8_t, int32_t, double, const uint8_t*, size_t);
void net_log_values_rust_signed(int64_t, void*, NetLogValuesRustSink);
void net_log_values_rust_unsigned(uint64_t, void*, NetLogValuesRustSink);
void net_log_values_rust_bool(bool, void*, NetLogValuesRustSink);
void net_log_values_rust_bytes(const uint8_t*,
                               size_t,
                               uint8_t,
                               void*,
                               NetLogValuesRustSink);
}

#endif  // NET_LOG_NET_LOG_VALUES_RUST_FFI_H_
