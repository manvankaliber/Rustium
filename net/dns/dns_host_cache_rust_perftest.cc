// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/time.h"
#include "net/dns/dns_host_cache_rust_reference.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver_internal_result.h"
namespace net::dns_host_cache_rust_test {
template <class Cache>
typename Cache::Key Key(uint64_t x) {
  typename Cache::Key key(
      std::string("host") + std::to_string(x % 12) + ".test",
      static_cast<DnsQueryType>((x >> 5) % 6), static_cast<int>((x >> 8) % 3),
      HostResolverSource::ANY, NetworkAnonymizationKey(),
      handles::kInvalidNetworkHandle);
  key.secure = (x >> 11) & 1;
  return key;
}
template <class Cache>
typename Cache::Entry Entry(uint64_t x) {
  IPAddress ip;
  CHECK(ip.AssignFromIPLiteral((x & 1) ? "192.0.2.3" : "2001:db8::1"));
  typename Cache::Entry e((x % 7) == 0 ? ERR_NAME_NOT_RESOLVED : OK,
                          std::vector<IPEndPoint>{IPEndPoint(
                              ip, static_cast<uint16_t>(x % 3 == 0 ? 0 : 443))},
                          base::flat_set<std::string>{"alias.test"},
                          Cache::Entry::SOURCE_DNS,
                          base::Microseconds(x % 100));
  e.set_canonical_names({"canonical.test"});
  e.set_text_records({std::to_string(x % 37)});
  e.set_hostnames({HostPortPair("host.test", 0)});
  if (x % 3 != 0) {
    e.set_pinning(x % 5 == 0);
  }
  return e;
}
template <class Cache>
base::ListValue Dump(const Cache& c) {
  base::ListValue out;
  c.GetList(out, true, Cache::SerializationType::kDebug);
  return out;
}
void Compare(const HostCache& a, const DnsHostCacheRustReference& b) {
  CHECK_EQ(a.size(), b.size());
  CHECK_EQ(a.network_changes(), b.network_changes());
  CHECK_EQ(a.last_restore_size(), b.last_restore_size());

  auto ai = a.entries().begin();
  auto bi = b.entries().begin();
  for (; ai != a.entries().end(); ++ai, ++bi) {
    CHECK(HostCache::Key::GetTuple(&ai->first) ==
          DnsHostCacheRustReference::Key::GetTuple(&bi->first));
    const auto& ae = ai->second;
    const auto& be = bi->second;
    CHECK_EQ(ae.error(), be.error());
    CHECK_EQ(static_cast<int>(ae.source()), static_cast<int>(be.source()));
    CHECK(ae.pinning() == be.pinning());
    CHECK_EQ(ae.ttl(), be.ttl());
    CHECK_EQ(ae.expires(), be.expires());
    CHECK_EQ(ae.network_changes(), be.network_changes());
    CHECK(ae.ip_endpoints() == be.ip_endpoints());
    CHECK(ae.aliases() == be.aliases());
    CHECK(ae.text_records() == be.text_records());
    CHECK(ae.hostnames() == be.hostnames());
    CHECK(ae.canonical_names() == be.canonical_names());
    CHECK(ai->second.GetMetadatas() == bi->second.GetMetadatas());
    CHECK_EQ(ai->second.did_complete(), bi->second.did_complete());
    CHECK(ai->second.GetEndpoints() == bi->second.GetEndpoints());
    CHECK(ai->second.ConvertToServiceEndpoints(8443) ==
          bi->second.ConvertToServiceEndpoints(8443));
  }
}
void Verify(uint64_t count, uint64_t seed) {
  HostCache a(8);
  DnsHostCacheRustReference b(8);
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::FromInternalValue(100000));
  a.set_tick_clock_for_testing(&clock);
  b.set_tick_clock_for_testing(&clock);
  std::mt19937_64 rng(seed);
  std::array<uint64_t, 12> operations{};
  for (uint64_t i = 0; i < count; ++i) {
    const uint64_t x = rng();
    auto ka = Key<HostCache>(x);
    auto kb = Key<DnsHostCacheRustReference>(x);
    auto ea = Entry<HostCache>(x);
    auto eb = Entry<DnsHostCacheRustReference>(x);
    const auto now = clock.NowTicks();
    int op = (x >> 16) % 12;
    ++operations[op];
    switch (op) {
      case 0:
      case 1:
      case 2:
        a.Set(ka, ea, now, base::Microseconds((x >> 24) % 200));
        b.Set(kb, eb, now, base::Microseconds((x >> 24) % 200));
        break;
      case 3:
      case 4: {
        auto* pa = a.Lookup(ka, now, op == 4);
        auto* pb = b.Lookup(kb, now, op == 4);
        CHECK_EQ(pa != nullptr, pb != nullptr);
        if (pa) {
          CHECK_EQ(pa->first.secure, pb->first.secure);
        }
        break;
      }
      case 5: {
        HostCache::EntryStaleness sa;
        DnsHostCacheRustReference::EntryStaleness sb;
        auto* pa = a.LookupStale(ka, now, &sa, x & 1);
        auto* pb = b.LookupStale(kb, now, &sb, x & 1);
        CHECK_EQ(pa != nullptr, pb != nullptr);
        if (pa) {
          CHECK_EQ(sa.expired_by, sb.expired_by);
          CHECK_EQ(sa.network_changes, sb.network_changes);
          CHECK_EQ(sa.stale_hits, sb.stale_hits);
        }
        break;
      }
      case 6:
        a.Invalidate();
        b.Invalidate();
        break;
      case 7: {
        auto filter = base::BindRepeating([](const std::string& h) {
          return h.find('3') != std::string::npos;
        });
        a.ClearForHosts(filter);
        b.ClearForHosts(filter);
        break;
      }
      case 8:
        if (x % 17 == 0) {
          a.clear();
          b.clear();
        } else {
          clock.Advance(base::Microseconds(x % 100));
        }
        break;
      case 9: {
        auto ma = HostCache::Entry::MergeEntries(ea, Entry<HostCache>(x >> 1));
        auto mb = DnsHostCacheRustReference::Entry::MergeEntries(
            eb, Entry<DnsHostCacheRustReference>(x >> 1));
        auto va = ma.CopyWithDefaultPort(80).NetLogParams();
        auto vb = mb.CopyWithDefaultPort(80).NetLogParams();
        va.GetDict().Remove("expiration");
        vb.GetDict().Remove("expiration");
        CHECK(va == vb);
        break;
      }
      case 10: {
        auto* pa = a.GetMatchingKeyForTesting("host3.test");
        auto* pb = b.GetMatchingKeyForTesting("host3.test");
        CHECK_EQ(pa != nullptr, pb != nullptr);
        break;
      }
      case 11: {
        base::ListValue input;
        auto v = ea.NetLogParams();
        v.GetDict().Set("expiration", "0");
        v.GetDict().Set("hostname", "restore.test");
        v.GetDict().Set("flags", 0);
        v.GetDict().Set("dns_query_type", static_cast<int>((x >> 20) % 10));
        base::Value nak;
        CHECK(NetworkAnonymizationKey().ToValue(&nak));
        v.GetDict().Set("network_anonymization_key", std::move(nak));
        if (x % 2 == 0) {
          v.GetDict().Set("host_ports", base::ListValue());
        }
        input.Append(std::move(v));
        CHECK_EQ(a.RestoreFromListValue(input), b.RestoreFromListValue(input));
        break;
      }
    }
    // Every sequence observes public native state. Expiration restored from
    // wall clocks is normalized only for that case, since real services sample
    // twice.
    if (op == 11) {
      a.ClearForHosts(base::BindRepeating(
          [](const std::string& h) { return h == "restore.test"; }));
      b.ClearForHosts(base::BindRepeating(
          [](const std::string& h) { return h == "restore.test"; }));
    }
    Compare(a, b);
    if ((i % 1024) == 0) {
      CHECK(Dump(a) == Dump(b));
    }
  }
  CHECK(Dump(a) == Dump(b));
  std::printf("stateful native differential inputs=%llu seed=%llu\n",
              static_cast<unsigned long long>(count),
              static_cast<unsigned long long>(seed));
  for (int j = 0; j < 12; ++j) {
    std::printf("operation[%d]=%llu\n", j,
                static_cast<unsigned long long>(operations[j]));
  }
}
}  // namespace net::dns_host_cache_rust_test

