// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#ifndef NET_DNS_DNS_CLIENT_RUST_SERVICES_H_
#define NET_DNS_DNS_CLIENT_RUST_SERVICES_H_
#include <stdint.h>
#include "net/dns/dns_resolution_policy_ffi.h"
namespace net {
// Query-scoped ownership/provider frame. Open, use and destroy synchronously
// during one client policy query. Never retain across callbacks, client
// replacement or owner destruction. Custom clients return null from Make.
class DnsClientRustServices {
 public:
  virtual ~DnsClientRustServices() = default;
  virtual DnsResolutionWords Dispatch(uint32_t operation,
                                      DnsResolutionWords input) = 0;
};
}  // namespace net
#endif
