// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/host_cache.h"

#include <algorithm>
#include <array>
#include <deque>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/stack_allocated.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/types/optional_util.h"
#include "base/value_iterators.h"
#include "net/base/address_family.h"
#include "net/base/ip_endpoint.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/trace_constants.h"
#include "net/base/url_util.h"
#include "net/dns/dns_host_cache_rust_ffi.h"
#include "net/dns/dns_host_cache_rust_ffi.rs.h"
#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_internal_result.h"
#include "net/dns/https_record_rdata.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/public/host_resolver_source.h"
#include "net/log/net_log.h"
#include "url/scheme_host_port.h"

namespace net {

// Lock scalar discriminants transported to the safe policy core. Extending
// native enums must explicitly update policy rather than silently drifting.
static_assert(ERR_NAME_NOT_RESOLVED == -105 && ERR_NETWORK_CHANGED == -21 &&
              ERR_HOST_RESOLVER_QUEUE_TOO_LARGE == -119);
static_assert(kDnsQueryTypes.size() == 7 &&
              static_cast<int>(DnsQueryType::HTTPS) == 7);
static_assert(static_cast<int>(base::Value::Type::DICT) == 6 &&
              static_cast<int>(base::Value::Type::LIST) == 7);
static_assert(static_cast<int>(HostResolverInternalResult::Type::kData) == 0 &&
              static_cast<int>(HostResolverInternalResult::Type::kAlias) == 3 &&
              static_cast<int>(HostResolverInternalResult::Source::kDns) == 0 &&
              static_cast<int>(HostResolverInternalResult::Source::kUnknown) ==
                  2);

namespace {

// String constants for dictionary keys.
const char kInterfaceNameKey[] = "interface_name";

base::Value IpEndpointToValue(const IPEndPoint& endpoint);

std::optional<IPEndPoint> IpEndpointFromValue(const base::Value& value);

base::Value EndpointMetadataPairToValue(
    const std::pair<HttpsRecordPriority, ConnectionEndpointMetadata>& pair);

std::optional<std::pair<HttpsRecordPriority, ConnectionEndpointMetadata>>
EndpointMetadataPairFromValue(const base::Value& value);

template <typename T>
void MergeLists(T& target, const T& source) {
  target.insert(target.end(), source.begin(), source.end());
}

template <typename T>
void MergeContainers(T& target, const T& source) {
  target.insert(source.begin(), source.end());
}

// Used to reject empty and IP literal (whether or not surrounded by brackets)
// hostnames.
bool IsValidHostname(std::string_view hostname);

const std::string& GetHostname(
    const std::variant<url::SchemeHostPort, std::string>& host) {
  const std::string* hostname;
  if (std::holds_alternative<url::SchemeHostPort>(host)) {
    hostname = &std::get<url::SchemeHostPort>(host).host();
  } else {
    DCHECK(std::holds_alternative<std::string>(host));
    hostname = &std::get<std::string>(host);
  }

  DCHECK(IsValidHostname(*hostname));
  return *hostname;
}

const std::string GetHistogramName(std::string_view histogram_name,
                                   const HostCache::Key& key) {
  constexpr std::string_view kHistogramPrefix = "Net.DNS.HostCache.";

  return base::StrCat(
      {kHistogramPrefix, histogram_name,
       IsGoogleHostWithAlpnH3(GetHostname(key.host)) ? ".GoogleHost" : ""});
}

}  // namespace

// Used in histograms; do not modify existing values.
enum HostCache::SetOutcome : int {
  SET_INSERT = 0,
  SET_UPDATE_VALID = 1,
  SET_UPDATE_STALE = 2,
  MAX_SET_OUTCOME
};

struct DnsHostCacheRustServices::State {
  STACK_ALLOCATED();

