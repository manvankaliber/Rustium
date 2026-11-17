// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_NEGOTIATE_FACTORY_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_NEGOTIATE_FACTORY_H_
#include <utility>
#include "net/net_buildflags.h"
#if BUILDFLAG(USE_KERBEROS)
#include "net/http/http_auth_handler_negotiate.h"
namespace net {
class ConnectionAuthOriginalNegotiateFactory : public HttpAuthHandlerFactory {
 public:
  using AuthLibrary = HttpAuthHandlerNegotiate::AuthLibrary;
  explicit ConnectionAuthOriginalNegotiateFactory(HttpAuthMechanismFactory factory)
      : mechanism_factory_(factory) {}
  void set_library(scoped_refptr<AuthLibrary> library) { library_ = std::move(library); }
  int CreateAuthHandler(HttpAuthChallengeTokenizer*, HttpAuth::Target,
                        const SSLInfo&, const NetworkAnonymizationKey&,
                        const url::SchemeHostPort&, CreateReason, int,
                        const NetLogWithSource&, HostResolver*,
                        std::unique_ptr<HttpAuthHandler>*) override;
 private:
  HttpAuthMechanismFactory mechanism_factory_;
  bool is_unsupported_ = false;
  scoped_refptr<AuthLibrary> library_;
};
}  // namespace net
#endif
#endif  // NET_HTTP_CONNECTION_AUTH_ORIGINAL_NEGOTIATE_FACTORY_H_
