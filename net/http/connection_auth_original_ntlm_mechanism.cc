// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/connection_auth_original_ntlm_mechanism.h"

#include <string_view>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "net/base/net_errors.h"
#include "net/base/network_interfaces.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/connection_auth_original_rounds.h"
#include "net/http/auth_challenge_reference.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_auth_scheme.h"

namespace net {

namespace {

uint64_t GetMSTime() {
  return base::Time::Now().since_origin().InMicroseconds() * 10;
}

void GenerateRandom(base::span<uint8_t> output) {
  base::RandBytes(output);
}

// static
ConnectionAuthOriginalNtlmMechanism::GetMSTimeProc g_get_ms_time_proc = GetMSTime;

// static
ConnectionAuthOriginalNtlmMechanism::GenerateRandomProc g_generate_random_proc =
    GenerateRandom;

// static
ConnectionAuthOriginalNtlmMechanism::HostNameProc g_host_name_proc = GetHostName;

template <typename T>
T SwapOut(T* target, T source) {
  T t = *target;
  *target = source;
  return t;
}

int SetAuthTokenFromBinaryToken(std::string* auth_token,
                                const std::vector<uint8_t>& next_token) {
  if (next_token.empty())
    return ERR_UNEXPECTED;

  std::string encode_output = auth_challenge_reference::FrozenEncode(std::string_view(
      reinterpret_cast<const char*>(next_token.data()), next_token.size()));

  *auth_token = base::StrCat({"NTLM ", encode_output});
  return OK;
}

}  // namespace

ConnectionAuthOriginalNtlmMechanism::ScopedProcSetter::ScopedProcSetter(
    GetMSTimeProc ms_time_proc,
    GenerateRandomProc random_proc,
    HostNameProc host_name_proc) {
  old_ms_time_proc_ = SwapOut(&g_get_ms_time_proc, ms_time_proc);
  old_random_proc_ = SwapOut(&g_generate_random_proc, random_proc);
  old_host_name_proc_ = SwapOut(&g_host_name_proc, host_name_proc);
}

ConnectionAuthOriginalNtlmMechanism::ScopedProcSetter::~ScopedProcSetter() {
  g_get_ms_time_proc = old_ms_time_proc_;
  g_generate_random_proc = old_random_proc_;
  g_host_name_proc = old_host_name_proc_;
}

ConnectionAuthOriginalNtlmMechanism::ConnectionAuthOriginalNtlmMechanism(
    const HttpAuthPreferences* http_auth_preferences)
    : ntlm_client_(ntlm::NtlmFeatures(
          http_auth_preferences ? http_auth_preferences->NtlmV2Enabled()
                                : true)) {}

ConnectionAuthOriginalNtlmMechanism::~ConnectionAuthOriginalNtlmMechanism() = default;

bool ConnectionAuthOriginalNtlmMechanism::Init(const NetLogWithSource& net_log) {
  return true;
}

bool ConnectionAuthOriginalNtlmMechanism::NeedsIdentity() const {
  // This gets called for each round-trip. Only require identity on the first
  // call (when challenge_token_ is empty). On subsequent calls, we use the
  // initially established identity.
  return challenge_token_.empty();
}

bool ConnectionAuthOriginalNtlmMechanism::AllowsExplicitCredentials() const {
  return true;
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalNtlmMechanism::ParseChallenge(
    HttpAuthChallengeTokenizer* tok) {
  if (!first_token_sent_)
    return connection_auth_original_rounds::First(HttpAuth::Scheme::AUTH_SCHEME_NTLM, *tok);

  challenge_token_.clear();
  std::string encoded_token;
  return connection_auth_original_rounds::Later(HttpAuth::Scheme::AUTH_SCHEME_NTLM, *tok,
                                           &encoded_token, &challenge_token_);
}

int ConnectionAuthOriginalNtlmMechanism::GenerateAuthToken(
    const AuthCredentials* credentials,
    const std::string& spn,
    const std::string& channel_bindings,
    std::string* auth_token,
    const NetLogWithSource& net_log,
    CompletionOnceCallback callback) {
  if (!credentials) {
    LOG(ERROR) << "Username and password are expected to be non-nullptr.";
    return ERR_MISSING_AUTH_CREDENTIALS;
  }

  if (challenge_token_.empty()) {
    if (first_token_sent_)
      return ERR_UNEXPECTED;
    first_token_sent_ = true;
    return SetAuthTokenFromBinaryToken(auth_token,
                                       ntlm_client_.GetNegotiateMessage());
  }

  // The username may be in the form "DOMAIN\user".  Parse it into the two
  // components.
  std::u16string domain;
  std::u16string user;
  const std::u16string& username = credentials->username();
  const char16_t backslash_character = '\\';
  size_t backslash_idx = username.find(backslash_character);
  if (backslash_idx == std::u16string::npos) {
    user = username;
  } else {
    domain = username.substr(0, backslash_idx);
    user = username.substr(backslash_idx + 1);
  }

  std::string hostname = g_host_name_proc();
  if (hostname.empty())
    return ERR_UNEXPECTED;

  uint8_t client_challenge[8];
  g_generate_random_proc(base::span<uint8_t>(client_challenge));

  auto next_token = ntlm_client_.GenerateAuthenticateMessage(
      domain, user, credentials->password(), hostname, channel_bindings, spn,
      g_get_ms_time_proc(), client_challenge,
      base::as_byte_span(challenge_token_));

  return SetAuthTokenFromBinaryToken(auth_token, next_token);
}

void ConnectionAuthOriginalNtlmMechanism::SetDelegation(
    HttpAuth::DelegationType delegation_type) {
  // Nothing to do.
}

}  // namespace net
