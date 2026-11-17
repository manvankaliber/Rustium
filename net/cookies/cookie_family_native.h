// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_FAMILY_NATIVE_H_
#define NET_COOKIES_COOKIE_FAMILY_NATIVE_H_
#include "base/memory/raw_ptr.h"
#include <optional>
#include <exception>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include "net/cookies/cookie_family_ffi.h"
#include "net/cookies/cookie_access_params.h"
#include "net/cookies/cookie_access_result.h"
#include "net/cookies/cookie_base.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_partition_key_collection.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/canonical_cookie.h"
#include "url/gurl.h"
namespace net {
// Narrow friendship for native storage facts, never CookiePartitionKey policy.
// In particular IsThirdParty cannot substitute for an arbitrary stored raw bit.
struct CookieFamilyNativeAccess {
  static int32_t Ancestor(const CookiePartitionKey& key) {
    return static_cast<int32_t>(key.ancestor_chain_bit_);
  }
  static const base::flat_set<CookiePartitionKey>* Keys(
      const CookiePartitionKeyCollection& collection) {
    return collection.state_ ? &*collection.state_ : nullptr;
  }
  static int64_t AgeThreshold(const CookieBase& cookie) {
    return cookie.GetLaxAllowUnsafeThresholdAge().InMicroseconds();
  }
  static void PostInclude(const CookieBase& cookie,const CookieAccessResult& access,
      const CookieOptions& options,CookieOptions::SameSiteCookieContext::ContextType context) {
    cookie.PostIncludeForRequestURL(access,options,context);
  }
  static void PostSet(const CookieBase& cookie,const CookieAccessResult& access,
      const CookieOptions& options) {
    cookie.PostIsSetPermittedInContext(access,options);
  }
};
namespace cookie_family {
CookieFamilyView View(std::string_view);
std::string_view StringView(CookieFamilyView);
std::string Take(CookieFamilyBuffer);
CookieFamilyUrl Url(const GURL&);
CookieFamilyContext Snapshot(const CookieOptions::SameSiteCookieContext&);
CookieOptions::SameSiteCookieContext Materialize(CookieFamilyContext);
CookieFamilyOptions Snapshot(const CookieOptions&);
CookieFamilyAccess Snapshot(const CookieAccessResult&);
CookieAccessResult Materialize(CookieFamilyAccess);
CookieInclusionStatus Materialize(CookieInclusionStatusRustState);
CookieFamilyAccess SnapshotStatus(const CookieInclusionStatus&);
struct NativeContext {
  std::exception_ptr exception;
  raw_ptr<const GURL> url=nullptr;
  raw_ptr<const CookieBase> cookie=nullptr;
  raw_ptr<const CookieOptions> options=nullptr;
  raw_ptr<const CookieAccessParams> params=nullptr;
  raw_ptr<const std::vector<std::string>> cookieable_schemes=nullptr;
  int64_t (*threshold)(const CookieBase*)=nullptr;
  raw_ptr<const std::vector<GURL>> chain=nullptr;
  raw_ptr<const SiteForCookies> site_for_cookies=nullptr;
  raw_ptr<const std::optional<url::Origin>> initiator=nullptr;
  raw_ptr<const url::Origin> request_initiator=nullptr;
  raw_ptr<cookie_util::ParsedRequestCookies> parsed_output=nullptr;
  raw_ptr<const cookie_util::ParsedRequestCookies> parsed_input=nullptr;
  raw_ptr<const CookieAccessResultList> included=nullptr;
  raw_ptr<const CookieAccessResultList> excluded=nullptr;
  raw_ptr<CookieList> cookie_output=nullptr;
  raw_ptr<const CookiePartitionKeyCollection> collection=nullptr;
  raw_ptr<const CookiePartitionKeyCollection> other_collection=nullptr;
  raw_ptr<const CookiePartitionKey> partition=nullptr;
  raw_ptr<const CookiePartitionKey> compare_left=nullptr;
  raw_ptr<const CookiePartitionKey> compare_right=nullptr;
  raw_ptr<const SchemefulSite> site=nullptr;
  raw_ptr<base::flat_set<CookiePartitionKey>> key_output=nullptr;
  raw_ptr<std::ostream> stream=nullptr;
  std::optional<base::flat_set<CookiePartitionKey>::const_iterator> iterator;
  std::optional<base::flat_set<CookiePartitionKey>::const_iterator> end;
  bool iterator_started=false;
  raw_ptr<const CookieOptions::SameSiteCookieContext> other_context=nullptr;
  std::string scratch;
};
void Native(void*,const CookieFamilyCall*,CookieFamilyNativeResult*);
CookieFamilyResult Run(uint32_t,const CookieFamilyRequest&,NativeContext* context=nullptr);
}  // namespace cookie_family
}  // namespace net
#endif
