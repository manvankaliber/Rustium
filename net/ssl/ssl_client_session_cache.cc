// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ssl/ssl_client_session_cache.h"

#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory_coordinator/memory_coordinator_features.h"
#include "base/memory_coordinator/traits.h"
#include "base/memory_coordinator/utils.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "build/build_config.h"
#include "net/base/features.h"
#include "net/ssl/tls_session_cache_rust_abi.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"

namespace net {

namespace {

TlsSessionCacheRustServer ServerView(const HostPortPair& server) {
  return {{reinterpret_cast<const uint8_t*>(server.host().data()),
           server.host().size()},
          server.port()};
}

// Storage views only: the tuple and every owned cache comparison run in Rust.
TlsSessionCacheRustKey KeyView(const SSLClientSessionCache::Key& key,
                               std::vector<TlsSessionCacheRustProxy>& proxies) {
  if (key.proxy_chain.IsValid()) {
    for (const auto& proxy : key.proxy_chain.proxy_servers()) {
      proxies.push_back({ServerView(proxy.host_port_pair()), proxy.scheme()});
    }
  }
  return {ServerView(key.server),
          {key.dest_ip_addr ? key.dest_ip_addr->bytes().data() : nullptr,
           key.dest_ip_addr ? key.dest_ip_addr->size() : 0},
          &key.network_anonymization_key,
          proxies.data(),
          proxies.size(),
          key.proxy_chain_index,
          key.privacy_mode,
          static_cast<int32_t>(key.session_usage),
          key.proxy_chain.opaque_data().value_or(0),
          key.dest_ip_addr.has_value(),
          key.proxy_chain.IsValid(),
          key.proxy_chain.opaque_data().has_value()};
}

int CompareKeys(const SSLClientSessionCache::Key& a,
                const SSLClientSessionCache::Key& b) {
  std::vector<TlsSessionCacheRustProxy> ap, bp;
  auto av = KeyView(a, ap);
  auto bv = KeyView(b, bp);
  return tls_session_cache_rust_compare(&av, &bv);
}

constexpr base::MemoryConsumerTraits kSSLClientSessionCacheTraits(
    // Bounded capacity of sessions; way under 10MB.
    base::MemoryConsumerTraits::EstimatedMemoryUsage::kSmall,
    // Rust traverses cache entries and releases native BoringSSL sessions.
    base::MemoryConsumerTraits::ReleaseMemoryDataAccess::kReads,
    // Full handshake can be done if not cached.
    base::MemoryConsumerTraits::InformationRetention::kLossless,
    // Cache trimming runs synchronously on the calling thread.
    base::MemoryConsumerTraits::ExecutionType::kSynchronous);

// Registration ownership remains native; Rust decides platform suppression.

}  // namespace

SSLClientSessionCache::Key::Key() = default;
SSLClientSessionCache::Key::Key(const Key& other) = default;
SSLClientSessionCache::Key::Key(Key&& other) = default;
SSLClientSessionCache::Key::~Key() = default;
SSLClientSessionCache::Key& SSLClientSessionCache::Key::operator=(
    const Key& other) = default;
SSLClientSessionCache::Key& SSLClientSessionCache::Key::operator=(Key&& other) =
    default;

bool SSLClientSessionCache::Key::operator==(const Key& other) const {
  return CompareKeys(*this, other) == 0;
}
bool SSLClientSessionCache::Key::operator<(const Key& other) const {
  return CompareKeys(*this, other) < 0;
}

SSLClientSessionCache::SSLClientSessionCache(const Config& config)
    : clock_(base::DefaultClock::GetInstance()),
      state_(tls_session_cache_rust_new(config.max_entries,
                                        config.expiration_check_count)) {
  if (tls_session_cache_rust_register_memory_consumer(BUILDFLAG(IS_MAC),
                                                      BUILDFLAG(IS_WIN))) {
    memory_consumer_registration_.emplace(
        "SSLClientSessionCache", kSSLClientSessionCacheTraits, this,
        base::AsyncMemoryConsumerRegistration::CheckUnregister::kDisabled);
  }
}
SSLClientSessionCache::~SSLClientSessionCache() {
  // Cancel queued observer callbacks before sessions can invoke ex-data frees.
  memory_consumer_registration_.reset();
  state_.reset();
}
void SSLClientSessionCache::StateDeleter::operator()(
    TlsSessionCacheRustState* state) const {
  tls_session_cache_rust_delete(state);
}
size_t SSLClientSessionCache::size() const {
  return tls_session_cache_rust_stat(state_.get(), 0);
}
size_t SSLClientSessionCache::max_size() const {
  return tls_session_cache_rust_stat(state_.get(), 1);
}
uint64_t SSLClientSessionCache::generation_number() const {
  return tls_session_cache_rust_stat(state_.get(), 2);
}
bssl::UniquePtr<SSL_SESSION> SSLClientSessionCache::Lookup(
    const Key& cache_key) {
  std::vector<TlsSessionCacheRustProxy> proxies;
  auto key = KeyView(cache_key, proxies);
  return bssl::UniquePtr<SSL_SESSION>(
      reinterpret_cast<SSL_SESSION*>(tls_session_cache_rust_run(
          state_.get(), 0, &key, 0, 0, clock_, nullptr, nullptr, 0)));
}
void SSLClientSessionCache::Insert(uint64_t generation_number,
                                   const Key& cache_key,
                                   bssl::UniquePtr<SSL_SESSION> session) {
  std::vector<TlsSessionCacheRustProxy> proxies;
  auto key = KeyView(cache_key, proxies);
  tls_session_cache_rust_run(state_.get(), 1, &key,
                             reinterpret_cast<uintptr_t>(session.release()),
                             generation_number, clock_, nullptr, nullptr, 0);
}
void SSLClientSessionCache::ClearEarlyData(const Key& cache_key) {
  std::vector<TlsSessionCacheRustProxy> proxies;
  auto key = KeyView(cache_key, proxies);
  tls_session_cache_rust_run(state_.get(), 2, &key, 0, 0, clock_, nullptr,
                             nullptr, 0);
}
void SSLClientSessionCache::FlushForServers(
    const base::flat_set<HostPortPair>& servers) {
  std::vector<TlsSessionCacheRustServer> views;
  for (const auto& server : servers) {
    views.push_back(ServerView(server));
  }
  tls_session_cache_rust_run(state_.get(), 3, nullptr, 0, 0, clock_, nullptr,
                             views.data(), views.size());
}
void SSLClientSessionCache::Flush() {
  tls_session_cache_rust_run(state_.get(), 4, nullptr, 0, 0, clock_, nullptr,
                             nullptr, 0);
}
void SSLClientSessionCache::SetClockForTesting(base::Clock* clock) {
  clock_ = clock;
}
bool SSLClientSessionCache::IsExpired(SSL_SESSION* session, time_t now) {
  return tls_session_cache_rust_expired(reinterpret_cast<uintptr_t>(session),
                                        now);
}
void SSLClientSessionCache::OnUpdateMemoryLimit() {
  const TlsSessionCacheRustMemory memory{memory_limit().percent()};
  tls_session_cache_rust_run(state_.get(), 5, nullptr, 0, 0, clock_, &memory,
                             nullptr, 0);
}
void SSLClientSessionCache::OnReleaseMemory() {
  const TlsSessionCacheRustMemory memory{memory_limit().percent()};
  tls_session_cache_rust_run(state_.get(), 6, nullptr, 0, 0, clock_, &memory,
                             nullptr, 0);
}

extern "C" NET_EXPORT const base::Feature*
tls_session_cache_rust_ignore_feature() {
  return &features::kIgnoreMemoryPressureForSslClientSessionCache;
}

// Explicit immutable value and BoringSSL ownership/crypto services. None of
// the query/up-ref/copy services runs user callbacks or modifies cache policy.
extern "C" bool tls_session_cache_rust_feature(uint32_t which) {
  return base::FeatureList::IsEnabled(
      which == 0 ? base::kStatefulMemoryPressure
                 : features::kIgnoreMemoryPressureForSslClientSessionCache);
}
extern "C" uintptr_t tls_session_cache_rust_nak_clone(const void* nak) {
  return reinterpret_cast<uintptr_t>(new NetworkAnonymizationKey(
      *static_cast<const NetworkAnonymizationKey*>(nak)));
}
extern "C" void tls_session_cache_rust_nak_drop(uintptr_t nak) {
  delete reinterpret_cast<NetworkAnonymizationKey*>(nak);
}
extern "C" int32_t tls_session_cache_rust_nak_compare(uintptr_t a,
                                                      uintptr_t b) {
  const auto order = *reinterpret_cast<const NetworkAnonymizationKey*>(a) <=>
                     *reinterpret_cast<const NetworkAnonymizationKey*>(b);
  return order < 0 ? -1 : order > 0 ? 1 : 0;
}
extern "C" int64_t tls_session_cache_rust_now(const void* clock) {
  return static_cast<const base::Clock*>(clock)->Now().ToTimeT();
}
extern "C" uint64_t tls_session_cache_rust_time(uintptr_t session) {
  return SSL_SESSION_get_time(reinterpret_cast<SSL_SESSION*>(session));
}
extern "C" uint64_t tls_session_cache_rust_timeout(uintptr_t session) {
  return SSL_SESSION_get_timeout(reinterpret_cast<SSL_SESSION*>(session));
}
extern "C" bool tls_session_cache_rust_single(uintptr_t session) {
  return SSL_SESSION_should_be_single_use(
      reinterpret_cast<SSL_SESSION*>(session));
}
extern "C" uintptr_t tls_session_cache_rust_up_ref(uintptr_t session) {
  SSL_SESSION_up_ref(reinterpret_cast<SSL_SESSION*>(session));
  return session;
}
extern "C" uintptr_t tls_session_cache_rust_copy(uintptr_t session) {
  return reinterpret_cast<uintptr_t>(SSL_SESSION_copy_without_early_data(
      reinterpret_cast<SSL_SESSION*>(session)));
}
extern "C" void tls_session_cache_rust_free(uintptr_t session) {
  SSL_SESSION_free(reinterpret_cast<SSL_SESSION*>(session));
}

bool TlsSessionCacheRegisterMemoryConsumerForTesting(bool is_mac, bool is_win) {
  return tls_session_cache_rust_register_memory_consumer(is_mac, is_win);
}
}  // namespace net
