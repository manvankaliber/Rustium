// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/websockets/websocket_extension_parser.h"

#include "net/websockets/websocket_extension_rust_ffi.h"
namespace net {
std::vector<WebSocketExtension> ParseWebSocketExtensions(
    std::string_view data) {
  std::vector<WebSocketExtension> result;
  // Rust validates the entire input before emitting any owned objects. The
  // callback only copies storage; grammar and failure policy stay in Rust.
  rustium_websocket_extension_parse(
      websocket_extension_rust::View(data), &result,
      [](void* context, size_t kind, WebSocketExtensionRustBytes name,
         WebSocketExtensionRustBytes value) {
        auto& extensions =
            *static_cast<std::vector<WebSocketExtension>*>(context);
        if (kind == 0) {
          extensions.emplace_back(websocket_extension_rust::Copy(name));
        } else {
          auto key = websocket_extension_rust::Copy(name);
          extensions.back().Add(
              value.len ? WebSocketExtension::Parameter(
                              key, websocket_extension_rust::Copy(value))
                        : WebSocketExtension::Parameter(key));
        }
      });
  return result;
}
}  // namespace net
