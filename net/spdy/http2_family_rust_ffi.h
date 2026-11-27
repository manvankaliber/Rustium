// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#ifndef NET_SPDY_HTTP2_FAMILY_RUST_FFI_H_
#define NET_SPDY_HTTP2_FAMILY_RUST_FFI_H_

#include <cstdint>
#include <type_traits>

using Http2FamilyNativeCall = int64_t (*)(void*, uint32_t, int64_t);
extern "C" int64_t http2_family_rust_dispatch(uint32_t family,
                                              uint32_t method,
                                              void* context,
                                              Http2FamilyNativeCall callback);

// Callable storage belongs to the active native stack frame, independently of
// the owner. No captured owner is touched by this trampoline itself.
template <class Function>
int64_t Http2FamilyRun(uint32_t family, uint32_t method, Function&& function) {
  using Callback = std::remove_reference_t<Function>;
  return http2_family_rust_dispatch(
      family, method, &function,
      [](void* context, uint32_t operation, int64_t argument) -> int64_t {
        return (*static_cast<Callback*>(context))(operation, argument);
      });
}

#endif  // NET_SPDY_HTTP2_FAMILY_RUST_FFI_H_
