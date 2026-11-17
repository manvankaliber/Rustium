// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_DNS_RESOLUTION_RESULT_TYPES_RUST_ORIGINAL_H_
#define NET_DNS_DNS_RESOLUTION_RESULT_TYPES_RUST_ORIGINAL_H_

#include "net/dns/host_resolver_cache_result_reference.h"

// The DNS pair agreed to reuse this immutable full original provider closure.
// Include before local declarations; remove reference-only class forward
// declarations which would shadow these aliases. No production result cast.
namespace net::dns_resolution_original {
using HostResolverInternalResult =
    ::net::host_resolver_cache_reference::HostResolverInternalResult;
using HostResolverInternalDataResult =
    ::net::host_resolver_cache_reference::HostResolverInternalDataResult;
using HostResolverInternalMetadataResult =
    ::net::host_resolver_cache_reference::HostResolverInternalMetadataResult;
using HostResolverInternalErrorResult =
    ::net::host_resolver_cache_reference::HostResolverInternalErrorResult;
using HostResolverInternalAliasResult =
    ::net::host_resolver_cache_reference::HostResolverInternalAliasResult;
}  // namespace net::dns_resolution_original

#endif  // NET_DNS_DNS_RESOLUTION_RESULT_TYPES_RUST_ORIGINAL_H_
