// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "net/cookies/cookie_util.h"
#include "net/cookies/cookie_family_native.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "net/base/features.h"
#include "net/base/url_util.h"
#include "net/cookies/parsed_cookie.h"
#include "net/cookies/cookie_access_result.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_partition_key.h"
#include "url/gurl.h"
namespace net::cookie_util {
namespace cf=cookie_family;
namespace {
CookieFamilyRequest Strings(std::string_view first={},std::string_view second={},std::string_view third={}) {
  CookieFamilyRequest request{};request.first=cf::View(first);request.second=cf::View(second);request.third=cf::View(third);return request;
}
CookieOptions::SameSiteCookieContext SameSite(uint32_t kind,std::string_view method,
    const std::vector<GURL>& chain,const SiteForCookies& site,const std::optional<url::Origin>& initiator,
    bool main,bool force,bool ignore_unsafe=false) {
  auto request=Strings(method);request.scalar[0]=kind;request.scalar[1]=main;request.scalar[2]=force;request.scalar[3]=ignore_unsafe;
  cf::NativeContext context;context.chain=&chain;context.site_for_cookies=&site;context.initiator=&initiator;
  return cf::Materialize(cf::Run(16,request,&context).context);
}
bool Feature(uint32_t kind) {CookieFamilyRequest request{};request.scalar[0]=kind;return cf::Run(17,request).scalar;}
}
bool IsCookieUrlHostValid(const GURL& url) {CookieFamilyRequest request{};request.url=cf::Url(url);return cf::Run(28,request).scalar;}
bool DomainIsHostOnly(std::string_view domain) {return cf::Run(0,Strings(domain)).scalar;}
std::string CookieDomainAsHost(std::string_view domain) {return cf::Take(cf::Run(1,Strings(domain)).buffer);}
std::string GetEffectiveDomain(std::string_view scheme,std::string_view host) {return cf::Take(cf::Run(2,Strings(scheme,host)).buffer);}
std::optional<std::string> GetCookieDomainWithString(const GURL& url,std::string_view domain,CookieInclusionStatus& status) {
  auto request=Strings(domain);request.url=cf::Url(url);request.access=cf::SnapshotStatus(status);
  request.scalar[0]=base::FeatureList::IsEnabled(features::kCookieDomainRejectNonASCII);
  auto result=cf::Run(3,request);status=cf::Materialize(result.access.status);
  if(!result.buffer.present) {cookie_family_buffer_free(result.buffer);return std::nullopt;}
  return cf::Take(result.buffer);
}
base::Time ParseCookieExpirationTime(std::string_view input) {return base::Time::FromInternalValue(cf::Run(4,Strings(input)).scalar);}
std::string CanonPathWithString(const GURL& url,std::string_view path) {return cf::Take(cf::Run(5,Strings(url.path(),path)).buffer);}
GURL CookieDomainAndPathToURL(std::string_view domain,std::string_view path,std::string_view scheme) {return GURL(cf::Take(cf::Run(6,Strings(domain,path,scheme)).buffer));}
GURL CookieDomainAndPathToURL(std::string_view domain,std::string_view path,bool https) {auto request=Strings(domain,path);request.scalar[0]=https;return GURL(cf::Take(cf::Run(26,request).buffer));}
GURL CookieDomainAndPathToURL(std::string_view domain,std::string_view path,CookieSourceScheme scheme) {auto request=Strings(domain,path);request.scalar[0]=static_cast<int32_t>(scheme);return GURL(cf::Take(cf::Run(27,request).buffer));}
GURL CookieOriginToURL(std::string_view domain,bool https) {return CookieDomainAndPathToURL(domain,"/",https);}
GURL SimulatedCookieSource(const CanonicalCookie& cookie,std::string_view scheme) {return CookieDomainAndPathToURL(cookie.Domain(),cookie.Path(),scheme);}
CookieAccessScheme ProvisionalAccessScheme(const GURL& url) {auto request=Strings();request.url=cf::Url(url);return static_cast<CookieAccessScheme>(cf::Run(7,request).scalar);}
bool IsDomainMatch(std::string_view domain,std::string_view host) {return cf::Run(8,Strings(domain,host)).scalar;}
bool IsOnPath(std::string_view cookie_path,std::string_view url_path) {return cf::Run(9,Strings(cookie_path,url_path)).scalar;}
CookiePrefix GetCookiePrefix(std::string_view name) {return static_cast<CookiePrefix>(cf::Run(10,Strings(name)).scalar);}
bool HasHiddenPrefixName(std::string_view value) {return cf::Run(11,Strings(value)).scalar;}
bool IsCookiePrefixValid(CookiePrefix prefix,const GURL& url,const ParsedCookie& parsed) {return IsCookiePrefixValid(prefix,url,parsed.IsSecure(),parsed.IsHttpOnly(),parsed.Domain().value_or(""),parsed.Path().value_or(""));}
bool IsCookiePrefixValid(CookiePrefix prefix,base::optional_ref<const GURL> url,bool secure,bool httponly,std::string_view domain,std::string_view path) {
  auto request=Strings(domain,path);request.scalar[0]=static_cast<int32_t>(prefix);request.scalar[1]=url.has_value();request.scalar[2]=secure;request.scalar[3]=httponly;
  if(url) request.url=cf::Url(*url);return cf::Run(12,request).scalar;
}
bool IsCookiePartitionedValid(base::optional_ref<const GURL> url,bool secure,base::optional_ref<const CookiePartitionKey> key) {
  auto request=Strings();request.scalar[0]=url.has_value();request.scalar[1]=secure;request.scalar[2]=key.has_value();request.scalar[3]=key&&key->nonce().has_value();
  if(url) request.url=cf::Url(*url);const bool valid=cf::Run(13,request).scalar;
  DLOG_IF(WARNING,!valid)<<"Cookie has invalid Partitioned attribute";return valid;
}
void ParseRequestCookieLine(std::string_view line,ParsedRequestCookies* output) {cf::NativeContext context;context.parsed_output=output;cf::Run(14,Strings(line),&context);}
std::string SerializeRequestCookieLine(const ParsedRequestCookies& input) {cf::NativeContext context;context.parsed_input=&input;return cf::Take(cf::Run(15,Strings(),&context).buffer);}
CookieOptions::SameSiteCookieContext ComputeSameSiteContextForRequest(std::string_view method,const std::vector<GURL>& chain,const SiteForCookies& site,const std::optional<url::Origin>& initiator,bool main,bool force,bool ignore_unsafe) {return SameSite(0,method,chain,site,initiator,main,force,ignore_unsafe);}
CookieOptions::SameSiteCookieContext ComputeSameSiteContextForScriptGet(const GURL& url,const SiteForCookies& site,const std::optional<url::Origin>& initiator,bool force) {return SameSite(1,{},std::vector<GURL>{url},site,initiator,false,force);}
CookieOptions::SameSiteCookieContext ComputeSameSiteContextForResponse(const std::vector<GURL>& chain,const SiteForCookies& site,const std::optional<url::Origin>& initiator,bool main,bool force) {return SameSite(2,{},chain,site,initiator,main,force);}
CookieOptions::SameSiteCookieContext ComputeSameSiteContextForScriptSet(const GURL& url,const SiteForCookies& site,bool force) {return SameSite(3,{},std::vector<GURL>{url},site,std::nullopt,false,force);}
CookieOptions::SameSiteCookieContext ComputeSameSiteContextForSubresource(const GURL& url,const SiteForCookies& site,bool force) {return SameSite(4,{},std::vector<GURL>{url},site,std::nullopt,false,force);}
bool IsPortBoundCookiesEnabled() {return Feature(0);}
bool IsSchemeBoundCookiesEnabled() {return Feature(1);}
bool IsOriginBoundCookiesPartiallyEnabled() {return Feature(2);}
bool IsTimeLimitedInsecureCookiesEnabled() {return Feature(3);}
CookieOptions::SameSiteCookieContext::ContextMetadata::HttpMethod HttpMethodStringToEnum(std::string_view method) {return static_cast<CookieOptions::SameSiteCookieContext::ContextMetadata::HttpMethod>(cf::Run(18,Strings(method)).scalar);}
bool IsCookieAccessResultInclude(CookieAccessResult result) {auto request=Strings();request.access=cf::Snapshot(result);return cf::Run(19,request).scalar;}
CookieList StripAccessResults(const CookieAccessResultList& input) {CookieList output;cf::NativeContext context;context.included=&input;context.cookie_output=&output;cf::Run(20,Strings(),&context);return output;}
void RecordCookiePortOmniboxHistograms(const GURL& url) {auto request=Strings();request.scalar[0]=url.EffectiveIntPort();request.scalar[1]=IsLocalhost(url);cf::Run(21,request);}
void DCheckIncludedAndExcludedCookieLists(const CookieAccessResultList& included,const CookieAccessResultList& excluded) {
#if DCHECK_IS_ON()
  cf::NativeContext context;context.included=&included;context.excluded=&excluded;cf::Run(22,Strings(),&context);
#endif
}
bool IsForceThirdPartyCookieBlockingEnabled() {return cf::Run(23,Strings()).scalar;}
bool ShouldAddInitialStorageAccessApiOverride(const GURL& url,StorageAccessApiStatus status,base::optional_ref<const url::Origin> initiator) {
  auto request=Strings();request.scalar[0]=static_cast<int32_t>(status);request.scalar[1]=initiator.has_value();
  cf::NativeContext context;context.url=&url;context.request_initiator=initiator?&*initiator:nullptr;
  return cf::Run(24,request,&context).scalar;
}
size_t NameValueSizeBytes(const CanonicalCookie& cookie) {auto request=Strings();request.scalar[0]=cookie.Name().size();request.scalar[1]=cookie.ValueSize().InBytes();return static_cast<size_t>(cf::Run(25,request).scalar);}
}  // namespace net::cookie_util
