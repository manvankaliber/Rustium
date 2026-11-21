// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/dcheck_is_on.h"
#include "net/websockets/websocket_extension_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
#if DCHECK_IS_ON()
TEST(WebSocketExtensionRustTest, ConstructorDiagnostics) {
  EXPECT_DEATH_IF_SUPPORTED(WebSocketExtension::Parameter("p", ""), "");
  EXPECT_DEATH_IF_SUPPORTED(WebSocketExtension::Parameter("p", "bad value"),
                            "");
}
#endif
TEST(WebSocketExtensionRustTest, AllBytesAndFailureBoundaries) {
  websocket_extension_rust_test::Check(std::string_view());
  for (int byte = 0; byte < 256; ++byte) {
    std::string c(1, static_cast<char>(byte));
    for (const auto& input :
         {c, "a" + c, "a; " + c, "a; b=" + c, "a; b=\"" + c + "\"",
          "a; b=\"\\" + c + "\"", "a; b=c, " + c}) {
      websocket_extension_rust_test::Check(input);
    }
  }
  for (std::string_view s :
       {"a", "a; x=\"one\\t\\wo\"; x=three, b; z", "a; x=\"one\\",
        "a; x=\"one\"junk", "a,", "a; x=\"\"",
        " \t a \t ; b \t = \t c \t , \t d \t", "a; b=\"unterminated",
        "a; b=\"bad\\\"\"", "a; b=\"bad\\\\\""}) {
    for (size_t i = 0; i <= s.size(); ++i) {
      websocket_extension_rust_test::Check(s.substr(0, i));
    }
  }
}
TEST(WebSocketExtensionRustTest, DuplicateValueOrderAndOwnership) {
  WebSocketExtension a("x"), b("x");
  a.Add(WebSocketExtension::Parameter("p", "1"));
  a.Add(WebSocketExtension::Parameter("q"));
  a.Add(WebSocketExtension::Parameter("p", "2"));
  b.Add(WebSocketExtension::Parameter("p", "2"));
  b.Add(WebSocketExtension::Parameter("q"));
  b.Add(WebSocketExtension::Parameter("p", "1"));
  EXPECT_FALSE(a.Equivalent(b));
  b = a;
  EXPECT_TRUE(a.Equivalent(b));
  a.Add(a.parameters().front());
  EXPECT_EQ("x; p=1; q; p=2; p=1", a.ToString());
  EXPECT_EQ("x; p=1; q; p=2", b.ToString());
  WebSocketExtension empty;
  empty.Add(WebSocketExtension::Parameter("ignored", "value"));
  EXPECT_EQ("", empty.ToString());
}
TEST(WebSocketExtensionRustTest, LongGrammarAndStableSort) {
  std::string header = "large";
  WebSocketExtension a("large"), b("large");
  websocket_extension_rust_reference::WebSocketExtension x("large"), y("large");
  for (int i = 0; i < 128; ++i) {
    const auto name = "key" + std::to_string(i % 16);
    const auto value = "v" + std::to_string(i / 16);
    header += "; " + name + "=\"\\" + value + "\"";
    a.Add(WebSocketExtension::Parameter(name, value));
    x.Add(websocket_extension_rust_reference::WebSocketExtension::Parameter(
        name, value));
  }
  for (int key = 15; key >= 0; --key) {
    for (int value = 0; value < 8; ++value) {
      const auto name = "key" + std::to_string(key);
      const auto text = "v" + std::to_string(value);
      b.Add(WebSocketExtension::Parameter(name, text));
      y.Add(websocket_extension_rust_reference::WebSocketExtension::Parameter(
          name, text));
    }
  }
  EXPECT_EQ(a.Equivalent(b), x.Equivalent(y));
  EXPECT_TRUE(a.Equivalent(b));
  websocket_extension_rust_test::Check(header);
  auto parsed = ParseWebSocketExtensions(header);
  ASSERT_EQ(1u, parsed.size());
  EXPECT_TRUE(parsed.front().Equivalent(a));
  header += ", broken; value=\"unterminated";
  EXPECT_TRUE(ParseWebSocketExtensions(header).empty());
  websocket_extension_rust_test::Check(header);
}
}  // namespace net
