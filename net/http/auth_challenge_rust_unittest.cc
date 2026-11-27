// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/auth_challenge_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
TEST(AuthChallengeRustTest, BoundaryAndAliasCases) {
  auth_challenge_test::Check({}, true);
  const std::string_view cases[] = {"",
                                    " ",
                                    "\t \t",
                                    "NTLM",
                                    " NTLM \t",
                                    "Negotiate Zm9vYmFy",
                                    "Negotiate Zh==",
                                    "Negotiate Zg===",
                                    "Negotiate =",
                                    "Negotiate ====",
                                    "Negotiate ***",
                                    "Negotiate +///",
                                    "Negotiate Zm8=",
                                    "Negotiate !!!!",
                                    "Negotiate a=b",
                                    "Negotiate Zm9v\t",
                                    "NTLM\r\n abc",
                                    std::string_view("NTLM \0\xff==", 9)};
  for (auto input : cases) {
    auth_challenge_test::Check(input);
  }
}
TEST(AuthChallengeRustTest, LongSchemesAndMutableBacking) {
  for (size_t length : {size_t{63}, size_t{64}, size_t{65}, size_t{256}}) {
    std::string input(length, 'N');
    input += " Zm9v";
    auth_challenge_test::Check(input);
  }
  std::string actual_input = "Negotiate Zg====";
  std::string expected_input = actual_input;
  HttpAuthChallengeTokenizer actual(actual_input);
  auth_challenge_reference::Tokenizer expected(expected_input);
  EXPECT_EQ(actual.base64_param(), expected.base64_param());
  actual_input.back() = 'A';
  expected_input.back() = 'A';
  EXPECT_EQ(actual.base64_param(), expected.base64_param());
  EXPECT_EQ(ParseFirstRoundChallenge(HttpAuth::AUTH_SCHEME_NEGOTIATE, &actual),
            auth_challenge_reference::First(HttpAuth::AUTH_SCHEME_NEGOTIATE,
                                            &expected));
  std::string ae = "before", ad = "decoded", be = ae, bd = ad;
  EXPECT_EQ(ParseLaterRoundChallenge(HttpAuth::AUTH_SCHEME_NEGOTIATE, &actual,
                                     &ae, &ad),
            auth_challenge_reference::Later(HttpAuth::AUTH_SCHEME_NEGOTIATE,
                                            &expected, &be, &bd));
  EXPECT_EQ(ae, be);
  EXPECT_EQ(ad, bd);
}
}  // namespace net
