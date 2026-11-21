// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_QUIC_HTTP3_CLIENT_STREAM_RUST_FFI_H_
#define NET_QUIC_HTTP3_CLIENT_STREAM_RUST_FFI_H_
#include <stdint.h>

#include <memory>
extern "C" int64_t http3_client_stream_run(uint32_t method,
                                           void* context,
                                           int64_t (*service)(void*,
                                                              uint32_t,
                                                              int64_t));
extern "C" bool http3_client_stream_callback_gate(bool current, bool expected);
extern "C" bool http3_client_stream_is_first(uint32_t id, uint32_t first);
namespace net {
// The stack closure survives deletion of its captured Stream/Handle. Services
// cannot throw or retain context. Public API reentry uses a fresh stack
// closure.
template <class Service>
int64_t RunHttp3StreamPolicy(uint32_t method, Service& services) {
  return http3_client_stream_run(
      method, std::addressof(services),
      [](void* context, uint32_t operation, int64_t argument) -> int64_t {
        return (*static_cast<Service*>(context))(operation, argument);
      });
}
}  // namespace net
#endif  // NET_QUIC_HTTP3_CLIENT_STREAM_RUST_FFI_H_
