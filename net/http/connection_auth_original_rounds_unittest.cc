// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Prepared only. This verifies the adapter, not provider routing.
#include "net/http/connection_auth_original_rounds.h"

#include <string>
#include <string_view>

#include "net/http/auth_challenge_reference.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::connection_auth_original_rounds {
namespace {
constexpr auto kScheme = HttpAuth::AUTH_SCHEME_NEGOTIATE;
TEST(ConnectionAuthOriginalRoundsTest, FirstAndLaterResults) {
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT, FirstFields(kScheme, "negotiate", ""));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_INVALID, FirstFields(kScheme, "negotiate", "Zm9vYmFy"));
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_INVALID, FirstFields(kScheme, "other", ""));
  std::string encoded = "before", decoded = "untouched";
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT,
            LaterFields(kScheme, "negotiate", "Zm9vYmFy", &encoded, &decoded));
  EXPECT_EQ("Zm9vYmFy", encoded);
  EXPECT_EQ("foobar", decoded);
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_REJECT,
            LaterFields(kScheme, "negotiate", "", &encoded, &decoded));
  EXPECT_TRUE(encoded.empty());
  EXPECT_EQ("foobar", decoded);
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_INVALID,
            LaterFields(kScheme, "negotiate", "***", &encoded, &decoded));
  EXPECT_EQ("***", encoded);
  EXPECT_EQ("foobar", decoded);
}
TEST(ConnectionAuthOriginalRoundsTest, PaddingAndAliasedStorage) {
  std::string encoded = "Zg===", decoded = "before";
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT,
            LaterFields(kScheme, "negotiate", encoded, &encoded, &decoded));
  EXPECT_EQ("Zg==", encoded);
  EXPECT_EQ("f", decoded);
  std::string both = "Zm9v";
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_ACCEPT,
            LaterFields(kScheme, "negotiate", both, &both, &both));
  EXPECT_EQ("foo", both);
  encoded = "keep";
  decoded = "keep too";
  EXPECT_EQ(HttpAuth::AUTHORIZATION_RESULT_INVALID,
            LaterFields(kScheme, "other", "Zm9v", &encoded, &decoded));
  EXPECT_EQ("keep", encoded);
  EXPECT_EQ("keep too", decoded);
}
TEST(ConnectionAuthOriginalRoundsTest, FrozenOracleAndAllOutputAliases) {
  // Reuse the independent tokenizer and strict decoder, never the Rust
  // tokenizer. Input ownership and both-output alias modes match the existing
  // auth_challenge_test::Check contract.
  for (std::string_view input : {"Negotiate", "Negotiate Zm9vYmFy",
                                 "Negotiate Zg===", "Negotiate ***",
                                 "NTLM Zm9v", "Other Zm9v"}) {
    for (auto scheme : {HttpAuth::AUTH_SCHEME_NEGOTIATE,
                        HttpAuth::AUTH_SCHEME_NTLM}) {
      for (unsigned mode = 0; mode < 5; ++mode) {
        std::string ae = mode == 2 || mode == 4 ? std::string(input) : "encoded-before";
        std::string be = ae;
        std::string ad = mode == 3 ? std::string(input) : "decoded-before";
        std::string bd = ad;
        std::string ai(input), bi(input);
        std::string_view av = mode == 2 || mode == 4 ? std::string_view(ae)
                              : mode == 3 ? std::string_view(ad) : std::string_view(ai);
        std::string_view bv = mode == 2 || mode == 4 ? std::string_view(be)
                              : mode == 3 ? std::string_view(bd) : std::string_view(bi);
        auth_challenge_reference::Tokenizer a(av), b(bv);
        EXPECT_EQ(auth_challenge_reference::First(scheme, &b),
                  FirstFields(scheme, a.auth_scheme(), a.params()));
        std::string* ao = mode == 1 || mode == 4 ? &ae : &ad;
        std::string* bo = mode == 1 || mode == 4 ? &be : &bd;
        auto actual = LaterFields(scheme, a.auth_scheme(), a.params(), &ae, ao);
        auto expected = auth_challenge_reference::Later(scheme, &b, &be, bo);
        EXPECT_EQ(expected, actual);
        EXPECT_EQ(be, ae);
        EXPECT_EQ(bd, ad);
      }
    }
  }
}
}  // namespace
}  // namespace net::connection_auth_original_rounds
