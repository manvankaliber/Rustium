// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_auth_cache_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
TEST(HttpAuthCacheRustTest, StatefulMillionOperations) {
  uint64_t seed = 0x41a37d690285efbc;
  for (int sequence = 0; sequence < 4096; ++sequence) {
    std::vector<uint8_t> data(1536);
    for (auto& byte : data) {
      seed ^= seed << 13;
      seed ^= seed >> 7;
      seed ^= seed << 17;
      byte = static_cast<uint8_t>(seed);
    }
    HttpAuthCacheRustDifferential(data);
  }
}
TEST(HttpAuthCacheRustTest, PathMutationAndFailureOutput) {
  HttpAuthCache cache(false);
  HttpAuthCacheOriginal original(false);
  HttpAuthCacheRustTick rt, ct;
  HttpAuthCacheRustClock rw, cw;
  cache.set_tick_clock_for_testing(&rt);
  original.set_tick_clock_for_testing(&ct);
  cache.set_clock_for_testing(&rw);
  original.set_clock_for_testing(&cw);
  const url::SchemeHostPort origin(GURL("http://a.test"));
  auto* r = cache.Add(origin, HttpAuth::AUTH_SERVER, "r",
                      HttpAuth::AUTH_SCHEME_BASIC, {}, "c", {}, "/0/x");
  auto* c = original.Add(origin, HttpAuth::AUTH_SERVER, "r",
                         HttpAuth::AUTH_SCHEME_BASIC, {}, "c", {}, "/0/x");
  for (int i = 0; i < 20; ++i) {
    auto path = "/" + std::to_string(i) + "/x";
    HttpAuthCacheRustTestPeer::AddPath(*r, path);
    HttpAuthCacheRustTestPeer::AddPath(*c, path);
  }
  for (const std::string dir : {"/19/", "/18/", "/0/", "/", ""}) {
    size_t rn = 123, cn = 123;
    CHECK_EQ(HttpAuthCacheRustTestPeer::HasPath(*r, dir, &rn),
             HttpAuthCacheRustTestPeer::HasPath(*c, dir, &cn));
    CHECK_EQ(rn, cn);
    CHECK_EQ(HttpAuthCacheRustTestPeer::HasPath(*r, dir, nullptr),
             HttpAuthCacheRustTestPeer::HasPath(*c, dir, nullptr));
  }
  HttpAuthCacheRustTestPeer::AddPath(*r, "/");
  HttpAuthCacheRustTestPeer::AddPath(*c, "/");
  CHECK(HttpAuthCacheRustTestPeer::States(cache) ==
        HttpAuthCacheRustTestPeer::States(original));
}
TEST(HttpAuthCacheRustTest, StablePointersReferencesAndSelfCopy) {
  HttpAuthCache cache(false);
  const url::SchemeHostPort origin(GURL("http://a.test"));
  auto* entry =
      cache.Add(origin, HttpAuth::AUTH_PROXY, "r", HttpAuth::AUTH_SCHEME_BASIC,
                {}, "c", AuthCredentials(u"\xd800", u"\0\xdc00"), "/a/x");
  const auto* realm = &entry->realm();
  const auto* challenge = &entry->auth_challenge();
  const auto* credentials = &entry->credentials();
  for (int i = 0; i < 16; ++i) {
    cache.Add(origin, HttpAuth::AUTH_SERVER, std::to_string(i),
              HttpAuth::AUTH_SCHEME_BASIC, {}, "x", {}, "/");
  }
  CHECK(entry == cache.Lookup(origin, HttpAuth::AUTH_PROXY, "r",
                              HttpAuth::AUTH_SCHEME_BASIC, {}));
  CHECK_EQ(realm, &entry->realm());
  CHECK_EQ(challenge, &entry->auth_challenge());
  CHECK_EQ(credentials, &entry->credentials());
  cache.Add(origin, HttpAuth::AUTH_PROXY, "r", HttpAuth::AUTH_SCHEME_BASIC, {},
            "c", entry->credentials(), "/b/x");
  cache.Add(origin, HttpAuth::AUTH_PROXY, "r", HttpAuth::AUTH_SCHEME_BASIC, {},
            "c", entry->credentials(), "/c/x");
  entry->IncrementNonceCount();
  cache.CopyProxyEntriesFrom(cache);
  CHECK_EQ(2, entry->IncrementNonceCount());
  CHECK_EQ(realm, &entry->realm());
  CHECK_EQ(credentials, &entry->credentials());
}
TEST(HttpAuthCacheRustTest, MaximumTimestampFullClear) {
  HttpAuthCache cache(false);
  HttpAuthCacheOriginal original(false);
  HttpAuthCacheRustClock r, c;
  r.value = c.value = INT64_MAX;
  cache.set_clock_for_testing(&r);
  original.set_clock_for_testing(&c);
  const url::SchemeHostPort origin(GURL("http://a.test"));
  cache.Add(origin, HttpAuth::AUTH_SERVER, "r", HttpAuth::AUTH_SCHEME_BASIC, {},
            "c", {}, "/");
  original.Add(origin, HttpAuth::AUTH_SERVER, "r", HttpAuth::AUTH_SCHEME_BASIC,
               {}, "c", {}, "/");
  CHECK_EQ(
      cache.ClearEntriesAddedBetween(base::Time::Min(), base::Time::Max(), {}),
      original.ClearEntriesAddedBetween(base::Time::Min(), base::Time::Max(),
                                        {}));
  CHECK_EQ(0u, cache.GetEntriesSizeForTesting());
}

