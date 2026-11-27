#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_CACHE_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_CACHE_H_
#include "net/http/http_auth_cache.h"
namespace net {
// No cache owner or adapter storage. Access only via native mutable pointers.
class ConnectionAuthOriginalCache {
 public:
  using Entry = HttpAuthCache::Entry;
  using EntryMap = HttpAuthCache::EntryMap;
  using EntryMapKey = HttpAuthCache::EntryMapKey;
  using PathList = Entry::PathList;
  static constexpr int kMaxNumRealmEntries = HttpAuthCache::kMaxNumRealmEntries;
  static constexpr int kMaxNumPathsPerRealmEntry = HttpAuthCache::kMaxNumPathsPerRealmEntry;
  static int IncrementNonceCount(Entry* self) { return ++self->nonce_count_; }
  static EntryMapKey MakeKey(const url::SchemeHostPort& origin, HttpAuth::Target target,
      const NetworkAnonymizationKey& nak, bool keyed) {
    return EntryMapKey(origin, target, nak, target == HttpAuth::AUTH_SERVER && keyed);
  }
  static void SetKeyServerEntriesByNetworkAnonymizationKey(HttpAuthCache* self,
    bool key_server_entries_by_network_anonymization_key);
  static HttpAuthCache::Entry* Lookup(HttpAuthCache* self,
    const url::SchemeHostPort& scheme_host_port,
    HttpAuth::Target target,
    const std::string& realm,
    HttpAuth::Scheme scheme,
    const NetworkAnonymizationKey& network_anonymization_key);
  static HttpAuthCache::Entry* LookupByPath(HttpAuthCache* self,
    const url::SchemeHostPort& scheme_host_port,
    HttpAuth::Target target,
    const NetworkAnonymizationKey& network_anonymization_key,
    const std::string& path);
  static HttpAuthCache::Entry* Add(HttpAuthCache* self,
    const url::SchemeHostPort& scheme_host_port,
    HttpAuth::Target target,
    const std::string& realm,
    HttpAuth::Scheme scheme,
    const NetworkAnonymizationKey& network_anonymization_key,
    const std::string& auth_challenge,
    const AuthCredentials& credentials,
    const std::string& path);
  static bool Remove(HttpAuthCache* self,
    const url::SchemeHostPort& scheme_host_port,
    HttpAuth::Target target,
    const std::string& realm,
    HttpAuth::Scheme scheme,
    const NetworkAnonymizationKey& network_anonymization_key,
    const AuthCredentials& credentials);
  static bool ClearEntriesAddedBetween(HttpAuthCache* self,
    base::Time begin_time,
    base::Time end_time,
    base::RepeatingCallback<bool(const GURL&)> url_matcher);
  static bool ClearAllEntries(HttpAuthCache* self);
  static bool UpdateStaleChallenge(HttpAuthCache* self,
    const url::SchemeHostPort& scheme_host_port,
    HttpAuth::Target target,
    const std::string& realm,
    HttpAuth::Scheme scheme,
    const NetworkAnonymizationKey& network_anonymization_key,
    const std::string& auth_challenge);
  static void CopyProxyEntriesFrom(HttpAuthCache* self,
    const HttpAuthCache& other);
  static size_t GetEntriesSizeForTesting(HttpAuthCache* self);
  static HttpAuthCache::EntryMap::iterator LookupEntryIt(HttpAuthCache* self,
    const url::SchemeHostPort& scheme_host_port,
    HttpAuth::Target target,
    const std::string& realm,
    HttpAuth::Scheme scheme,
    const NetworkAnonymizationKey& network_anonymization_key);
  static void EvictLeastRecentlyUsedEntry(HttpAuthCache* self);
  static void EntryUpdateStaleChallenge(Entry* self,
    const std::string& auth_challenge);
  static bool EntryIsEqualForTesting(const Entry* self,
    const Entry& other);
  static void EntryAddPath(Entry* self,
    const std::string& path);
  static bool EntryHasEnclosingPath(Entry* self,
    const std::string& dir,
                                                    size_t* path_len);
};
}
#endif
