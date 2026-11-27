// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Default verification mode performs no elapsed-time reads. --benchmark is
// reserved for the parent's coordinated quiet window and prints paired JSONL.
#include <array>
#include <chrono>
#include <cstdio>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/feature_list.h"
#include "base/memory_coordinator/memory_consumer_registry.h"
#include "base/memory_coordinator/memory_coordinator_features.h"
#include "base/test/simple_test_clock.h"
#include "net/base/features.h"
#include "net/ssl/ssl_client_session_cache.h"
#include "net/ssl/tls_session_cache_rust_original.h"
#include "net/ssl/tls_session_cache_rust_test_support.h"
#include "url/gurl.h"
namespace {
using Rust = net::SSLClientSessionCache;
using Original = net::tls_session_cache_rust_original::SSLClientSessionCache;
constexpr std::array<const char*, 15> kCases = {
    "tls_cache_key_partition",       "tls_cache_lookup_reusable",
    "tls_cache_miss_prune",          "tls_cache_lookup_single_use",
    "tls_cache_lookup_expired",      "tls_cache_replace_reusable",
    "tls_cache_replace_dual_ticket", "tls_cache_lru_eviction",
    "tls_cache_zero_and_stale",      "tls_cache_clear_early_data",
    "tls_cache_flush_generation",    "tls_cache_flush_servers",
    "tls_cache_memory_stateful",     "tls_cache_memory_legacy",
    "tls_cache_destruction_stats"};
class Notifications : public base::MemoryConsumerRegistry {
 public:
  using MemoryConsumerRegistry::NotifyReleaseMemory;
  using MemoryConsumerRegistry::NotifyUpdateMemoryLimit;
};
class Features {
 public:
  explicit Features(bool stateful)
      : old_(base::FeatureList::ClearInstanceForTesting()) {
    auto f = std::make_unique<base::FeatureList>();
    f->RegisterExtraFeatureOverrides(
        {{base::kStatefulMemoryPressure,
          stateful ? base::FeatureList::OVERRIDE_ENABLE_FEATURE
                   : base::FeatureList::OVERRIDE_DISABLE_FEATURE},
         {net::features::kIgnoreMemoryPressureForSslClientSessionCache,
          base::FeatureList::OVERRIDE_DISABLE_FEATURE}});
    base::FeatureList::SetInstance(std::move(f));
  }
  ~Features() {
    base::FeatureList::ClearInstanceForTesting();
    base::FeatureList::RestoreInstanceForTesting(std::move(old_));
  }

