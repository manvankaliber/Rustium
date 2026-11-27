// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_cache.h"

#include <algorithm>
#include <iterator>
#include <tuple>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "net/http/http_auth_cache_rust_ffi.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace net {

// Native storage/value/clock services only. All cache policy, including key
// partition selection and traversal order, is in http_auth_cache_rust_core.rs.
// Nodes and their value subobjects retain their original C++ ownership, so an
// Entry pointer or field reference is stable until that exact node is erased.
struct HttpAuthCacheRustServices {
  enum Scalar : uint32_t {
    Count,
    CompareKey,
    Target,
    Scheme,
    PathCount,
    PathSwap,
    PathErase,
    PathPop,
    Nonce,
    SetNonce,
    LastUse,
    SetLastUse,
    Creation,
    SetCreation,
    SetCreationTicks,
    Tick,
    Clock,
    Erase,
    Insert,
    SetIdentity,
    SetValues,
    SetChallenge,
    CredentialEquals,
    UrlMatches,
    Keyed,
    SetKeyed,
    SourceFirst,
    SourceNext,
    SourceTarget,
    BindSource,
    OriginEquals,
    CredentialsEqual,
    HasMatcher,
    At,
    First,
    Next,
    WarnCacheLimit,
    WarnPathLimit,
  };
  enum Bytes : uint32_t { QueryRealm, QueryPath, Realm, Path, Challenge };
  enum Operation : uint32_t {
    Lookup,
    LookupPath,
    Add,
    Remove,
    ClearBetween,
    ClearAll,
    Stale,
    SetPartition,
    CopyProxy,
    IncrementNonce,
    EntryStale,
    AddPath,
    HasPath,
    Equal,
  };
  using Entry = HttpAuthCache::Entry;
  using Map = HttpAuthCache::EntryMap;
  // Synchronous stack-only borrows. A successful Remove may erase the Entry
  // whose field references were supplied as arguments; they are never read
  // after erasure. raw_ptr would change allocation tracking/dangling detection
  // for this valid public alias case. No service retains these pointers.
  RAW_PTR_EXCLUSION HttpAuthCache* cache = nullptr;
  RAW_PTR_EXCLUSION const HttpAuthCache* source = nullptr;
  RAW_PTR_EXCLUSION Entry* direct = nullptr;
  RAW_PTR_EXCLUSION const Entry* source_entry = nullptr;
  RAW_PTR_EXCLUSION const url::SchemeHostPort* origin = nullptr;
  RAW_PTR_EXCLUSION const NetworkAnonymizationKey* nak = nullptr;
  RAW_PTR_EXCLUSION const std::string* realm = nullptr;
  RAW_PTR_EXCLUSION const std::string* path = nullptr;
  RAW_PTR_EXCLUSION const std::string* challenge = nullptr;
  RAW_PTR_EXCLUSION const AuthCredentials* credentials = nullptr;
  HttpAuth::Target target = HttpAuth::AUTH_NONE;
  HttpAuth::Scheme scheme = HttpAuth::AUTH_SCHEME_MAX;
  base::RepeatingCallback<bool(const GURL&)> matcher;

  // Opaque integer handles cross Rust without creating any Rust references to
  // native storage. Only handles returned by Handle() are accepted here.
  static int64_t Handle(const Entry* entry) {
    return static_cast<int64_t>(reinterpret_cast<intptr_t>(entry));
  }
  auto It(int64_t id) {
    return std::find_if(
        cache->entries_.begin(), cache->entries_.end(),
        [id](const auto& pair) { return Handle(&pair.second) == id; });
  }
  int64_t HandleIt(Map::iterator it) {
    return it == cache->entries_.end() ? -1 : Handle(&it->second);
  }
  auto SourceIt(int64_t id) const {
    return std::find_if(
        source->entries_.begin(), source->entries_.end(),
        [id](const auto& pair) { return Handle(&pair.second) == id; });
  }
  const Entry& Read(int64_t id) const {
    if (id == -1) {
      return *direct;
    }
    if (id == -2) {
      return *source_entry;
    }
    // SAFETY: id is an opaque handle produced by Handle(). Native map/list
    // nodes retain identity across unrelated insertions, including reentrant
    // clock/filter callbacks. Rust never dereferences or borrows this pointer.
    return *reinterpret_cast<const Entry*>(static_cast<intptr_t>(id));
  }
  Entry& Write(int64_t id) {
    return id == -1 ? *direct
                    : *reinterpret_cast<Entry*>(static_cast<intptr_t>(id));
  }

