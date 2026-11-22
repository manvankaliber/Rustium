#include "net/http/connection_auth_original_policy.h"
// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/connection_auth_original_basic.h"
#include "net/http/auth_challenge_reference.h"
#include "net/http/auth_challenge_reference_tokenizer.h"

#include <string>
#include <string_view>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/net_errors.h"
#include "net/base/net_string_util.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_auth_scheme.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

namespace net {

namespace {

std::string_view TrimParameterWhitespace(std::string_view value) {
  const size_t begin = value.find_first_not_of(" \t");
  if (begin == std::string_view::npos) {
    return {};
  }
  return value.substr(begin, value.find_last_not_of(" \t") - begin + 1);
}

// Frozen lenient unquoting for required Basic parameters. Accept an opening
// quote without a matching end quote and retain its remaining bytes.
std::string_view UnquoteParameter(std::string_view value, std::string& storage) {
  if (value.front() != '"') {
    return value;
  }
  value.remove_prefix(1);
  if (value.empty() || value.back() != '"') {
    return value;
  }
  value.remove_suffix(1);
  bool escaped = false;
  for (char c : value) {
    if (c == '\\' && !escaped) {
      escaped = true;
      continue;
    }
    escaped = false;
    storage.push_back(c);
  }
  return storage;
}

// Parses a realm from an auth challenge, and converts to UTF8-encoding.
// Returns whether the realm is invalid or the parameters are invalid.
//
// Note that if a realm was not specified, we will default it to "";
// so specifying 'Basic realm=""' is equivalent to 'Basic'.
//
// This is more generous than RFC 2617, which is pretty clear in the
// production of challenge that realm is required.
//
// We allow it to be compatibility with certain embedded webservers that don't
// include a realm (see http://crbug.com/20984.)
//
// The over-the-wire realm is encoded as ISO-8859-1 (aka Latin-1).
//
// TODO(cbentzel): Realm may need to be decoded using RFC 2047 rules as
// well, see http://crbug.com/25790.
bool ParseRealm(const HttpAuthChallengeTokenizer& tokenizer,
                std::string* realm) {
  CHECK(realm);
  realm->clear();
  // The original required-value, lenient-quote path is frozen at
  // 478cd0e02a3a22af2ce2b64bd5c440ba87c5943c in net/http/http_util.cc.
  const std::string_view input = tokenizer.params();
  auth_challenge_reference::frozen::StringViewTokenizer parameters(input, ",");
  parameters.set_quote_chars("\"");
  while (const auto token = parameters.GetNextTokenView()) {
    const std::string_view pair = TrimParameterWhitespace(*token);
    if (pair.empty()) {
      continue;
    }
    const size_t equals = pair.find('=');
    if (equals == 0 || equals == std::string_view::npos) {
      return false;
    }
    const std::string_view name = TrimParameterWhitespace(pair.substr(0, equals));
    const std::string_view value =
        TrimParameterWhitespace(pair.substr(equals + 1));
    if (name.find('"') != std::string_view::npos || value.empty()) {
      return false;
    }
    if (base::EqualsCaseInsensitiveASCII(name, "realm")) {
      std::string unquoted;
      if (!ConvertToUtf8AndNormalize(UnquoteParameter(value, unquoted),
                                     kCharsetLatin1, realm)) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace

bool ConnectionAuthOriginalBasic::Init(
    HttpAuthChallengeTokenizer* challenge,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key) {
  auth_scheme_ = HttpAuth::AUTH_SCHEME_BASIC;
  score_ = 1;
  properties_ = 0;
  return ParseChallenge(challenge);
}

bool ConnectionAuthOriginalBasic::ParseChallenge(
    HttpAuthChallengeTokenizer* challenge) {
  if (challenge->auth_scheme() != kBasicAuthScheme)
    return false;

  std::string realm;
  if (!ParseRealm(*challenge, &realm))
    return false;

  realm_ = realm;
  return true;
}

int ConnectionAuthOriginalBasic::GenerateAuthTokenImpl(
    const AuthCredentials* credentials,
    const HttpRequestInfo*,
    CompletionOnceCallback callback,
    std::string* auth_token) {
  DCHECK(credentials);
  // Firefox, Safari and Chromium all use UTF-8 encoding; IE uses iso-8859-1.
  // RFC7617 does not specify a default encoding, but UTF-8 is the only allowed
  // value for the optional charset parameter on the challenge.
  std::string base64_username_password = auth_challenge_reference::FrozenEncode(
      base::StrCat({base::UTF16ToUTF8(credentials->username()), ":",
                    base::UTF16ToUTF8(credentials->password())}));
  *auth_token = "Basic " + base64_username_password;
  return OK;
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalBasic::HandleAnotherChallengeImpl(
    HttpAuthChallengeTokenizer* challenge) {
  // Basic authentication is always a single round, so any responses
  // should be treated as a rejection.  However, if the new challenge
  // is for a different realm, then indicate the realm change.
  std::string realm;
  if (!ParseRealm(*challenge, &realm))
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  return (realm_ != realm) ? HttpAuth::AUTHORIZATION_RESULT_DIFFERENT_REALM
                           : HttpAuth::AUTHORIZATION_RESULT_REJECT;
}

ConnectionAuthOriginalBasic::Factory::Factory() = default;

ConnectionAuthOriginalBasic::Factory::~Factory() = default;

int ConnectionAuthOriginalBasic::Factory::CreateAuthHandler(
    HttpAuthChallengeTokenizer* challenge,
    HttpAuth::Target target,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    CreateReason reason,
    int digest_nonce_count,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler) {
  if (http_auth_preferences() &&
      !http_auth_preferences()->basic_over_http_enabled() &&
      scheme_host_port.scheme() == url::kHttpScheme) {
    return ERR_UNSUPPORTED_AUTH_SCHEME;
  }
  // TODO(cbentzel): Move towards model of parsing in the factory
  //                 method and only constructing when valid.
  auto tmp_handler = std::make_unique<ConnectionAuthOriginalBasic>();
  if (!ConnectionAuthOriginalHandlerPolicy::InitFromChallenge(tmp_handler.get(), challenge, target, ssl_info,
                                      network_anonymization_key,
                                      scheme_host_port, net_log)) {
    return ERR_INVALID_RESPONSE;
  }
  *handler = std::move(tmp_handler);
  return OK;
}

}  // namespace net
