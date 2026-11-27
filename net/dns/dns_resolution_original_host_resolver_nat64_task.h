// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_RESOLUTION_ORIGINAL_HOST_RESOLVER_NAT64_TASK_H_
#define NET_DNS_DNS_RESOLUTION_ORIGINAL_HOST_RESOLVER_NAT64_TASK_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "net/base/network_handle.h"
#include "net/base/request_priority.h"
#include "net/dns/host_resolver.h"
#include "net/dns/dns_resolution_original_host_resolver_manager.h"
#include "net/dns/public/dns_query_type.h"

#include "base/component_export.h"
#include "net/dns/dns_resolution_native_provider_aliases.h"
#include "net/dns/dns_resolution_manager_native_providers.h"
#include "net/dns/dns_resolution_result_types_rust_original.h"
#include "net/dns/dns_resolution_host_value_rust_original.h"
#include "net/dns/host_resolver_cache_rust_reference.h"
#include "net/dns/dns_resolution_request_interfaces_rust_original.h"
#include "net/base/host_port_rust_reference.h"

namespace net::dns_resolution_original {

using HostPortPair = ::net::HostPortRustReference;
using HostResolverCache = ::net::host_resolver_cache_reference::HostResolverCache;
using HostResolverInternalResult = ::net::host_resolver_cache_reference::HostResolverInternalResult;
using HostResolverInternalDataResult = ::net::host_resolver_cache_reference::HostResolverInternalDataResult;
using HostResolverInternalMetadataResult = ::net::host_resolver_cache_reference::HostResolverInternalMetadataResult;
using HostResolverInternalErrorResult = ::net::host_resolver_cache_reference::HostResolverInternalErrorResult;
using HostResolverInternalAliasResult = ::net::host_resolver_cache_reference::HostResolverInternalAliasResult;



// Representation of a single HostResolverImpl::Job task to convert an IPv4
// address literal to an IPv4-Embedded IPv6 according to rfc6052.
// https://www.rfc-editor.org/rfc/rfc6052
// When a DNS64 is not found returns the original IPv4 address.
// Destruction cancels the task and prevents any callbacks from being invoked.
class HostResolverNat64Task {
 public:
  using CallbackType =
      base::OnceCallback<void(std::unique_ptr<HostResolverInternalResult>)>;

  HostResolverNat64Task(std::string_view hostname,
                        NetworkAnonymizationKey network_anonymization_key,
                        handles::NetworkHandle target_network,
                        NetLogWithSource net_log,
                        ResolveContext* resolve_context,
                        base::WeakPtr<HostResolverManager> resolver,
                        RequestPriority priority);

  HostResolverNat64Task(const HostResolverNat64Task&) = delete;
  HostResolverNat64Task& operator=(const HostResolverNat64Task&) = delete;

  ~HostResolverNat64Task();

  // Should only be called once.
  void Start(CallbackType completion_callback);

 private:
  const std::string hostname_;
  const NetworkAnonymizationKey network_anonymization_key_;
  const handles::NetworkHandle target_network_;
  NetLogWithSource net_log_;
  const raw_ptr<ResolveContext> resolve_context_;
  CallbackType completion_callback_;
  base::WeakPtr<HostResolverManager> resolver_;
  const RequestPriority priority_;

  SEQUENCE_CHECKER(sequence_checker_);

  int DoResolve();
  int DoResolveComplete(int result);
  int DoSynthesizeToIpv6();

  void OnIOComplete(int result);
  int DoLoop(int result);

  enum class State {
    kResolve,
    kResolveComplete,
    kSynthesizeToIpv6,
    kStateNone,
  };

  State next_state_ = State::kStateNone;

  std::unique_ptr<ResolveHostRequestOriginal> request_ipv4onlyarpa_;
  std::unique_ptr<HostResolverInternalResult> result_;

  base::WeakPtrFactory<HostResolverNat64Task> weak_ptr_factory_{this};
};

}  // namespace net::dns_resolution_original

#endif  // NET_DNS_DNS_RESOLUTION_ORIGINAL_HOST_RESOLVER_NAT64_TASK_H_
