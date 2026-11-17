// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_BASIC_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_BASIC_H_

#include <memory>
#include <string>

#include "net/base/completion_once_callback.h"
#include "net/base/net_export.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"

namespace net {

// Code for handling http basic authentication.
class ConnectionAuthOriginalBasic : public HttpAuthHandler {
 public:
  class Factory : public HttpAuthHandlerFactory {
   public:
    Factory();
    ~Factory() override;

    int CreateAuthHandler(
        HttpAuthChallengeTokenizer* challenge,
        HttpAuth::Target target,
        const SSLInfo& ssl_info,
        const NetworkAnonymizationKey& network_anonymization_key,
        const url::SchemeHostPort& scheme_host_port,
        CreateReason reason,
        int digest_nonce_count,
        const NetLogWithSource& net_log,
        HostResolver* host_resolver,
        std::unique_ptr<HttpAuthHandler>* handler) override;
  };

  ~ConnectionAuthOriginalBasic() override = default;

 private:
  // HttpAuthHandler
  bool Init(HttpAuthChallengeTokenizer* challenge,
            const SSLInfo& ssl_info,
            const NetworkAnonymizationKey& network_anonymization_key) override;
  int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                            const HttpRequestInfo* request,
                            CompletionOnceCallback callback,
                            std::string* auth_token) override;
  HttpAuth::AuthorizationResult HandleAnotherChallengeImpl(
      HttpAuthChallengeTokenizer* challenge) override;

  bool ParseChallenge(HttpAuthChallengeTokenizer* challenge);
};

}  // namespace net

#endif  // NET_HTTP_CONNECTION_AUTH_ORIGINAL_BASIC_H_
