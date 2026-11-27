// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <functional>
#include <thread>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/memory_coordinator/memory_consumer_registry.h"
#include "base/memory_coordinator/memory_coordinator_features.h"
#include "base/memory_coordinator/test_memory_consumer_registry.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/test/simple_test_clock.h"
#include "net/base/features.h"
#include "net/ssl/ssl_client_session_cache.h"
#include "net/ssl/tls_session_cache_rust_abi.h"
#include "net/ssl/tls_session_cache_rust_original.h"
#include "net/ssl/tls_session_cache_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/boringssl/src/include/openssl/bio.h"
#include "third_party/boringssl/src/include/openssl/mem.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"
#include "url/gurl.h"

namespace net {
namespace {
using Original = tls_session_cache_rust_original::SSLClientSessionCache;
class FeatureReset {
 public:
  FeatureReset() : old_(base::FeatureList::ClearInstanceForTesting()) {}
  ~FeatureReset() {
    base::FeatureList::ClearInstanceForTesting();
    base::FeatureList::RestoreInstanceForTesting(std::move(old_));
  }

 private:
  std::unique_ptr<base::FeatureList> old_;
};
class Notifications : public base::MemoryConsumerRegistry {
 public:
  using MemoryConsumerRegistry::NotifyReleaseMemory;
  using MemoryConsumerRegistry::NotifyUpdateMemoryLimit;
};
Original::Key OriginalKey(const SSLClientSessionCache::Key& k) {
  Original::Key o;
  o.server = k.server;
  o.dest_ip_addr = k.dest_ip_addr;
  o.network_anonymization_key = k.network_anonymization_key;
  o.privacy_mode = k.privacy_mode;
  o.session_usage = k.session_usage;
  o.proxy_chain = k.proxy_chain;
  o.proxy_chain_index = k.proxy_chain_index;
  return o;
}
bssl::UniquePtr<SSL_SESSION> Session(SSL_CTX* ctx,
                                     uint16_t version,
                                     int64_t time = 1000,
                                     uint32_t timeout = 100) {
  bssl::UniquePtr<SSL_SESSION> s(SSL_SESSION_new(ctx));
  EXPECT_TRUE(SSL_SESSION_set_protocol_version(s.get(), version));
  SSL_SESSION_set_time(s.get(), time);
  SSL_SESSION_set_timeout(s.get(), timeout);
  return s;
}
uint64_t Next(uint64_t& x) {
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return x;
}
struct Fixture {
  bssl::UniquePtr<SSL_CTX> ctx{SSL_CTX_new(TLS_method())};
  base::SimpleTestClock clock;
  Fixture() {
    SSL_CTX_set_current_time_cb(
        ctx.get(), tls_session_cache_rust_testing::FixedCryptoTime);
    clock.SetNow(base::Time::FromTimeT(1000));
  }
};
TEST(TlsSessionCacheRust, DifferentialMillionOperations) {
  Fixture f;
  uint64_t seed = 0x123d85db15;
  size_t operations = 0, comparisons = 0;
  std::array<size_t, 10> counts{};
  for (size_t batch = 0; batch < 1000; ++batch) {
    SSLClientSessionCache::Config config;
    config.max_entries = batch % 33;
    config.expiration_check_count = batch % 19;
    SSLClientSessionCache a(config);
    Original::Config oc;
    oc.max_entries = config.max_entries;
    oc.expiration_check_count = config.expiration_check_count;
    Original b(oc);
    a.SetClockForTesting(&f.clock);
    b.SetClockForTesting(&f.clock);
    std::vector<SSLClientSessionCache::Key> keys(24);
    std::vector<bssl::UniquePtr<SSL_SESSION>> sessions;
    for (size_t i = 0; i < keys.size(); ++i) {
      auto& k = keys[i];
      k.server = HostPortPair("host" + std::to_string(i % 6), 443 + i % 3);
      if (i % 3 == 1) {
        k.dest_ip_addr = IPAddress::IPv4Localhost();
      }
      if (i % 3 == 2) {
        k.dest_ip_addr = IPAddress::IPv6Localhost();
      }
      if (i % 4) {
        k.network_anonymization_key = NetworkAnonymizationKey::CreateFromParts(
            SchemefulSite(GURL(i % 2 ? "https://a.test" : "https://b.test")),
            i % 3 == 0);
      }
      k.privacy_mode = static_cast<PrivacyMode>(i % 4);
      k.session_usage =
          i % 2 ? SessionUsage::kProxy : SessionUsage::kDestination;
      if (i % 5 == 1) {
        k.proxy_chain = ProxyChain();
      }
      if (i % 5 == 2) {
        k.proxy_chain = ProxyChain::FromSchemeHostAndPort(
            ProxyServer::SCHEME_HTTPS, "proxy", 443);
      }
      if (i % 5 == 3) {
        k.proxy_chain = ProxyChain::WithOpaqueData({}, static_cast<int>(i));
      }
      k.proxy_chain_index = i % 2;
      sessions.push_back(Session(f.ctx.get(),
                                 i % 2 ? TLS1_3_VERSION : TLS1_2_VERSION,
                                 1000 + i % 7, 50 + i % 11));
    }
    for (size_t i = 0; i < keys.size(); ++i) {
      for (size_t j = 0; j < keys.size(); ++j) {
        auto x = OriginalKey(keys[i]), y = OriginalKey(keys[j]);
        ASSERT_EQ(keys[i] < keys[j], x < y);
        ASSERT_EQ(keys[i] == keys[j], x == y);
        ++comparisons;
      }
    }
    uint64_t old = 0;
    for (size_t step = 0; step < 1000; ++step) {
      uint64_t bits = Next(seed);
      size_t idx = (bits >> 8) % keys.size();
      auto& k = keys[idx];
      auto o = OriginalKey(k);
      ++counts[bits % 10];
      switch (bits % 10) {
        case 0:
        case 1:
        case 2: {
          auto* s = sessions[(bits >> 16) % sessions.size()].get();
          uint64_t g = (bits & 128) ? a.generation_number() : old;
          a.Insert(g, k, bssl::UpRef(s));
          b.Insert(g, o, bssl::UpRef(s));
          break;
        }
        case 3:
        case 4: {
          auto x = a.Lookup(k), y = b.Lookup(o);
          ASSERT_EQ(x.get(), y.get());
          break;
        }
        case 5:
          a.ClearEarlyData(k);
          b.ClearEarlyData(o);
          break;
        case 6:
          old = a.generation_number();
          a.FlushForServers({k.server});
          b.FlushForServers({k.server});
          break;
        case 7:
          old = a.generation_number();
          a.Flush();
          b.Flush();
          break;
        case 8:
          f.clock.SetNow(base::Time::FromTimeT(
              static_cast<int64_t>((bits >> 24) % 1200) - 50));
          break;
        case 9: {
          auto* s = sessions[idx].get();
          auto now = f.clock.Now().ToTimeT();
          ASSERT_EQ(SSLClientSessionCache::IsExpired(s, now),
                    Original::IsExpired(s, now));
          break;
        }
      }
      ASSERT_EQ(a.size(), b.size());
      ASSERT_EQ(a.max_size(), b.max_size());
      ASSERT_EQ(a.generation_number(), b.generation_number());
      ++operations;
    }
  }
  for (size_t i = 0; i < counts.size(); ++i) {
    RecordProperty("operation_bucket_" + std::to_string(i),
                   std::to_string(counts[i]));
  }
  EXPECT_EQ(operations, 1000000u);
  EXPECT_EQ(comparisons, 576000u);
}
TEST(TlsSessionCacheRust, MemoryPolicyDifferential) {
  for (bool stateful : {false, true}) {
    for (bool ignore : {false, true}) {
      FeatureReset reset;
      auto flags = std::make_unique<base::FeatureList>();
      flags->RegisterExtraFeatureOverrides(
          {{base::kStatefulMemoryPressure,
            stateful ? base::FeatureList::OVERRIDE_ENABLE_FEATURE
                     : base::FeatureList::OVERRIDE_DISABLE_FEATURE},
           {features::kIgnoreMemoryPressureForSslClientSessionCache,
            ignore ? base::FeatureList::OVERRIDE_ENABLE_FEATURE
                   : base::FeatureList::OVERRIDE_DISABLE_FEATURE}});
      base::FeatureList::SetInstance(std::move(flags));
      Fixture f;
      SSLClientSessionCache::Config c;
      c.max_entries = 10;
      SSLClientSessionCache a(c);
      Original::Config oc;
      oc.max_entries = 10;
      Original b(oc);
      a.SetClockForTesting(&f.clock);
      b.SetClockForTesting(&f.clock);
      for (int percent : {100, 50, 0, 200, 75, 0, 100}) {
        for (size_t i = 0; i < 15; ++i) {
          SSLClientSessionCache::Key k;
          k.server = HostPortPair(std::to_string(i), 443);
          auto s = Session(f.ctx.get(), TLS1_2_VERSION);
          a.Insert(a.generation_number(), k, bssl::UpRef(s));
          b.Insert(b.generation_number(), OriginalKey(k), bssl::UpRef(s));
        }
        auto size = a.size();
        Notifications::NotifyUpdateMemoryLimit(
            &a, base::MemoryLimit::FromPercent(percent));
        Notifications::NotifyUpdateMemoryLimit(
            &b, base::MemoryLimit::FromPercent(percent));
        EXPECT_EQ(a.size(), size);
        EXPECT_EQ(a.max_size(), b.max_size());
        Notifications::NotifyReleaseMemory(&a);
        Notifications::NotifyReleaseMemory(&b);
        ASSERT_EQ(a.size(), b.size());
        ASSERT_EQ(a.max_size(), b.max_size());
        ASSERT_EQ(a.generation_number(), b.generation_number());
        for (size_t i = 0; i < 15; ++i) {
          SSLClientSessionCache::Key k;
          k.server = HostPortPair(std::to_string(i), 443);
          EXPECT_EQ(a.Lookup(k).get(), b.Lookup(OriginalKey(k)).get());
        }
      }
    }
  }
}
TEST(TlsSessionCacheRust, ExpirationUnsignedEdges) {
  Fixture f;
  auto s = Session(f.ctx.get(), TLS1_2_VERSION);
  for (int64_t start : {INT64_MIN, int64_t{-1}, int64_t{0}, int64_t{1},
                        int64_t{2}, INT64_MAX}) {
    for (uint32_t timeout : {0u, 1u, 2u, UINT32_MAX}) {
      SSL_SESSION_set_time(s.get(), start);
      SSL_SESSION_set_timeout(s.get(), timeout);
      for (int64_t now : {INT64_MIN, int64_t{-1}, int64_t{0}, int64_t{1},
                          int64_t{2}, int64_t{1000}, INT64_MAX}) {
        EXPECT_EQ(SSLClientSessionCache::IsExpired(s.get(), now),
                  Original::IsExpired(s.get(), now));
      }
    }
  }
  EXPECT_TRUE(SSLClientSessionCache::IsExpired(nullptr, 0));
}
}  // namespace
}  // namespace net
// Callback fixtures use real BoringSSL refcounts and ex-data finalizers.
namespace net {
namespace {
struct Finalizer {
  raw_ptr<std::vector<int>> events;
  int id;
  std::function<void()> action;
};
void Finalize(void*, void* ptr, CRYPTO_EX_DATA*, int, long, void*) {
  std::unique_ptr<Finalizer> f(static_cast<Finalizer*>(ptr));
  if (f) {
    f->events->push_back(f->id);
    if (f->action) {
      f->action();
    }
  }
}
int FinalizerIndex() {
  static int i =
      SSL_SESSION_get_ex_new_index(0, nullptr, nullptr, nullptr, Finalize);
  return i;
}
bssl::UniquePtr<SSL_SESSION> Tracked(SSL_CTX* ctx,
                                     std::vector<int>& events,
                                     int id,
                                     std::function<void()> action = {}) {
  auto s = Session(ctx, TLS1_3_VERSION);
  EXPECT_TRUE(
      SSL_SESSION_set_ex_data(s.get(), FinalizerIndex(),
                              new Finalizer{&events, id, std::move(action)}));
  return s;
}
template <class C>
std::vector<int> FinalizerTrace() {
  Fixture f;
  std::vector<int> events;
  {
    typename C::Config config;
    config.max_entries = 2;
    C cache(config);
    cache.SetClockForTesting(&f.clock);
    typename C::Key k, k2, k3;
    k2.server = HostPortPair("b", 443);
    k3.server = HostPortPair("c", 443);
    for (int i = 1; i <= 3; ++i) {
      cache.Insert(cache.generation_number(), k,
                   Tracked(f.ctx.get(), events, i));
    }
    cache.Lookup(k).reset();
    cache.Insert(cache.generation_number(), k2,
                 Tracked(f.ctx.get(), events, 4));
    cache.Insert(cache.generation_number(), k3,
                 Tracked(f.ctx.get(), events, 5));
    cache.Flush();
    cache.Insert(cache.generation_number() - 1, k,
                 Tracked(f.ctx.get(), events, 6));
    cache.Insert(cache.generation_number(), k, Tracked(f.ctx.get(), events, 7));
  }
  return events;
}
TEST(TlsSessionCacheRust, FinalizerOrderDifferential) {
  auto a = FinalizerTrace<SSLClientSessionCache>(),
       b = FinalizerTrace<Original>();
  EXPECT_EQ(a, b);
  EXPECT_EQ(a, (std::vector<int>{1, 3, 2, 5, 4, 6, 7}));
}
TEST(TlsSessionCacheRust, FinalizerCanReenterAndDeleteOwner) {
  Fixture f;
  std::vector<int> events;
  SSLClientSessionCache::Config cfg;
  cfg.max_entries = 1;
  auto cache = std::make_unique<SSLClientSessionCache>(cfg);
  cache->SetClockForTesting(&f.clock);
  SSLClientSessionCache::Key k, other;
  other.server = HostPortPair("other", 443);
  cache->Insert(0, k, Tracked(f.ctx.get(), events, 1, [&] {
                  EXPECT_EQ(cache->size(), 1u);
                  cache->Flush();
                }));
  cache->Insert(0, other, Tracked(f.ctx.get(), events, 2));
  EXPECT_EQ(cache->size(), 0u);
  EXPECT_EQ(events, (std::vector<int>{1, 2}));
  cache->Insert(cache->generation_number(), k,
                Tracked(f.ctx.get(), events, 3, [&] { cache.reset(); }));
  cache->Flush();
  EXPECT_FALSE(cache);
  EXPECT_EQ(events, (std::vector<int>{1, 2, 3}));
}
// Full native TLS fixture, without touching provider/handshake production code.
using tls_session_cache_rust_testing::EarlySession;
std::vector<uint8_t> Serialized(SSL_SESSION* s) {
  uint8_t* bytes = nullptr;
  size_t len = 0;
  if (!SSL_SESSION_to_bytes(s, &bytes, &len)) {
    ADD_FAILURE() << "Failed to serialize the test TLS session";
    return {};
  }
  // SAFETY: successful BoringSSL serialization returns len initialized bytes
  // owned by bytes until OPENSSL_free below. The span is copied before free.
  const auto serialized = UNSAFE_BUFFERS(base::span(bytes, len));
  std::vector<uint8_t> v(serialized.begin(), serialized.end());
  OPENSSL_free(bytes);
  return v;
}
TEST(TlsSessionCacheRust, RealEarlyDataCopyDifferential) {
  auto s = EarlySession();
  ASSERT_TRUE(s);
  ASSERT_TRUE(SSL_SESSION_early_data_capable(s.get()));
  ASSERT_TRUE(SSL_SESSION_should_be_single_use(s.get()));
  base::SimpleTestClock clock;
  clock.SetNow(base::Time::FromTimeT(SSL_SESSION_get_time(s.get())));
  SSLClientSessionCache::Config c;
  SSLClientSessionCache a(c);
  Original::Config oc;
  Original b(oc);
  a.SetClockForTesting(&clock);
  b.SetClockForTesting(&clock);
  SSLClientSessionCache::Key k;
  auto o = OriginalKey(k);
  a.Insert(0, k, bssl::UpRef(s));
  a.Insert(0, k, bssl::UpRef(s));
  b.Insert(0, o, bssl::UpRef(s));
  b.Insert(0, o, bssl::UpRef(s));
  a.ClearEarlyData(k);
  b.ClearEarlyData(o);
  for (int i = 0; i < 2; ++i) {
    auto x = a.Lookup(k), y = b.Lookup(o);
    ASSERT_TRUE(x);
    ASSERT_TRUE(y);
    EXPECT_FALSE(SSL_SESSION_early_data_capable(x.get()));
    EXPECT_FALSE(SSL_SESSION_early_data_capable(y.get()));
    EXPECT_NE(x.get(), s.get());
    EXPECT_EQ(Serialized(x.get()), Serialized(y.get()));
  }
  EXPECT_EQ(a.size(), 0u);
  EXPECT_EQ(b.size(), 0u);
  EXPECT_TRUE(SSL_SESSION_early_data_capable(s.get()));
}
}  // namespace
}  // namespace net
// Additional public key, ownership and independent-sequence fixtures.
namespace net {
namespace {
TEST(TlsSessionCacheRust, EveryPartitionFieldSeparatesEntries) {
  Fixture f;
  SSLClientSessionCache::Key base;
  base.server = HostPortPair("same", 443);
  std::vector<SSLClientSessionCache::Key> keys{base};
  auto add = [&](auto change) {
    auto k = base;
    change(k);
    keys.push_back(std::move(k));
  };
  add([](auto& k) { k.server.set_port(444); });
  add([](auto& k) { k.server.set_host("different"); });
  add([](auto& k) { k.server.set_host(std::string_view("s\0ame", 5)); });
  add([](auto& k) { k.server.set_host(std::string_view("\xff", 1)); });
  add([](auto& k) { k.dest_ip_addr = IPAddress(); });
  add([](auto& k) { k.dest_ip_addr = IPAddress::IPv4Localhost(); });
  add([](auto& k) { k.dest_ip_addr = IPAddress::IPv6Localhost(); });
  add([](auto& k) {
    k.network_anonymization_key = NetworkAnonymizationKey::CreateTransient();
  });
  add([](auto& k) {
    k.network_anonymization_key = NetworkAnonymizationKey::CreateTransient();
  });
  add([](auto& k) {
    k.network_anonymization_key = NetworkAnonymizationKey::CreateSameSite(
        SchemefulSite(GURL("https://site.test")));
  });
  add([](auto& k) {
    k.network_anonymization_key = NetworkAnonymizationKey::CreateCrossSite(
        SchemefulSite(GURL("https://site.test")));
  });
  for (int m = 1; m < 4; ++m) {
    add([&](auto& k) { k.privacy_mode = static_cast<PrivacyMode>(m); });
  }
  add([](auto& k) { k.session_usage = SessionUsage::kProxy; });
  add([](auto& k) { k.proxy_chain = ProxyChain(); });
  add([](auto& k) { k.proxy_chain = ProxyChain::WithOpaqueData({}, -1); });
  add([](auto& k) { k.proxy_chain = ProxyChain::WithOpaqueData({}, 1); });
  add([](auto& k) {
    k.proxy_chain =
        ProxyChain::FromSchemeHostAndPort(ProxyServer::SCHEME_HTTPS, "p", 443);
  });
  add([](auto& k) {
    k.proxy_chain =
        ProxyChain::FromSchemeHostAndPort(ProxyServer::SCHEME_HTTPS, "p", 444);
  });
  add([](auto& k) {
    k.proxy_chain =
        ProxyChain::FromSchemeHostAndPort(ProxyServer::SCHEME_HTTPS, "q", 443);
  });
  add([](auto& k) {
    k.proxy_chain =
        ProxyChain::FromSchemeHostAndPort(ProxyServer::SCHEME_HTTP, "p", 443);
  });
  add([](auto& k) { k.proxy_chain_index = 1; });
  add([](auto& k) { k.proxy_chain_index = SIZE_MAX; });
  SSLClientSessionCache::Config config;
  SSLClientSessionCache a(config);
  Original::Config oc;
  Original b(oc);
  a.SetClockForTesting(&f.clock);
  b.SetClockForTesting(&f.clock);
  std::vector<bssl::UniquePtr<SSL_SESSION>> sessions;
  for (auto& k : keys) {
    sessions.push_back(Session(f.ctx.get(), TLS1_2_VERSION));
    a.Insert(0, k, bssl::UpRef(sessions.back()));
    b.Insert(0, OriginalKey(k), bssl::UpRef(sessions.back()));
  }
  ASSERT_EQ(a.size(), keys.size());
  ASSERT_EQ(a.size(), b.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    EXPECT_EQ(a.Lookup(keys[i]).get(), sessions[i].get());
    EXPECT_EQ(b.Lookup(OriginalKey(keys[i])).get(), sessions[i].get());
    for (auto& k : keys) {
      EXPECT_EQ(keys[i] < k, OriginalKey(keys[i]) < OriginalKey(k));
      EXPECT_EQ(keys[i] == k, OriginalKey(keys[i]) == OriginalKey(k));
    }
  }
}
TEST(TlsSessionCacheRust, IndependentSequencesShareNativeReferences) {
  Fixture f;
  auto session = Session(f.ctx.get(), TLS1_2_VERSION);
  std::vector<std::thread> threads;
  for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&] {
      base::SimpleTestClock clock;
      clock.SetNow(base::Time::FromTimeT(1000));
      SSLClientSessionCache::Config config;
      SSLClientSessionCache cache(config);
      cache.SetClockForTesting(&clock);
      SSLClientSessionCache::Key k;
      for (size_t i = 0; i < 8192; ++i) {
        cache.Insert(cache.generation_number(), k, bssl::UpRef(session));
        EXPECT_EQ(cache.Lookup(k).get(), session.get());
        if (i % 16 == 0) {
          cache.Flush();
        }
      }
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }
}
}  // namespace
}  // namespace net
namespace net {
namespace {
constexpr base::MemoryConsumerTraits TestTraits(
    base::MemoryConsumerTraits::EstimatedMemoryUsage::kSmall,
    base::MemoryConsumerTraits::ReleaseMemoryDataAccess::kReads,
    base::MemoryConsumerTraits::InformationRetention::kLossless,
    base::MemoryConsumerTraits::ExecutionType::kSynchronous);
}
class TlsSessionCacheRustTestPeer {
 public:
  static void Register(SSLClientSessionCache& c) {
    if (!c.memory_consumer_registration_) {
      c.memory_consumer_registration_.emplace(
          "SSLClientSessionCache", TestTraits, &c,
          base::AsyncMemoryConsumerRegistration::CheckUnregister::kDisabled);
    }
  }
};
namespace {
template <class C>
void PendingObserverCancellation() {
  base::SingleThreadTaskExecutor executor(base::MessagePumpType::DEFAULT, true);
  base::TestMemoryConsumerRegistry registry;
  Fixture f;
  std::vector<int> events;
  typename C::Config config;
  auto cache = std::make_unique<C>(config);
  cache->SetClockForTesting(&f.clock);
  if constexpr (std::is_same_v<C, SSLClientSessionCache>) {
    TlsSessionCacheRustTestPeer::Register(*cache);
  } else if (!cache->memory_consumer_registration_) {
    cache->memory_consumer_registration_.emplace(
        "SSLClientSessionCache", TestTraits, cache.get(),
        base::AsyncMemoryConsumerRegistration::CheckUnregister::kDisabled);
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(registry.size(), 1u);
  typename C::Key k;
  cache->Insert(0, k, Tracked(f.ctx.get(), events, 1));
  registry.NotifyUpdateMemoryLimitAndRelease(
      base::MemoryLimit::CriticalPressureThreshold());
  cache.reset();
  EXPECT_EQ(events, (std::vector<int>{1}));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(registry.size(), 0u);
  EXPECT_EQ(events, (std::vector<int>{1}));
}
TEST(TlsSessionCacheRust, QueuedMemoryNotificationsCancelledOnDestruction) {
  PendingObserverCancellation<SSLClientSessionCache>();
  PendingObserverCancellation<Original>();
}
}  // namespace
}  // namespace net
namespace net {
namespace {
TEST(TlsSessionCacheRust, EmptySlotsAndExpiredSecondaryDifferential) {
  Fixture f;
  SSLClientSessionCache::Config c;
  SSLClientSessionCache a(c);
  Original::Config oc;
  Original b(oc);
  a.SetClockForTesting(&f.clock);
  b.SetClockForTesting(&f.clock);
  SSLClientSessionCache::Key k;
  auto o = OriginalKey(k);
  auto old = Session(f.ctx.get(), TLS1_3_VERSION, 900, 10),
       valid = Session(f.ctx.get(), TLS1_2_VERSION);
  a.Insert(0, k, bssl::UpRef(old));
  b.Insert(0, o, bssl::UpRef(old));
  a.Insert(0, k, bssl::UpRef(valid));
  b.Insert(0, o, bssl::UpRef(valid));
  EXPECT_EQ(a.Lookup(k).get(), valid.get());
  EXPECT_EQ(b.Lookup(o).get(), valid.get());
  EXPECT_EQ(a.size(), b.size());
  a.Insert(0, k, nullptr);
  b.Insert(0, o, nullptr);
  EXPECT_FALSE(a.Lookup(k));
  EXPECT_FALSE(b.Lookup(o));
  EXPECT_EQ(a.size(), 0u);
  EXPECT_EQ(b.size(), 0u);
  a.Insert(0, k, bssl::UpRef(old));
  b.Insert(0, o, bssl::UpRef(old));
  a.Insert(0, k, nullptr);
  b.Insert(0, o, nullptr);
  EXPECT_FALSE(a.Lookup(k));
  EXPECT_FALSE(b.Lookup(o));
  EXPECT_EQ(a.size(), b.size());
}
}  // namespace
}  // namespace net
namespace net {
namespace {
TEST(TlsSessionCacheRust, PlatformRegistrationAndSaturatingLimitPolicy) {
  EXPECT_TRUE(TlsSessionCacheRegisterMemoryConsumerForTesting(false, false));
  EXPECT_FALSE(TlsSessionCacheRegisterMemoryConsumerForTesting(false, true));
  EXPECT_FALSE(TlsSessionCacheRegisterMemoryConsumerForTesting(true, false));
  EXPECT_FALSE(TlsSessionCacheRegisterMemoryConsumerForTesting(true, true));
  FeatureReset reset;
  auto flags = std::make_unique<base::FeatureList>();
  flags->RegisterExtraFeatureOverrides(
      {{base::kStatefulMemoryPressure,
        base::FeatureList::OVERRIDE_ENABLE_FEATURE},
       {features::kIgnoreMemoryPressureForSslClientSessionCache,
        base::FeatureList::OVERRIDE_DISABLE_FEATURE}});
  base::FeatureList::SetInstance(std::move(flags));
  for (size_t max : {size_t{0}, size_t{1}, size_t{3}, size_t{10},
                     size_t{UINT32_MAX}, SIZE_MAX / 2, SIZE_MAX}) {
    SSLClientSessionCache::Config c;
    c.max_entries = max;
    SSLClientSessionCache a(c);
    Original::Config oc;
    oc.max_entries = max;
    Original b(oc);
    for (int percent : {0, 1, 33, 50, 99, 100, 101, 200, INT_MAX}) {
      auto limit = base::MemoryLimit::FromPercent(percent);
      Notifications::NotifyUpdateMemoryLimit(&a, limit);
      Notifications::NotifyUpdateMemoryLimit(&b, limit);
      EXPECT_EQ(a.max_size(), b.max_size());
      Notifications::NotifyReleaseMemory(&a);
      Notifications::NotifyReleaseMemory(&b);
      EXPECT_EQ(a.max_size(), b.max_size());
      EXPECT_EQ(a.size(), b.size());
      EXPECT_EQ(a.generation_number(), b.generation_number());
    }
  }
}
}  // namespace
}  // namespace net
