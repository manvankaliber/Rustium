#include "net/log/connection_original_log.h"
#include "net/base/net_error_rust_reference.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/log/connection_original_log.h"
// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_handler.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"

namespace net {

bool ConnectionAuthOriginalHandlerPolicy::InitFromChallenge(HttpAuthHandler* self,
    HttpAuthChallengeTokenizer* challenge,
    HttpAuth::Target target,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key,
    const url::SchemeHostPort& scheme_host_port,
    const NetLogWithSource& net_log) {
  self->scheme_host_port_ = scheme_host_port;
  self->target_ = target;
  self->score_ = -1;
  self->properties_ = -1;
  self->net_log_ = net_log;

  self->auth_challenge_ = challenge->challenge_text();
  self->net_log_.BeginEvent(NetLogEventType::AUTH_HANDLER_INIT);
  bool ok = self->Init(challenge, ssl_info, network_anonymization_key);
  self->net_log_.EndEvent(NetLogEventType::AUTH_HANDLER_INIT, [&]() {
    base::DictValue params;
    params.Set("succeeded", ok);
    params.Set("allows_default_credentials", self->AllowsDefaultCredentials());
    return params;
  });

  // self->Init() is expected to set the scheme, realm, score, and properties.  The
  // realm may be empty.
  DCHECK(!ok || self->score_ != -1);
  DCHECK(!ok || self->properties_ != -1);
  DCHECK(!ok || self->auth_scheme_ != HttpAuth::AUTH_SCHEME_MAX);

  return ok;
}

int ConnectionAuthOriginalHandlerPolicy::GenerateAuthToken(HttpAuthHandler* self,
    const AuthCredentials* credentials,
                                       const HttpRequestInfo* request,
                                       CompletionOnceCallback callback,
                                       std::string* auth_token) {
  DCHECK(!callback.is_null());
  DCHECK(request);
  DCHECK(credentials != nullptr || self->AllowsDefaultCredentials());
  DCHECK(auth_token != nullptr);
  DCHECK(self->callback_.is_null());
  self->callback_ = std::move(callback);
  self->net_log_.BeginEvent(NetLogEventType::AUTH_GENERATE_TOKEN);
  int rv = self->GenerateAuthTokenImpl(
      credentials, request,
      base::BindOnce(&ConnectionAuthOriginalHandlerPolicy::OnGenerateAuthTokenComplete,
                     base::Unretained(self)),
      auth_token);
  if (rv != ERR_IO_PENDING)
    FinishGenerateAuthToken(self, rv);
  return rv;
}

void ConnectionAuthOriginalHandlerPolicy::OnGenerateAuthTokenComplete(HttpAuthHandler* self,
    int rv) {
  CompletionOnceCallback callback = std::move(self->callback_);
  FinishGenerateAuthToken(self, rv);
  DCHECK(!callback.is_null());
  std::move(callback).Run(rv);
}

void ConnectionAuthOriginalHandlerPolicy::FinishGenerateAuthToken(HttpAuthHandler* self,
    int rv) {
  DCHECK_NE(rv, ERR_IO_PENDING);
  ConnectionOriginalLog::EndEventWithNetErrorCode(self->net_log_, NetLogEventType::AUTH_GENERATE_TOKEN, rv);
  self->callback_.Reset();
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalHandlerPolicy::HandleAnotherChallenge(HttpAuthHandler* self,
    HttpAuthChallengeTokenizer* challenge) {
  auto authorization_result = self->HandleAnotherChallengeImpl(challenge);
  self->net_log_.AddEvent(NetLogEventType::AUTH_HANDLE_CHALLENGE, [&] {
    return ConnectionOriginalAuthorizationResultParams("authorization_result",
                                                     authorization_result);
  });
  return authorization_result;
}
}  // namespace net
