// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/websockets/websocket_extension.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "net/websockets/websocket_extension_rust_ffi.h"

namespace net {

WebSocketExtension::Parameter::Parameter(const std::string& name)
    : name_(name) {}

WebSocketExtension::Parameter::Parameter(const std::string& name,
                                         const std::string& value)
    : name_(name), value_(value) {
  DCHECK(!value.empty());
  // |extension-param| must be a token.
  DCHECK(
      rustium_websocket_extension_token(websocket_extension_rust::View(value)));
}

bool WebSocketExtension::Parameter::operator==(const Parameter& other) const {
  return rustium_websocket_extension_equal(
      websocket_extension_rust::View(*this),
      websocket_extension_rust::View(other));
}

WebSocketExtension::WebSocketExtension() = default;

WebSocketExtension::WebSocketExtension(const std::string& name) : name_(name) {}

WebSocketExtension::WebSocketExtension(const WebSocketExtension& other) =
    default;

WebSocketExtension::~WebSocketExtension() = default;

bool WebSocketExtension::Equivalent(const WebSocketExtension& other) const {
  const auto mine = websocket_extension_rust::Views(parameters_);
  const auto theirs = websocket_extension_rust::Views(other.parameters_);
  return rustium_websocket_extension_equivalent(
      websocket_extension_rust::View(name_), mine.data(), mine.size(),
      websocket_extension_rust::View(other.name_), theirs.data(),
      theirs.size());
}

std::string WebSocketExtension::ToString() const {
#if DCHECK_IS_ON()
  // Preserve the original diagnostic boundary: an empty extension name
  // suppresses parameter validation as well as serialization.
  if (!name_.empty()) {
    for (const auto& p : parameters_) {
      if (p.HasValue()) {
        DCHECK(rustium_websocket_extension_token(
            websocket_extension_rust::View(p.value())));
      }
    }
  }
#endif
  const auto parameters = websocket_extension_rust::Views(parameters_);
  std::string result;
  rustium_websocket_extension_serialize(
      websocket_extension_rust::View(name_), parameters.data(),
      parameters.size(), &result,
      [](void* context, WebSocketExtensionRustBytes bytes) {
        *static_cast<std::string*>(context) =
            websocket_extension_rust::Copy(bytes);
      });
  return result;
}
}  // namespace net
