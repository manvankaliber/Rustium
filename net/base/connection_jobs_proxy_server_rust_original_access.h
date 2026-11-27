// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_CONNECTION_JOBS_PROXY_SERVER_RUST_ORIGINAL_ACCESS_H_
#define NET_BASE_CONNECTION_JOBS_PROXY_SERVER_RUST_ORIGINAL_ACCESS_H_

#include "base/check.h"
#include "net/base/proxy_server.h"

namespace net {

// Reference-only access to native immutable storage. The matching parent-owned
// friend declaration changes no object field, layout, virtual or public API.
// No public is_valid()/host_port_pair() call is made: their current diagnostic
// path otherwise reaches migrated Rust policy even for this storage getter.
struct ConnectionJobsProxyServerRustOriginalAccess {
  static const HostPortPair& HostPort(const ProxyServer& server) {
    // Exact original validity predicate from the frozen ProxyServer header.
    // Keep the original DCHECK(is_valid()) diagnostic expression spelling.
    [[maybe_unused]] const auto is_valid = [&server] {
      return server.scheme() != ProxyServer::SCHEME_INVALID;
    };
    DCHECK(is_valid());
    return server.host_port_pair_;
  }
};

}  // namespace net
#endif  // NET_BASE_CONNECTION_JOBS_PROXY_SERVER_RUST_ORIGINAL_ACCESS_H_
