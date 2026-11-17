// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/gtest_util.h"
#include "net/log/net_log_values_rust_differential.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
TEST(NetLogValuesRustTest, WholePublicApiBoundaries) {
  net_log_values_rust_test::Boundaries();
}

TEST(NetLogValuesRustTest, EveryTwoByteString) {
  for (unsigned i = 0; i < 65536; ++i) {
    const std::string input{static_cast<char>(i >> 8), static_cast<char>(i)};
    net_log_values_rust_test::Check(input, i);
  }
}

TEST(NetLogValuesRustTest, ReturnedDictionaryOwnsAliasedKeyAndValue) {
  auto check = [] {
    std::string storage("a\0b\xE2\x98\x83", 6);
    auto actual = NetLogParamsWithString(storage, storage);
    auto expected =
        net_log_values_rust_reference::NetLogParamsWithString(storage, storage);
    storage.assign(10000, 'z');
    EXPECT_EQ(actual, expected);
    return actual;
  };
  auto value = check();
  EXPECT_EQ(value.size(), 1u);
  EXPECT_EQ(*value.FindString(std::string_view("a\0b\xE2\x98\x83", 6)),
            std::string("a\0b\xE2\x98\x83", 6));
}

TEST(NetLogValuesRustTest, DictionaryRetainsUtf8Diagnostics) {
  EXPECT_DCHECK_DEATH((void)NetLogParamsWithString("key", "\xFF"));
  EXPECT_DCHECK_DEATH(
      (void)net_log_values_rust_reference::NetLogParamsWithString("key",
                                                                  "\xFF"));
  EXPECT_DCHECK_DEATH((void)NetLogParamsWithInt("\xFF", 1));
  EXPECT_DCHECK_DEATH(
      (void)net_log_values_rust_reference::NetLogParamsWithInt("\xFF", 1));
}
}  // namespace net
