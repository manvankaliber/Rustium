// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_DNS_RESOLUTION_NATIVE_PROVIDER_ALIASES_H_
#define NET_DNS_DNS_RESOLUTION_NATIVE_PROVIDER_ALIASES_H_

#include "net/dns/address_sorter.h"
#include "net/dns/dns_session.h"
#include "net/log/net_log.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"

// Reference-only provider identity bindings, NOT original-policy declarations.
// The provider's actual policy/source/runtime remains explicitly shared unless
// an independently original implementation is injected through its interface.
namespace net::dns_resolution_original {
using AddressSorter = ::net::AddressSorter;
using ClientSocketFactory = ::net::ClientSocketFactory;
using DnsSession = ::net::DnsSession;
using NetLog = ::net::NetLog;
using NetLogWithSource = ::net::NetLogWithSource;
}  // namespace net::dns_resolution_original

#endif  // NET_DNS_DNS_RESOLUTION_NATIVE_PROVIDER_ALIASES_H_