 public:
  using Pair = HostCache::EntryMap::value_type;
  HostCache* ca = nullptr;
  std::array<HostCache::Entry*, 2> en = {};
  std::array<const HostCache::Key*, 2> ke = {};
  base::TimeTicks now;
  base::Time wall;
  base::TimeDelta ttl;
  int network_arg = 0;
  bool hit_arg = false;
  bool ignore = false, metrics = true, include_stale = false, debug = false;
  int outcome = 0;
  uint16_t port = 0;
  IPAddress ip;
  std::optional<uint32_t> scope;
  std::optional<IPEndPoint> parsed_endpoint;
  std::optional<ConnectionEndpointMetadata> parsed_metadata;
  std::optional<std::pair<HttpsRecordPriority, ConnectionEndpointMetadata>>
      parsed_pair;
  const IPEndPoint* endpoint = nullptr;
  const std::pair<HttpsRecordPriority, ConnectionEndpointMetadata>*
      metadata_pair = nullptr;
  bool* alive = nullptr;
  std::deque<HostCache::Key> filter_keys;
  HostCache::EntryStaleness* stale = nullptr;
  HostCache::Entry::Source* source = nullptr;
  const base::RepeatingCallback<bool(const std::string&)>* filter = nullptr;
  std::string_view hostname;
  std::vector<Pair*> rows;
  // Call-local native iterator snapshots avoid repeated tree walks. No native
  // collection element is exposed to Rust, and SelectRow invalidates snapshots.
  std::vector<
      const std::pair<const HttpsRecordPriority, ConnectionEndpointMetadata>*>
      metadata_rows;
  std::vector<const std::string*> canonical_rows;
  const auto& metadata_at(size_t i) {
    if (metadata_rows.empty()) {
      for (const auto& p : en[0]->endpoint_metadatas_) {
        metadata_rows.push_back(&p);
      }
    }
    return *metadata_rows[i];
  }
  const std::string& canonical_at(size_t i) {
    if (canonical_rows.empty()) {
      for (const auto& name : en[0]->canonical_names_) {
        canonical_rows.push_back(&name);
      }
    }
    return *canonical_rows[i];
  }
  std::vector<HostResolverEndpointResult> endpoints;
  std::vector<ServiceEndpoint> services;
  std::vector<ConnectionEndpointMetadata> metadatas;
  std::vector<IPEndPoint> v4, v6;
  const std::set<std::unique_ptr<HostResolverInternalResult>>* results =
      nullptr;
  std::vector<const HostResolverInternalResult*> internal;
  HostCache::Entry::Source empty_source = HostCache::Entry::SOURCE_UNKNOWN;
  std::optional<base::TimeDelta> optional_ttl;
  std::deque<base::Value> owned;
  std::vector<const base::Value*> values;
  base::ListValue* output = nullptr;
  const base::ListValue* input = nullptr;
  std::variant<url::SchemeHostPort, std::string> restore_host;
  NetworkAnonymizationKey restore_nak;
  base::TimeTicks restore_expiration;
  HostResolverFlags restore_flags = 0;
  int restore_error = OK;
  std::unique_ptr<HostCache::Entry> restored;
  std::unique_ptr<HostCache::Key> restored_key;
  int64_t restore_raw_time = 0;
  int64_t row(Pair* p) {
    rows.push_back(p);
    return rows.size() + 1;
  }
  HostCache::Entry& entry(int64_t i) {
    if (i < 2) {
      return *en[i];
    }
    return rows[i - 2]->second;
  }
  const HostCache::Key& key(int64_t i) {
    if (i < 2) {
      return *ke[i];
    }
    return rows[i - 2]->first;
  }
  Pair* pair(int64_t i) { return i < 0 ? nullptr : rows[i - 2]; }
  int64_t ref(const base::Value& v) {
    values.push_back(&v);
    return values.size() - 1;
  }
  int64_t own(base::Value v) {
    owned.push_back(std::move(v));
    return ref(owned.back());
  }
  const base::Value& value(int64_t i) { return *values[i]; }
  base::Value& mutable_value(int64_t i) {
    return const_cast<base::Value&>(value(i));
  }
};
int64_t DnsHostCacheRustServices::Call(uint32_t op,
                                       int64_t a,
                                       int64_t b,
                                       int64_t c,
                                       int64_t d) const {
  State& s = state_;
  static constexpr std::array kFields = {"scheme",
                                         "hostname",
                                         "port",
                                         "dns_query_type",
                                         "flags",
                                         "host_resolver_source",
                                         "secure",
                                         "network_anonymization_key",
                                         "expiration",
                                         "ttl",
                                         "pinned",
                                         "network_changes",
                                         "net_error",
                                         "ip_endpoints",
                                         "endpoint_address",
                                         "endpoint_port",
                                         "interface_name",
                                         "endpoint_metadatas",
                                         "endpoint_metadata_weight",
                                         "endpoint_metadata_value",
                                         "aliases",
                                         "addresses",
                                         "text_records",
                                         "hostname_results",
                                         "host_ports",
                                         "canonical_names",
                                         "target_network"};
  switch (static_cast<DnsHostCacheRustOp>(op)) {
    case DnsHostCacheRustOp::Max: {
      return s.ca->max_entries_;
    }
    case DnsHostCacheRustOp::Size: {
      return s.ca->entries_.size();
    }
    case DnsHostCacheRustOp::Network: {
      return s.ca->network_changes_;
    }
    case DnsHostCacheRustOp::Invalidate: {
      ++s.ca->network_changes_;
      return 0;
    }
    case DnsHostCacheRustOp::Find: {
      auto it = s.ca->entries_.find(*s.ke[0]);
      return it == s.ca->entries_.end() ? -1 : s.row(&*it);
    }
    case DnsHostCacheRustOp::FindOther: {
      auto key = *s.ke[0];
      key.secure = !key.secure;
      auto it = s.ca->entries_.find(key);
      return it == s.ca->entries_.end() ? -1 : s.row(&*it);
    }
    case DnsHostCacheRustOp::Rows: {
      for (auto& p : s.ca->entries_) {
        s.rows.push_back(&p);
      }
      return s.rows.size();
    }
    case DnsHostCacheRustOp::Expires: {
      return s.entry(a).expires_.ToInternalValue();
    }
    case DnsHostCacheRustOp::Changes: {
      return s.entry(a).network_changes_;
    }
    case DnsHostCacheRustOp::Pin: {
      return s.entry(a).pinning_.has_value() ? (*s.entry(a).pinning_ ? 1 : 0)
                                             : -1;
    }
    case DnsHostCacheRustOp::Secure: {
      return s.key(a).secure;
    }
    case DnsHostCacheRustOp::ExpiredBy: {
      return (s.now - s.entry(a).expires_).InMicroseconds();
    }
    case DnsHostCacheRustOp::TotalHits: {
      return s.entry(a).total_hits_;
    }
    case DnsHostCacheRustOp::StaleHits: {
      return s.entry(a).stale_hits_;
    }
    case DnsHostCacheRustOp::SetHits: {
      s.entry(a).total_hits_ = b;
      s.entry(a).stale_hits_ = c;
      return 0;
    }
    case DnsHostCacheRustOp::FillStale: {
      s.stale->expired_by = s.now - s.entry(a).expires_;
      s.stale->network_changes = b;
      s.stale->stale_hits = s.entry(a).stale_hits_;
      return 0;
    }
    case DnsHostCacheRustOp::StaleOutput: {
      return s.stale != nullptr;
    }
    case DnsHostCacheRustOp::Metrics: {
      return s.metrics;
    }
    case DnsHostCacheRustOp::IgnoreSecure: {
      return s.ignore;
    }
    case DnsHostCacheRustOp::LookupMetric: {
      s.ca->RecordLookup(static_cast<HostCache::LookupOutcome>(a), s.now,
                         b < 0 ? *s.ke[0] : s.key(b),
                         b < 0 ? nullptr : &s.entry(b));
      return 0;
    }
    case DnsHostCacheRustOp::EraseMetric: {
      s.ca->RecordErase(static_cast<HostCache::EraseReason>(a), s.now, s.key(b),
                        s.entry(b));
      return 0;
    }
    case DnsHostCacheRustOp::Erase: {
      s.ca->entries_.erase(s.key(a));
      return 0;
    }
    case DnsHostCacheRustOp::Error: {
      return s.entry(a).error_;
    }
    case DnsHostCacheRustOp::Contents: {
      return s.entry(a).ContentsEqual(s.entry(b));
    }
    case DnsHostCacheRustOp::Insert: {
      HostCache::Entry e(*s.en[0], s.now, s.ttl, s.ca->network_changes_);
      e.set_pinning(a != 0);
      s.ca->AddEntry(*s.ke[0], std::move(e));
      return 0;
    }
    case DnsHostCacheRustOp::HasDelegate: {
      return s.ca->delegate_ != nullptr;
    }
    case DnsHostCacheRustOp::Schedule: {
      s.ca->delegate_->ScheduleWrite();
      return 0;
    }
    case DnsHostCacheRustOp::ClearMap: {
      s.ca->entries_.clear();
      return 0;
    }
    case DnsHostCacheRustOp::FilterNull: {
      return s.filter->is_null();
    }
    case DnsHostCacheRustOp::Filter: {
      std::string hostname = GetHostname(s.filter_keys[a - 2].host);
      return s.filter->Run(hostname);
    }
    case DnsHostCacheRustOp::Clock: {
      s.now = s.ca->tick_clock_->NowTicks();
      return 0;
    }
    case DnsHostCacheRustOp::MatchingHost: {
      return GetHostname(s.key(a).host) == s.hostname;
    }
    case DnsHostCacheRustOp::MatchingOut: {
      if (s.source) {
        *s.source = s.entry(a).source_;
      }
      if (s.stale) {
        s.entry(a).GetStaleness(s.ca->tick_clock_->NowTicks(),
                                s.ca->network_changes_, s.stale);
      }
      return 0;
    }
    case DnsHostCacheRustOp::Eq0: {
      return s.en[0]->error_ == s.en[1]->error_;
    }
    case DnsHostCacheRustOp::Eq1: {
      return s.en[0]->ip_endpoints_ == s.en[1]->ip_endpoints_;
    }
    case DnsHostCacheRustOp::Eq2: {
      return s.en[0]->endpoint_metadatas_ == s.en[1]->endpoint_metadatas_;
    }
    case DnsHostCacheRustOp::Eq3: {
      return s.en[0]->aliases_ == s.en[1]->aliases_;
    }
    case DnsHostCacheRustOp::Eq4: {
      return s.en[0]->text_records_ == s.en[1]->text_records_;
    }
    case DnsHostCacheRustOp::Eq5: {
      return s.en[0]->hostnames_ == s.en[1]->hostnames_;
    }
    case DnsHostCacheRustOp::Eq6: {
      return s.en[0]->canonical_names_ == s.en[1]->canonical_names_;
    }
    case DnsHostCacheRustOp::Eq7: {
      return s.en[0]->source_ == s.en[1]->source_;
    }
    case DnsHostCacheRustOp::Eq8: {
      return s.en[0]->pinning_ == s.en[1]->pinning_;
    }
    case DnsHostCacheRustOp::Eq9: {
      return s.en[0]->ttl_ == s.en[1]->ttl_;
    }
    case DnsHostCacheRustOp::Eq10: {
      return s.en[0]->expires_ == s.en[1]->expires_;
    }
    case DnsHostCacheRustOp::Eq11: {
      return s.en[0]->network_changes_ == s.en[1]->network_changes_;
    }
    case DnsHostCacheRustOp::Eq12: {
      return s.en[0]->total_hits_ == s.en[1]->total_hits_;
    }
    case DnsHostCacheRustOp::Eq13: {
      return s.en[0]->stale_hits_ == s.en[1]->stale_hits_;
    }
    case DnsHostCacheRustOp::KeyCmp0: {
      return (*s.ke[0]).dns_query_type < (*s.ke[1]).dns_query_type
                 ? -1
                 : ((*s.ke[1]).dns_query_type < (*s.ke[0]).dns_query_type ? 1
                                                                          : 0);
    }
    case DnsHostCacheRustOp::KeyCmp1: {
      return (*s.ke[0]).host_resolver_flags < (*s.ke[1]).host_resolver_flags
                 ? -1
                 : ((*s.ke[1]).host_resolver_flags <
                            (*s.ke[0]).host_resolver_flags
                        ? 1
                        : 0);
    }
    case DnsHostCacheRustOp::KeyCmp2: {
      return (*s.ke[0]).host < (*s.ke[1]).host
                 ? -1
                 : ((*s.ke[1]).host < (*s.ke[0]).host ? 1 : 0);
    }
    case DnsHostCacheRustOp::KeyCmp3: {
      return (*s.ke[0]).host_resolver_source < (*s.ke[1]).host_resolver_source
                 ? -1
                 : ((*s.ke[1]).host_resolver_source <
                            (*s.ke[0]).host_resolver_source
                        ? 1
                        : 0);
    }
    case DnsHostCacheRustOp::KeyCmp4: {
      return (*s.ke[0]).network_anonymization_key <
                     (*s.ke[1]).network_anonymization_key
                 ? -1
                 : ((*s.ke[1]).network_anonymization_key <
                            (*s.ke[0]).network_anonymization_key
                        ? 1
                        : 0);
    }
    case DnsHostCacheRustOp::KeyCmp5: {
      return (*s.ke[0]).secure < (*s.ke[1]).secure
                 ? -1
                 : ((*s.ke[1]).secure < (*s.ke[0]).secure ? 1 : 0);
    }
    case DnsHostCacheRustOp::KeyCmp6: {
      return (*s.ke[0]).target_network < (*s.ke[1]).target_network
                 ? -1
                 : ((*s.ke[1]).target_network < (*s.ke[0]).target_network ? 1
                                                                          : 0);
    }
    case DnsHostCacheRustOp::Merge0: {
      s.en[0]->ip_endpoints_.insert(s.en[0]->ip_endpoints_.end(),
                                    s.en[1]->ip_endpoints_.begin(),
                                    s.en[1]->ip_endpoints_.end());
      return 0;
    }
    case DnsHostCacheRustOp::Merge1: {
      s.en[0]->endpoint_metadatas_.insert(s.en[1]->endpoint_metadatas_.begin(),
                                          s.en[1]->endpoint_metadatas_.end());
      return 0;
    }
    case DnsHostCacheRustOp::Merge2: {
      s.en[0]->aliases_.insert(s.en[1]->aliases_.begin(),
                               s.en[1]->aliases_.end());
      return 0;
    }
    case DnsHostCacheRustOp::Merge3: {
      s.en[0]->text_records_.insert(s.en[0]->text_records_.end(),
                                    s.en[1]->text_records_.begin(),
                                    s.en[1]->text_records_.end());
      return 0;
    }
    case DnsHostCacheRustOp::Merge4: {
      s.en[0]->hostnames_.insert(s.en[0]->hostnames_.end(),
                                 s.en[1]->hostnames_.begin(),
                                 s.en[1]->hostnames_.end());
      return 0;
    }
    case DnsHostCacheRustOp::Merge5: {
      s.en[0]->canonical_names_.insert(s.en[1]->canonical_names_.begin(),
                                       s.en[1]->canonical_names_.end());
      return 0;
    }
    case DnsHostCacheRustOp::Ttl: {
      return s.entry(a).ttl_.InMicroseconds();
    }
    case DnsHostCacheRustOp::SetMergeScalars: {
      s.en[0]->error_ = a;
      s.en[0]->ttl_ = base::Microseconds(b);
      s.en[0]->expires_ = base::TimeTicks::FromInternalValue(c);
      s.en[0]->network_changes_ = d;
      return 0;
    }
    case DnsHostCacheRustOp::EndpointCount: {
      return s.en[0]->ip_endpoints_.size();
    }
    case DnsHostCacheRustOp::HostCount: {
      return s.en[0]->hostnames_.size();
    }
    case DnsHostCacheRustOp::EndpointPort: {
      return s.en[0]->ip_endpoints_[a].port();
    }
    case DnsHostCacheRustOp::HostPort: {
      return s.en[0]->hostnames_[a].port();
    }
    case DnsHostCacheRustOp::SetEndpointPort: {
      s.en[0]->ip_endpoints_[a] =
          s.en[0]->ip_endpoints_[a].CopyWithPort(s.port);
      return 0;
    }
    case DnsHostCacheRustOp::SetHostPort: {
      s.en[0]->hostnames_[a].set_port(s.port);
      return 0;
    }
    case DnsHostCacheRustOp::MetadataCount: {
      return s.en[0]->endpoint_metadatas_.size();
    }
    case DnsHostCacheRustOp::CanonicalCount: {
      return s.en[0]->canonical_names_.size();
    }
    case DnsHostCacheRustOp::TargetMatches: {
      return s.en[0]->canonical_names_.contains(
          s.metadata_at(a).second.target_name);
    }
    case DnsHostCacheRustOp::AppendEndpoint: {
      HostResolverEndpointResult e;
      e.ip_endpoints = s.en[0]->ip_endpoints_;
      if (a >= 0) {
        e.metadata = s.metadata_at(a).second;
      }
      s.endpoints.push_back(std::move(e));
      return 0;
    }
    case DnsHostCacheRustOp::AppendMetadata: {
      s.metadatas.push_back(s.metadata_at(a).second);
      return 0;
    }
    case DnsHostCacheRustOp::IsV6: {
      return s.en[0]->ip_endpoints_[a].address().IsIPv6();
    }
    case DnsHostCacheRustOp::ServiceIp: {
      auto ep = s.en[0]->ip_endpoints_[a];
      if (b) {
        ep = ep.CopyWithPort(s.port);
      }
      (c ? s.v6 : s.v4).push_back(std::move(ep));
      return 0;
    }
    case DnsHostCacheRustOp::AppendService: {
      ServiceEndpoint e;
      e.ipv4_endpoints = s.v4;
      e.ipv6_endpoints = s.v6;
      if (a >= 0) {
        e.metadata = s.metadata_at(a).second;
      }
      s.services.push_back(std::move(e));
      return 0;
    }
    case DnsHostCacheRustOp::UmaEnum: {
      if (a == 0) {
        base::UmaHistogramEnumeration(GetHistogramName("Lookup", *s.ke[0]),
                                      static_cast<HostCache::LookupOutcome>(b));
      } else {
        base::UmaHistogramEnumeration(GetHistogramName("Erase", *s.ke[0]),
                                      static_cast<HostCache::EraseReason>(b));
      }
      return 0;
    }
    case DnsHostCacheRustOp::UmaTime: {
      static constexpr std::array names = {"LookupStale.ExpiredBy",
                                           "EraseStale.ExpiredBy",
                                           "EraseValid.ValidFor"};
      base::UmaHistogramLongTimes(GetHistogramName(names[a], *s.ke[0]),
                                  base::Microseconds(b));
      return 0;
    }
    case DnsHostCacheRustOp::UmaCount: {
      static constexpr std::array names = {"LookupStale.NetworkChanges",
                                           "EraseStale.NetworkChanges",
                                           "EraseStale.StaleHits"};
      base::UmaHistogramCounts1000(GetHistogramName(names[a], *s.ke[0]), b);
      return 0;
    }
    case DnsHostCacheRustOp::Outcome: {
      return s.outcome;
    }
    case DnsHostCacheRustOp::ResultCount: {
      for (const auto& p : *s.results) {
        s.internal.push_back(p.get());
      }
      return s.internal.size();
    }
    case DnsHostCacheRustOp::ResultType: {
      return static_cast<int>(s.internal[a]->type());
    }
    case DnsHostCacheRustOp::ResultSource: {
      return static_cast<int>(s.internal[a]->source());
    }
    case DnsHostCacheRustOp::ResultTtlCount: {
      return s.internal[a]->expiration().has_value() +
             s.internal[a]->timed_expiration().has_value();
    }
    case DnsHostCacheRustOp::ResultTtl: {
      if (b == 0 && s.internal[a]->expiration()) {
        return (*s.internal[a]->expiration() - s.now).InMicroseconds();
      }
      if (b == 1 && s.internal[a]->timed_expiration()) {
        return (*s.internal[a]->timed_expiration() - s.wall).InMicroseconds();
      }
      return INT64_MAX;
    }
    case DnsHostCacheRustOp::ResultV6: {
      return !s.internal[a]->AsData().endpoints().empty() &&
             s.internal[a]->AsData().endpoints().front().GetFamily() ==
                 ADDRESS_FAMILY_IPV6;
    }
    case DnsHostCacheRustOp::ResultError: {
      return s.internal[a]->AsError().error();
    }
    case DnsHostCacheRustOp::ResultQuery: {
      return static_cast<int>(s.internal[a]->query_type());
    }
    case DnsHostCacheRustOp::Check: {
      CHECK(a);
      return 0;
    }
    case DnsHostCacheRustOp::Dcheck: {
      DCHECK(a);
      return 0;
    }
    case DnsHostCacheRustOp::DataAppend: {
      const auto& p = s.internal[a]->AsData();
      MergeLists(s.en[0]->ip_endpoints_, p.endpoints());
      MergeLists(s.en[0]->text_records_, p.strings());
      MergeLists(s.en[0]->hostnames_, p.hosts());
      s.en[0]->canonical_names_.insert(p.domain_name());
      s.en[0]->aliases_.insert(p.domain_name());
      return 0;
    }
    case DnsHostCacheRustOp::DataNonempty: {
      const auto& p = s.internal[a]->AsData();
      return !p.endpoints().empty() || !p.strings().empty() ||
             !p.hosts().empty();
    }
    case DnsHostCacheRustOp::AliasAppend: {
      s.en[0]->aliases_.insert(s.internal[a]->domain_name());
      s.en[0]->aliases_.insert(s.internal[a]->AsAlias().alias_target());
      return 0;
    }
    case DnsHostCacheRustOp::MetadataCopy: {
      s.en[0]->endpoint_metadatas_ = s.internal[a]->AsMetadata().metadatas();
      return s.en[0]->endpoint_metadatas_.empty();
    }
    case DnsHostCacheRustOp::InternalScalars: {
      s.en[0]->error_ = a;
      s.en[0]->source_ = static_cast<HostCache::Entry::Source>(b);
      s.en[0]->ttl_ = base::Microseconds(c);
      return 0;
    }
    case DnsHostCacheRustOp::EmptySource: {
      return s.empty_source;
    }
    case DnsHostCacheRustOp::NewDict: {
      return s.own(base::Value(base::DictValue()));
    }
    case DnsHostCacheRustOp::NewList: {
      return s.own(base::Value(base::ListValue()));
    }
    case DnsHostCacheRustOp::IntValue: {
      return s.own(base::Value(static_cast<int>(a)));
    }
    case DnsHostCacheRustOp::BoolValue: {
      return s.own(base::Value(a != 0));
    }
    case DnsHostCacheRustOp::SetField: {
      s.mutable_value(a).GetDict().Set(kFields[b], s.value(c).Clone());
      return 0;
    }
    case DnsHostCacheRustOp::Append: {
      s.mutable_value(a).GetList().Append(s.value(b).Clone());
      return 0;
    }
    case DnsHostCacheRustOp::Type: {
      return static_cast<int>(s.value(a).type());
    }
    case DnsHostCacheRustOp::Field: {
      const base::Value* v = s.value(a).GetDict().Find(kFields[b]);
      return v ? s.ref(*v) : -1;
    }
    case DnsHostCacheRustOp::AsInt: {
      return s.value(a).GetInt();
    }
    case DnsHostCacheRustOp::AsBool: {
      return s.value(a).GetBool();
    }
    case DnsHostCacheRustOp::ListSize: {
      return s.value(a).GetList().size();
    }
    case DnsHostCacheRustOp::ListAt: {
      return s.ref(s.value(a).GetList()[b]);
    }
    case DnsHostCacheRustOp::SerializeExpiration: {
      if (a) {
        return s.own(
            base::Value(NetLog::TickCountToString(s.en[0]->expires())));
      }
      auto expiration =
          base::Time::Now() - (base::TimeTicks::Now() - s.en[0]->expires());
      return s.own(
          base::Value(base::NumberToString(expiration.ToInternalValue())));
    }
    case DnsHostCacheRustOp::SerializeTtl: {
      return base::saturated_cast<int>(s.en[0]->ttl().InMilliseconds());
    }
    case DnsHostCacheRustOp::CollectionCount: {
      switch (a) {
        case 0:
          return s.en[0]->ip_endpoints_.size();
        case 1:
          return s.en[0]->endpoint_metadatas_.size();
        case 2:
          return s.en[0]->aliases_.size();
        case 3:
          return s.en[0]->text_records_.size();
        case 4:
        case 5:
          return s.en[0]->hostnames_.size();
        case 6:
          return s.en[0]->canonical_names_.size();
      }
      NOTREACHED();
    }
    case DnsHostCacheRustOp::CollectionValue: {
      switch (a) {
        case 0:
          return s.own(IpEndpointToValue(s.en[0]->ip_endpoints_[b]));
        case 1:
          return s.own(EndpointMetadataPairToValue(s.metadata_at(b)));
        case 2:
          return s.own(base::Value(*std::next(s.en[0]->aliases_.begin(), b)));
        case 3:
          return s.own(base::Value(s.en[0]->text_records_[b]));
        case 4:
          return s.own(base::Value(s.en[0]->hostnames_[b].host()));
        case 5:
          return s.own(base::Value(s.en[0]->hostnames_[b].port()));
        case 6:
          return s.own(base::Value(s.canonical_at(b)));
      }
      NOTREACHED();
    }
    case DnsHostCacheRustOp::IncludeStale: {
      return s.include_stale;
    }
    case DnsHostCacheRustOp::Debug: {
      return s.debug;
    }
    case DnsHostCacheRustOp::SelectRow: {
      s.metadata_rows.clear();
      s.canonical_rows.clear();
      s.en[0] = &s.entry(a);
      s.ke[0] = &s.key(a);
      return 0;
    }
    case DnsHostCacheRustOp::NakToValue: {
      base::Value value;
      if (!s.ke[0]->network_anonymization_key.ToValue(&value)) {
        return -1;
      }
      return s.own(std::move(value));
    }
    case DnsHostCacheRustOp::NakDebug: {
      return s.own(
          base::Value(s.ke[0]->network_anonymization_key.ToDebugString()));
    }
    case DnsHostCacheRustOp::TargetNetwork: {
      return s.ke[0]->target_network;
    }
    case DnsHostCacheRustOp::TargetNetworkValue: {
      return s.own(base::Value(base::NumberToString(s.ke[0]->target_network)));
    }
    case DnsHostCacheRustOp::SchemeHost: {
      return std::holds_alternative<url::SchemeHostPort>(s.ke[0]->host);
    }
    case DnsHostCacheRustOp::KeyValue: {
      switch (a) {
        case 0:
          return s.own(base::Value(
              std::get<url::SchemeHostPort>(s.ke[0]->host).scheme()));
        case 1:
          return s.own(base::Value(GetHostname(s.ke[0]->host)));
        case 2:
          return s.own(
              base::Value(std::get<url::SchemeHostPort>(s.ke[0]->host).port()));
        case 3:
          return s.own(base::Value(static_cast<int>(s.ke[0]->dns_query_type)));
        case 4:
          return s.own(base::Value(s.ke[0]->host_resolver_flags));
        case 5:
          return s.own(
              base::Value(static_cast<int>(s.ke[0]->host_resolver_source)));
        case 6:
          return s.own(base::Value(s.ke[0]->secure));
      }
      NOTREACHED();
    }
    case DnsHostCacheRustOp::OutputClear: {
      s.output->clear();
      return 0;
    }
    case DnsHostCacheRustOp::OutputAppend: {
      s.output->Append(s.value(a).Clone());
      return 0;
    }
    case DnsHostCacheRustOp::InputList: {
      return s.input->size();
    }
    case DnsHostCacheRustOp::InputAt: {
      return s.ref((*s.input)[a]);
    }
    case DnsHostCacheRustOp::ResetRestore: {
      s.ca->restore_size_ = 0;
      return 0;
    }
    case DnsHostCacheRustOp::ValidHostname: {
      return IsValidHostname(s.value(a).GetString());
    }
    case DnsHostCacheRustOp::RestoreHost: {
      url::SchemeHostPort host(s.value(a).GetString(), s.value(b).GetString(),
                               c);
      bool valid = host.IsValid();
      s.restore_host = std::move(host);
      return valid;
    }
    case DnsHostCacheRustOp::RestoreNak: {
      return NetworkAnonymizationKey::FromValue(s.value(a), &s.restore_nak);
    }
    case DnsHostCacheRustOp::RestoreExpiration: {
      return base::StringToInt64(s.value(a).GetString(), &s.restore_raw_time);
    }
    case DnsHostCacheRustOp::ResetRestoredEntry: {
      s.restored = std::make_unique<HostCache::Entry>(
          ERR_FAILED, HostCache::Entry::SOURCE_UNKNOWN);
      return 0;
    }
    case DnsHostCacheRustOp::RestoreEndpoint: {
      auto ep = IpEndpointFromValue(s.value(a));
      if (!ep) {
        return 0;
      }
      s.restored->ip_endpoints_.push_back(std::move(*ep));
      return 1;
    }
    case DnsHostCacheRustOp::RestoreMetadata: {
      auto ep = EndpointMetadataPairFromValue(s.value(a));
      if (!ep) {
        return 0;
      }
      s.restored->endpoint_metadatas_.insert(std::move(*ep));
      return 1;
    }
    case DnsHostCacheRustOp::RestoreString: {
      const auto& str = s.value(b).GetString();
      switch (a) {
        case 0:
          s.restored->aliases_.insert(str);
          break;
        case 1:
          s.restored->text_records_.push_back(str);
          break;
        case 2:
          s.restored->canonical_names_.insert(str);
          break;
      }
      return 0;
    }
    case DnsHostCacheRustOp::RestoreLegacy: {
      IPAddress ip;
      if (!ip.AssignFromIPLiteral(s.value(a).GetString())) {
        return 0;
      }
      s.restored->ip_endpoints_.emplace_back(ip, 0);
      return 1;
    }
    case DnsHostCacheRustOp::RestoreHostRecord: {
      s.restored->hostnames_.emplace_back(s.value(a).GetString(),
                                          static_cast<uint16_t>(b));
      return 0;
    }
    case DnsHostCacheRustOp::RestoredEndpointCount: {
      return s.restored->ip_endpoints_.size();
    }
    case DnsHostCacheRustOp::RestoreFinish: {
      s.restored_key = std::make_unique<HostCache::Key>(
          std::move(s.restore_host), static_cast<DnsQueryType>(a),
          s.restore_flags, static_cast<HostResolverSource>(b), s.restore_nak,
          handles::kInvalidNetworkHandle);
      s.restored_key->secure = c != 0;
      return s.ca->entries_.contains(*s.restored_key);
    }
    case DnsHostCacheRustOp::RestoreFlags: {
      s.restore_flags = a;
      return 0;
    }
    case DnsHostCacheRustOp::RestoreError: {
      s.restore_error = a;
      return 0;
    }
    case DnsHostCacheRustOp::EvictNested: {
      return s.ca->EvictOneEntry(s.now);
    }
    case DnsHostCacheRustOp::TtlOutput: {
      s.optional_ttl =
          a ? std::make_optional(base::Microseconds(b)) : std::nullopt;
      return 0;
    }
    case DnsHostCacheRustOp::EntryPresent: {
      return s.en[0] != nullptr;
    }
    case DnsHostCacheRustOp::NetworkArg: {
      return s.network_arg;
    }
    case DnsHostCacheRustOp::HitArg: {
      return s.hit_arg;
    }
    case DnsHostCacheRustOp::HostnameEmpty: {
      return s.hostname.empty();
    }
    case DnsHostCacheRustOp::HostnameIp: {
      IPAddress ip;
      return ip.AssignFromIPLiteral(s.hostname);
    }
    case DnsHostCacheRustOp::HostnameUrlIp: {
      IPAddress ip;
      return ParseURLHostnameToAddress(s.hostname, &ip);
    }
    case DnsHostCacheRustOp::EndpointIp: {
      return s.ip.AssignFromIPLiteral(s.value(a).GetString());
    }
    case DnsHostCacheRustOp::EndpointScope: {
      s.scope = IPEndPoint::ScopeIdFromInterfaceName(
          s.value(a).GetDict().Find(kInterfaceNameKey));
      return s.scope ? static_cast<int64_t>(*s.scope) : -1;
    }
    case DnsHostCacheRustOp::IpV6LinkLocal: {
      return s.ip.IsIPv6() && s.ip.IsLinkLocal();
    }
    case DnsHostCacheRustOp::StoreEndpoint: {
      s.parsed_endpoint = IPEndPoint(s.ip, static_cast<uint16_t>(a), s.scope);
      return 0;
    }
    case DnsHostCacheRustOp::ParseMetadata: {
      s.parsed_metadata = ConnectionEndpointMetadata::FromValue(s.value(a));
      return s.parsed_metadata.has_value();
    }
    case DnsHostCacheRustOp::StoreMetadataPair: {
      s.parsed_pair = std::make_pair(static_cast<HttpsRecordPriority>(a),
                                     std::move(*s.parsed_metadata));
      return 0;
    }
    case DnsHostCacheRustOp::EndpointAddressValue: {
      return s.own(base::Value(s.endpoint->ToStringWithoutPort()));
    }
    case DnsHostCacheRustOp::EndpointNativePort: {
      return s.endpoint->port();
    }
    case DnsHostCacheRustOp::EndpointHasScope: {
      return s.endpoint->scope_id().has_value();
    }
    case DnsHostCacheRustOp::EndpointInterfaceValue: {
      return s.own(
          IPEndPoint::ScopeIdToInterfaceNameValue(s.endpoint->scope_id()));
    }
    case DnsHostCacheRustOp::MetadataPriority: {
      return s.metadata_pair->first;
    }
    case DnsHostCacheRustOp::MetadataValue: {
      return s.own(s.metadata_pair->second.ToValue());
    }
    case DnsHostCacheRustOp::SnapshotFilterKeys: {
      for (const auto* p : s.rows) {
        s.filter_keys.push_back(p->first);
      }
      return 0;
    }
    case DnsHostCacheRustOp::OwnerAlive: {
      return *s.alive;
    }
    case DnsHostCacheRustOp::RefreshFilterRow: {
      auto it = s.ca->entries_.find(s.filter_keys[a - 2]);
      if (it == s.ca->entries_.end()) {
        return 0;
      }
      s.rows[a - 2] = &*it;
      return 1;
    }
    case DnsHostCacheRustOp::RestoreStringHost: {
      s.restore_host = s.value(a).GetString();
      return 0;
    }
    case DnsHostCacheRustOp::RestoreTime: {
      s.restore_expiration = s.ca->tick_clock_->NowTicks() -
                             (base::Time::Now() - base::Time::FromInternalValue(
                                                      s.restore_raw_time));
      return 0;
    }
    case DnsHostCacheRustOp::RestoreInsert: {
      s.restored->expires_ = s.restore_expiration;
      s.restored->network_changes_ = b;
      s.restored->error_ = s.restore_error;
      s.restored->pinning_ = a != 0;
      s.ca->AddEntry(*s.restored_key, std::move(*s.restored));
      ++s.ca->restore_size_;
      return 0;
    }
  }
  NOTREACHED();
}
namespace {
using RustState = DnsHostCacheRustServices::State;
int64_t RunRust(RustState& s, uint32_t method) {
  DnsHostCacheRustServices service(s);
  return dns_host_cache_rust::execute(service, method);
}
}  // namespace

namespace {
bool IsValidHostname(std::string_view hostname) {
  RustState s;
  s.hostname = hostname;
  return RunRust(s, 30) != 0;
}
std::optional<IPEndPoint> IpEndpointFromValue(const base::Value& value) {
  RustState s;
  s.ref(value);
  RunRust(s, 31);
  return std::move(s.parsed_endpoint);
}
std::optional<std::pair<HttpsRecordPriority, ConnectionEndpointMetadata>>
EndpointMetadataPairFromValue(const base::Value& value) {
  RustState s;
  s.ref(value);
  RunRust(s, 32);
  return std::move(s.parsed_pair);
}
base::Value IpEndpointToValue(const IPEndPoint& endpoint) {
  RustState s;
  s.endpoint = &endpoint;
  return s.value(RunRust(s, 33)).Clone();
}
base::Value EndpointMetadataPairToValue(
    const std::pair<HttpsRecordPriority, ConnectionEndpointMetadata>& pair) {
  RustState s;
  s.metadata_pair = &pair;
  return s.value(RunRust(s, 34)).Clone();
}
}  // namespace

HostCache::Key::Key(std::variant<url::SchemeHostPort, std::string> host,
                    DnsQueryType dns_query_type,
                    HostResolverFlags host_resolver_flags,
                    HostResolverSource host_resolver_source,
                    const NetworkAnonymizationKey& network_anonymization_key,
                    handles::NetworkHandle target_network)
    : host(std::move(host)),
      dns_query_type(dns_query_type),
      host_resolver_flags(host_resolver_flags),
      host_resolver_source(host_resolver_source),
      network_anonymization_key(network_anonymization_key),
      target_network(target_network) {
  DCHECK(IsValidHostname(GetHostname(this->host)));
  if (std::holds_alternative<url::SchemeHostPort>(this->host)) {
    DCHECK(std::get<url::SchemeHostPort>(this->host).IsValid());
  }
}

HostCache::Key::Key() = default;
HostCache::Key::Key(const Key& key) = default;
HostCache::Key::Key(Key&& key) = default;

HostCache::Key::~Key() = default;

HostCache::Entry::Entry(int error,
                        Source source,
                        std::optional<base::TimeDelta> ttl)
    : error_(error), source_(source), ttl_(ttl.value_or(kUnknownTtl)) {
  // If |ttl| has a value, must not be negative.
  DCHECK_GE(ttl.value_or(base::TimeDelta()), base::TimeDelta());
  DCHECK_NE(OK, error_);

  // host_cache.h defines its own `HttpsRecordPriority` due to
  // https_record_rdata.h not being allowed in the same places, but the types
  // should still be the same thing.
  static_assert(std::is_same<net::HttpsRecordPriority,
                             HostCache::Entry::HttpsRecordPriority>::value,
                "`net::HttpsRecordPriority` and "
                "`HostCache::Entry::HttpsRecordPriority` must be same type");
}

HostCache::Entry::Entry(
    const std::set<std::unique_ptr<HostResolverInternalResult>>& results,
    base::Time now,
    base::TimeTicks now_ticks,
    Source empty_source) {
  RustState s;
  s.en[0] = this;
  s.results = &results;
  s.now = now_ticks;
  s.wall = now;
  s.empty_source = empty_source;
  RunRust(s, 19);
}

HostCache::Entry::Entry(const Entry& entry) = default;

HostCache::Entry::Entry(Entry&& entry) = default;

HostCache::Entry::~Entry() = default;

std::vector<HostResolverEndpointResult> HostCache::Entry::GetEndpoints() const {
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  RunRust(s, 15);
  return std::move(s.endpoints);
}

std::vector<ConnectionEndpointMetadata> HostCache::Entry::GetMetadatas() const {
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  RunRust(s, 17);
  return std::move(s.metadatas);
}

std::optional<base::TimeDelta> HostCache::Entry::GetOptionalTtl() const {
  if (has_ttl()) {
    return ttl();
  } else {
    return std::nullopt;
  }
}

// static
HostCache::Entry HostCache::Entry::MergeEntries(Entry front, Entry back) {
  RustState s;
  s.en[0] = &front;
  s.en[1] = &back;
  RunRust(s, 13);
  return front;
}

HostCache::Entry HostCache::Entry::CopyWithDefaultPort(uint16_t port) const {
  Entry copy(*this);
  RustState s;
  s.en[0] = &copy;
  s.port = port;
  RunRust(s, 14);
  return copy;
}

std::vector<ServiceEndpoint> HostCache::Entry::ConvertToServiceEndpoints(
    uint16_t port) const {
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  s.port = port;
  RunRust(s, 16);
  return std::move(s.services);
}

HostCache::Entry& HostCache::Entry::operator=(const Entry& entry) = default;

HostCache::Entry& HostCache::Entry::operator=(Entry&& entry) = default;

HostCache::Entry::Entry(int error,
                        std::vector<IPEndPoint> ip_endpoints,
                        base::flat_set<std::string> aliases,
                        Source source,
                        std::optional<base::TimeDelta> ttl)
    : error_(error),
      ip_endpoints_(std::move(ip_endpoints)),
      aliases_(std::move(aliases)),
      source_(source),
      ttl_(ttl ? ttl.value() : kUnknownTtl) {
  DCHECK(!ttl || ttl.value() >= base::TimeDelta());
}

HostCache::Entry::Entry(const HostCache::Entry& entry,
                        base::TimeTicks now,
                        base::TimeDelta ttl,
                        int network_changes)
    : error_(entry.error()),
      ip_endpoints_(entry.ip_endpoints_),
      endpoint_metadatas_(entry.endpoint_metadatas_),
      aliases_(entry.aliases()),
      text_records_(entry.text_records()),
      hostnames_(entry.hostnames()),
      source_(entry.source()),
      pinning_(entry.pinning()),
      canonical_names_(entry.canonical_names()),
      ttl_(entry.ttl()),
      expires_(now + ttl),
      network_changes_(network_changes) {}

HostCache::Entry::Entry(
    int error,
    std::vector<IPEndPoint> ip_endpoints,
    std::multimap<HttpsRecordPriority, ConnectionEndpointMetadata>
        endpoint_metadatas,
    base::flat_set<std::string> aliases,
    std::vector<std::string>&& text_records,
    std::vector<HostPortPair>&& hostnames,
    Source source,
    base::TimeTicks expires,
    int network_changes)
    : error_(error),
      ip_endpoints_(std::move(ip_endpoints)),
      endpoint_metadatas_(std::move(endpoint_metadatas)),
      aliases_(std::move(aliases)),
      text_records_(std::move(text_records)),
      hostnames_(std::move(hostnames)),
      source_(source),
      expires_(expires),
      network_changes_(network_changes) {}

bool HostCache::Entry::IsStale(base::TimeTicks now, int network_changes) const {
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  s.now = now;
  s.network_arg = network_changes;
  return RunRust(s, 25) != 0;
}

void HostCache::Entry::CountHit(bool hit_is_stale) {
  RustState s;
  s.en[0] = this;
  s.hit_arg = hit_is_stale;
  RunRust(s, 27);
}

void HostCache::Entry::GetStaleness(base::TimeTicks now,
                                    int network_changes,
                                    EntryStaleness* out) const {
  DCHECK(out);
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  s.now = now;
  s.network_arg = network_changes;
  s.stale = out;
  RunRust(s, 26);
}

base::Value HostCache::Entry::NetLogParams() const {
  return base::Value(GetAsValue(false /* include_staleness */));
}

base::DictValue HostCache::Entry::GetAsValue(bool include_staleness) const {
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  s.include_stale = include_staleness;
  return s.value(RunRust(s, 22)).GetDict().Clone();
}

// static
std::optional<base::TimeDelta> HostCache::Entry::TtlFromInternalResults(
    const std::set<std::unique_ptr<HostResolverInternalResult>>& results,
    base::Time now,
    base::TimeTicks now_ticks) {
  RustState s;
  s.results = &results;
  s.now = now_ticks;
  s.wall = now;
  RunRust(s, 18);
  return s.optional_ttl;
}

// static
const HostCache::EntryStaleness HostCache::kNotStale = {base::Seconds(-1), 0,
                                                        0};

HostCache::HostCache(size_t max_entries)
    : max_entries_(max_entries),
      tick_clock_(base::DefaultTickClock::GetInstance()) {}

HostCache::~HostCache() {
  for (auto* frame = rust_callback_frames_.get(); frame;
       frame = frame->next.get()) {
    frame->alive = false;
  }
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  RecordEraseAll(EraseReason::kEraseDestruct, tick_clock_->NowTicks());
}

const std::pair<const HostCache::Key, HostCache::Entry>* HostCache::Lookup(
    const Key& key,
    base::TimeTicks now,
    bool ignore_secure,
    bool record_metrics) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  RustState s;
  s.ca = this;
  s.ke[0] = &key;
  s.now = now;
  s.ignore = ignore_secure;
  s.metrics = record_metrics;
  return s.pair(RunRust(s, 0));
}

