// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_util.h"
#include "net/url_request/referrer_policy_differential.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
using referrer_policy_test::CheckSlice;
using referrer_policy_test::kTokens;

TEST(ReferrerPolicyRust, PersistedValuesAndAsciiCase) {
  constexpr std::array<int, 8> kValues = {7, 0, 4, 2, 5, 6, 1, 3};
  for (size_t i = 0; i < kTokens.size(); ++i) {
    const auto result = ReferrerPolicyFromHeader(kTokens[i]);
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<int>(*result), kValues[i]);
    CheckSlice(kTokens[i]);
    CheckSlice(base::ToUpperASCII(kTokens[i]));
  }
}

TEST(ReferrerPolicyRust, AsciiWhitespaceAndCommas) {
  for (char space : std::string("\t\n\v\f\r ")) {
    for (auto token : kTokens) {
      CheckSlice(std::string(3, space) + std::string(token) +
                 std::string(4, space));
      CheckSlice(std::string(1, space) + ",," + std::string(token) + ", " +
                 space);
    }
  }
  CheckSlice("");
  CheckSlice(std::string_view());
  CheckSlice(", , \t\r\n,\v\f");
}

TEST(ReferrerPolicyRust, LastRecognizedValueWins) {
  for (auto first : kTokens) {
    for (auto last : kTokens) {
      const std::string input =
          std::string(first) + ", unknown, " + std::string(last) + ", unknown";
      EXPECT_EQ(ReferrerPolicyFromHeader(input),
                ReferrerPolicyFromHeader(last));
      CheckSlice(input);
    }
  }
}

TEST(ReferrerPolicyRust, EveryByteMutationAndTruncation) {
  for (auto token : kTokens) {
    for (size_t length = 0; length <= token.size(); ++length) {
      CheckSlice(token.substr(0, length));
    }
    for (size_t index = 0; index < token.size(); ++index) {
      for (int byte = 0; byte < 256; ++byte) {
        std::string changed(token);
        changed[index] = static_cast<char>(byte);
        CheckSlice(changed);
      }
    }
  }
}

TEST(ReferrerPolicyRust, NulAndNonAsciiAreNotWhitespaceOrTerminators) {
  for (int byte = 0; byte < 256; ++byte) {
    const std::string ch(1, static_cast<char>(byte));
    CheckSlice(ch + "origin" + ch);
    CheckSlice("origin," + ch + "unsafe-url");
  }
  CheckSlice(std::string("origin\0,unsafe-url", 18));
  CheckSlice(std::string("unsafe-url\0,origin", 18));
}

TEST(ReferrerPolicyRust, LongListsAndInvalidSuffixes) {
  const std::string unknown(4096, 'x');
  CheckSlice(unknown);
  CheckSlice("origin," + unknown);
  CheckSlice(unknown + ",strict-origin");
  std::string list;
  for (int i = 0; i < 512; ++i) {
    list += "unknown,origin,";
  }
  CheckSlice(list);
}
}  // namespace
}  // namespace net
