#include "net/log/connection_original_log.h"
#include "net/base/net_error_rust_reference.h"
#include "net/http/connection_auth_original_basic.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/http/auth_challenge_reference_tokenizer.h"
#include "net/http/http_response_headers.h"
#include "net/log/connection_original_log.h"
// Copyright 2010 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_handler_factory.h"

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <string_view>

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "net/base/net_errors.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_filter.h"
#include "net/http/http_auth_handler_basic.h"
#include "net/http/http_auth_handler_digest.h"
#include "net/http/http_auth_handler_ntlm.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_auth_scheme.h"
#include "net/log/net_log_values.h"
#include "net/net_buildflags.h"
#include "net/ssl/ssl_info.h"
#include "url/scheme_host_port.h"

#if BUILDFLAG(USE_KERBEROS)
#include "net/http/http_auth_handler_negotiate.h"
#endif

namespace net {

namespace {

base::DictValue NetLogParamsForCreateAuth(
    std::string_view scheme,
    std::string_view challenge,
    const int net_error,
    const url::SchemeHostPort& scheme_host_port,
    const std::optional<bool>& allows_default_credentials,
    NetLogCaptureMode capture_mode) {
  base::DictValue dict;
  dict.Set("scheme", net_log_values_rust_reference::NetLogStringValue(scheme));
  if (NetLogCaptureIncludesSensitive(capture_mode)) {
    dict.Set("challenge", net_log_values_rust_reference::NetLogStringValue(challenge));
  }
  dict.Set("origin", scheme_host_port.Serialize());
  if (allows_default_credentials)
    dict.Set("allows_default_credentials", *allows_default_credentials);
  if (net_error < 0)
    dict.Set("net_error", net_error);
  return dict;
}

// Since there are at most 4 schemes, a linear search is faster than a binary
// search because it can cheaply discard strings when the length doesn't match.
constexpr auto kDefaultAuthSchemes =
    std::to_array<std::string_view>({kBasicAuthScheme, kDigestAuthScheme,
#if BUILDFLAG(USE_KERBEROS)
                                     kNegotiateAuthScheme,
#endif
                                     kNtlmAuthScheme});

bool IsLws(char c) { return c == ' ' || c == '\t'; }
}  // namespace

[[clang::noinline]] std::string_view ConnectionFrozenTrim(std::string_view string) {
  size_t begin_offset = 0;
  size_t end_offset = string.size();
  while (begin_offset < end_offset && IsLws(string[begin_offset])) {
    ++begin_offset;
  }
  while (begin_offset < end_offset && IsLws(string[end_offset - 1])) {
    --end_offset;
  }
  return string.substr(begin_offset, end_offset - begin_offset);
}

void ConnectionAuthOriginalFactoryPolicy::InitTokenizer(HttpAuthChallengeTokenizer* self, std::string_view challenge) {
  auth_challenge_reference::frozen::StringViewTokenizer tok(challenge, " \t");
  if (!tok.GetNext()) {
    return;
  }
  self->lower_case_scheme_ = base::ToLowerASCII(tok.token_piece());
  self->params_ = ConnectionFrozenTrim(std::string_view(tok.token_end(), challenge.end()));
}

int ConnectionAuthOriginalFactoryPolicy::CreateAuthHandlerFromString(
    HttpAuthHandlerFactory* self,
    std::string_view challenge,
    HttpAuth::Target target,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler) {
  HttpAuthChallengeTokenizer props(HttpAuthChallengeTokenizer::OriginalPolicyTag{}, challenge);
  InitTokenizer(&props, challenge);
  return self->CreateAuthHandler(&props, target, ssl_info, network_anonymization_key,
                           scheme_host_port, HttpAuthHandlerFactory::CREATE_CHALLENGE, 1, net_log,
                           host_resolver, handler);
}

int ConnectionAuthOriginalFactoryPolicy::CreatePreemptiveAuthHandlerFromString(
    HttpAuthHandlerFactory* self,
    const std::string& challenge,
    HttpAuth::Target target,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    int digest_nonce_count,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler) {
  HttpAuthChallengeTokenizer props(HttpAuthChallengeTokenizer::OriginalPolicyTag{}, challenge);
  InitTokenizer(&props, challenge);
  SSLInfo null_ssl_info;
  return self->CreateAuthHandler(&props, target, null_ssl_info,
                           network_anonymization_key, scheme_host_port,
                           HttpAuthHandlerFactory::CREATE_PREEMPTIVE, digest_nonce_count, net_log,
                           host_resolver, handler);
}

void ConnectionAuthOriginalFactoryPolicy::ChooseBestChallenge(
    HttpAuthHandlerFactory* http_auth_handler_factory,
    const HttpResponseHeaders& response_headers,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    HttpAuth::Target target,
    const url::SchemeHostPort& scheme_host_port,
    const std::set<HttpAuth::Scheme>& disabled_schemes,
    const NetLogWithSource& net_log,
    HostResolver* host_resolver,
    std::unique_ptr<HttpAuthHandler>* handler) {
  DCHECK(http_auth_handler_factory);
  DCHECK(handler->get() == nullptr);

  // Choose the challenge whose authentication handler gives the maximum score.
  std::unique_ptr<HttpAuthHandler> best;
  const auto header_name = HttpAuth::GetChallengeHeaderName(target);
  std::optional<std::string_view> cur_challenge;
  size_t iter = 0;
  while (
      (cur_challenge = response_headers.EnumerateHeader(&iter, header_name))) {
    std::unique_ptr<HttpAuthHandler> cur;
    int rv = CreateAuthHandlerFromString(http_auth_handler_factory, 
        *cur_challenge, target, ssl_info, network_anonymization_key,
        scheme_host_port, net_log, host_resolver, &cur);
    if (rv != OK) {
      VLOG(1) << "Unable to create AuthHandler. Status: " << net_error_rust_reference::ErrorToString(rv)
              << " Challenge: " << *cur_challenge;
      continue;
    }
    if (cur.get() && (!best.get() || best->score() < cur->score()) &&
        !disabled_schemes.contains(cur->auth_scheme())) {
      best.swap(cur);
    }
  }
  handler->swap(best);
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalFactoryPolicy::HandleChallengeResponse(
    HttpAuthHandler* handler,
    const HttpResponseHeaders& response_headers,
    HttpAuth::Target target,
    const std::set<HttpAuth::Scheme>& disabled_schemes,
    std::string* challenge_used) {
  DCHECK(handler);
  DCHECK(challenge_used);

  challenge_used->clear();
  HttpAuth::Scheme current_scheme = handler->auth_scheme();
  if (disabled_schemes.contains(current_scheme)) {
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  }
  const auto current_scheme_name = HttpAuth::SchemeToString(current_scheme);
  const std::string_view header_name = HttpAuth::GetChallengeHeaderName(target);
  size_t iter = 0;
  std::optional<std::string_view> challenge;
  HttpAuth::AuthorizationResult authorization_result =
      HttpAuth::AUTHORIZATION_RESULT_INVALID;
  while ((challenge = response_headers.EnumerateHeader(&iter, header_name))) {
    HttpAuthChallengeTokenizer challenge_tokens(HttpAuthChallengeTokenizer::OriginalPolicyTag{}, *challenge);
    InitTokenizer(&challenge_tokens, *challenge);
    if (challenge_tokens.auth_scheme() != current_scheme_name)
      continue;
    authorization_result = ConnectionAuthOriginalHandlerPolicy::HandleAnotherChallenge(handler, &challenge_tokens);
    if (authorization_result != HttpAuth::AUTHORIZATION_RESULT_INVALID) {
      *challenge_used = *challenge;
      return authorization_result;
    }
  }
  // Finding no matches is equivalent to rejection.
  return HttpAuth::AUTHORIZATION_RESULT_REJECT;
}

std::string_view ConnectionAuthOriginalFactoryPolicy::Base64Param(const HttpAuthChallengeTokenizer& self) {
  size_t n = self.params().size();
  while (n > 0 && n % 4 != 0 && self.params()[n - 1] == '=') {
    --n;
  }
  return self.params().substr(0, n);
}

bool ConnectionAuthOriginalRegistry::IsSchemeAllowed(
    const std::string& scheme) const {
  if (http_auth_preferences() && http_auth_preferences()->allowed_schemes()) {
    return http_auth_preferences()->allowed_schemes()->contains(scheme);
  }
  return std::ranges::contains(kDefaultAuthSchemes, scheme);
}

HttpAuthHandlerFactory* ConnectionAuthOriginalRegistry::GetSchemeFactory(
    const std::string& scheme) const {
  std::string lower_scheme = base::ToLowerASCII(scheme);
  auto it = factory_map_.find(lower_scheme);
  if (it == factory_map_.end()) {
    return nullptr;  // |scheme| is not registered.
  }
  return it->second.get();
}

int ConnectionAuthOriginalRegistry::CreateAuthHandler(
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
  auto scheme = challenge->auth_scheme();

  int net_error;
  if (scheme.empty()) {
    handler->reset();
    net_error = ERR_INVALID_RESPONSE;
  } else {
    const bool all_schemes_allowed =
        http_auth_preferences() &&
        http_auth_preferences()->IsAllowedToUseAllHttpAuthSchemes(
            scheme_host_port, target);
    bool is_scheme_allowed = IsSchemeAllowed(scheme);
    auto* factory = all_schemes_allowed || is_scheme_allowed
                        ? GetSchemeFactory(scheme)
                        : nullptr;
    if (!factory) {
      handler->reset();
      net_error = ERR_UNSUPPORTED_AUTH_SCHEME;
    } else {
      if (all_schemes_allowed && !is_scheme_allowed) {
        base::UmaHistogramEnumeration("Net.HttpAuth.SchemeAllowedByAllSchemesPolicy",
                                      target);
      }
      net_error = factory->CreateAuthHandler(
          challenge, target, ssl_info, network_anonymization_key,
          scheme_host_port, reason, digest_nonce_count, net_log, host_resolver,
          handler);
    }
  }

  net_log.AddEvent(
      NetLogEventType::AUTH_HANDLER_CREATE_RESULT,
      [&](NetLogCaptureMode capture_mode) {
        return NetLogParamsForCreateAuth(
            scheme, challenge->challenge_text(), net_error, scheme_host_port,
            *handler
                ? std::make_optional((*handler)->AllowsDefaultCredentials())
                : std::nullopt,
            capture_mode);
      });
  return net_error;
}

std::unique_ptr<ConnectionAuthOriginalRegistry>
ConnectionAuthOriginalRegistry::Create(
    const HttpAuthPreferences* prefs
#if BUILDFLAG(USE_EXTERNAL_GSSAPI)
    ,
    const std::string& gssapi_library_name
#endif
#if BUILDFLAG(USE_KERBEROS)
    ,
    HttpAuthMechanismFactory negotiate_auth_system_factory
#endif
) {
  auto registry_factory =
      std::make_unique<ConnectionAuthOriginalRegistry>(prefs);

  registry_factory->RegisterSchemeFactory(
      kBasicAuthScheme, std::make_unique<ConnectionAuthOriginalBasic::Factory>());

  registry_factory->RegisterSchemeFactory(
      kDigestAuthScheme, std::make_unique<HttpAuthHandlerDigest::Factory>());

  auto ntlm_factory = std::make_unique<HttpAuthHandlerNTLM::Factory>();
#if BUILDFLAG(IS_WIN)
  ntlm_factory->set_sspi_library(
      base::MakeRefCounted<SSPILibraryDefault>(NTLMSP_NAME));
#endif  // BUILDFLAG(IS_WIN)
  registry_factory->RegisterSchemeFactory(kNtlmAuthScheme,
                                          std::move(ntlm_factory));

#if BUILDFLAG(USE_KERBEROS)
  auto negotiate_factory = std::make_unique<HttpAuthHandlerNegotiate::Factory>(
      negotiate_auth_system_factory);
#if BUILDFLAG(IS_WIN)
  negotiate_factory->set_library(
      base::MakeRefCounted<SSPILibraryDefault>(NEGOSSP_NAME));
#elif BUILDFLAG(USE_EXTERNAL_GSSAPI)
  negotiate_factory->set_library(
      base::MakeRefCounted<GSSAPISharedLibrary>(gssapi_library_name));
#endif
  registry_factory->RegisterSchemeFactory(kNegotiateAuthScheme,
                                          std::move(negotiate_factory));
#endif  // BUILDFLAG(USE_KERBEROS)

  if (prefs) {
    registry_factory->set_http_auth_preferences(prefs);
    for (auto& factory_entry : registry_factory->factory_map_) {
      factory_entry.second->set_http_auth_preferences(prefs);
    }
  }
  return registry_factory;
}
}  // namespace net