TEST(HttpAuthCacheRustTest, AllBytesAndUtf16Credentials) {
  HttpAuthCache rust(true);
  HttpAuthCacheOriginal cpp(true);
  HttpAuthCacheRustTick rt, ct;
  HttpAuthCacheRustClock rw, cw;
  rust.set_tick_clock_for_testing(&rt);
  cpp.set_tick_clock_for_testing(&ct);
  rust.set_clock_for_testing(&rw);
  cpp.set_clock_for_testing(&cw);
  const url::SchemeHostPort origin(GURL("https://auth.test"));
  for (unsigned byte = 0; byte < 256; ++byte) {
    const std::string realm{char(byte), char(0), char(255 - byte)};
    const std::string challenge{char(0), char(byte)};
    const std::string path = std::string("/a") + char(byte) + "/file";
    const AuthCredentials credentials(
        std::u16string{0, char16_t(byte), char16_t(0xd800 + byte)},
        std::u16string{char16_t(0xdc00 + byte), 0, char16_t(0xffff - byte)});
    auto* r =
        rust.Add(origin, HttpAuth::AUTH_SERVER, realm,
                 HttpAuth::AUTH_SCHEME_BASIC, {}, challenge, credentials, path);
    auto* c =
        cpp.Add(origin, HttpAuth::AUTH_SERVER, realm,
                HttpAuth::AUTH_SCHEME_BASIC, {}, challenge, credentials, path);
    HttpAuthCacheRustCheckEntry(r, c);
    CHECK_EQ(r->IncrementNonceCount(), c->IncrementNonceCount());
    CHECK_EQ(rust.Remove(origin, HttpAuth::AUTH_SERVER, realm,
                         HttpAuth::AUTH_SCHEME_BASIC, {}, AuthCredentials()),
             cpp.Remove(origin, HttpAuth::AUTH_SERVER, realm,
                        HttpAuth::AUTH_SCHEME_BASIC, {}, AuthCredentials()));
    HttpAuthCacheRustTestPeer::AddPath(*r, path);
    HttpAuthCacheRustTestPeer::AddPath(*c, path);
    CHECK(HttpAuthCacheRustTestPeer::States(rust) ==
          HttpAuthCacheRustTestPeer::States(cpp));
  }
}

