// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_SPDY_SESSION_POOL_RUST_FFI_H_
#define NET_SPDY_SPDY_SESSION_POOL_RUST_FFI_H_

#include <cstdint>
#include <type_traits>

namespace net {

using SpdySessionPoolRustOperation =
    int64_t (*)(void*, uint32_t, uint64_t, uint64_t, uint64_t);

extern "C" int64_t spdy_session_pool_rust_dispatch(
    uint32_t method,
    void* context,
    SpdySessionPoolRustOperation operation);

// The stack closure owns all native local objects/iterators/WeakPtrs used by a
// single public operation. Rust sees scalar results only. User callbacks may
// change live native state; Rust re-queries at the original observation points.
// No closure/native reference may be retained when the synchronous call ends.
template <class F>
int64_t RunSpdySessionPoolRust(uint32_t method, F&& operation) {
  using Operation = std::remove_reference_t<F>;
  return spdy_session_pool_rust_dispatch(
      method, &operation,
      [](void* context, uint32_t action, uint64_t a, uint64_t b,
         uint64_t c) -> int64_t {
        return (*static_cast<Operation*>(context))(action, a, b, c);
      });
}

}  // namespace net
#endif  // NET_SPDY_SPDY_SESSION_POOL_RUST_FFI_H_