  static int64_t ScalarCall(void* context,
                            uint32_t operation,
                            int64_t id,
                            int64_t a,
                            int64_t b) {
    auto& s = *static_cast<HttpAuthCacheRustServices*>(context);
    switch (operation) {
      case WarnCacheLimit:
        DLOG(WARNING) << "Num auth cache entries reached limit -- evicting";
        return 0;
      case WarnPathLimit:
        DLOG(WARNING) << "Num path entries for "
                      << s.Read(id).scheme_host_port()
                      << " has grown too large -- evicting";
        return 0;
      case At:
        return Handle(&std::next(s.cache->entries_.begin(), id)->second);
      case First:
        return s.HandleIt(s.cache->entries_.begin());
      case Next:
        return s.HandleIt(std::next(s.It(id)));
      case HasMatcher:
        return !!s.matcher;
      case Count:
        return s.cache->entries_.size();
      case CompareKey: {
        HttpAuthCache::EntryMapKey key(*s.origin, s.target, *s.nak, a != 0);
        const auto& stored = s.It(id)->first;
        return stored < key ? -1 : key < stored ? 1 : 0;
      }
      case Target:
        return s.It(id)->first.target;
      case Scheme:
        return s.Read(id).scheme_;
      case PathCount:
        return s.Read(id).paths_.size();
      case PathSwap: {
        auto& paths = s.Write(id).paths_;
        std::iter_swap(std::next(paths.begin(), a),
                       std::next(paths.begin(), b));
        return 0;
      }
      case PathErase: {
        auto& paths = s.Write(id).paths_;
        paths.erase(std::next(paths.begin(), a));
        return 0;
      }
      case PathPop:
        s.Write(id).paths_.pop_back();
        return 0;
      case Nonce:
        return s.Read(id).nonce_count_;
      case SetNonce:
        s.Write(id).nonce_count_ = static_cast<int>(a);
        return 0;
      case LastUse:
        return s.Read(id).last_use_time_ticks_.since_origin().InMicroseconds();
      case SetLastUse:
        s.Write(id).last_use_time_ticks_ =
            base::TimeTicks() + base::Microseconds(a);
        return 0;
      case Creation:
        return s.Read(id).creation_time_.ToInternalValue();
      case SetCreation:
        s.Write(id).creation_time_ = base::Time::FromInternalValue(a);
        return 0;
      case SetCreationTicks:
        s.Write(id).creation_time_ticks_ =
            base::TimeTicks() + base::Microseconds(a);
        return 0;
      case Tick:
        return s.cache->tick_clock_->NowTicks().since_origin().InMicroseconds();
      case Clock:
        return s.cache->clock_->Now().ToInternalValue();
      case Erase:
        return s.HandleIt(s.cache->entries_.erase(s.It(id)));
      case Insert: {
        auto it = s.cache->entries_.insert(
            {HttpAuthCache::EntryMapKey(*s.origin, s.target, *s.nak, a != 0),
             Entry()});
        return Handle(&it->second);
      }
      case SetIdentity: {
        auto& e = s.Write(id);
        e.scheme_host_port_ = *s.origin;
        e.realm_ = *s.realm;
        e.scheme_ = s.scheme;
        return 0;
      }
      case SetValues: {
        auto& e = s.Write(id);
        e.auth_challenge_ = *s.challenge;
        e.credentials_ = *s.credentials;
        return 0;
      }
      case SetChallenge:
        s.Write(id).auth_challenge_ = *s.challenge;
        return 0;
      case CredentialEquals:
        return s.credentials->Equals(s.Read(id).credentials_);
      case UrlMatches:
        return !s.matcher ||
               s.matcher.Run(s.Read(id).scheme_host_port_.GetURL());
      case Keyed:
        return s.cache->key_server_entries_by_network_anonymization_key_;
      case SetKeyed:
        s.cache->key_server_entries_by_network_anonymization_key_ = a != 0;
        return 0;
      case SourceFirst:
        return s.source->entries_.empty()
                   ? -1
                   : Handle(&s.source->entries_.begin()->second);
      case SourceNext: {
        const auto next = std::next(s.SourceIt(id));
        return next == s.source->entries_.end() ? -1 : Handle(&next->second);
      }
      case SourceTarget:
        return s.SourceIt(id)->first.target;
      case BindSource: {
        const auto& pair = *s.SourceIt(id);
        const auto& e = pair.second;
        s.source_entry = &e;
        s.origin = &e.scheme_host_port_;
        s.realm = &e.realm_;
        s.scheme = e.scheme_;
        s.target = pair.first.target;
        s.nak = &pair.first.network_anonymization_key;
        s.challenge = &e.auth_challenge_;
        s.credentials = &e.credentials_;
        s.path = &e.paths_.back();
        return s.scheme;
      }
      case OriginEquals:
        return s.Read(id).scheme_host_port_ ==
               s.source_entry->scheme_host_port_;
      case CredentialsEqual:
        return s.Read(id).credentials_.Equals(s.source_entry->credentials_);
      default:
        CHECK(false);
    }
  }
  static HttpAuthCacheRustBytes BytesCall(const void* context,
                                          uint32_t operation,
                                          int64_t id,
                                          int64_t index) {
    const auto& s = *static_cast<const HttpAuthCacheRustServices*>(context);
    const std::string* value = nullptr;
    switch (operation) {
      case QueryRealm:
        value = s.realm;
        break;
      case QueryPath:
        value = s.path;
        break;
      case Realm:
        value = &s.Read(id).realm_;
        break;
      case Path:
        value = &*std::next(s.Read(id).paths_.begin(), index);
        break;
      case Challenge:
        value = &s.Read(id).auth_challenge_;
        break;
      default:
        CHECK(false);
    }
    return {reinterpret_cast<const uint8_t*>(value->data()), value->size()};
  }
  static void PushPath(void* context,
                       int64_t id,
                       const uint8_t* data,
                       size_t len) {
    auto& s = *static_cast<HttpAuthCacheRustServices*>(context);
    // SAFETY: synchronous Rust slice; initialized and valid for exactly len
    // bytes. std::string copies it before the callback returns. Empty input
    // uses no pointer construction, including the proxy empty path.
    s.Write(id).paths_.push_front(
        len == 0 ? std::string()
                 : UNSAFE_BUFFERS(
                       std::string(reinterpret_cast<const char*>(data), len)));
  }
  int64_t Run(Operation op, int64_t a = 0, int64_t b = 0) {
    constexpr HttpAuthCacheRustVtable table{ScalarCall, BytesCall, PushPath};
    return http_auth_cache_rust_run(this, &table, op, target, scheme, a, b);
  }
  Entry* Result(int64_t id) { return id == -1 ? nullptr : &Write(id); }
};