class HttpAuthCacheRustReentrantTick final : public base::TickClock {
 public:
  base::TimeTicks NowTicks() const override;
  mutable int64_t value = 1000;
  mutable bool armed = false;
  mutable int skip = 0;
  base::RepeatingClosure action;
};
base::TimeTicks HttpAuthCacheRustReentrantTick::NowTicks() const {
  if (armed) {
    if (skip > 0) {
      --skip;
    } else {
      armed = false;
      action.Run();
    }
  }
  return base::TimeTicks() + base::Microseconds(value++);
}
template <class Cache>
void HttpAuthCacheRustInsert(Cache* cache, const url::SchemeHostPort& origin) {
  cache->Add(origin, HttpAuth::AUTH_SERVER, "a", HttpAuth::AUTH_SCHEME_BASIC,
             {}, "a", {}, "/");
}
template <class Cache>
bool HttpAuthCacheRustFilter(Cache* cache,
                             const url::SchemeHostPort& origin,
                             const GURL& url) {
  HttpAuthCacheRustInsert(cache, origin);
  return true;
}
TEST(HttpAuthCacheRustTest, ReentrantClockKeepsSelectedNode) {
  HttpAuthCache rust(false);
  HttpAuthCacheOriginal cpp(false);
  HttpAuthCacheRustReentrantTick rt, ct;
  HttpAuthCacheRustClock rw, cw;
  rust.set_tick_clock_for_testing(&rt);
  cpp.set_tick_clock_for_testing(&ct);
  rust.set_clock_for_testing(&rw);
  cpp.set_clock_for_testing(&cw);
  const url::SchemeHostPort z(GURL("https://z.test")), a(GURL("http://a.test"));
  rt.action =
      base::BindRepeating(&HttpAuthCacheRustInsert<HttpAuthCache>, &rust, a);
  ct.action = base::BindRepeating(
      &HttpAuthCacheRustInsert<HttpAuthCacheOriginal>, &cpp, a);
  auto* r = rust.Add(z, HttpAuth::AUTH_SERVER, "z", HttpAuth::AUTH_SCHEME_BASIC,
                     {}, "z", {}, "/");
  auto* c = cpp.Add(z, HttpAuth::AUTH_SERVER, "z", HttpAuth::AUTH_SCHEME_BASIC,
                    {}, "z", {}, "/");
  rt.armed = ct.armed = true;
  CHECK(r == rust.Lookup(z, HttpAuth::AUTH_SERVER, "z",
                         HttpAuth::AUTH_SCHEME_BASIC, {}));
  CHECK(c == cpp.Lookup(z, HttpAuth::AUTH_SERVER, "z",
                        HttpAuth::AUTH_SCHEME_BASIC, {}));
  CHECK(HttpAuthCacheRustTestPeer::States(rust) ==
        HttpAuthCacheRustTestPeer::States(cpp));
}
TEST(HttpAuthCacheRustTest, ReentrantFilterKeepsIteratorOrder) {
  HttpAuthCache rust(false);
  HttpAuthCacheOriginal cpp(false);
  HttpAuthCacheRustTick rt, ct;
  HttpAuthCacheRustClock rw, cw;
  rust.set_tick_clock_for_testing(&rt);
  cpp.set_tick_clock_for_testing(&ct);
  rust.set_clock_for_testing(&rw);
  cpp.set_clock_for_testing(&cw);
  const url::SchemeHostPort z(GURL("https://z.test")), a(GURL("http://a.test"));
  rust.Add(z, HttpAuth::AUTH_SERVER, "z", HttpAuth::AUTH_SCHEME_BASIC, {}, "z",
           {}, "/");
  cpp.Add(z, HttpAuth::AUTH_SERVER, "z", HttpAuth::AUTH_SCHEME_BASIC, {}, "z",
          {}, "/");
  // Inserting before the current iterator is permitted by std::multimap and
  // must not cause the new node to be visited or the selected node to survive.
  CHECK_EQ(rust.ClearEntriesAddedBetween(
               base::Time::Min(), base::Time::Max(),
               base::BindRepeating(&HttpAuthCacheRustFilter<HttpAuthCache>,
                                   &rust, a)),
           cpp.ClearEntriesAddedBetween(
               base::Time::Min(), base::Time::Max(),
               base::BindRepeating(
                   &HttpAuthCacheRustFilter<HttpAuthCacheOriginal>, &cpp, a)));
  CHECK(HttpAuthCacheRustTestPeer::States(rust) ==
        HttpAuthCacheRustTestPeer::States(cpp));
  CHECK_EQ(1u, rust.GetEntriesSizeForTesting());
}

