// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_WEBSOCKETS_WEBSOCKET_EXTENSION_RUST_REFERENCE_H_
#define NET_WEBSOCKETS_WEBSOCKET_EXTENSION_RUST_REFERENCE_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/component_export.h"

namespace net::websocket_extension_rust_reference {

// A WebSocketExtension instance represents a WebSocket extension specified
// in RFC6455.
class COMPONENT_EXPORT(WEBSOCKET_EXTENSION_RUST_REFERENCE) WebSocketExtension {
 public:
  // Note that RFC6455 does not allow a parameter with an empty value.
  class COMPONENT_EXPORT(WEBSOCKET_EXTENSION_RUST_REFERENCE) Parameter final {
   public:
    // Construct a parameter which does not have a value.
    explicit Parameter(const std::string& name);
    // Construct a parameter with a non-empty value.
    Parameter(const std::string& name, const std::string& value);

    bool operator==(const Parameter&) const;

    bool HasValue() const { return !value_.empty(); }
    const std::string& name() const { return name_; }
    const std::string& value() const { return value_; }

    // The default copy constructor and the assignment operator are defined:
    // we need them.
   private:
    std::string name_;
    std::string value_;
  };

  WebSocketExtension();
  explicit WebSocketExtension(const std::string& name);
  WebSocketExtension(const WebSocketExtension& other);
  ~WebSocketExtension();

  void Add(const Parameter& parameter) { parameters_.push_back(parameter); }
  const std::string& name() const { return name_; }
  const std::vector<Parameter>& parameters() const { return parameters_; }

  // Equivalent() returns true if `name_` and `parameters_` are the same, even
  // if the order of keys in `parameters_` is different.
  bool Equivalent(const WebSocketExtension& other) const;

  std::string ToString() const;

  // The default copy constructor and the assignment operator are defined:
  // we need them.
 private:
  std::string name_;
  std::vector<Parameter> parameters_;
};

COMPONENT_EXPORT(WEBSOCKET_EXTENSION_RUST_REFERENCE)
std::vector<WebSocketExtension> ParseWebSocketExtensions(std::string_view);
bool IsTokenChar(char);
bool IsToken(std::string_view);
}  // namespace net::websocket_extension_rust_reference

#endif  // NET_WEBSOCKETS_WEBSOCKET_EXTENSION_RUST_REFERENCE_H_