namespace {
volatile uint64_t g_checksum = 0;
constexpr std::array kCases = {
    "host_cache_lookup_fresh",         "host_cache_lookup_stale",
    "host_cache_lookup_secure",        "host_cache_replace",
    "host_cache_evict_pins",           "host_cache_invalidate",
    "host_cache_clear_hosts",          "host_cache_serialize_debug",
    "host_cache_serialize_restorable", "host_cache_restore_valid",
    "host_cache_restore_invalid",      "host_cache_merge",
    "host_cache_default_port",         "host_cache_endpoints",
    "host_cache_service_endpoints",    "host_cache_internal_result_ttl",
    "host_cache_key_entry_compare"};
template <class Cache>
uint64_t Workload(unsigned kind, unsigned iterations) {
  using namespace net;
  using namespace net::dns_host_cache_rust_test;
  uint64_t sum = 0;
  Cache cache(16);
  auto now = base::TimeTicks::FromInternalValue(100000);
  auto key = Key<Cache>(1);
  auto entry = Entry<Cache>(1);
  auto second = Entry<Cache>(2);
  std::multimap<uint16_t, ConnectionEndpointMetadata> metadata;
  ConnectionEndpointMetadata m;
  m.target_name = "canonical.test";
  m.supported_protocol_alpns = {"h2", "h3"};
  metadata.emplace(1, m);
  auto alternative =
      typename Cache::Entry(OK, metadata, Cache::Entry::SOURCE_DNS);
  entry = Cache::Entry::MergeEntries(std::move(entry), std::move(alternative));
  cache.Set(key, entry, now, base::Seconds(1));
  auto secure = key;
  secure.secure = true;
  cache.Set(secure, entry, now, base::Seconds(2));
  base::DictValue restore;
  restore.Set("hostname", "restore.test");
  restore.Set("flags", 0);
  restore.Set("dns_query_type", 1);
  restore.Set("expiration", "0");
  restore.Set("net_error", ERR_NAME_NOT_RESOLVED);
  base::Value nak;
  CHECK(NetworkAnonymizationKey().ToValue(&nak));
  restore.Set("network_anonymization_key", std::move(nak));
  base::ListValue list;
  list.Append(std::move(restore));
  if (kind == 10) {
    list.Append(42);
  }
  std::set<std::unique_ptr<HostResolverInternalResult>> results;
  results.insert(std::make_unique<HostResolverInternalDataResult>(
      "canonical.test", DnsQueryType::A, now + base::Seconds(3),
      base::Time() + base::Seconds(3), HostResolverInternalResult::Source::kDns,
      entry.ip_endpoints(), std::vector<std::string>{},
      std::vector<HostPortPair>{}));
  auto filter = base::BindRepeating(
      [](const std::string& host) { return host == "host1.test"; });
  for (unsigned i = 0; i < iterations; ++i) {
    switch (kind) {
      case 0:
        sum += cache.Lookup(key, now, false, false) != nullptr;
        break;
      case 1: {
        typename Cache::EntryStaleness stale;
        sum += cache.LookupStale(key, now + base::Seconds(3), &stale, false,
                                 false) != nullptr;
        sum += stale.stale_hits;
        break;
      }
      case 2:
        sum += cache.Lookup(key, now, true, false) != nullptr;
        break;
      case 3:
        cache.Set(key, (i & 1) ? entry : second, now, base::Seconds(1));
        sum += cache.size();
        break;
      case 4: {
        auto k = Key<Cache>(i + 4);
        auto e = entry;
        e.set_pinning(i % 7 == 0);
        cache.Set(k, e, now, base::Microseconds(i % 100));
        if (i % 31 == 0) {
          cache.Invalidate();
        }
        sum += cache.size();
        break;
      }
      case 5:
        cache.Invalidate();
        sum += cache.network_changes();
        break;
      case 6:
        cache.Set(key, entry, now, base::Seconds(1));
        cache.ClearForHosts(filter);
        sum += cache.size();
        break;
      case 7:
      case 8: {
        base::ListValue out;
        cache.GetList(out, true,
                      kind == 7 ? Cache::SerializationType::kDebug
                                : Cache::SerializationType::kRestorable);
        sum += out.size();
        break;
      }
      case 9:
      case 10: {
        Cache restored(16);
        sum += restored.RestoreFromListValue(list);
        sum += restored.last_restore_size();
        break;
      }
      case 11: {
        auto e = Cache::Entry::MergeEntries(entry, second);
        sum += e.ip_endpoints().size();
        break;
      }
      case 12: {
        auto e = entry.CopyWithDefaultPort(8443);
        sum += e.ip_endpoints()[0].port();
        break;
      }
      case 13:
        sum += entry.GetEndpoints().size();
        break;
      case 14:
        sum += entry.ConvertToServiceEndpoints(8443).size();
        break;
      case 15: {
        typename Cache::Entry e(results, base::Time(), now);
        sum += e.ip_endpoints().size();
        sum += Cache::Entry::TtlFromInternalResults(results, base::Time(), now)
                   ->InMicroseconds();
        break;
      }
      case 16:
        sum += key < secure;
        sum += key == secure;
        sum += entry == second;
        sum += entry.ContentsEqual(second);
        break;
      default:
        NOTREACHED();
    }
  }
  return sum;
}
void Benchmarks(bool timing) {
  std::mt19937 rng(20261113);
  constexpr unsigned iterations = 2000;
  for (unsigned kind = 0; kind < std::size(kCases); ++kind) {
    CHECK_EQ(Workload<net::HostCache>(kind, 20),
             Workload<net::DnsHostCacheRustReference>(kind, 20));
    if (!timing) {
      continue;
    }
    for (unsigned pair = 0; pair < 20; ++pair) {
      double cpp = 0, rust = 0;
      bool first = (rng() & 1) != 0;
      auto measure = [&](bool use_rust) {
        auto start = std::chrono::steady_clock::now();
        auto checksum = use_rust ? Workload<net::HostCache>(kind, iterations)
                                 : Workload<net::DnsHostCacheRustReference>(
                                       kind, iterations);
        auto end = std::chrono::steady_clock::now();
        g_checksum = checksum;
        return std::chrono::duration<double, std::nano>(end - start).count() /
               iterations;
      };
      if (first) {
        rust = measure(true);
        cpp = measure(false);
      } else {
        cpp = measure(false);
        rust = measure(true);
      }
      // SAFETY: case names and boolean strings are static NUL-terminated
      // literals; numeric arguments match the fixed format.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%u,\"rust_first\":%s}\n",
          kCases[kind], pair, cpp, rust, iterations, first ? "true" : "false"));
    }
  }
}
}  // namespace
int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--benchmark") {
    Benchmarks(true);
    return 0;
  }
  if (argc == 2 && std::string_view(argv[1]) == "--verify-only") {
    Benchmarks(false);
    return 0;
  }
  uint64_t count = 1000000;
  if (argc == 3 && std::string_view(argv[1]) == "--verify") {
    CHECK(base::StringToUint64(argv[2], &count));
  }
  Benchmarks(false);
  net::dns_host_cache_rust_test::Verify(count, 0xd15ca5e3);
  return 0;
}
