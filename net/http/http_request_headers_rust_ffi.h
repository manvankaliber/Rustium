// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_REQUEST_HEADERS_RUST_FFI_H_
#define NET_HTTP_HTTP_REQUEST_HEADERS_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "base/memory/raw_ptr_exclusion.h"
// Synchronous read-only service. Fields are readable until the call returns.
// No Rust borrows survive a C++ mutation; all mutations happen after planning.
struct RequestHeadersRustSpan {
  const char* data;
  size_t len;
};
struct RequestHeadersRustReader {
  // C ABI pointer, synchronous lifetime; raw_ptr would change the ABI.
  RAW_PTR_EXCLUSION const void* context;
  size_t count;
  RequestHeadersRustSpan (*field)(const void*, size_t, bool);
};
struct RequestHeadersRustPlan {
  uint32_t action;
  size_t index;
};
struct RequestHeadersRustParsed {
  uint32_t error;
  size_t key_len;
  size_t value_begin;
  size_t value_len;
};
static_assert(std::is_standard_layout_v<RequestHeadersRustReader>);
static_assert(sizeof(RequestHeadersRustReader) == 3 * sizeof(size_t));
static_assert(sizeof(RequestHeadersRustSpan) == 2 * sizeof(size_t));
static_assert(offsetof(RequestHeadersRustPlan, index) == sizeof(size_t));
static_assert(sizeof(RequestHeadersRustPlan) == 2 * sizeof(size_t));
static_assert(offsetof(RequestHeadersRustParsed, key_len) == sizeof(size_t));
static_assert(sizeof(RequestHeadersRustParsed) == 4 * sizeof(size_t));
extern "C" {
size_t rustium_request_headers_crlf(const char*, size_t);
size_t rustium_request_headers_format(const char*,
                                      size_t,
                                      const char*,
                                      size_t,
                                      char*);
size_t rustium_request_headers_find(RequestHeadersRustReader,
                                    const char*,
                                    size_t);
RequestHeadersRustPlan rustium_request_headers_plan(RequestHeadersRustReader,
                                                    const char*,
                                                    size_t,
                                                    uint32_t);
bool rustium_request_headers_validate(const char*, size_t, uint32_t);
RequestHeadersRustParsed rustium_request_headers_parse(const char*, size_t);
void rustium_request_headers_block(const char*,
                                   size_t,
                                   void*,
                                   void (*)(void*, size_t, size_t));
size_t rustium_request_headers_serialize(RequestHeadersRustReader,
                                         char*,
                                         size_t);
uint32_t rustium_request_headers_step(bool, bool);
void rustium_request_headers_each(size_t, void*, void (*)(void*, size_t));
uint32_t rustium_request_headers_encoding_mode(RequestHeadersRustReader);
size_t rustium_request_headers_encoding(uint32_t,
                                        uint32_t,
                                        bool,
                                        bool,
                                        bool,
                                        char*);
}
#endif
