// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_AUTH_CHALLENGE_TEST_SUPPORT_H_
#define NET_HTTP_AUTH_CHALLENGE_TEST_SUPPORT_H_
#include <cstdlib>
#include <string>
#include <string_view>

#include "net/http/auth_challenge_reference.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_multi_round_parse.h"
namespace net::auth_challenge_test {
inline void Require(bool b) {
  if (!b) {
    std::abort();
  }
}
inline ptrdiff_t Offset(std::string_view input, std::string_view part) {
  if (!part.data()) {
    return -1;
  }
  return part.data() - input.data();
}
template <typename A, typename B>
void CheckIterator(A& a, B& b) {
  auto x = a.param_pairs();
  auto y = b.param_pairs();
  for (;;) {
    Require(x.valid() == y.valid());
    const bool xn = x.GetNext(), yn = y.GetNext();
    Require(xn == yn && x.valid() == y.valid());
    if (!xn) {
      break;
    }
    Require(x.name() == y.name() && x.value() == y.value());
  }
}
inline void Check(std::string_view bytes, bool null_empty = false) {
  // Independent buffers make ownership and substring offsets comparable.
  std::string a(bytes), b(bytes);
  std::string_view av = null_empty ? std::string_view() : std::string_view(a);
  std::string_view bv = null_empty ? std::string_view() : std::string_view(b);
  HttpAuthChallengeTokenizer actual(av);
  auth_challenge_reference::Tokenizer expected(bv);
  Require(actual.challenge_text() == expected.challenge_text());
  Require((actual.challenge_text().data() == nullptr) ==
          (expected.challenge_text().data() == nullptr));
  Require(actual.auth_scheme() == expected.auth_scheme());
  Require(actual.params() == expected.params());
  Require(Offset(av, actual.params()) == Offset(bv, expected.params()));
  Require(actual.base64_param() == expected.base64_param());
  Require(Offset(av, actual.base64_param()) ==
          Offset(bv, expected.base64_param()));
  CheckIterator(actual, expected);
  for (auto scheme :
       {HttpAuth::AUTH_SCHEME_NEGOTIATE, HttpAuth::AUTH_SCHEME_NTLM}) {
    Require(ParseFirstRoundChallenge(scheme, &actual) ==
            auth_challenge_reference::First(scheme, &expected));
    for (unsigned mode = 0; mode < 5; ++mode) {
      // 0 distinct, 1 both output pointers identical, 2 input owned by
      // encoded, 3 input owned by decoded, 4 input and both outputs alias.
      std::string ae =
          mode == 2 || mode == 4 ? std::string(bytes) : "encoded-before";
      std::string be = ae;
      std::string ad = mode == 3 ? std::string(bytes) : "decoded-before";
      std::string bd = ad;
      std::string ai(bytes), bi(bytes);
      std::string_view ax = (mode == 2 || mode == 4) ? std::string_view(ae)
                            : mode == 3              ? std::string_view(ad)
                                                     : std::string_view(ai);
      std::string_view bx = (mode == 2 || mode == 4) ? std::string_view(be)
                            : mode == 3              ? std::string_view(bd)
                                                     : std::string_view(bi);
      HttpAuthChallengeTokenizer at(ax);
      auth_challenge_reference::Tokenizer bt(bx);
      std::string* ao = mode == 1 || mode == 4 ? &ae : &ad;
      std::string* bo = mode == 1 || mode == 4 ? &be : &bd;
      auto ar = ParseLaterRoundChallenge(scheme, &at, &ae, ao);
      auto br = auth_challenge_reference::Later(scheme, &bt, &be, bo);
      Require(ar == br && ae == be && ad == bd);
    }
  }
}
}  // namespace net::auth_challenge_test
#endif
