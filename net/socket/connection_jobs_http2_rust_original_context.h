// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#ifndef NET_SOCKET_CONNECTION_JOBS_HTTP2_RUST_ORIGINAL_CONTEXT_H_
#define NET_SOCKET_CONNECTION_JOBS_HTTP2_RUST_ORIGINAL_CONTEXT_H_
#include <memory>
#include "net/spdy/spdy_session_pool_family_original.h"
namespace net {
class HttpNetworkSession;
// Native resolver/SSL/auth services are borrowed; original pool owns all H2
// sessions/streams. Must be destroyed before the provider HttpNetworkSession.
class ConnectionJobsHttp2RustOriginalContext {
 public:
  ConnectionJobsHttp2RustOriginalContext(HttpNetworkSession& providers,
                                       bool enable_sending_initial_data);
  ~ConnectionJobsHttp2RustOriginalContext();
  SpdySessionPoolOriginal* pool() { return pool_.get(); }
 private:
  std::unique_ptr<SpdySessionPoolOriginal> pool_;
};
}  // namespace net
#endif