const std::pair<const HostCache::Key, HostCache::Entry>* HostCache::LookupStale(
    const Key& key,
    base::TimeTicks now,
    HostCache::EntryStaleness* stale_out,
    bool ignore_secure,
    bool record_metrics) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  RustState s;
  s.ca = this;
  s.ke[0] = &key;
  s.now = now;
  s.ignore = ignore_secure;
  s.metrics = record_metrics;
  s.stale = stale_out;
  return s.pair(RunRust(s, 1));
}

// static
std::pair<const HostCache::Key, HostCache::Entry>*
HostCache::GetLessStaleMoreSecureResult(
    base::TimeTicks now,
    std::pair<const HostCache::Key, HostCache::Entry>* result1,
    std::pair<const HostCache::Key, HostCache::Entry>* result2) {
  if (!result1) {
    return result2;
  }
  if (!result2) {
    return result1;
  }
  RustState s;
  s.en[0] = &result1->second;
  s.ke[0] = &result1->first;
  s.en[1] = &result2->second;
  s.ke[1] = &result2->first;
  s.now = now;
  return RunRust(s, 2) == 0 ? result1 : result2;
}

std::pair<const HostCache::Key, HostCache::Entry>*
HostCache::LookupInternalIgnoringFields(const Key& initial_key,
                                        base::TimeTicks now,
                                        bool ignore_secure) {
  RustState s;
  s.ca = this;
  s.ke[0] = &initial_key;
  s.now = now;
  s.ignore = ignore_secure;
  return s.pair(RunRust(s, 3));
}

