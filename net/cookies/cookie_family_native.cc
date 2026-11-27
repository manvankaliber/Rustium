// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "net/cookies/cookie_family_native.h"
#include "base/check.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/numerics/checked_math.h"
#include "base/strings/strcat.h"
#include "net/base/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_partition_key_rust_ffi.h"
namespace net::cookie_family {
static_assert(static_cast<int32_t>(CookieAccessScheme::kNonCryptographic)==0);
static_assert(static_cast<int32_t>(CookieAccessScheme::kCryptographic)==1);
static_assert(static_cast<int32_t>(CookieAccessScheme::kTrustworthy)==2);
static_assert(static_cast<int32_t>(CookieSameSite::UNSPECIFIED)==-1);
static_assert(static_cast<int32_t>(CookieSameSite::NO_RESTRICTION)==0);
static_assert(static_cast<int32_t>(CookieSameSite::LAX_MODE)==1);
static_assert(static_cast<int32_t>(CookieSameSite::STRICT_MODE)==2);
static_assert(static_cast<int32_t>(CookieEffectiveSameSite::LAX_MODE_ALLOW_UNSAFE)==3);
static_assert(static_cast<int32_t>(CookieEffectiveSameSite::UNDEFINED)==4);
static_assert(static_cast<int32_t>(CookieSourceScheme::kSecure)==2);
static_assert(static_cast<int32_t>(CookieAccessSemantics::LEGACY)==1);
static_assert(static_cast<int32_t>(CookieScopeSemantics::LEGACY)==1);
static_assert(static_cast<int32_t>(CookiePrefix::kHostHttp)==4);
static_assert(url::PORT_UNSPECIFIED==-1&&url::PORT_INVALID==-2);
CookieFamilyView View(std::string_view value) {
  return {reinterpret_cast<const uint8_t*>(value.data()),value.size()};
}
std::string_view StringView(CookieFamilyView view) {
  if (!view.size) return {};
  return {reinterpret_cast<const char*>(view.data),view.size};
}
std::string Take(CookieFamilyBuffer buffer) {
  struct Release {
    CookieFamilyBuffer buffer;
    ~Release() {cookie_family_buffer_free(buffer);}
  } release{buffer};
  return std::string(StringView({buffer.data,buffer.size}));
}
CookieFamilyUrl Url(const GURL& url) {
  return {0,View(url.scheme()),View(url.host()),View(url.path()),url.IsStandard(),
    url.HostIsIPAddress(),url.SchemeIsCryptographic(),IsLocalhost(url),
    IsCanonicalizedHostCompliant(url.host()),url.EffectiveIntPort()};
}
CookieFamilyContext Snapshot(const CookieOptions::SameSiteCookieContext& context) {
  auto metadata=[](const auto& value)->CookieFamilyMetadata {
    return {static_cast<int32_t>(value.cross_site_redirect_downgrade),value.has_null_initiator,
      static_cast<int32_t>(value.redirect_type_bug_1221316)};
  };
  return {static_cast<int32_t>(context.context()),static_cast<int32_t>(context.schemeful_context()),
    metadata(context.metadata()),metadata(context.schemeful_metadata())};
}
CookieOptions::SameSiteCookieContext Materialize(CookieFamilyContext value) {
  using Context=CookieOptions::SameSiteCookieContext;
  auto metadata=[](CookieFamilyMetadata input)->Context::ContextMetadata {
    Context::ContextMetadata out;
    out.cross_site_redirect_downgrade=static_cast<Context::ContextMetadata::ContextDowngradeType>(input.downgrade);
    out.has_null_initiator=input.null_initiator;
    out.redirect_type_bug_1221316=static_cast<Context::ContextMetadata::ContextRedirectTypeBug1221316>(input.redirect);
    return out;
  };
  return Context(static_cast<Context::ContextType>(value.context),static_cast<Context::ContextType>(value.schemeful_context),
    metadata(value.metadata),metadata(value.schemeful_metadata));
}
CookieFamilyOptions Snapshot(const CookieOptions& options) {
  return {options.exclude_httponly(),Snapshot(options.same_site_cookie_context()),options.update_access_time(),options.return_excluded_cookies()};
}
CookieFamilyAccess SnapshotStatus(const CookieInclusionStatus& status) {
  CookieFamilyAccess out{};
  out.status={status.exclusion_reasons().ToEnumBitmask(),status.warning_reasons().ToEnumBitmask(),
    static_cast<int32_t>(status.exemption_reason()),0};return out;
}
CookieFamilyAccess Snapshot(const CookieAccessResult& result) {
  CookieFamilyAccess out=SnapshotStatus(result.status);
  out.effective_same_site=static_cast<int32_t>(result.effective_same_site);
  out.access_semantics=static_cast<int32_t>(result.access_semantics);
  out.scope_semantics=static_cast<int32_t>(result.scope_semantics);
  out.allowed_secure=result.is_allowed_to_access_secure_cookies;return out;
}
CookieInclusionStatus Materialize(CookieInclusionStatusRustState state) {
  auto out=CookieInclusionStatus::MakeFromComponents(
    CookieInclusionStatus::ExclusionReasonBitset::FromEnumBitmask(state.exclusions),
    CookieInclusionStatus::WarningReasonBitset::FromEnumBitmask(state.warnings),
    static_cast<CookieInclusionStatus::ExemptionReason>(state.exemption));
  CHECK(out);return *out;
}
CookieAccessResult Materialize(CookieFamilyAccess value) {
  return {static_cast<CookieEffectiveSameSite>(value.effective_same_site),Materialize(value.status),
    static_cast<CookieAccessSemantics>(value.access_semantics),static_cast<CookieScopeSemantics>(value.scope_semantics),value.allowed_secure};
}
namespace {
base::Time::Exploded Exploded(CookieFamilyCalendar value) {
  return {value.year,value.month,value.day_of_week,value.day,value.hour,value.minute,value.second,value.millisecond};
}
bool SharedCookieFeature(int64_t id) {
  switch(id) {
    case 0:return base::FeatureList::IsEnabled(features::kEnablePortBoundCookies);
    case 1:return base::FeatureList::IsEnabled(features::kEnableSchemeBoundCookies);
    case 2:return base::FeatureList::IsEnabled(features::kTimeLimitedInsecureCookies);
    case 3:return base::FeatureList::IsEnabled(features::kForceThirdPartyCookieBlocking);
    case 4:return base::FeatureList::IsEnabled(features::kThirdPartyStoragePartitioning);
  }
  CHECK(false);
}
}
void NativeUnchecked(void* raw,const CookieFamilyCall* input,CookieFamilyNativeResult* output) {
  auto& context=*static_cast<NativeContext*>(raw);
  const auto& call=*input;auto& out=*output;
  switch(call.operation) {
    case 1:{url::CanonHostInfo info;context.scratch=CanonicalizeHost(StringView(call.bytes),&info);out.bytes=View(context.scratch);return;}
    case 2:context.scratch=registry_controlled_domains::GetDomainAndRegistry(StringView(call.bytes),registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);out.bytes=View(context.scratch);return;
    case 3:DCHECK(call.first);return;
    case 10:{base::Time time;out.number2=base::Time::FromUTCExploded(Exploded(call.calendar),&time);out.number=time.ToInternalValue();return;}
    case 11:out.number=Exploded(call.calendar).HasValidValues();return;
    case 12:out.number=base::Time::kExplodedMinYear;out.number2=base::Time::kExplodedMaxYear;return;
    case 20:DCHECK(!context.chain->empty());out.number=context.chain->size();return;
    case 21:out.number=context.initiator->has_value();return;
    case 22:out.number=context.site_for_cookies->IsFirstPartyWithSchemefulMode((*context.chain)[call.first],call.second);return;
    case 23:out.number=SiteForCookies::FromOrigin(context.initiator->value()).IsFirstPartyWithSchemefulMode((*context.chain)[call.first],call.second);return;
    case 24:out.number=context.site_for_cookies->IsNull();return;
    case 25:DCHECK(call.first||context.site_for_cookies->IsNull());DCHECK(!(*context.chain)[call.second].SchemeIsWSOrWSS());return;
    case 26:out.number=base::FeatureList::IsEnabled(features::kCookieSameSiteConsidersRedirectChain);return;
    case 27:DCHECK(context.site_for_cookies->IsFirstPartyWithSchemefulMode((*context.chain)[call.first],true));DCHECK(!(*context.chain)[call.first].SchemeIsWSOrWSS());return;
    case 100:out.number=context.cookie->SecureAttribute();return;
    case 101:out.number=context.cookie->IsHttpOnly();return;
    case 102:out.number=static_cast<int32_t>(context.cookie->SameSite());return;
    case 103:out.number=static_cast<int32_t>(context.cookie->SourceScheme());return;
    case 104:out.number=context.cookie->SourcePort();return;
    case 105:out.bytes=View(context.cookie->Domain());return;
    case 106:out.bytes=View(context.cookie->Path());return;
    case 107:out.options=Snapshot(*context.options);return;
    case 108:out.params={static_cast<int32_t>(context.params->access_semantics),static_cast<int32_t>(context.params->scope_semantics),context.params->delegate_treats_url_as_trustworthy};return;
    case 109:
      if(call.first==0) out.number=base::FeatureList::IsEnabled(features::kEnableSchemeBoundCookies);
      else if(call.first==1) out.number=base::FeatureList::IsEnabled(features::kEnablePortBoundCookies);
      else if(call.first==2) out.number=base::FeatureList::IsEnabled(features::kCookieRequireCompliantHost);
      else CHECK(false);return;
    case 110:out.number=context.threshold(context.cookie);return;
    case 111:out.number=(base::Time::Now()-context.cookie->CreationDate()).InMicroseconds();return;
    case 112:
      switch(call.first) {
        case 0:CHECK(call.second)<<"url_port != url::PORT_INVALID";return;
        case 1:DCHECK(call.second)<<"schemeful_context <= context";return;
        case 2:DCHECK(call.second)<<"effective_same_site != UNDEFINED";return;
        case 3:DCHECK(call.second)<<"SameSite == UNSPECIFIED";return;
      }CHECK(false)<<"Unknown cookie inclusion invariant";
    case 113:
      switch(call.first) {
        case 0:DVLOG(cookie_util::kVlogSetCookies)<<"HttpOnly cookie not permitted in script context.";return;
        case 1:DVLOG(cookie_util::kVlogSetCookies)<<"SetCookie() rejecting insecure cookie with SameSite=None.";return;
        case 2:DVLOG(cookie_util::kVlogSetCookies)<<"Trying to set a `SameSite=Strict` cookie from a cross-site URL.";return;
        case 3:DVLOG(cookie_util::kVlogSetCookies)<<"Cookies with no known SameSite attribute being treated as lax; attempt to set from a cross-site URL denied.";return;
        case 4:DVLOG(cookie_util::kVlogSetCookies)<<"Trying to set a `SameSite=Lax` cookie from a cross-site URL.";return;
      }CHECK(false);
    case 114:out.number=context.cookie->PartitionKey()->site().IsSameSiteWith(GURL(std::string(StringView(call.bytes))));return;
    case 200:out.number=CookieFamilyNativeAccess::Keys(*context.collection)==nullptr;return;
    case 201:{const auto& keys=context.collection->PartitionKeys();context.iterator=keys.begin();context.end=keys.end();context.iterator_started=false;return;}
    case 202:if(context.iterator_started) ++*context.iterator;context.iterator_started=true;out.number=*context.iterator!=*context.end;return;
    case 203:*context.stream<<**context.iterator;return;
    case 204:*context.stream<<StringView(call.bytes);return;
    case 206:context.key_output->insert(CookiePartitionKey::FromWire(*context.site,static_cast<CookiePartitionKey::AncestorChainBit>(call.first)));return;
    case 208:out.number=CookieFamilyNativeAccess::Keys(*context.collection)->empty();return;
    case 210:out.number=CookieFamilyNativeAccess::Keys(call.first?*context.other_collection:*context.collection)->size();return;
    case 212:{
      const auto& left=*CookieFamilyNativeAccess::Keys(*context.collection);
      context.compare_left=&*(left.begin()+call.first);
      if(call.second<0) context.compare_right=context.partition;
      else {
        const auto& right=*CookieFamilyNativeAccess::Keys(*context.other_collection);
        context.compare_right=&*(right.begin()+call.second);
      }return;
    }
    case 213:out.number=CookieFamilyNativeAccess::Ancestor(call.first?*context.compare_right:*context.compare_left);return;
    case 214:{
      using namespace cookie_partition_key_rust;
      const auto& left=*context.compare_left;const auto& right=*context.compare_right;
      switch(call.first) {
        case kSiteEqual:out.number=left.site()==right.site();return;
        case kSiteCompare:{const auto order=left.site()<=>right.site();out.number=(order>0)-(order<0);return;}
        case kLeftNoncePresent:out.number=left.nonce().has_value();return;
        case kRightNoncePresent:out.number=right.nonce().has_value();return;
        case kNonceEqual:out.number=left.nonce().value()==right.nonce().value();return;
        case kNonceCompare:{const auto order=left.nonce().value()<=>right.nonce().value();out.number=(order>0)-(order<0);return;}
      }CHECK(false);
    }
    case 300:context.parsed_output->emplace_back(std::string(StringView(call.bytes)),std::string(StringView(call.bytes2)));return;
    case 301:out.number=context.parsed_input->size();return;
    case 302:out.bytes=View((*context.parsed_input)[call.first].first);out.bytes2=View((*context.parsed_input)[call.first].second);return;
    case 303:out.number=SharedCookieFeature(call.first);return;
    case 304:out.number=context.included->size();return;
    case 305:context.cookie_output->push_back((*context.included)[call.first].cookie);return;
    case 306:base::UmaHistogramEnumeration(std::string(StringView(call.bytes)),ReducePortRangeForCookieHistogram(static_cast<int>(call.first)));return;
    case 307:out.number=call.first?context.excluded->size():context.included->size();return;
    case 308:out.number=(call.second?*context.excluded:*context.included)[call.first].access_result.status.IsInclude();return;
    case 309:{const auto& a=(*context.included)[call.first].cookie;const auto& b=(*context.included)[call.second].cookie;
      out.number=a.Path().size();out.number2=b.Path().size();out.number3=a.CreationDate().ToInternalValue();out.number4=b.CreationDate().ToInternalValue();return;}
    case 310:DCHECK(call.second);return;
    case 311:CHECK(false)<<"Cookie name/value size overflow";
    case 312:out.context=Snapshot(*context.other_context);return;
    case 313:out.number=context.request_initiator->IsSameOriginWith(*context.url);return;
    case 320:out.number=context.cookieable_schemes->size();return;
    case 321:out.bytes=View((*context.cookieable_schemes)[call.first]);return;
    case 399:CHECK(false)<<"Invalid CookieFamily operation: "<<call.first;
  }
  CHECK(false)<<"Invalid CookieFamily native primitive: "<<call.operation;
}
void Native(void* raw,const CookieFamilyCall* input,CookieFamilyNativeResult* output) {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {NativeUnchecked(raw,input,output);}
  catch(...) {static_cast<NativeContext*>(raw)->exception=std::current_exception();output->failed=1;}
#else
  NativeUnchecked(raw,input,output);
#endif
}
CookieFamilyResult Run(uint32_t operation,const CookieFamilyRequest& request,NativeContext* context) {
  CookieFamilyResult result{};NativeContext local;
  auto* current=context?context:&local;
  cookie_family_call(operation,&request,&result,current,Native);
  if(current->exception) {cookie_family_buffer_free(result.buffer);std::rethrow_exception(current->exception);}
  return result;
}
}  // namespace net::cookie_family