 private:
  std::unique_ptr<base::FeatureList> old_;
};
struct Native {
  bssl::UniquePtr<SSL_CTX> ctx{SSL_CTX_new(TLS_method())};
  bssl::UniquePtr<SSL_SESSION> reusable{SSL_SESSION_new(ctx.get())},
      single{SSL_SESSION_new(ctx.get())}, expired{SSL_SESSION_new(ctx.get())};
  bssl::UniquePtr<SSL_SESSION> early{
      net::tls_session_cache_rust_testing::EarlySession()};
  Native() {
    CHECK(early);
    CHECK(SSL_SESSION_early_data_capable(early.get()));
    for (auto* s : {reusable.get(), single.get(), expired.get()}) {
      SSL_SESSION_set_time(s, 1000);
      SSL_SESSION_set_timeout(s, 10000);
    }
    CHECK(SSL_SESSION_set_protocol_version(reusable.get(), TLS1_2_VERSION));
    CHECK(SSL_SESSION_set_protocol_version(single.get(), TLS1_3_VERSION));
    CHECK(SSL_SESSION_set_protocol_version(expired.get(), TLS1_3_VERSION));
    SSL_SESSION_set_time(expired.get(), 900);
    SSL_SESSION_set_timeout(expired.get(), 10);
  }
};
uint64_t Observe(const bssl::UniquePtr<SSL_SESSION>& s) {
  return s ? 1 + SSL_SESSION_get_time(s.get()) +
                 SSL_SESSION_early_data_capable(s.get())
           : 0;
}
template <class C>
struct Work {
  std::vector<typename C::Key> keys;
  base::SimpleTestClock clock;
  raw_ptr<Native> native;
  explicit Work(Native& n) : native(&n) {
    clock.SetNow(base::Time::FromTimeT(1000));
    for (size_t i = 0; i < 64; ++i) {
      typename C::Key k;
      k.server = net::HostPortPair("host" + std::to_string(i % 8), 443 + i % 2);
      k.proxy_chain_index = i / 8;
      if (i % 3 == 1) {
        k.dest_ip_addr = net::IPAddress::IPv4Localhost();
      }
      if (i % 3 == 2) {
        k.dest_ip_addr = net::IPAddress::IPv6Localhost();
      }
      k.network_anonymization_key =
          net::NetworkAnonymizationKey::CreateSameSite(
              net::SchemefulSite(GURL("https://partition.test")));
      keys.push_back(std::move(k));
    }
  }
  uint64_t Run(size_t which, size_t iterations) {
    typename C::Config cfg;
    cfg.max_entries = which == 8 ? 0 : which == 7 ? 16 : 128;
    cfg.expiration_check_count = 256;
    C c(cfg);
    c.SetClockForTesting(&clock);
    uint64_t result = 0;
    auto fill = [&](size_t count) {
      for (size_t j = 0; j < count; ++j) {
        c.Insert(c.generation_number(), keys[j], bssl::UpRef(native->reusable));
      }
    };
    if (which == 1 || which == 2) {
      fill(64);
    }
    for (size_t i = 0; i < iterations; ++i) {
      auto& k = keys[i % 64];
      switch (which) {
        case 0:
          result += (k == keys[(i + 1) % 64]) + 2 * (k < keys[(i + 17) % 64]);
          break;
        case 1:
          result += Observe(c.Lookup(k));
          break;
        case 2: {
          typename C::Key missing;
          missing.server = net::HostPortPair("absent", 443);
          result += Observe(c.Lookup(missing));
          break;
        }
        case 3:
          c.Insert(c.generation_number(), k, bssl::UpRef(native->single));
          result += Observe(c.Lookup(k));
          break;
        case 4:
          c.Insert(c.generation_number(), k, bssl::UpRef(native->expired));
          result += Observe(c.Lookup(k));
          break;
        case 5:
          c.Insert(c.generation_number(), keys[0],
                   bssl::UpRef(native->reusable));
          break;
        case 6:
          c.Insert(c.generation_number(), keys[0], bssl::UpRef(native->single));
          break;
        case 7:
          c.Insert(c.generation_number(), k, bssl::UpRef(native->reusable));
          break;
        case 8:
          c.Insert(i % 2, k, bssl::UpRef(native->reusable));
          break;
        case 9:
          c.Insert(c.generation_number(), k, bssl::UpRef(native->early));
          c.Insert(c.generation_number(), k, bssl::UpRef(native->early));
          c.ClearEarlyData(k);
          result += Observe(c.Lookup(k));
          result += Observe(c.Lookup(k));
          break;
        case 10:
          fill(16);
          c.Flush();
          result += c.generation_number();
          break;
        case 11:
          fill(16);
          c.FlushForServers(
              {keys[0].server, keys[1].server, keys[2].server, keys[3].server});
          result += c.size() + c.generation_number();
          break;
        case 12:
          fill(16);
          Notifications::NotifyUpdateMemoryLimit(
              &c, base::MemoryLimit::FromPercent(5));
          Notifications::NotifyReleaseMemory(&c);
          result += c.size() + c.max_size();
          Notifications::NotifyUpdateMemoryLimit(&c,
                                                 base::MemoryLimit::Default());
          break;
        case 13:
          fill(16);
          Notifications::NotifyUpdateMemoryLimit(
              &c, base::MemoryLimit::ModeratePressureThreshold());
          Notifications::NotifyReleaseMemory(&c);
          Notifications::NotifyUpdateMemoryLimit(
              &c, base::MemoryLimit::CriticalPressureThreshold());
          Notifications::NotifyReleaseMemory(&c);
          result += c.generation_number();
          break;
        case 14: {
          typename C::Config small;
          small.max_entries = 4;
          C local(small);
          local.SetClockForTesting(&clock);
          for (size_t j = 0; j < 8; ++j) {
            local.Insert(0, keys[j], bssl::UpRef(native->single));
          }
          result += local.size() + local.max_size() + local.generation_number();
          break;
        }
      }
      result += c.size() + c.max_size();
    }
    return result;
  }
};
struct Measurement {
  uint64_t checksum;
  double ns;
};
template <class C>
Measurement Measure(Work<C>& w, size_t which, size_t iterations) {
  // This function is reachable only in explicit --benchmark mode.
  auto start = std::chrono::steady_clock::now();
  auto checksum = w.Run(which, iterations);
  auto end = std::chrono::steady_clock::now();
  return {checksum,
          std::chrono::duration<double, std::nano>(end - start).count() /
              iterations};
}
uint64_t Random(uint64_t& s) {
  s ^= s << 13;
  s ^= s >> 7;
  s ^= s << 17;
  return s;
}
}  // namespace
int main(int argc, char** argv) {
  bool timing = false;
  if (argc == 2 && std::string_view(argv[1]) == "--benchmark") {
    timing = true;
  } else if (argc > 1 &&
             (argc != 2 || std::string_view(argv[1]) != "--verify-only")) {
    std::fprintf(stderr, "Use --verify-only (default) or --benchmark\n");
    return 2;
  }
  Native native;
  Work<Rust> rust(native);
  Work<Original> cpp(native);
  uint64_t seed = 20261113;
  for (size_t which = 0; which < kCases.size(); ++which) {
    Features features(which == 12);
    auto now = which == 9 ? SSL_SESSION_get_time(native.early.get()) : 1000;
    rust.clock.SetNow(base::Time::FromTimeT(now));
    cpp.clock.SetNow(base::Time::FromTimeT(now));
    auto r = rust.Run(which, 512), c = cpp.Run(which, 512);
    CHECK_EQ(r, c);
    if (!timing) {
      // SAFETY: case names are static NUL-terminated literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"verified\":true,\"iterations\":512,\"checksum\":%"
          "llu,\"timing\":false}\n",
          kCases[which], static_cast<unsigned long long>(r)));
      continue;
    }
    constexpr size_t iterations = 4096;
    for (size_t pair = 0; pair < 20; ++pair) {
      bool first = Random(seed) & 1;
      Measurement a, b;
      if (first) {
        a = Measure(cpp, which, iterations);
        b = Measure(rust, which, iterations);
      } else {
        b = Measure(rust, which, iterations);
        a = Measure(cpp, which, iterations);
      }
      CHECK_EQ(a.checksum, b.checksum);
      // SAFETY: case names and boolean strings are static NUL-terminated
      // literals; numeric arguments match the fixed format.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"pair\":%zu,\"cpp_ns\":%.9f,\"rust_ns\":%.9f,"
          "\"iterations\":%zu,\"cpp_first\":%s}\n",
          kCases[which], pair, a.ns, b.ns, iterations,
          first ? "true" : "false"));
    }
  }
  return 0;
}