std::pair<const HostCache::Key, HostCache::Entry>* HostCache::LookupInternal(
    const Key& key) {
  auto it = entries_.find(key);
  return (it != entries_.end()) ? &*it : nullptr;
}

void HostCache::Set(const Key& key,
                    const Entry& entry,
                    base::TimeTicks now,
                    base::TimeDelta ttl) {
  TRACE_EVENT0(NetTracingCategory(), "HostCache::Set");
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Preserve inputs when they alias a map node being replaced/evicted.
  Key key_snapshot(key);
  Entry entry_snapshot(entry);
  RustState s;
  s.ca = this;
  s.ke[0] = &key_snapshot;
  s.en[0] = &entry_snapshot;
  s.now = now;
  s.ttl = ttl;
  RunRust(s, 4);
}

const HostCache::Key* HostCache::GetMatchingKeyForTesting(
    std::string_view hostname,
    HostCache::Entry::Source* source_out,
    HostCache::EntryStaleness* stale_out) const {
  RustState s;
  s.ca = const_cast<HostCache*>(this);
  s.hostname = hostname;
  s.source = source_out;
  s.stale = stale_out;
  auto id = RunRust(s, 9);
  return id < 0 ? nullptr : &s.key(id);
}

void HostCache::AddEntry(const Key& key, Entry&& entry) {
  DCHECK_EQ(0u, entries_.count(key));
  DCHECK(entry.pinning().has_value());
  entries_.emplace(key, std::move(entry));
}

