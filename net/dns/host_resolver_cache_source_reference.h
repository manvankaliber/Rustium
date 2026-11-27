// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_HOST_RESOLVER_CACHE_SOURCE_REFERENCE_H_
#define NET_DNS_HOST_RESOLVER_CACHE_SOURCE_REFERENCE_H_
#include <optional>
#include "base/values.h"
#include "net/dns/public/host_resolver_source.h"
namespace net::host_resolver_cache_reference {
base::Value OriginalHostResolverSourceToValue(HostResolverSource source);
std::optional<HostResolverSource> OriginalHostResolverSourceFromValue(
    const base::Value& value);
}  // namespace net::host_resolver_cache_reference
#endif
