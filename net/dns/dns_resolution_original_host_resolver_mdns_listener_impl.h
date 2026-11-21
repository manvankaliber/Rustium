// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_RESOLUTION_ORIGINAL_HOST_RESOLVER_MDNS_LISTENER_IMPL_H_
#define NET_DNS_DNS_RESOLUTION_ORIGINAL_HOST_RESOLVER_MDNS_LISTENER_IMPL_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "net/base/net_errors.h"
#include "net/dns/host_resolver.h"
#include "net/dns/dns_resolution_original_mdns_client.h"
#include "net/dns/public/dns_query_type.h"

#include "base/component_export.h"
#include "net/dns/dns_resolution_native_provider_aliases.h"
#include "net/dns/dns_resolution_manager_native_providers.h"
#include "net/dns/dns_resolution_result_types_rust_original.h"
#include "net/dns/dns_resolution_host_value_rust_original.h"
#include "net/dns/host_resolver_cache_rust_reference.h"
#include "net/dns/dns_resolution_request_interfaces_rust_original.h"
#include "net/base/host_port_rust_reference.h"

#include "net/dns/dns_resolution_mdns_listener_interface_rust_original.h"

namespace net::dns_resolution_original {

using HostPortPair = ::net::HostPortRustReference;
using HostResolverCache = ::net::host_resolver_cache_reference::HostResolverCache;
using HostResolverInternalResult = ::net::host_resolver_cache_reference::HostResolverInternalResult;
using HostResolverInternalDataResult = ::net::host_resolver_cache_reference::HostResolverInternalDataResult;
using HostResolverInternalMetadataResult = ::net::host_resolver_cache_reference::HostResolverInternalMetadataResult;
using HostResolverInternalErrorResult = ::net::host_resolver_cache_reference::HostResolverInternalErrorResult;
using HostResolverInternalAliasResult = ::net::host_resolver_cache_reference::HostResolverInternalAliasResult;


class RecordParsed;

// Intermediary between the HostResolver::CreateMdnsListener API and the
// underlying listener functionality in MDnsClient.
class HostResolverMdnsListenerImpl : public MdnsListenerOriginal,
                                     public MDnsListener::Delegate {
 public:
  using Delegate = MdnsListenerOriginal::Delegate;

  HostResolverMdnsListenerImpl(const HostPortPair& query_host,
                               DnsQueryType query_type);
  ~HostResolverMdnsListenerImpl() override;

  void set_inner_listener(std::unique_ptr<MDnsListener> inner_listener) {
    DCHECK_EQ(OK, initialization_error_);
    inner_listener_ = std::move(inner_listener);
  }

  void set_initialization_error(int error) {
    DCHECK(!inner_listener_);
    initialization_error_ = error;
  }

  // MdnsListenerOriginal implementation
  int Start(Delegate* delegate) override;

  // MDnsListener::Delegate implementation
  void OnRecordUpdate(MDnsListener::UpdateType update,
                      const RecordParsed* record) override;
  void OnNsecRecord(const std::string& name, unsigned type) override;
  void OnCachePurged() override;

 private:
  HostPortPair query_host_;
  DnsQueryType query_type_;

  int initialization_error_ = OK;
  std::unique_ptr<MDnsListener> inner_listener_;
  raw_ptr<Delegate> delegate_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace net::dns_resolution_original

#endif  // NET_DNS_DNS_RESOLUTION_ORIGINAL_HOST_RESOLVER_MDNS_LISTENER_IMPL_H_