namespace {
void CheckOrigin(const url::SchemeHostPort& origin) {
  DCHECK(origin.IsValid());
  DCHECK(origin.scheme() == url::kHttpScheme ||
         origin.scheme() == url::kHttpsScheme ||
         origin.scheme() == url::kWsScheme ||
         origin.scheme() == url::kWssScheme);
}
void CheckPath(const std::string& path) {
  DCHECK(path.empty() || path[0] == '/');
}
}  // namespace

HttpAuthCache::HttpAuthCache(bool keyed)
    : key_server_entries_by_network_anonymization_key_(keyed) {}
HttpAuthCache::~HttpAuthCache() = default;
HttpAuthCache::Entry::Entry() = default;
HttpAuthCache::Entry::Entry(const Entry&) = default;
HttpAuthCache::Entry::~Entry() = default;

void HttpAuthCache::SetKeyServerEntriesByNetworkAnonymizationKey(bool keyed) {
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.Run(HttpAuthCacheRustServices::SetPartition, keyed);
}

HttpAuthCache::Entry* HttpAuthCache::Lookup(
    const url::SchemeHostPort& origin,
    HttpAuth::Target target,
    const std::string& realm,
    HttpAuth::Scheme scheme,
    const NetworkAnonymizationKey& nak) {
  CheckOrigin(origin);
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.origin = &origin;
  s.target = target;
  s.realm = &realm;
  s.scheme = scheme;
  s.nak = &nak;
  return s.Result(s.Run(HttpAuthCacheRustServices::Lookup));
}
HttpAuthCache::Entry* HttpAuthCache::LookupByPath(
    const url::SchemeHostPort& origin,
    HttpAuth::Target target,
    const NetworkAnonymizationKey& nak,
    const std::string& path) {
  CheckOrigin(origin);
  CheckPath(path);
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.origin = &origin;
  s.target = target;
  s.nak = &nak;
  s.path = &path;
  return s.Result(s.Run(HttpAuthCacheRustServices::LookupPath));
}
HttpAuthCache::Entry* HttpAuthCache::Add(const url::SchemeHostPort& origin,
                                         HttpAuth::Target target,
                                         const std::string& realm,
                                         HttpAuth::Scheme scheme,
                                         const NetworkAnonymizationKey& nak,
                                         const std::string& challenge,
                                         const AuthCredentials& credentials,
                                         const std::string& path) {
  CheckOrigin(origin);
  CheckPath(path);
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.origin = &origin;
  s.target = target;
  s.realm = &realm;
  s.scheme = scheme;
  s.nak = &nak;
  s.challenge = &challenge;
  s.credentials = &credentials;
  s.path = &path;
  return s.Result(s.Run(HttpAuthCacheRustServices::Add));
}
bool HttpAuthCache::Remove(const url::SchemeHostPort& origin,
                           HttpAuth::Target target,
                           const std::string& realm,
                           HttpAuth::Scheme scheme,
                           const NetworkAnonymizationKey& nak,
                           const AuthCredentials& credentials) {
  CheckOrigin(origin);
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.origin = &origin;
  s.target = target;
  s.realm = &realm;
  s.scheme = scheme;
  s.nak = &nak;
  s.credentials = &credentials;
  return s.Run(HttpAuthCacheRustServices::Remove);
}
bool HttpAuthCache::UpdateStaleChallenge(const url::SchemeHostPort& origin,
                                         HttpAuth::Target target,
                                         const std::string& realm,
                                         HttpAuth::Scheme scheme,
                                         const NetworkAnonymizationKey& nak,
                                         const std::string& challenge) {
  CheckOrigin(origin);
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.origin = &origin;
  s.target = target;
  s.realm = &realm;
  s.scheme = scheme;
  s.nak = &nak;
  s.challenge = &challenge;
  return s.Run(HttpAuthCacheRustServices::Stale);
}
bool HttpAuthCache::ClearAllEntries() {
  HttpAuthCacheRustServices s;
  s.cache = this;
  return s.Run(HttpAuthCacheRustServices::ClearAll);
}
bool HttpAuthCache::ClearEntriesAddedBetween(
    base::Time begin,
    base::Time end,
    base::RepeatingCallback<bool(const GURL&)> matcher) {
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.matcher = std::move(matcher);
  // Min/Max with no matcher includes the representable Max creation timestamp.
  // This original public API shortcut is selected in the Rust policy below.
  return s.Run(HttpAuthCacheRustServices::ClearBetween, begin.ToInternalValue(),
               end.ToInternalValue());
}
void HttpAuthCache::CopyProxyEntriesFrom(const HttpAuthCache& other) {
  HttpAuthCacheRustServices s;
  s.cache = this;
  s.source = &other;
  s.Run(HttpAuthCacheRustServices::CopyProxy);
}
size_t HttpAuthCache::GetEntriesSizeForTesting() {
  return entries_.size();
}
int HttpAuthCache::Entry::IncrementNonceCount() {
  HttpAuthCacheRustServices s;
  s.direct = this;
  return s.Run(HttpAuthCacheRustServices::IncrementNonce);
}
void HttpAuthCache::Entry::UpdateStaleChallenge(const std::string& challenge) {
  HttpAuthCacheRustServices s;
  s.direct = this;
  s.challenge = &challenge;
  s.Run(HttpAuthCacheRustServices::EntryStale);
}
void HttpAuthCache::Entry::AddPath(const std::string& path) {
  HttpAuthCacheRustServices s;
  s.direct = this;
  s.path = &path;
  s.Run(HttpAuthCacheRustServices::AddPath);
}
bool HttpAuthCache::Entry::HasEnclosingPath(const std::string& dir,
                                            size_t* length) {
  HttpAuthCacheRustServices s;
  s.direct = this;
  s.path = &dir;
  auto result = s.Run(HttpAuthCacheRustServices::HasPath);
  if (result < 0) {
    return false;
  }
  if (length) {
    *length = result;
  }
  return true;
}
bool HttpAuthCache::Entry::IsEqualForTesting(const Entry& other) const {
  // Equal only reads; the synchronous service interface also supports writes
  // for other operations. No service in Equal writes through this pointer.
  HttpAuthCacheRustServices s;
  s.direct = const_cast<Entry*>(this);
  s.source_entry = &other;
  return s.Run(HttpAuthCacheRustServices::Equal);
}
HttpAuthCache::EntryMapKey::EntryMapKey(const url::SchemeHostPort& origin,
                                        HttpAuth::Target target,
                                        const NetworkAnonymizationKey& nak,
                                        bool use_nak)
    : scheme_host_port(origin),
      target(target),
      network_anonymization_key(use_nak ? nak : NetworkAnonymizationKey()) {}
HttpAuthCache::EntryMapKey::~EntryMapKey() = default;
bool HttpAuthCache::EntryMapKey::operator<(const EntryMapKey& other) const {
  return std::tie(scheme_host_port, target, network_anonymization_key) <
         std::tie(other.scheme_host_port, other.target,
                  other.network_anonymization_key);
}
}  // namespace net