TEST(HttpAuthCacheRustTest, RemoveWithAliasedEntryReferences) {
  HttpAuthCache rust(false);
  HttpAuthCacheOriginal cpp(false);
  const url::SchemeHostPort origin(GURL("https://a.test"));
  auto* r = rust.Add(origin, HttpAuth::AUTH_SERVER, "realm",
                     HttpAuth::AUTH_SCHEME_BASIC, {}, "challenge",
                     AuthCredentials(u"user", u"pw"), "/");
  auto* c = cpp.Add(origin, HttpAuth::AUTH_SERVER, "realm",
                    HttpAuth::AUTH_SCHEME_BASIC, {}, "challenge",
                    AuthCredentials(u"user", u"pw"), "/");
  CHECK(rust.Remove(r->scheme_host_port(), HttpAuth::AUTH_SERVER, r->realm(),
                    r->scheme(), {}, r->credentials()));
  CHECK(cpp.Remove(c->scheme_host_port(), HttpAuth::AUTH_SERVER, c->realm(),
                   c->scheme(), {}, c->credentials()));
  CHECK_EQ(0u, rust.GetEntriesSizeForTesting());
  CHECK_EQ(0u, cpp.GetEntriesSizeForTesting());
}

template <class Cache>
void HttpAuthCacheRustRemoveFirst(Cache* cache,
                                  const url::SchemeHostPort& origin) {
  CHECK(cache->Remove(origin, HttpAuth::AUTH_SERVER, "0",
                      HttpAuth::AUTH_SCHEME_BASIC, {}, {}));
}
TEST(HttpAuthCacheRustTest, EvictionClockMayRemoveUnrelatedEntry) {
  HttpAuthCache rust(false);
  HttpAuthCacheOriginal cpp(false);
  HttpAuthCacheRustReentrantTick rt, ct;
  HttpAuthCacheRustClock rw, cw;
  rust.set_tick_clock_for_testing(&rt);
  cpp.set_tick_clock_for_testing(&ct);
  rust.set_clock_for_testing(&rw);
  cpp.set_clock_for_testing(&cw);
  const url::SchemeHostPort origin(GURL("https://a.test"));
  for (int i = 0; i < 20; ++i) {
    rust.Add(origin, HttpAuth::AUTH_SERVER, std::to_string(i),
             HttpAuth::AUTH_SCHEME_BASIC, {}, "c", {}, "/");
    cpp.Add(origin, HttpAuth::AUTH_SERVER, std::to_string(i),
            HttpAuth::AUTH_SCHEME_BASIC, {}, "c", {}, "/");
  }
  rt.action = base::BindRepeating(&HttpAuthCacheRustRemoveFirst<HttpAuthCache>,
                                  &rust, origin);
  ct.action = base::BindRepeating(
      &HttpAuthCacheRustRemoveFirst<HttpAuthCacheOriginal>, &cpp, origin);
  rt.armed = ct.armed = true;
  rt.skip = ct.skip = 1;
  // Skip Add's initial tick; fire inside the eviction clock read.
  rust.Add(origin, HttpAuth::AUTH_SERVER, "new", HttpAuth::AUTH_SCHEME_BASIC,
           {}, "c", {}, "/");
  cpp.Add(origin, HttpAuth::AUTH_SERVER, "new", HttpAuth::AUTH_SCHEME_BASIC, {},
          "c", {}, "/");
  CHECK_EQ(19u, rust.GetEntriesSizeForTesting());
  CHECK(HttpAuthCacheRustTestPeer::States(rust) ==
        HttpAuthCacheRustTestPeer::States(cpp));
}
}  // namespace net
