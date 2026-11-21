// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_VARY_DATA_RUST_FFI_H_
#define NET_HTTP_HTTP_VARY_DATA_RUST_FFI_H_
#include <cstddef>
#include <cstdint>

struct HttpVaryDataRustState {
  uint8_t digest[32];
  int32_t kind;
  uint32_t valid;
};
struct HttpVaryDataRustReply {
  const uint8_t* data;
  size_t len;
  size_t value;
};
// Private synchronous services. 0 wildcard; 1 next Vary field (arg=cursor);
// 2 request value (input=name); 3 crypto begin (arg=kind);
// 4 copy iterator to probe; 5 probe ReadBytes(arg); 6 ReadInt;
// 7 ReadBytes(arg); 8 WriteInt(arg); 9 WriteBytes(input);
// 10 crypto Update(input); 11 crypto Finish(arg=kind).
// Header views live through the outer call. Hash/read views are copied before
// another callback; missing views are null, including failed reads.
using HttpVaryDataRustCallback =
    HttpVaryDataRustReply (*)(void*, uint32_t, const uint8_t*, size_t, size_t);
extern "C" HttpVaryDataRustState rustium_http_vary_data(
    HttpVaryDataRustState,
    void*,
    HttpVaryDataRustCallback,
    uint32_t,
    int32_t);
static_assert(sizeof(HttpVaryDataRustState) == 40);
static_assert(alignof(HttpVaryDataRustState) == alignof(uint32_t));
static_assert(sizeof(HttpVaryDataRustReply) == 3 * sizeof(size_t));
#endif
