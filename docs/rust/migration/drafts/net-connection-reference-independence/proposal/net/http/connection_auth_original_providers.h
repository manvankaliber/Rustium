// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_PROVIDERS_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_PROVIDERS_H_

#include <memory>
#include <utility>

#include "build/build_config.h"
#include "net/http/http_auth_handler_ntlm.h"
#include "net/net_buildflags.h"
#if BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_POSIX)
#include "net/http/http_auth_gssapi_posix.h"
#endif

namespace net {
// Reference-only access to the existing mechanism state. The corresponding
// friend declarations are proposals; no production class layout is changed.
class ConnectionAuthOriginalProviders {
 public:
#if BUILDFLAG(IS_WIN)
  static HttpAuth::AuthorizationResult Parse(HttpAuthSSPI*,
                                              HttpAuthChallengeTokenizer*);
#else
  static HttpAuth::AuthorizationResult Parse(HttpAuthNtlmMechanism*,
                                              HttpAuthChallengeTokenizer*);
#endif
#if BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_POSIX)
  static HttpAuth::AuthorizationResult Parse(HttpAuthGSSAPI*,
                                              HttpAuthChallengeTokenizer*);
#endif
};

class ConnectionAuthOriginalNtlmHandler : public HttpAuthHandlerNTLM {
 public:
#if BUILDFLAG(IS_WIN)
  ConnectionAuthOriginalNtlmHandler(scoped_refptr<SSPILibrary> library,
                                    const HttpAuthPreferences* preferences);
#else
  explicit ConnectionAuthOriginalNtlmHandler(const HttpAuthPreferences* preferences);
#endif
 protected:
  bool Init(HttpAuthChallengeTokenizer*, const SSLInfo&,
            const NetworkAnonymizationKey&) override;
  HttpAuth::AuthorizationResult HandleAnotherChallengeImpl(
      HttpAuthChallengeTokenizer*) override;
};

class ConnectionAuthOriginalNtlmFactory : public HttpAuthHandlerFactory {
 public:
#if BUILDFLAG(IS_WIN)
  void set_sspi_library(scoped_refptr<SSPILibrary> library) {
    library_ = std::move(library);
  }
#endif
  int CreateAuthHandler(HttpAuthChallengeTokenizer*, HttpAuth::Target,
                        const SSLInfo&, const NetworkAnonymizationKey&,
                        const url::SchemeHostPort&, CreateReason, int,
                        const NetLogWithSource&, HostResolver*,
                        std::unique_ptr<HttpAuthHandler>*) override;
 private:
#if BUILDFLAG(IS_WIN)
  scoped_refptr<SSPILibrary> library_;
#endif
};

#if BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_WIN)
class ConnectionAuthOriginalSSPI : public HttpAuthSSPI {
 public:
  using HttpAuthSSPI::HttpAuthSSPI;
  HttpAuth::AuthorizationResult ParseChallenge(HttpAuthChallengeTokenizer*) override;
};
#elif BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_POSIX)
class ConnectionAuthOriginalGSSAPI : public HttpAuthGSSAPI {
 public:
  using HttpAuthGSSAPI::HttpAuthGSSAPI;
  HttpAuth::AuthorizationResult ParseChallenge(HttpAuthChallengeTokenizer*) override;
  int GenerateAuthToken(const AuthCredentials*, const std::string&,
                        const std::string&, std::string*,
                        const NetLogWithSource&, CompletionOnceCallback) override;
 private:
  int GetNextSecurityToken(const std::string&, const std::string&,
                           gss_buffer_t, gss_buffer_t, const NetLogWithSource&);
};
#endif
}  // namespace net
#endif  // NET_HTTP_CONNECTION_AUTH_ORIGINAL_PROVIDERS_H_
