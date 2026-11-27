// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_AUTH_CACHE_RUST_FFI_H_
#define NET_HTTP_HTTP_AUTH_CACHE_RUST_FFI_H_

#include <stddef.h>
#include <stdint.h>

// Synchronous storage services. Handles are opaque stable node identities,
// invalid after erasure. Returned byte views live until the next mutation; Rust
// ends each borrow before any write (copying when needed), including when
// public API inputs alias an existing Entry.
struct HttpAuthCacheRustBytes {
  const uint8_t* data;
  size_t len;
};
struct HttpAuthCacheRustVtable {
  int64_t (*scalar)(void*, uint32_t, int64_t, int64_t, int64_t);
  HttpAuthCacheRustBytes (*bytes)(const void*, uint32_t, int64_t, int64_t);
  void (*push_path)(void*, int64_t, const uint8_t*, size_t);
};
extern "C" int64_t http_auth_cache_rust_run(void*,
                                            const HttpAuthCacheRustVtable*,
                                            uint32_t,
                                            int64_t,
                                            int64_t,
                                            int64_t,
                                            int64_t);

#endif
