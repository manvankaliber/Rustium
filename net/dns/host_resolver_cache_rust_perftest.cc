// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "net/dns/host_resolver_cache_rust_test_support.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"

namespace net::host_resolver_cache_rust_test {
namespace {

constexpr std::array<const char*, 15> kCases = {
    "lookup_hit_64", "lookup_canonical_64", "lookup_miss_64",
    "lookup_security_64", "lookup_stale_64", "replace_64", "insert_evict_64",
    "evict_stale_64", "wildcard_replace_64", "network_partition_64",
    "serialize_64", "logging_64", "restore_64", "restore_full_64", "move_64"};
volatile uint64_t g_checksum = 0;

// Setup and cleanup remain inside Workload and therefore inside timing. The
// fixed preload is amortized over the reported count, never subtracted. Native
// SimpleTest clocks avoid event-vector/hooks in ordinary test Clocks.
template <class Api>
uint64_t Workload(unsigned kind, unsigned count) {
  using Cache = typename Api::Cache;
  Clocks data_times;
  base::SimpleTestClock wall;
  wall.SetNow(data_times.wall.now);
  base::SimpleTestTickClock ticks;
  ticks.SetNowTicks(data_times.ticks.now);
  Cache cache(64, wall, ticks);
  std::array<std::string, 64> names;
  for (unsigned i = 0; i < names.size(); ++i) {
    names.at(i) = "host" + std::to_string(i) + ".test";
    // Security case reserves its64th slot for a second host0 candidate so
    // eviction cannot remove the insecure candidate before the lookup.
    if (kind != 3 || i + 1 < names.size()) {
      Set<Api>(cache, Data<Api>(data_times, names.at(i), std::to_string(i),
                               base::Seconds(30), base::Seconds(30),
                               DnsQueryType::A, true, kind >= 10), (i % 2) != 0);
    }
  }
  if (kind == 3) {
    Set<Api>(cache, Data<Api>(data_times, names.front(), "secure"), true);
  }
  if (kind == 4) {
    cache.MakeAllResultsStale();
  }
  // Build only the inputs a workload uses. Restore consumes an independently
  // constructed implementation-specific Value; C++ never parses Rust output.
  std::optional<base::Value> serialized;
  if (kind == 12 || kind == 13) {
    serialized = cache.Serialize();
  }
  uint64_t checksum = 0;
  for (unsigned iteration = 0; iteration < count; ++iteration) {
    const auto& name = names.at(iteration % names.size());
    switch (kind) {
      case 0:
      case 1:
      case 2:
      case 3: {
        const std::string lookup = kind == 1 ? "HOST0.TEST"
                                  : kind == 2 ? "absent.test"
                                  : kind == 3 ? names.front() : name;
        const auto* result = Lookup(cache, lookup);
        if (result) {
          checksum += result->AsData().strings().front().size() + 1;
        }
        break;
      }
      case 4: {
        auto result = LookupStale(cache, name);
        if (result) {
          checksum += result->stale_by_generation;
          checksum += result->result.get().AsData().strings().front().size();
        }
        break;
      }
      case 5:
        Set<Api>(cache, Data<Api>(data_times, name, "replacement"),
                 (iteration % 2) != 0);
        checksum += cache.size_for_testing();
        break;
      case 6:
        Set<Api>(cache, Data<Api>(data_times,
                                 "new" + std::to_string(iteration) + ".test"));
        checksum += cache.size_for_testing();
        break;
      case 7:
        cache.MakeAllResultsStale();
        Set<Api>(cache, Data<Api>(data_times,
                                 "new" + std::to_string(iteration) + ".test"));
        checksum += cache.size_for_testing();
        break;
      case 8:
        Set<Api>(cache, Data<Api>(data_times, name, "wildcard", base::Seconds(30),
                                 base::Seconds(30), DnsQueryType::UNSPECIFIED),
                 (iteration % 2) != 0);
        Set<Api>(cache, Data<Api>(data_times, name, "specific", base::Seconds(30),
                                 base::Seconds(30), DnsQueryType::AAAA),
                 (iteration % 2) != 0);
        checksum += cache.size_for_testing();
        break;
      case 9:
        Set<Api>(cache, Data<Api>(data_times, name), false,
                 HostResolverSource::DNS, 7);
        checksum += Lookup(cache, name, DnsQueryType::A, false,
                           HostResolverSource::DNS, 7) != nullptr;
        break;
      case 10:
        checksum += cache.Serialize().GetList().size();
        break;
      case 11:
        checksum += cache.SerializeForLogging().GetDict()
                        .FindList("entries")->size();
        break;
      case 12: {
        Cache restored(64, wall, ticks);
        checksum += restored.RestoreFromValue(*serialized);
        checksum += restored.size_for_testing();
        checksum += LookupStale(restored, name).has_value();
        break;
      }
      case 13:
        checksum += cache.RestoreFromValue(*serialized);
        checksum += cache.size_for_testing();
        break;
      case 14: {
        Cache moved(std::move(cache));
        cache = std::move(moved);
        checksum += Lookup(cache, name) != nullptr;
        break;
      }
      default:
        CHECK(false);
    }
  }
  return checksum;
}

void Run(bool timing) {
  std::mt19937 random(20261113);
  constexpr unsigned kIterations = 1000;
  constexpr unsigned kPairs = 20;
  for (unsigned kind = 0; kind < kCases.size(); ++kind) {
    CHECK_EQ(Workload<Production>(kind, 32), Workload<Original>(kind, 32));
    // Count129 crosses the64-entry initial capacity in eviction workloads.
    CHECK_EQ(Workload<Production>(kind, 129), Workload<Original>(kind, 129));
    if (!timing) {
      continue;
    }
    for (unsigned pair = 0; pair < kPairs; ++pair) {
      double cpp = 0;
      double rust = 0;
      const bool rust_first = (random() & 1) != 0;
      auto measure = [&](bool production) {
        const auto start = std::chrono::steady_clock::now();
        const auto checksum = production ? Workload<Production>(kind, kIterations)
                                         : Workload<Original>(kind, kIterations);
        const auto finish = std::chrono::steady_clock::now();
        g_checksum = checksum;
        return std::chrono::duration<double, std::nano>(finish - start).count() /
               kIterations;
      };
      if (rust_first) {
        rust = measure(true);
        cpp = measure(false);
      } else {
        cpp = measure(false);
        rust = measure(true);
      }
      // Format operands are static NUL-terminated case strings and numerics.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,"
          "\"rust_ns\":%.9f,\"iterations\":%u,\"rust_first\":%s}\n",
          kCases.at(kind), pair, cpp, rust, kIterations,
          rust_first ? "true" : "false"));
    }
  }
}

}  // namespace
}  // namespace net::host_resolver_cache_rust_test

int main(int argc, char** argv) {
  if (argc != 2) {
    return 2;
  }
  // argv[1] exists under the checked argc contract.
  const std::string_view mode = UNSAFE_BUFFERS(argv[1]);
  if (mode != "--benchmark" && mode != "--verify-only") {
    return 2;
  }
  net::host_resolver_cache_rust_test::Run(mode == "--benchmark");
  return 0;
}
