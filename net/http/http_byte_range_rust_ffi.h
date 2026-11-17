// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_BYTE_RANGE_RUST_FFI_H_
#define NET_HTTP_HTTP_BYTE_RANGE_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
// Value-only ABI, independent of the public C++ object's storage. No aliasing,
// null/empty pointer contracts, retained borrows or callbacks are possible.
struct HttpByteRangeRustState {
  int64_t first;
  int64_t last;
  int64_t suffix;
  bool computed;
};
struct HttpByteRangeRustBounds {
  HttpByteRangeRustState state;
  bool success;
};
struct HttpByteRangeRustHeader {
  char bytes[48];
  uint8_t len;
};
static_assert(sizeof(HttpByteRangeRustState) == 32);
static_assert(alignof(HttpByteRangeRustState) == 8);
static_assert(offsetof(HttpByteRangeRustState, computed) == 24);
static_assert(sizeof(HttpByteRangeRustHeader) == 49);
extern "C" {
HttpByteRangeRustState http_byte_range_rust_construct(uint32_t,
                                                      int64_t,
                                                      int64_t);
bool http_byte_range_rust_is_suffix(int64_t);
bool http_byte_range_rust_has_first(int64_t);
bool http_byte_range_rust_has_last(int64_t);
bool http_byte_range_rust_is_valid(int64_t, int64_t, int64_t);
HttpByteRangeRustBounds http_byte_range_rust_compute(HttpByteRangeRustState,
                                                     int64_t);
HttpByteRangeRustHeader http_byte_range_rust_header(int64_t, int64_t, int64_t);
}
#endif
