// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#ifndef NET_DNS_DNS_REQUEST_FAMILY_DRIFT_CHECKS_H_
#define NET_DNS_DNS_REQUEST_FAMILY_DRIFT_CHECKS_H_
#include "net/base/ech_mode.h"
#include "net/base/net_errors.h"
#include "net/dns/host_resolver.h"
#include "net/dns/dns_client.h"
#include "net/dns/dns_transaction.h"
#include "net/dns/public/insecure_dns_mode.h"
#include "net/dns/public/resolution_details.h"
#include "net/dns/public/secure_dns_mode.h"
#include "net/dns/public/dns_protocol.h"
static_assert(net::OK==0 && net::ERR_IO_PENDING==-1 && net::ERR_FAILED==-2 &&
              net::ERR_ABORTED==-3 && net::ERR_INVALID_ARGUMENT==-4 &&
              net::ERR_BLOCKED_BY_CLIENT==-20 && net::ERR_CONTEXT_SHUT_DOWN==-26 &&
              net::ERR_NAME_NOT_RESOLVED==-105 && net::ERR_DNS_SERVER_REQUIRES_TCP==-801 &&
              net::ERR_DNS_TIMED_OUT==-803 && net::ERR_DNS_CACHE_MISS==-804 &&
              net::ERR_DNS_SEARCH_EMPTY==-805 && net::ERR_DNS_REQUEST_CANCELLED==-810 &&
              net::ERR_DNS_SECURE_PROBE_RECORD_INVALID==-814);
static_assert(static_cast<int>(net::EchMode::kStrict)==2);
static_assert(static_cast<int>(net::InsecureDnsMode::kDisabled)==0 &&
              static_cast<int>(net::InsecureDnsMode::kEnabledBuiltIn)==1 &&
              static_cast<int>(net::InsecureDnsMode::kEnabledPlatform)==2 &&
              static_cast<int>(net::InsecureDnsMode::kEnabledPlatformNoSystem)==3);
static_assert(static_cast<int>(net::SecureDnsMode::kAutomatic)==1);
static_assert(static_cast<int>(net::ResolutionSource::kCache)==1 &&
              static_cast<int>(net::ResolutionSource::kLocal)==2);
static_assert(static_cast<int>(net::DnsConfigLocalNameserverState::kNoLocal)==0 &&
              static_cast<int>(net::DnsConfigLocalNameserverState::kOnlyLoopback)==1 &&
              static_cast<int>(net::DnsConfigLocalNameserverState::kOnlyNonLoopbackLocal)==2 &&
              static_cast<int>(net::DnsConfigLocalNameserverState::kLoopbackAndNonLoopback)==3);
static_assert(static_cast<int>(net::FallbackFromSecureTransactionPreferredReason::kFallbackNotPreferred)==0 &&
              static_cast<int>(net::FallbackFromSecureTransactionPreferredReason::kFallbackPreferredCannotUseSecureDns)==1 &&
              static_cast<int>(net::FallbackFromSecureTransactionPreferredReason::kFallbackPreferredNoAvailableDohServers)==4);
static_assert(static_cast<int>(net::DnsTransactionFactory::AttemptMode::kClassic)==0 &&
              static_cast<int>(net::DnsTransactionFactory::AttemptMode::kHttp)==1 &&
              static_cast<int>(net::DnsTransactionFactory::AttemptMode::kPlatform)==2);
static_assert(static_cast<int>(net::HostResolver::ResolveHostParameters::CacheUsage::ALLOWED)==0 &&
              static_cast<int>(net::HostResolver::ResolveHostParameters::CacheUsage::STALE_ALLOWED)==1 &&
              static_cast<int>(net::HostResolver::ResolveHostParameters::CacheUsage::STALE_ALLOWED_WHILE_REFRESHING)==2);
static_assert(net::dns_protocol::kTypeA==1 && net::dns_protocol::kTypeAAAA==28 &&
              net::dns_protocol::kTypeHttps==65);
#endif
