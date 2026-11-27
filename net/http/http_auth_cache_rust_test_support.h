// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_AUTH_CACHE_RUST_TEST_SUPPORT_H_
#define NET_HTTP_HTTP_AUTH_CACHE_RUST_TEST_SUPPORT_H_

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/clock.h"
#include "base/time/tick_clock.h"
#include "net/base/schemeful_site.h"
#include "net/http/http_auth_cache.h"
#include "net/http/http_auth_cache_rust_original.h"
#include "url/gurl.h"

namespace net {
// Authored inspection only; operations always use the real public adapters.
struct HttpAuthCacheRustTestPeer {
  struct State {
    url::SchemeHostPort origin;
    HttpAuth::Target target;
    NetworkAnonymizationKey nak;
    std::string realm;
    HttpAuth::Scheme scheme;
    std::string challenge;
    std::u16string username, password;
    int nonce;
    std::vector<std::string> paths;
    base::TimeTicks created_ticks, used_ticks;
    base::Time created;
    bool operator==(const State&) const = default;
  };
  template <class Cache>
  static std::vector<State> States(const Cache& cache) {
    std::vector<State> out;
    for (const auto& [key, e] : cache.entries_) {
      out.push_back({e.scheme_host_port_,
                     key.target,
                     key.network_anonymization_key,
                     e.realm_,
                     e.scheme_,
                     e.auth_challenge_,
                     e.credentials_.username(),
                     e.credentials_.password(),
                     e.nonce_count_,
                     {e.paths_.begin(), e.paths_.end()},
                     e.creation_time_ticks_,
                     e.last_use_time_ticks_,
                     e.creation_time_});
    }
    return out;
  }
  template <class Cache>
  static auto Pointers(const Cache& cache) {
    std::vector<const typename Cache::Entry*> out;
    for (const auto& [key, e] : cache.entries_) {
      out.push_back(&e);
    }
    return out;
  }
  template <class Entry>
  static void AddPath(Entry& e, const std::string& path) {
    e.AddPath(path);
  }
  template <class Entry>
  static bool HasPath(Entry& e, const std::string& path, size_t* n) {
    return e.HasEnclosingPath(path, n);
  }
};

class HttpAuthCacheRustTick final : public base::TickClock {
 public:
  base::TimeTicks NowTicks() const override;
  mutable int64_t value = 1000;
  mutable uint64_t calls = 0;
};
class HttpAuthCacheRustClock final : public base::Clock {
 public:
  base::Time Now() const override;
  mutable int64_t value = 1000;
  mutable uint64_t calls = 0;
};

inline base::TimeTicks HttpAuthCacheRustTick::NowTicks() const {
  ++calls;
  const auto now = value;
  if (value != INT64_MAX) {
    ++value;
  }
  return base::TimeTicks() + base::Microseconds(now);
}
inline base::Time HttpAuthCacheRustClock::Now() const {
  ++calls;
  const auto now = value;
  if (value != INT64_MAX) {
    ++value;
  }
  return base::Time::FromInternalValue(now);
}

template <class A, class B>
void HttpAuthCacheRustCheckEntry(A* a, B* b) {
  CHECK_EQ(a != nullptr, b != nullptr);
  if (!a) {
    return;
  }
  CHECK(a->scheme_host_port() == b->scheme_host_port());
  CHECK(a->realm() == b->realm());
  CHECK(a->scheme() == b->scheme());
  CHECK(a->auth_challenge() == b->auth_challenge());
  CHECK(a->credentials().Equals(b->credentials()));
}

inline void HttpAuthCacheRustDifferential(const std::vector<uint8_t>& bytes) {
  HttpAuthCache rust(false), rust_source(true);
  HttpAuthCacheOriginal cpp(false), cpp_source(true);
  HttpAuthCacheRustTick rt, ct;
  HttpAuthCacheRustClock rw, cw;
  rust.set_tick_clock_for_testing(&rt);
  cpp.set_tick_clock_for_testing(&ct);
  rust.set_clock_for_testing(&rw);
  cpp.set_clock_for_testing(&cw);
  rust_source.set_tick_clock_for_testing(&rt);
  cpp_source.set_tick_clock_for_testing(&ct);
  rust_source.set_clock_for_testing(&rw);
  cpp_source.set_clock_for_testing(&cw);
  const auto nak = NetworkAnonymizationKey::CreateSameSite(
      SchemefulSite(GURL("https://partition.test")));
  const NetworkAnonymizationKey empty;
  std::array<url::SchemeHostPort, 4> origins{
      url::SchemeHostPort(GURL("http://a.test")),
      url::SchemeHostPort(GURL("https://b.test:8443")),
      url::SchemeHostPort(GURL("ws://c.test")),
      url::SchemeHostPort(GURL("wss://d.test"))};
  std::vector<std::string> callbacks_r, callbacks_c;
  for (size_t pos = 0; pos + 5 < bytes.size(); pos += 6) {
    auto v = [&](size_t n) { return bytes[pos + n]; };
    auto& origin = origins[v(1) % origins.size()];
    auto target = (v(1) & 4) ? HttpAuth::AUTH_SERVER : HttpAuth::AUTH_PROXY;
    auto scheme =
        static_cast<HttpAuth::Scheme>(v(2) % HttpAuth::AUTH_SCHEME_MAX);
    auto& key = (v(1) & 8) ? nak : empty;
    const std::string realm{char(v(3)), char(v(4))};
    const std::string challenge{char(v(4)), char(v(5)), char(0xff)};
    const std::string path = (v(5) & 1)
                                 ? ""
                                 : "/" + std::to_string(v(4) % 16) +
                                       ((v(5) & 2) ? "/leaf/file" : "/file");
    const AuthCredentials credentials(
        std::u16string{char16_t(0xd800 + v(3)), 0, char16_t(v(4))},
        std::u16string{char16_t(0xdc00 + v(4)), char16_t(v(5))});
    switch (v(0) % 16) {
      case 0:
      case 1:
      case 2:
      case 3:
        HttpAuthCacheRustCheckEntry(rust.Add(origin, target, realm, scheme, key,
                                             challenge, credentials, path),
                                    cpp.Add(origin, target, realm, scheme, key,
                                            challenge, credentials, path));
        break;
      case 4:
        HttpAuthCacheRustCheckEntry(
            rust.Lookup(origin, target, realm, scheme, key),
            cpp.Lookup(origin, target, realm, scheme, key));
        break;
      case 5:
        HttpAuthCacheRustCheckEntry(
            rust.LookupByPath(origin, target, key, path),
            cpp.LookupByPath(origin, target, key, path));
        break;
      case 6:
        CHECK_EQ(rust.Remove(origin, target, realm, scheme, key, credentials),
                 cpp.Remove(origin, target, realm, scheme, key, credentials));
        break;
      case 7:
        CHECK_EQ(rust.UpdateStaleChallenge(origin, target, realm, scheme, key,
                                           challenge),
                 cpp.UpdateStaleChallenge(origin, target, realm, scheme, key,
                                          challenge));
        break;
      case 8:
        rust.SetKeyServerEntriesByNetworkAnonymizationKey(v(5) & 1);
        cpp.SetKeyServerEntriesByNetworkAnonymizationKey(v(5) & 1);
        break;
      case 9:
        CHECK_EQ(rust.ClearAllEntries(), cpp.ClearAllEntries());
        break;
      case 10: {
        const auto begin = base::Time::FromInternalValue(1000 + (v(3) % 32));
        const auto end = base::Time::FromInternalValue(1000 + (v(4) % 64));
        auto filter = [](std::vector<std::string>* calls, const GURL& url) {
          calls->push_back(url.spec());
          return url.SchemeIs("http");
        };
        CHECK_EQ(rust.ClearEntriesAddedBetween(
                     begin, end, base::BindRepeating(filter, &callbacks_r)),
                 cpp.ClearEntriesAddedBetween(
                     begin, end, base::BindRepeating(filter, &callbacks_c)));
        CHECK(callbacks_r == callbacks_c);
        break;
      }
      case 11:
        rust_source.Add(origin, target, realm, scheme, key, challenge,
                        credentials, path);
        cpp_source.Add(origin, target, realm, scheme, key, challenge,
                       credentials, path);
        rust.CopyProxyEntriesFrom(rust_source);
        cpp.CopyProxyEntriesFrom(cpp_source);
        break;
      case 12:
        rust.CopyProxyEntriesFrom(rust);
        cpp.CopyProxyEntriesFrom(cpp);
        break;
      default: {
        auto* r = rust.LookupByPath(origin, target, key, path);
        auto* c = cpp.LookupByPath(origin, target, key, path);
        HttpAuthCacheRustCheckEntry(r, c);
        if (r) {
          // Returned references are deliberately passed back as input aliases.
          if (v(0) % 16 == 13) {
            r->UpdateStaleChallenge(r->auth_challenge());
            c->UpdateStaleChallenge(c->auth_challenge());
          } else if (v(0) % 16 == 14) {
            CHECK_EQ(r->IncrementNonceCount(), c->IncrementNonceCount());
          } else {
            CHECK(r == rust.Add(r->scheme_host_port(), target, r->realm(),
                                r->scheme(), key, r->auth_challenge(),
                                r->credentials(), path));
            CHECK(c == cpp.Add(c->scheme_host_port(), target, c->realm(),
                               c->scheme(), key, c->auth_challenge(),
                               c->credentials(), path));
          }
          HttpAuthCache::Entry rc(*r);
          HttpAuthCacheOriginal::Entry cc(*c);
          CHECK(r->IsEqualForTesting(rc));
          CHECK(c->IsEqualForTesting(cc));
        }
      }
    }
    CHECK(HttpAuthCacheRustTestPeer::States(rust) ==
          HttpAuthCacheRustTestPeer::States(cpp));
    CHECK(HttpAuthCacheRustTestPeer::States(rust_source) ==
          HttpAuthCacheRustTestPeer::States(cpp_source));
    CHECK_EQ(rust.key_server_entries_by_network_anonymization_key(),
             cpp.key_server_entries_by_network_anonymization_key());
    CHECK_EQ(rt.calls, ct.calls);
    CHECK_EQ(rw.calls, cw.calls);
  }
}
}  // namespace net
#endif
