// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_WEBSOCKETS_WEBSOCKET_EXTENSION_RUST_FFI_H_
#define NET_WEBSOCKETS_WEBSOCKET_EXTENSION_RUST_FFI_H_
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/compiler_specific.h"
#include "net/websockets/websocket_extension.h"
struct WebSocketExtensionRustBytes {
  const char* data;
  size_t len;
};
struct WebSocketExtensionRustPair {
  WebSocketExtensionRustBytes name;
  WebSocketExtensionRustBytes value;
};
static_assert(std::is_standard_layout_v<WebSocketExtensionRustPair>);
static_assert(sizeof(WebSocketExtensionRustPair) == 4 * sizeof(size_t));
static_assert(alignof(WebSocketExtensionRustPair) == alignof(size_t));
static_assert(offsetof(WebSocketExtensionRustPair, value) ==
              2 * sizeof(size_t));
static_assert(offsetof(WebSocketExtensionRustBytes, len) == sizeof(size_t));
extern "C" bool rustium_websocket_extension_token(WebSocketExtensionRustBytes);
extern "C" bool rustium_websocket_extension_equal(WebSocketExtensionRustPair,
                                                  WebSocketExtensionRustPair);
extern "C" bool rustium_websocket_extension_equivalent(
    WebSocketExtensionRustBytes,
    const WebSocketExtensionRustPair*,
    size_t,
    WebSocketExtensionRustBytes,
    const WebSocketExtensionRustPair*,
    size_t);
extern "C" void rustium_websocket_extension_parse(
    WebSocketExtensionRustBytes,
    void*,
    void (*)(void*,
             size_t,
             WebSocketExtensionRustBytes,
             WebSocketExtensionRustBytes));
extern "C" void rustium_websocket_extension_serialize(
    WebSocketExtensionRustBytes,
    const WebSocketExtensionRustPair*,
    size_t,
    void*,
    void (*)(void*, WebSocketExtensionRustBytes));
namespace net::websocket_extension_rust {
inline WebSocketExtensionRustBytes View(std::string_view s) {
  return {s.data(), s.size()};
}
inline WebSocketExtensionRustPair View(const WebSocketExtension::Parameter& p) {
  return {View(p.name()), View(p.value())};
}
inline std::vector<WebSocketExtensionRustPair> Views(
    const std::vector<WebSocketExtension::Parameter>& p) {
  std::vector<WebSocketExtensionRustPair> result;
  result.reserve(p.size());
  for (const auto& v : p) {
    result.push_back(View(v));
  }
  return result;
}
inline std::string Copy(WebSocketExtensionRustBytes b) {
  // SAFETY: Rust supplies a valid call-scoped readable range. Empty strings
  // avoid any requirement for a nonnull pointer.
  return b.len ? UNSAFE_BUFFERS(std::string(b.data, b.len)) : std::string();
}
}  // namespace net::websocket_extension_rust
#endif
