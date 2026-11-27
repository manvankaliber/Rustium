// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_FAMILY_TEST_SUPPORT_H_
#define NET_COOKIES_COOKIE_FAMILY_TEST_SUPPORT_H_
#include <array>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_access_params.h"
#include "net/cookies/cookie_partition_key_collection.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/cookie_family_original/cookies/canonical_cookie.h"
#include "net/cookies/cookie_family_original/cookies/cookie_access_params.h"
#include "net/cookies/cookie_family_original/cookies/cookie_partition_key_collection.h"
#include "net/cookies/cookie_family_original/cookies/cookie_util.h"
#include "url/gurl.h"
#include "url/origin.h"
namespace net::cookie_family_test {
namespace original=net::cookie_family_original;
struct Case {
  const char* id;const char* label;const char* url;const char* line;
  bool cross_site;bool redirect;bool main;bool partitioned;bool nonce;
};
inline constexpr std::array<Case,10> kCases={{
  {"create_include_domain","Canonical creation, domain/path, inclusion and output","https://sub.example.com/a/b","name=value; Domain=example.com; Path=/a; SameSite=Lax",false,false,false,false,false},
  {"secure_prefix","Secure prefix creation and access","https://example.com/a/b","__Secure-name=value; Secure; SameSite=None; Path=/",false,false,false,false,false},
  {"host_http_prefix","Host-Http prefix, HttpOnly and attributes","https://example.com/a/b","__Host-Http-name=value; Secure; HttpOnly; Path=/; SameSite=Strict",false,false,false,false,false},
  {"file_expiration","File cookie with expiration and native calendar","file://server/a/b","name=value; Expires=Wed, 09 Jun 2027 10:18:14 GMT; Path=/a",false,false,false,false,false},
  {"invalid_domain","Rejected domain and complete failure status","https://example.com/a/b","name=value; Domain=other.example",false,false,false,false,false},
  {"hidden_prefix","Nameless hidden-prefix rejection","https://example.com/a/b","=__Host-Http-name; Secure; HttpOnly; Path=/",false,false,false,false,false},
  {"strict_redirect","Strict cookie, cross-site redirect and unsafe request","https://example.com/a/b","name=value; Secure; SameSite=Strict; Path=/",false,true,true,false,false},
  {"partitioned","Partition key/collection, canonicalization and inclusion","https://example.com/a/b","name=value; Secure; SameSite=None; Partitioned; Path=/",true,false,false,true,false},
  {"nonced_partition","Nonced key and native token ownership","https://example.com/a/b","name=value; Secure; SameSite=None; Partitioned; Path=/",true,false,false,true,true},
  {"expiry_and_quote","Expiration policy plus request-cookie raw quote handling","https://example.com/a/b","name=\"quoted\"; Expires=Tue, 19 Jan 2038 03:14:07 GMT; Path=/a",false,false,false,false,false},
}};
inline uint64_t Hash(uint64_t seed,std::string_view bytes) {
  for(unsigned char byte:bytes) seed=(seed^byte)*1099511628211ULL;return seed;
}
template<class Result> std::string AccessDigest(const Result& result) {
  return result.status.GetDebugString()+":"+std::to_string(static_cast<int>(result.effective_same_site))+":"+
    std::to_string(static_cast<int>(result.access_semantics))+":"+std::to_string(static_cast<int>(result.scope_semantics))+":"+
    std::to_string(result.is_allowed_to_access_secure_cookies);
}
template<class Context> std::string ContextDigest(const Context& context) {
  auto metadata=[](const auto& m) {return std::to_string(static_cast<int>(m.cross_site_redirect_downgrade))+":"+
    std::to_string(m.has_null_initiator)+":"+std::to_string(static_cast<int>(m.redirect_type_bug_1221316));};
  return std::to_string(static_cast<int>(context.context()))+":"+std::to_string(static_cast<int>(context.schemeful_context()))+":"+
    metadata(context.metadata())+":"+metadata(context.schemeful_metadata());
}
// Namespace families are explicit, so the original pipeline never converts
// into production CookieBase/ParsedCookie/Status/Key objects before timing.
#define COOKIE_FAMILY_TRAITS(Name,Namespace) \
struct Name { \
  using Access=Namespace::CookieAccessResult; \
  using Base=Namespace::CookieBase;using SameSite=Namespace::CookieSameSite;using SourceScheme=Namespace::CookieSourceScheme; \
  using Cookie=Namespace::CanonicalCookie;using Status=Namespace::CookieInclusionStatus; \
  using Key=Namespace::CookiePartitionKey;using Collection=Namespace::CookiePartitionKeyCollection; \
  using Site=Namespace::SchemefulSite;using Sfc=Namespace::SiteForCookies; \
  using Options=Namespace::CookieOptions;using Params=Namespace::CookieAccessParams; \
  using AccessSemantics=Namespace::CookieAccessSemantics;using ScopeSemantics=Namespace::CookieScopeSemantics; \
  static std::unique_ptr<Cookie> Create(const GURL& url,std::string_view line,std::optional<Key> key,Status* status) { \
    return Cookie::Create(url,line,base::Time::FromInternalValue(13380163200000000LL),std::nullopt,std::move(key),Namespace::CookieSourceType::kOther,status); \
  } \
  static auto Context(const std::vector<GURL>& chain,const Sfc& site,const std::optional<url::Origin>& initiator,bool main,bool redirect) { \
    return Namespace::cookie_util::ComputeSameSiteContextForRequest(redirect?"POST":"GET",chain,site,initiator,main,false,false); \
  } \
  static Params AccessParams() {return Params(Namespace::CookieAccessSemantics::NONLEGACY,Namespace::CookieScopeSemantics::NONLEGACY,false);} \
  static std::string RequestLine(std::string_view input) {Namespace::cookie_util::ParsedRequestCookies pairs;Namespace::cookie_util::ParseRequestCookieLine(input,&pairs);return Namespace::cookie_util::SerializeRequestCookieLine(pairs);} \
};
COOKIE_FAMILY_TRAITS(Production,net)
COOKIE_FAMILY_TRAITS(Original,net::cookie_family_original)
#undef COOKIE_FAMILY_TRAITS

template<class Family> uint64_t Pipeline(const Case& input,uint64_t seed) {
  using Cookie=typename Family::Cookie;using Key=typename Family::Key;
  using Bit=typename Key::AncestorChainBit;
  const GURL url(input.url);
  const auto token=base::UnguessableToken::Deserialize(0x123456789abcdef0ULL,0x0fedcba987654321ULL).value();
  const auto key=Key::FromURLForTesting(GURL("https://top.example"),Bit::kSameSite,
      input.nonce?std::optional(token):std::nullopt);
  auto collection=Family::Collection::MatchesSite(typename Family::Site(GURL("https://top.example")));
  seed=Hash(seed,std::to_string(collection.Contains(key)));
  std::ostringstream key_output;key_output<<collection;seed=Hash(seed,key_output.str());
  const auto serialized=Key::Serialize(key);
  seed=Hash(seed,serialized?serialized->GetDebugString():serialized.error());
  if(key.nonce()) seed=Hash(seed,key.nonce()->ToString());
  typename Family::Status status;
  auto cookie=Family::Create(url,input.line,input.partitioned?std::optional(key):std::nullopt,&status);
  seed=Hash(seed,status.GetDebugString());
  if(cookie) {
    auto options=Family::Options::MakeAllInclusive();
    const auto site=input.cross_site?typename Family::Sfc():Family::Sfc::FromUrl(url);
    std::vector<GURL> chain={url};if(input.redirect) {chain.push_back(GURL("https://other.test"));chain.push_back(url);}
    const std::optional<url::Origin> initiator=url::Origin::Create(input.cross_site?GURL("https://other.test"):url);
    const auto context=Family::Context(chain,site,initiator,input.main,input.redirect);
    options.set_same_site_cookie_context(context);seed=Hash(seed,ContextDigest(context));
    const auto params=Family::AccessParams();
    const auto access=cookie->IncludeForRequestURL(url,options,params);
    const auto set=cookie->IsSetPermittedInContext(url,options,params,{"http","https","file"});
    seed=Hash(seed,AccessDigest(access));seed=Hash(seed,AccessDigest(set));
    seed=Hash(seed,std::to_string(access==set));
    seed=Hash(seed,cookie->DebugString());seed=Hash(seed,Cookie::BuildCookieAttributesLine(*cookie));
    seed=Hash(seed,std::to_string(cookie->IsSecure())+std::to_string(cookie->IsFirstPartyPartitioned())+std::to_string(cookie->IsThirdPartyPartitioned()));
    std::vector<Cookie> cookies;cookies.push_back(*cookie);seed=Hash(seed,Cookie::BuildCookieLine(cookies));
    auto copy=*cookie;copy=*cookie;auto moved=std::move(copy);seed=Hash(seed,moved.DebugString());
    seed=Hash(seed,std::to_string(cookie->IsEquivalent(moved))+std::to_string(*cookie==moved));
  }
  seed=Hash(seed,Family::RequestLine("a=one; b=\"quoted\"; raw=\xff; bad=\"unterminated"));
  return seed;
}
}  // namespace net::cookie_family_test
#endif