void HostCache::Invalidate() {
  RustState s;
  s.ca = this;
  RunRust(s, 29);
}

void HostCache::set_persistence_delegate(PersistenceDelegate* delegate) {
  // A PersistenceDelegate shouldn't be added if there already was one, and
  // shouldn't be removed (by setting to nullptr) if it wasn't previously there.
  DCHECK_NE(delegate == nullptr, delegate_ == nullptr);
  delegate_ = delegate;
}

void HostCache::clear() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  RustState s;
  s.ca = this;
  RunRust(s, 6);
}

void HostCache::ClearForHosts(
    const base::RepeatingCallback<bool(const std::string&)>& host_filter) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  RustState s;
  s.ca = this;
  auto filter_snapshot = host_filter;
  s.filter = &filter_snapshot;
  DnsHostCacheRustLifetimeFrame frame;
  frame.next = rust_callback_frames_;
  rust_callback_frames_ = &frame;
  s.alive = &frame.alive;
  RunRust(s, 7);
  if (frame.alive) {
    rust_callback_frames_ = frame.next;
  }
}

void HostCache::GetList(base::ListValue& entry_list,
                        bool include_staleness,
                        SerializationType serialization_type) const {
  RustState s;
  s.ca = const_cast<HostCache*>(this);
  s.output = &entry_list;
  s.include_stale = include_staleness;
  s.debug = serialization_type == SerializationType::kDebug;
  RunRust(s, 23);
}

