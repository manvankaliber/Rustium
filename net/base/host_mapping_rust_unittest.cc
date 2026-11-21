// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/logging.h"
#include "net/base/host_mapping_rust_ffi.rs.h"
#include "net/base/host_mapping_rust_test_support.h"
#include "net/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
namespace {
rust::Slice<const uint8_t> Bytes(std::string_view s) {
  return s.empty() ? rust::Slice<const uint8_t>()
                   : UNSAFE_BUFFERS(rust::Slice<const uint8_t>(
                         reinterpret_cast<const uint8_t*>(s.data()), s.size()));
}
TEST(HostMappingRust, MutableState) {
  const int level = logging::GetMinLogLevel();
  logging::SetMinLogLevel(logging::LOGGING_FATAL);
  uint64_t state = 20261113;
  for (int n = 0; n < 10000; ++n) {
    std::string input;
    for (int k = 0; k < 96; ++k) {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      input.push_back(static_cast<char>(state));
    }
    host_mapping_rust_test::State(input);
  }
  logging::SetMinLogLevel(level);
}
TEST(HostMappingRust, BytesAndPatterns) {
  const std::string patterns[] = {
      "",       "*",        "?",         "??",        "*?",  "a?b",
      "a*b",    "a\\*b",    "\\",        "a\\",       "*\\", "**a??b",
      "a?*b?c", "\xc3\xa9", "?\xc3\xa9", "*\xc3\xa9", "\\?", "a*b*c"};
  for (int c = 0; c < 256; ++c) {
    for (const auto& p : patterns) {
      const std::string s(1, static_cast<char>(c));
      EXPECT_EQ(HostMappingRustOriginalMatchPattern(s, p),
                host_mapping_rust::match_pattern(Bytes(s), Bytes(p)));
      EXPECT_EQ(HostMappingRustOriginalMatchPattern(p, s),
                host_mapping_rust::match_pattern(Bytes(p), Bytes(s)));
    }
  }
  const std::string utf8[] = {std::string("\xe0\x80", 2),
                              std::string("\xed\xa0\x80", 3),
                              std::string("\xf4\x90\x80\x80", 4),
                              "\xc3\xa9",
                              "\xf0\x9f\x99\x82",
                              "a\xf0\x9f\x99\x82"
                              "b",
                              "a\xc3\xa9"
                              "bc",
                              "ab",
                              "acb",
                              ""};
  for (auto& s : utf8) {
    for (auto& p : patterns) {
      EXPECT_EQ(HostMappingRustOriginalMatchPattern(s, p),
                host_mapping_rust::match_pattern(Bytes(s), Bytes(p)));
    }
  }
}
TEST(HostMappingRust, BoundariesAndAlias) {
  HostMappingRules a;
  HostMappingRustReference b;
  const std::string rules[] = {
      "MAP * x:0",       "MAP * x:65535",
      "MAP * x:65536",   "MAP * x:",
      "MAP * x:-1",      "MAP * x:+1",
      "MAP * x:00080",   "MAP * [::1]",
      "MAP * [::1]:0",   "MAP * []",
      "MAP * [bad]",     "MAP * a:b:12",
      "MAP * user@host", "MAP  x y",
      "MAP\tx y",        "map \t x",
      "exclude ",        "  EXCLUDE\t\v x \f",
      "MAP * x/y",       "MAP * x#y",
      "MAP * x?y",       std::string("MAP * x\0y", 10)};
  for (auto& rule : rules) {
    EXPECT_EQ(a.AddRuleFromString(rule), b.AddRuleFromString(rule));
    host_mapping_rust_test::CheckRewrite(a, b, "hello", 80);
  }
  HostPortPair pair("MAP * initial.test:65535", 80);
  a.SetRulesFromString(pair.host());
  b.SetRulesFromString(pair.host());
  pair.set_host("gone");
  host_mapping_rust_test::CheckRewrite(a, b, "hello", 0);
  a.SetRulesFromString(std::string_view());
  b.SetRulesFromString(std::string_view());
  EXPECT_FALSE(a.AddRuleFromString(std::string_view()));
  host_mapping_rust_test::CheckRewrite(a, b, "hello", 0);
}
TEST(HostMappingRust, LazyNullDiagnosticsAndFormatting) {
  HostMappingRules rules;
  HostMappingRustReference original;
  ASSERT_TRUE(rules.AddRuleFromString("MAP [a]:7 target:9"));
  ASSERT_TRUE(original.AddRuleFromString("MAP [a]:7 target:9"));
  HostPortPair a(std::string("a\0:b", 4), 7), b(std::string("a\0:b", 4), 7);
  bool matched = false;
  EXPECT_DFATAL(matched = rules.RewriteHost(&a),
                "Host has a null char: a%00:b");
  EXPECT_TRUE(matched);
  EXPECT_DFATAL(matched = original.RewriteHost(&b),
                "Host has a null char: a%00:b");
  EXPECT_TRUE(matched);
  EXPECT_EQ(a.host(), b.host());
  EXPECT_EQ(a.port(), b.port());
  rules.SetRulesFromString("MAP * lazy");
  original.SetRulesFromString("MAP * lazy");
  host_mapping_rust_test::CheckRewrite(rules, original, std::string("a\0:b", 4),
                                       7);
  for (const std::string& host :
       {std::string(), std::string("x"), std::string("::1"),
        std::string("a\0:b", 4)}) {
    const auto formatted = host_mapping_rust::format_for_match(Bytes(host), 7);
    EXPECT_FALSE(formatted.empty());
    EXPECT_EQ(formatted.back(), '7');
  }
}
}  // namespace
}  // namespace net
