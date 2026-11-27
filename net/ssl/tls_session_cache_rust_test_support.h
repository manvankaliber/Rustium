// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SSL_TLS_SESSION_CACHE_RUST_TEST_SUPPORT_H_
#define NET_SSL_TLS_SESSION_CACHE_RUST_TEST_SUPPORT_H_
#include <cstdlib>

#include "build/build_config.h"
#if BUILDFLAG(IS_WIN)
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include "third_party/boringssl/src/include/openssl/bio.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"
namespace net::tls_session_cache_rust_testing {
inline void FixedCryptoTime(const SSL*, timeval* out) {
  out->tv_sec = 1000;
  out->tv_usec = 0;
}

inline bssl::UniquePtr<SSL_SESSION> EarlySession() {
  bssl::UniquePtr<SSL_CTX> client_ctx(SSL_CTX_new(TLS_method())),
      server_ctx(SSL_CTX_new(TLS_method()));
  SSL_CTX_set_current_time_cb(client_ctx.get(), FixedCryptoTime);
  SSL_CTX_set_current_time_cb(server_ctx.get(), FixedCryptoTime);
  const char* pem = std::getenv("TLS_SESSION_CACHE_RUST_CERT_FILE");
  if (!pem) {
    pem = "../../net/data/ssl/certificates/ok_cert.pem";
  }
  if (!SSL_CTX_use_certificate_chain_file(server_ctx.get(), pem) ||
      !SSL_CTX_use_PrivateKey_file(server_ctx.get(), pem, SSL_FILETYPE_PEM)) {
    return nullptr;
  }
  SSL_CTX_set_min_proto_version(client_ctx.get(), TLS1_3_VERSION);
  SSL_CTX_set_max_proto_version(client_ctx.get(), TLS1_3_VERSION);
  SSL_CTX_set_min_proto_version(server_ctx.get(), TLS1_3_VERSION);
  SSL_CTX_set_max_proto_version(server_ctx.get(), TLS1_3_VERSION);
  SSL_CTX_set_early_data_enabled(client_ctx.get(), 1);
  SSL_CTX_set_early_data_enabled(server_ctx.get(), 1);
  SSL_CTX_set_session_cache_mode(client_ctx.get(), SSL_SESS_CACHE_BOTH);
  SSL_CTX_sess_set_new_cb(client_ctx.get(), [](SSL* ssl, SSL_SESSION* session) {
    static_cast<bssl::UniquePtr<SSL_SESSION>*>(SSL_get_app_data(ssl))
        ->reset(session);
    return 1;
  });
  SSL_CTX_set_session_cache_mode(server_ctx.get(), SSL_SESS_CACHE_BOTH);
  bssl::UniquePtr<SSL> client(SSL_new(client_ctx.get())),
      server(SSL_new(server_ctx.get()));
  bssl::UniquePtr<SSL_SESSION> saved;
  SSL_set_app_data(client.get(), &saved);
  BIO *cb = nullptr, *sb = nullptr;
  if (!BIO_new_bio_pair(&cb, 0, &sb, 0)) {
    return nullptr;
  }
  SSL_set_bio(client.get(), cb, cb);
  SSL_set_bio(server.get(), sb, sb);
  SSL_set_connect_state(client.get());
  SSL_set_accept_state(server.get());
  for (int i = 0; i < 100; ++i) {
    for (auto* ssl : {client.get(), server.get()}) {
      int r = SSL_do_handshake(ssl);
      if (r != 1) {
        int e = SSL_get_error(ssl, r);
        if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE) {
          return nullptr;
        }
      }
    }
    if (SSL_is_init_finished(client.get()) &&
        SSL_is_init_finished(server.get())) {
      break;
    }
  }
  if (!SSL_is_init_finished(client.get()) ||
      !SSL_is_init_finished(server.get())) {
    return nullptr;
  }
  char value = 'x';
  if (SSL_write(server.get(), &value, 1) != 1 ||
      SSL_read(client.get(), &value, 1) != 1) {
    return nullptr;
  }
  return saved;
}
}  // namespace net::tls_session_cache_rust_testing
#endif