bool HostCache::RestoreFromListValue(const base::ListValue& old_cache) {
  RustState s;
  s.ca = this;
  s.input = &old_cache;
  return RunRust(s, 24) != 0;
}

size_t HostCache::size() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return entries_.size();
}

size_t HostCache::max_entries() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return max_entries_;
}

bool HostCache::EvictOneEntry(base::TimeTicks now) {
  RustState s;
  s.ca = this;
  s.now = now;
  return RunRust(s, 5) != 0;
}

bool HostCache::HasActivePin(const Entry& entry) {
  RustState s;
  s.ca = this;
  s.en[0] = const_cast<Entry*>(&entry);
  return RunRust(s, 28) != 0;
}

void HostCache::RecordLookup(LookupOutcome outcome,
                             base::TimeTicks now,
                             const Key& key,
                             const Entry* entry) {
  RustState s;
  s.ca = this;
  s.ke[0] = &key;
  s.en[0] = const_cast<Entry*>(entry);
  s.now = now;
  s.outcome = static_cast<int>(outcome);
  RunRust(s, 20);
}

void HostCache::RecordErase(EraseReason reason,
                            base::TimeTicks now,
                            const Key& key,
                            const Entry& entry) {
  RustState s;
  s.ca = this;
  s.ke[0] = &key;
  s.en[0] = const_cast<Entry*>(&entry);
  s.now = now;
  s.outcome = static_cast<int>(reason);
  RunRust(s, 21);
}

