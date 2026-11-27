// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SSL_TLS_SESSION_CACHE_RUST_ABI_H_
#define NET_SSL_TLS_SESSION_CACHE_RUST_ABI_H_
#include <stddef.h>
#include <stdint.h>

#include "base/memory/raw_ptr_exclusion.h"
#include "net/base/net_export.h"
struct TlsSessionCacheRustState;
struct TlsSessionCacheRustBytes {
  const uint8_t* data;
  size_t len;
};
struct TlsSessionCacheRustServer {
  TlsSessionCacheRustBytes host;
  uint16_t port;
};
struct TlsSessionCacheRustProxy {
  TlsSessionCacheRustServer server;
  int32_t scheme;
};
struct TlsSessionCacheRustKey {
  TlsSessionCacheRustServer server;
  TlsSessionCacheRustBytes ip;
  // Call-scoped ABI pointers: raw_ptr would change the C layout.
  RAW_PTR_EXCLUSION const void* nak;
  RAW_PTR_EXCLUSION const TlsSessionCacheRustProxy* proxies;
  size_t proxy_count;
  size_t proxy_index;
  int32_t privacy;
  int32_t usage;
  int32_t opaque;
  bool has_ip;
  bool valid_proxies;
  bool has_opaque;
};
struct TlsSessionCacheRustMemory {
  int32_t percent;
};
// All calls run synchronously on the cache's original owning sequence. Byte
// views are initialized read-only inputs, copied before native service calls.
// Rust owns cache state, keys, LRU links and one SSL_SESSION ref per live slot.
// Query/up-ref/copy/NAK services are non-reentrant. Final session releases
// occur after state borrows end and can run ex-data destructors; no owner is
// used again.
extern "C" {
bool tls_session_cache_rust_register_memory_consumer(bool, bool);
TlsSessionCacheRustState* tls_session_cache_rust_new(size_t, size_t);
void tls_session_cache_rust_delete(TlsSessionCacheRustState*);
uint64_t tls_session_cache_rust_stat(const TlsSessionCacheRustState*, uint32_t);
int32_t tls_session_cache_rust_compare(const TlsSessionCacheRustKey*,
                                       const TlsSessionCacheRustKey*);
bool tls_session_cache_rust_expired(uintptr_t, int64_t);
uintptr_t tls_session_cache_rust_run(TlsSessionCacheRustState*,
                                     uint32_t,
                                     const TlsSessionCacheRustKey*,
                                     uintptr_t,
                                     uint64_t,
                                     const void*,
                                     const TlsSessionCacheRustMemory*,
                                     const TlsSessionCacheRustServer*,
                                     size_t);
}
namespace net {
// Tests query the production DLL's Rust policy without a second cache runtime.
NET_EXPORT_PRIVATE bool TlsSessionCacheRegisterMemoryConsumerForTesting(
    bool is_mac,
    bool is_win);
}  // namespace net
#endif
