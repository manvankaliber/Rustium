// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_DIGEST_FACTORY_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_DIGEST_FACTORY_H_
#include <memory>
#include <utility>
#include "net/http/http_auth_handler_digest.h"

namespace net {
// The nonce generator must outlive all handlers made by this factory, exactly
// as in HttpAuthHandlerDigest::Factory. The handler remains native C++.
class ConnectionAuthOriginalDigestFactory : public HttpAuthHandlerFactory {
 public:
  ConnectionAuthOriginalDigestFactory();
  int CreateAuthHandler(HttpAuthChallengeTokenizer*, HttpAuth::Target,
                        const SSLInfo&, const NetworkAnonymizationKey&,
                        const url::SchemeHostPort&, CreateReason, int,
                        const NetLogWithSource&, HostResolver*,
                        std::unique_ptr<HttpAuthHandler>*) override;
  void set_nonce_generator(
      std::unique_ptr<const HttpAuthHandlerDigest::NonceGenerator> generator) {
    nonce_generator_ = std::move(generator);
  }
 private:
  std::unique_ptr<const HttpAuthHandlerDigest::NonceGenerator> nonce_generator_;
};
}  // namespace net
#endif  // NET_HTTP_CONNECTION_AUTH_ORIGINAL_DIGEST_FACTORY_H_
