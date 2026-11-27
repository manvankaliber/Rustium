// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/base/host_port_pair.h"
#include "net/base/host_port_rust_ffi.h"
#include "net/base/host_port_rust_reference.h"
#include "net/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
namespace {
TEST(HostPortRustTest, OriginalHelperAndAliasing) {
  const std::string cases[] = {"",
                               "host",
                               "host:",
                               "host:0",
                               "host:65535",
                               "host:65536",
                               "a:b:c",
                               "[::1]:80",
                               "[bad]:1",
                               "user@host:1",
                               "host:001",
                               ":42",
                               std::string("a\0b:80", 6)};
  for (const auto& input : cases) {
    std::string a = "unchanged", b = a;
    int pa = 77, pb = pa;
    bool ra = HostPortRustOriginalParseHostAndPort(input, &a, &pa);
    bool rb = host_port_rust::ParseHostAndPort(input, &b, &pb);
    EXPECT_EQ(ra, rb);
    EXPECT_EQ(a, b);
    EXPECT_EQ(pa, pb);
    a = input;
    b = input;
    pa = pb = 77;
    ra = HostPortRustOriginalParseHostAndPort(a, &a, &pa);
    rb = host_port_rust::ParseHostAndPort(b, &b, &pb);
    EXPECT_EQ(ra, rb);
    EXPECT_EQ(a, b);
    EXPECT_EQ(pa, pb);
  }
}
TEST(HostPortRustTest, EveryByteAndOverlappingViews) {
  for (int byte = 0; byte <= 255; ++byte) {
    std::string input("prefix");
    input.push_back(static_cast<char>(byte));
    input += ":17suffix";
    std::string a = input, b = input;
    int pa = 99, pb = 99;
    const std::string_view av = std::string_view(a).substr(6, 4);
    const std::string_view bv = std::string_view(b).substr(6, 4);
    const bool ra = HostPortRustOriginalParseHostAndPort(av, &a, &pa);
    const bool rb = host_port_rust::ParseHostAndPort(bv, &b, &pb);
    EXPECT_EQ(ra, rb);
    EXPECT_EQ(a, b);
    EXPECT_EQ(pa, pb);
    auto old = HostPortRustReference::FromString(input);
    auto current = HostPortPair::FromString(input);
    EXPECT_EQ(old.host(), current.host());
    EXPECT_EQ(old.port(), current.port());
  }
  std::string output = "sentinel";
  int port = 17;
  EXPECT_FALSE(
      host_port_rust::ParseHostAndPort(std::string_view(), &output, &port));
  EXPECT_EQ(output, "sentinel");
  EXPECT_EQ(port, 17);
  HostPortPair pair("abcdef", 1);
  pair.set_host(std::string_view(pair.host()).substr(2, 3));
  EXPECT_EQ(pair.host(), "cde");
}
TEST(HostPortRustTest, NullFormattingRetainsOriginalTruncation) {
  const HostPortPair value(std::string("a\0:b", 4), 7);
  std::string actual;
  EXPECT_DFATAL(actual = value.ToString(), "Host has a null char: a%00:b");
  EXPECT_EQ(actual, "[a]:7");
}
TEST(HostPortRustTest, ValueAndOrdering) {
  for (int p : {-1, 0, 1, 65535, 65536}) {
    base::DictValue d;
    d.Set("host", "h");
    d.Set("port", p);
    base::Value v(std::move(d));
    auto a = HostPortRustReference::FromValue(v);
    auto b = HostPortPair::FromValue(v);
    ASSERT_EQ(a.has_value(), b.has_value());
    if (a) {
      EXPECT_EQ(a->host(), b->host());
      EXPECT_EQ(a->port(), b->port());
    }
  }
  EXPECT_LT(HostPortPair("z", 1), HostPortPair("a", 2));
}
}  // namespace
}  // namespace net
