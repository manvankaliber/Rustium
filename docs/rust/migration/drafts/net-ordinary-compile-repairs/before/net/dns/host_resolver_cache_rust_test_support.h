// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#ifndef NET_DNS_HOST_RESOLVER_CACHE_RUST_TEST_SUPPORT_H_
#define NET_DNS_HOST_RESOLVER_CACHE_RUST_TEST_SUPPORT_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/time/clock.h"
#include "base/time/tick_clock.h"
#include "base/values.h"
#include "net/base/host_port_pair.h"
#include "net/base/host_port_rust_reference.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/dns/host_resolver_cache.h"
#include "net/dns/host_resolver_cache_result_reference.h"
#include "net/dns/host_resolver_cache_rust_reference.h"
#include "net/dns/host_resolver_internal_result.h"

namespace net::host_resolver_cache_rust_test {

struct Production {
  using Cache = ::net::HostResolverCache;
  using Result = ::net::HostResolverInternalResult;
  using Data = ::net::HostResolverInternalDataResult;
  using Error = ::net::HostResolverInternalErrorResult;
  using Alias = ::net::HostResolverInternalAliasResult;
  using Metadata = ::net::HostResolverInternalMetadataResult;
  using Host = ::net::HostPortPair;
};
struct Original {
  using Cache = host_resolver_cache_reference::HostResolverCache;
  using Result = host_resolver_cache_reference::HostResolverInternalResult;
  using Data = host_resolver_cache_reference::HostResolverInternalDataResult;
  using Error = host_resolver_cache_reference::HostResolverInternalErrorResult;
  using Alias = host_resolver_cache_reference::HostResolverInternalAliasResult;
  using Metadata = host_resolver_cache_reference::HostResolverInternalMetadataResult;
  using Host = ::net::HostPortRustReference;
};

// Ordinary deterministic clock providers. Hooks model defined insertion and
// generation changes. They do not erase iterators or destroy the owner.
struct Clocks {
  struct Wall final : base::Clock {
    base::Time Now() const override {
      events.push_back('w');
      std::function<void()> next = std::exchange(hook, {});
      if (next) {
        next();
      }
      return now;
    }
    base::Time now = base::Time::FromSecondsSinceUnixEpoch(1000);
    mutable std::vector<char> events;
    mutable std::function<void()> hook;
  };
  struct Ticks final : base::TickClock {
    base::TimeTicks NowTicks() const override {
      if (wall) {
        wall->events.push_back('t');
      }
      std::function<void()> next = std::exchange(hook, {});
      if (next) {
        next();
      }
      return now;
    }
    base::TimeTicks now = base::TimeTicks() + base::Seconds(100);
    Wall* wall = nullptr;
    mutable std::function<void()> hook;
  };
  Clocks() { ticks.wall = &wall; }
  Clocks(const Clocks&) = delete;
  Clocks& operator=(const Clocks&) = delete;
  Wall wall;
  Ticks ticks;
};

template <class Api>
std::unique_ptr<typename Api::Result> Data(
    const Clocks& clocks,
    std::string name = "host.test",
    std::string id = "first",
    base::TimeDelta tick_ttl = base::Seconds(30),
    base::TimeDelta wall_ttl = base::Seconds(30),
    DnsQueryType query = DnsQueryType::A,
    bool with_ticks = true,
    bool with_host = false) {
  using Result = typename Api::Result;
  std::vector<typename Api::Host> hosts;
  if (with_host) {
    hosts.emplace_back("srv-target.test", 443);
  }
  return std::make_unique<typename Api::Data>(
      std::move(name), query,
      with_ticks ? std::optional(clocks.ticks.now + tick_ttl) : std::nullopt,
      clocks.wall.now + wall_ttl, Result::Source::kDns,
      std::vector<IPEndPoint>{IPEndPoint(IPAddress(192, 0, 2, 1), 443)},
      std::vector<std::string>{std::move(id)}, std::move(hosts));
}

template <class Api>
void Set(typename Api::Cache& cache,
         std::unique_ptr<typename Api::Result> result,
         bool secure = false,
         HostResolverSource source = HostResolverSource::DNS,
         handles::NetworkHandle network = handles::kInvalidNetworkHandle,
         const NetworkAnonymizationKey& nak = NetworkAnonymizationKey()) {
  cache.Set(std::move(result), nak, network, source, secure);
}

template <class Cache>
const auto* Lookup(const Cache& cache,
                   std::string_view name = "host.test",
                   DnsQueryType type = DnsQueryType::UNSPECIFIED,
                   std::optional<bool> secure = std::nullopt,
                   HostResolverSource source = HostResolverSource::ANY,
                   handles::NetworkHandle network = handles::kInvalidNetworkHandle,
                   const NetworkAnonymizationKey& nak = NetworkAnonymizationKey()) {
  return cache.Lookup(name, nak, network, type, source, secure);
}

template <class Cache>
auto LookupStale(const Cache& cache,
                 std::string_view name = "host.test",
                 DnsQueryType type = DnsQueryType::UNSPECIFIED,
                 std::optional<bool> secure = std::nullopt) {
  return cache.LookupStale(name, NetworkAnonymizationKey(),
                           handles::kInvalidNetworkHandle, type,
                           HostResolverSource::ANY, secure);
}

template <class Result>
std::string Id(const Result* result) {
  if (!result || result->type() != Result::Type::kData) {
    return "";
  }
  return result->AsData().strings().front();
}

template <class Cache>
base::Value Observe(const Cache& cache, std::string_view name = "host.test") {
  base::DictValue out;
  out.Set("size", static_cast<int>(cache.size_for_testing()));
  const auto* active = Lookup(cache, name);
  out.Set("active", active ? active->ToValue() : base::Value());
  auto stale = LookupStale(cache, name);
  if (stale) {
    out.Set("stale_result", stale->result.get().ToValue());
    out.Set("stale_generation", stale->stale_by_generation);
    out.Set("expired", stale->expired_by.has_value());
    if (stale->expired_by) {
      out.Set("expired_us", std::to_string(stale->expired_by->InMicroseconds()));
    }
  }
  out.Set("persist", cache.Serialize());
  out.Set("log", cache.SerializeForLogging());
  return base::Value(std::move(out));
}

}  // namespace net::host_resolver_cache_rust_test
#endif