void HostCache::RecordEraseAll(EraseReason reason, base::TimeTicks now) {
  RustState s;
  s.ca = this;
  s.now = now;
  s.outcome = static_cast<int>(reason);
  RunRust(s, 8);
}

}  // namespace net

// Debug logging support
std::ostream& operator<<(std::ostream& out,
                         const net::HostCache::EntryStaleness& s) {
  return out << "EntryStaleness{" << s.expired_by << ", " << s.network_changes
             << ", " << s.stale_hits << "}";
}

namespace net {
bool HostCache::Key::operator==(const Key& other) const {
  RustState s;
  s.ke[0] = this;
  s.ke[1] = &other;
  return RunRust(s, 12) == 0;
}
}  // namespace net

namespace net {
bool HostCache::Key::operator<(const Key& other) const {
  RustState s;
  s.ke[0] = this;
  s.ke[1] = &other;
  return RunRust(s, 12) < 0;
}
}  // namespace net

namespace net {
bool HostCache::Entry::operator==(const Entry& other) const {
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  s.en[1] = const_cast<Entry*>(&other);
  return RunRust(s, 11) != 0;
}
}  // namespace net

namespace net {
bool HostCache::Entry::ContentsEqual(const Entry& other) const {
  RustState s;
  s.en[0] = const_cast<Entry*>(this);
  s.en[1] = const_cast<Entry*>(&other);
  return RunRust(s, 10) != 0;
}
}  // namespace net

namespace net {
bool HostCache::EntryStaleness::is_stale() const {
  return dns_host_cache_rust::stale_value(expired_by.InMicroseconds(),
                                          network_changes);
}
bool HostCache::Entry::did_complete() const {
  return dns_host_cache_rust::completed(error_);
}
bool HostCache::Entry::has_ttl() const {
  return dns_host_cache_rust::known_ttl(ttl_.InMicroseconds());
}
}  // namespace net
