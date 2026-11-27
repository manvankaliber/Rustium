// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_WEBSOCKETS_WEBSOCKET_EXTENSION_RUST_TEST_SUPPORT_H_
#define NET_WEBSOCKETS_WEBSOCKET_EXTENSION_RUST_TEST_SUPPORT_H_
#include <array>
#include <cstdlib>
#include <string_view>

#include "net/websockets/websocket_extension_parser.h"
#include "net/websockets/websocket_extension_rust_reference.h"
namespace net::websocket_extension_rust_test {
inline void Require(bool condition) {
  if (!condition) {
    std::abort();
  }
}
template <class A, class B>
void Same(const A& a, const B& b) {
  Require(a.name() == b.name());
  Require(a.parameters().size() == b.parameters().size());
  for (size_t i = 0; i < a.parameters().size(); ++i) {
    Require(a.parameters()[i].name() == b.parameters()[i].name());
    Require(a.parameters()[i].value() == b.parameters()[i].value());
    Require(a.parameters()[i].HasValue() == b.parameters()[i].HasValue());
  }
  Require(a.ToString() == b.ToString());
}
inline void Check(std::string_view input) {
  auto rust = ParseWebSocketExtensions(input);
  auto cpp =
      websocket_extension_rust_reference::ParseWebSocketExtensions(input);
  Require(rust.size() == cpp.size());
  for (size_t i = 0; i < rust.size(); ++i) {
    Same(rust[i], cpp[i]);
    Require(rust[i].Equivalent(rust[i]));
    auto copy = rust[i];
    Same(copy, cpp[i]);
    Require(copy.Equivalent(rust[i]));
    auto rendered = ParseWebSocketExtensions(copy.ToString());
    Require(rendered.size() == 1 && rendered[0].Equivalent(copy));
  }
  // Exercise the complete object API independently of parse success. Names
  // have no token precondition; retain arbitrary bytes and embedded NULs.
  using R = WebSocketExtension;
  using C = websocket_extension_rust_reference::WebSocketExtension;
  std::string name(input);
  R a(name), b(name);
  C x(name), y(name);
  constexpr std::array values = {"one", "two", "one", "three"};
  for (size_t i = 0; i < 4; ++i) {
    std::string key = (i & 1) ? "other" : name;
    a.Add(R::Parameter(key, values[i]));
    x.Add(C::Parameter(key, values[i]));
  }
  // Shuffle distinct keys but retain duplicate-key value ordering.
  for (size_t i : {size_t{1}, size_t{3}, size_t{0}, size_t{2}}) {
    const auto& p = a.parameters()[i];
    b.Add(R::Parameter(p.name(), p.value()));
    y.Add(C::Parameter(p.name(), p.value()));
  }
  Same(a, x);
  Same(b, y);
  Require(a.Equivalent(b) == x.Equivalent(y));
  Require(a.parameters()[0] == a.parameters()[2]);
  Require(!(a.parameters()[0] == a.parameters()[1]));
  R changed = b;
  C changed_cpp = y;
  changed.Add(R::Parameter(name));
  changed_cpp.Add(C::Parameter(name));
  Require(a.Equivalent(changed) == x.Equivalent(changed_cpp));
  R other("different");
  C other_cpp("different");
  Require(a.Equivalent(other) == x.Equivalent(other_cpp));
  // Caller backing storage changes cannot change owned parser results.
  std::string backing(input);
  auto owned = ParseWebSocketExtensions(backing);
  backing.assign(backing.size() + 100, '!');
  Require(owned.size() == cpp.size());
  for (size_t i = 0; i < owned.size(); ++i) {
    Same(owned[i], cpp[i]);
  }
}
}  // namespace net::websocket_extension_rust_test
#endif
