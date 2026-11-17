// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_DNS_RESOLUTION_RUST_ORIGINAL_TYPES_H_
#define NET_DNS_DNS_RESOLUTION_RUST_ORIGINAL_TYPES_H_

#include <cstddef>
#include <type_traits>
#include <utility>

#include "net/dns/dns_resolution_original_dns_names_util.h"
#include "net/dns/dns_resolution_original_dns_query.h"
#include "net/dns/dns_resolution_original_dns_response.h"

// Reference-only namespace proposal. No alias is introduced into namespace net,
// so no public/production API changes or mixed object-layout casts are needed.
namespace net::dns_resolution_original {

// DNS7 owns a complete frozen C++ wire closure, including OPT serialization.
// No production Query/Response/RecordRdata types enter this namespace.

// Draft binding guards. They have not been compiled; these declarations make
// the exact matched API expectation explicit for the later family build.
static_assert(std::is_same_v<
              decltype(std::declval<DnsResponse&>().InitParse(
                  std::declval<std::size_t>(),
                  std::declval<const DnsQuery&>())),
              bool>);
static_assert(std::is_same_v<
              decltype(std::declval<const DnsResponse&>().Parser()),
              DnsRecordParser>);

}  // namespace net::dns_resolution_original

#endif  // NET_DNS_DNS_RESOLUTION_RUST_ORIGINAL_TYPES_H_
