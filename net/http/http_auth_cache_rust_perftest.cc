// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Parent-owned quiet-window execution only. No timing during agent validation.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <type_traits>
#include <vector>

#include "base/compiler_specific.h"
#include "net/http/http_auth_cache_rust_test_support.h"

namespace {
struct Case {
  const char* key;
  int workload;
  size_t iterations;
};
volatile uint64_t sink = 0;
struct Inputs {
  url::SchemeHostPort origin{GURL("https://auth.test:8443")};
  net::NetworkAnonymizationKey nak =
      net::NetworkAnonymizationKey::CreateSameSite(
          net::SchemefulSite(GURL("https://partition.test")));
  net::AuthCredentials credentials{std::u16string{u'u', 0, 0xd800},
                                   std::u16string{0xdc00, u'p'}};
  std::vector<std::string> realms, paths;
  Inputs() {
    for (int i = 0; i < 32; ++i) {
      realms.push_back("realm" + std::to_string(i));
      paths.push_back("/protected/" + std::to_string(i) + "/document");
    }
  }
};

template <class Cache>
uint64_t Run(const Case& c, const Inputs& in, size_t n) {
  uint64_t result = 0;
  for (size_t k = 0; k < n; ++k) {
    // Full cache lifetimes, stable node allocation, credentials, all clock
    // service calls, API dispatch/FFI and final destruction are inside timing.
    Cache cache(true), source(false);
    net::HttpAuthCacheRustTick ticks;
    net::HttpAuthCacheRustClock clock;
    cache.set_tick_clock_for_testing(&ticks);
    cache.set_clock_for_testing(&clock);
    source.set_tick_clock_for_testing(&ticks);
    source.set_clock_for_testing(&clock);
    const auto target = c.workload == 4 ? net::HttpAuth::AUTH_PROXY
                                        : net::HttpAuth::AUTH_SERVER;
    const int entries = c.workload == 3 ? 32 : 20;
    for (int i = 0; i < entries; ++i) {
      auto* e = cache.Add(in.origin, target, in.realms[i],
                          net::HttpAuth::AUTH_SCHEME_BASIC, in.nak,
                          "Basic realm=bytes", in.credentials, in.paths[i]);
      result += e->realm().size();
    }
    switch (c.workload) {
      case 0:
      case 1:
        for (int i = 0; i < 32; ++i) {
          auto* e = cache.Lookup(in.origin, target,
                                 in.realms[c.workload == 0 ? i % 20 : 31],
                                 net::HttpAuth::AUTH_SCHEME_BASIC, in.nak);
          result += e ? e->credentials().username().size() : 1;
        }
        break;
      case 2:
        for (int i = 0; i < 32; ++i) {
          auto* e =
              cache.LookupByPath(in.origin, target, in.nak, in.paths[i % 20]);
          result += e ? e->auth_challenge().size() : 1;
        }
        break;
      case 3:
        for (int i = 0; i < 32; ++i) {
          auto* e = cache.Add(in.origin, target, in.realms[i],
                              net::HttpAuth::AUTH_SCHEME_BASIC, in.nak,
                              "Digest stale", in.credentials, in.paths[i]);
          result += e->IncrementNonceCount();
        }
        break;
      case 4:
        source.CopyProxyEntriesFrom(cache);
        source.CopyProxyEntriesFrom(source);
        for (int i = 0; i < 20; ++i) {
          auto* e = source.Lookup(in.origin, target, in.realms[i],
                                  net::HttpAuth::AUTH_SCHEME_BASIC, {});
          result += e->IncrementNonceCount();
        }
        break;
      case 5: {
        auto filter = base::BindRepeating(
            [](const GURL& url) { return url.SchemeIs("https"); });
        result += cache.ClearEntriesAddedBetween(
            base::Time::FromInternalValue(1004),
            base::Time::FromInternalValue(1016), filter);
        result += cache.GetEntriesSizeForTesting();
        break;
      }
      case 6:
        cache.SetKeyServerEntriesByNetworkAnonymizationKey(false);
        result += cache.ClearAllEntries();
        break;
      case 8:
        result += cache.ClearAllEntries();
        result += cache.ClearAllEntries();
        break;
      case 7:
        for (int i = 0; i < 20; ++i) {
          auto* e = cache.Lookup(in.origin, target, in.realms[i],
                                 net::HttpAuth::AUTH_SCHEME_BASIC, in.nak);
          cache.Add(e->scheme_host_port(), target, e->realm(), e->scheme(),
                    in.nak, e->auth_challenge(), e->credentials(),
                    in.paths[(i + 1) % 20]);
          result += cache.UpdateStaleChallenge(in.origin, target, in.realms[i],
                                               net::HttpAuth::AUTH_SCHEME_BASIC,
                                               in.nak, e->auth_challenge());
          result += cache.Remove(in.origin, target, in.realms[i],
                                 net::HttpAuth::AUTH_SCHEME_BASIC, in.nak,
                                 in.credentials);
        }
        break;
    }
    result += cache.GetEntriesSizeForTesting() + ticks.calls + clock.calls;
  }
  return result;
}
template <class Cache>
double Measure(const Case& c, const Inputs& in) {
  const auto start = std::chrono::steady_clock::now();
  sink = Run<Cache>(c, in, c.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         c.iterations;
}
}  // namespace
int main(int argc, char** argv) {
  const Inputs inputs;
  const Case cases[] = {
      {"auth_cache_realm_hit", 0, 2000},  {"auth_cache_realm_miss", 1, 2000},
      {"auth_cache_path", 2, 2000},       {"auth_cache_eviction", 3, 2000},
      {"auth_cache_proxy_copy", 4, 2000}, {"auth_cache_time_clear", 5, 2000},
      {"auth_cache_partition", 6, 2000},  {"auth_cache_alias_replace", 7, 2000},
      {"auth_cache_clear_all", 8, 2000}};
  // Validation mode deliberately performs NO time reads or measurements.
  const bool validate =
      argc > 1 && std::string(UNSAFE_BUFFERS(argv[1])) == "--validate-only";
  std::mt19937 random(20261113);
  for (const auto& c : cases) {
    CHECK_EQ(Run<net::HttpAuthCache>(c, inputs, 16),
             Run<net::HttpAuthCacheOriginal>(c, inputs, 16));
    if (validate) {
      continue;
    }
    for (unsigned pair = 0; pair < 20; ++pair) {
      const bool first = (random() & 1) != 0;
      double cpp, rust;
      if (first) {
        rust = Measure<net::HttpAuthCache>(c, inputs);
        cpp = Measure<net::HttpAuthCacheOriginal>(c, inputs);
      } else {
        cpp = Measure<net::HttpAuthCacheOriginal>(c, inputs);
        rust = Measure<net::HttpAuthCache>(c, inputs);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"iterations\":%zu,\"repeat\":%u,\"rust_first\":%s,"
          "\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          c.key, c.iterations, pair, first ? "true" : "false", cpp, rust));
      std::fflush(stdout);
    }
  }
}
