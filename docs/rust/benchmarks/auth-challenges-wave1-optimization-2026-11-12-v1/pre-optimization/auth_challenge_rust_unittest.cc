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
}  // namespace net
