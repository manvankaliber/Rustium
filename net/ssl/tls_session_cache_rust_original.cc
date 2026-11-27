// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ssl/tls_session_cache_rust_original.h"

#include <algorithm>
#include <tuple>
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
#include "third_party/boringssl/src/include/openssl/ssl.h"

extern "C" const base::Feature* tls_session_cache_rust_ignore_feature();

namespace net::tls_session_cache_rust_original {

namespace {

// Independently reproduce ORIGINAL public views, never migrated operators.
// See archived helper-originals for original declaration/member order.
auto OriginalServer(const HostPortPair& p) {
  return std::tuple(p.port(), std::string_view(p.host()));
}
std::strong_ordering OriginalProxies(const ProxyChain& a, const ProxyChain& b) {
  const auto& ap = a.proxy_servers_if_valid();
  const auto& bp = b.proxy_servers_if_valid();
  if (auto c = ap.has_value() <=> bp.has_value(); c != 0) {
    return c;
  }
  if (ap) {
    for (size_t i = 0; i < std::min(ap->size(), bp->size()); ++i) {
      if (auto c = (*ap)[i].scheme() <=> (*bp)[i].scheme(); c != 0) {
        return c;
      }
      if (auto c = OriginalServer((*ap)[i].host_port_pair()) <=>
                   OriginalServer((*bp)[i].host_port_pair());
          c != 0) {
        return c;
      }
    }
    if (auto c = ap->size() <=> bp->size(); c != 0) {
      return c;
    }
  }
  return a.opaque_data() <=> b.opaque_data();
}
bool EqualKeyFields(const SSLClientSessionCache::Key& a,
                    const SSLClientSessionCache::Key& b) {
  if (OriginalServer(a.server) != OriginalServer(b.server) ||
      a.dest_ip_addr != b.dest_ip_addr ||
      a.network_anonymization_key != b.network_anonymization_key ||
      a.privacy_mode != b.privacy_mode || a.session_usage != b.session_usage) {
    return false;
  }
  const auto& ap = a.proxy_chain.proxy_servers_if_valid();
  const auto& bp = b.proxy_chain.proxy_servers_if_valid();
  if (ap.has_value() != bp.has_value()) {
    return false;
  }
  if (ap) {
    if (ap->size() != bp->size()) {
      return false;
    }
    for (size_t i = 0; i < ap->size(); ++i) {
      if ((*ap)[i].scheme() != (*bp)[i].scheme() ||
          OriginalServer((*ap)[i].host_port_pair()) !=
              OriginalServer((*bp)[i].host_port_pair())) {
        return false;
      }
    }
  }
  return a.proxy_chain.opaque_data() == b.proxy_chain.opaque_data() &&
         a.proxy_chain_index == b.proxy_chain_index;
}
int CompareKeyFields(const SSLClientSessionCache::Key& a,
                     const SSLClientSessionCache::Key& b) {
  // Original tuple member order, independently evaluating original helper
  // views. No allocations or migrated HostPortPair/ProxyChain comparisons.
  // std::tie keeps the NAK and optional IP native immutable services unchanged.
  if (auto c = OriginalServer(a.server) <=> OriginalServer(b.server); c != 0) {
    return c < 0 ? -1 : 1;
  }
  auto at = std::tie(a.dest_ip_addr, a.network_anonymization_key,
                     a.privacy_mode, a.session_usage);
  auto bt = std::tie(b.dest_ip_addr, b.network_anonymization_key,
                     b.privacy_mode, b.session_usage);
  if (auto c = at <=> bt; c != 0) {
    return c < 0 ? -1 : 1;
  }
  if (auto c = OriginalProxies(a.proxy_chain, b.proxy_chain); c != 0) {
    return c < 0 ? -1 : 1;
  }
  return a.proxy_chain_index < b.proxy_chain_index   ? -1
         : a.proxy_chain_index > b.proxy_chain_index ? 1
                                                     : 0;
}

constexpr base::MemoryConsumerTraits kSSLClientSessionCacheTraits(
    // Bounded capacity of sessions; way under 10MB.
    base::MemoryConsumerTraits::EstimatedMemoryUsage::kSmall,
    // Iterates base::LRUCache, reading the BoringSSL sessions being freed.
    base::MemoryConsumerTraits::ReleaseMemoryDataAccess::kReads,
    // Full handshake can be done if not cached.
    base::MemoryConsumerTraits::InformationRetention::kLossless,
    // Cache trimming runs synchronously on the calling thread.
    base::MemoryConsumerTraits::ExecutionType::kSynchronous);

// The SuppressMemoryListeners experiment showed that reacting to memory
// pressure is not beneficial on Mac and Windows.
constexpr bool kRegisterMemoryConsumer =
    !BUILDFLAG(IS_MAC) && !BUILDFLAG(IS_WIN);

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
  return EqualKeyFields(*this, other);
}

bool SSLClientSessionCache::Key::operator<(const Key& other) const {
  return CompareKeyFields(*this, other) < 0;
}

SSLClientSessionCache::SSLClientSessionCache(const Config& config)
    : clock_(base::DefaultClock::GetInstance()),
      config_(config),
      cache_(config.max_entries) {
  if (kRegisterMemoryConsumer) {
    memory_consumer_registration_.emplace(
        "SSLClientSessionCache", kSSLClientSessionCacheTraits, this,
        base::AsyncMemoryConsumerRegistration::CheckUnregister::kDisabled);
  }
}

SSLClientSessionCache::~SSLClientSessionCache() {
  Flush();
}

size_t SSLClientSessionCache::size() const {
  return cache_.size();
}

size_t SSLClientSessionCache::max_size() const {
  return cache_.max_size();
}

bssl::UniquePtr<SSL_SESSION> SSLClientSessionCache::Lookup(
    const Key& cache_key) {
  // Expire stale sessions.
  lookups_since_flush_++;
  if (lookups_since_flush_ >= config_.expiration_check_count) {
    lookups_since_flush_ = 0;
    FlushExpiredSessions();
  }

  auto iter = cache_.Get(cache_key);
  if (iter == cache_.end()) {
    return nullptr;
  }

  time_t now = clock_->Now().ToTimeT();
  bssl::UniquePtr<SSL_SESSION> session = iter->second.Pop();
  if (iter->second.ExpireSessions(now)) {
    cache_.Erase(iter);
  }

  if (IsExpired(session.get(), now)) {
    session = nullptr;
  }

  return session;
}

void SSLClientSessionCache::Insert(uint64_t generation_number,
                                   const Key& cache_key,
                                   bssl::UniquePtr<SSL_SESSION> session) {
  if (generation_number != generation_number_) {
    return;
  }
  auto iter = cache_.Get(cache_key);
  if (iter == cache_.end()) {
    iter = cache_.Put(cache_key, Entry());
  }

  // Insertion can fail if the max size was zero due to memory pressure.
  if (iter == cache_.end()) {
    CHECK_EQ(cache_.max_size(), 0U);
    return;
  }
  iter->second.Push(std::move(session));
}

void SSLClientSessionCache::ClearEarlyData(const Key& cache_key) {
  auto iter = cache_.Get(cache_key);
  if (iter != cache_.end()) {
    for (auto& session : iter->second.sessions) {
      if (session) {
        session.reset(SSL_SESSION_copy_without_early_data(session.get()));
      }
    }
  }
}

void SSLClientSessionCache::FlushForServers(
    const base::flat_set<HostPortPair>& servers) {
  // The generation number is incremented here, which affects all hosts, even
  // though this flush only applies to those matching `servers`. Only the
  // sessions related to `servers` are cleared, so any other already cached
  // sessions will remain valid despite the generation number changing. It
  // could prevent sessions unrelated to `servers` that are in-flight at the
  // time of this flush from being cached. That is not optimal but is a
  // trade-off for implementation simplicity.
  ++generation_number_;

  auto iter = cache_.begin();
  while (iter != cache_.end()) {
    if (std::binary_search(servers.begin(), servers.end(), iter->first.server,
                           [](const HostPortPair& a, const HostPortPair& b) {
                             return OriginalServer(a) < OriginalServer(b);
                           })) {
      iter = cache_.Erase(iter);
    } else {
      ++iter;
    }
  }
}

void SSLClientSessionCache::Flush() {
  ++generation_number_;
  cache_.Clear();
}

void SSLClientSessionCache::SetClockForTesting(base::Clock* clock) {
  clock_ = clock;
}

bool SSLClientSessionCache::IsExpired(SSL_SESSION* session, time_t now) {
  if (now < 0) {
    return true;
  }
  uint64_t now_u64 = static_cast<uint64_t>(now);

  // now_u64 may be slightly behind because of differences in how
  // time is calculated at this layer versus BoringSSL.
  // Add a second of wiggle room to account for this.
  return now_u64 < SSL_SESSION_get_time(session) - 1 ||
         now_u64 >=
             SSL_SESSION_get_time(session) + SSL_SESSION_get_timeout(session);
}

SSLClientSessionCache::Entry::Entry() = default;
SSLClientSessionCache::Entry::Entry(Entry&&) = default;
SSLClientSessionCache::Entry::~Entry() = default;

void SSLClientSessionCache::Entry::Push(bssl::UniquePtr<SSL_SESSION> session) {
  if (sessions[0] != nullptr &&
      SSL_SESSION_should_be_single_use(sessions[0].get())) {
    sessions[1] = std::move(sessions[0]);
  }
  sessions[0] = std::move(session);
}

bssl::UniquePtr<SSL_SESSION> SSLClientSessionCache::Entry::Pop() {
  if (sessions[0] == nullptr) {
    return nullptr;
  }
  bssl::UniquePtr<SSL_SESSION> session = bssl::UpRef(sessions[0]);
  if (SSL_SESSION_should_be_single_use(session.get())) {
    sessions[0] = std::move(sessions[1]);
    sessions[1] = nullptr;
  }
  return session;
}

bool SSLClientSessionCache::Entry::ExpireSessions(time_t now) {
  if (sessions[0] == nullptr) {
    return true;
  }

  if (SSLClientSessionCache::IsExpired(sessions[0].get(), now)) {
    return true;
  }

  if (sessions[1] != nullptr &&
      SSLClientSessionCache::IsExpired(sessions[1].get(), now)) {
    sessions[1] = nullptr;
  }

  return false;
}

void SSLClientSessionCache::FlushExpiredSessions() {
  time_t now = clock_->Now().ToTimeT();
  auto iter = cache_.begin();
  while (iter != cache_.end()) {
    if (iter->second.ExpireSessions(now)) {
      iter = cache_.Erase(iter);
    } else {
      ++iter;
    }
  }
}

void SSLClientSessionCache::OnUpdateMemoryLimit() {
  if (!base::FeatureList::IsEnabled(base::kStatefulMemoryPressure)) {
    return;
  }

  size_t target_size = memory_limit().Scale(config_.max_entries);

  // IMPORTANT: Ensure no memory is released during this call.
  // By using std::max, we ensure the new limit is at least the current size,
  // preventing growth without triggering immediate eviction.
  cache_.UpdateMaxSize(std::max(cache_.size(), target_size));
}

void SSLClientSessionCache::OnReleaseMemory() {
  if (base::FeatureList::IsEnabled(*tls_session_cache_rust_ignore_feature())) {
    // We don't want to clear the SSL session cache because the entries in it
    // are highly likely to be used again soon, and it causes more
    // fragmentation and increases user latency to clear it, then spend
    // additional roundtrips replacing all of the entries.
    return;
  }
  if (base::FeatureList::IsEnabled(base::kStatefulMemoryPressure)) {
    // Now we actually evict entries to reach the target size.
    cache_.UpdateMaxSize(memory_limit().Scale(config_.max_entries));
    return;
  }

  // Preserve the traditional "one-shot" logic for legacy memory pressure.
  if (memory_limit() <= base::kCriticalMemoryPressureThreshold) {
    Flush();
  } else if (memory_limit() <= base::kModerateMemoryPressureThreshold) {
    FlushExpiredSessions();
  }
}

}  // namespace net::tls_session_cache_rust_original
