// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#ifndef NET_DNS_DNS_REQUEST_PARTITION_PROVIDER_H_
#define NET_DNS_DNS_REQUEST_PARTITION_PROVIDER_H_
#include <utility>
#include "base/check_op.h"
#include "base/feature_list.h"
#include "net/base/features.h"
#include "net/base/network_anonymization_key.h"
#include "net/dns/dns_request_family_ffi.h"
namespace net {
inline NetworkAnonymizationKey DnsRequestRetainPartitionKey(
    NetworkAnonymizationKey input) {
  auto feature = [](void*, uint32_t operation, DnsResolverWords) {
    CHECK_EQ(operation, 0u);
    return dns_request_family::W(base::FeatureList::IsEnabled(
        features::kSplitHostCacheByNetworkAnonymizationKey));
  };
  const bool keep = rustium_dns_request_family_run(
      6, 0, dns_request_family::W(NetworkAnonymizationKey::IsPartitioningEnabled()),
      nullptr, feature).x;
  // Ownership selection follows the Rust decision; feature ordering lives there.
  return keep ? std::move(input) : NetworkAnonymizationKey();
}
}  // namespace net
#endif
