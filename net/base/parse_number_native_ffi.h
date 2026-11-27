// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_PARSE_NUMBER_NATIVE_FFI_H_
#define NET_BASE_PARSE_NUMBER_NATIVE_FFI_H_

#include <cstddef>
#include <cstdint>

#include "net/base/parse_number.h"

namespace net::parse_number_native_internal {
// Synchronous C ABI for the public numeric adapters. A nonempty input
// identifies len initialized live bytes (len <= PTRDIFF_MAX); null input is
// allowed for zero length. Only success writes output; its aligned storage may
// be uninitialized. Only failure writes a non-null error. No previous
// output/error value is read. Input may overlap either output: the input borrow
// ends before scalar writes. Formats preserve the public enum values and legacy
// unknown signed formats; unsigned format validation remains a CHECK in the
// public C++ wrapper.
extern "C" {
bool rustium_parse_number_native_i32(const uint8_t* input,
                                     size_t len,
                                     int32_t format,
                                     int32_t* output,
                                     ParseIntError* error);
bool rustium_parse_number_native_i64(const uint8_t* input,
                                     size_t len,
                                     int32_t format,
                                     int64_t* output,
                                     ParseIntError* error);
bool rustium_parse_number_native_u32(const uint8_t* input,
                                     size_t len,
                                     int32_t format,
                                     uint32_t* output,
                                     ParseIntError* error);
bool rustium_parse_number_native_u64(const uint8_t* input,
                                     size_t len,
                                     int32_t format,
                                     uint64_t* output,
                                     ParseIntError* error);
}
}  // namespace net::parse_number_native_internal

#endif  // NET_BASE_PARSE_NUMBER_NATIVE_FFI_H_
