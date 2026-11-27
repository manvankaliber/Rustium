// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "net/cookies/cookie_family_canonical_native.h"
#include <cstring>
#include <limits>
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "net/base/features.h"
#include "net/cookies/cookie_constants.h"
#include "net/http/http_util.h"
#include "url/url_canon.h"
namespace net::cookie_family {
namespace {
int64_t Sign(std::strong_ordering value){return (value>0)-(value<0);}
base::Time Time(int64_t value){return base::Time::FromInternalValue(value);}
int64_t Numeric(const CanonicalCookie& c,int64_t field){
  switch(field){
    case 4:return c.CreationDate().ToInternalValue();case 5:return c.ExpiryDate().ToInternalValue();
    case 6:return c.LastAccessDate().ToInternalValue();case 7:return c.LastUpdateDate().ToInternalValue();
    case 8:return c.SecureAttribute();case 9:return c.IsHttpOnly();case 10:return static_cast<int32_t>(c.SameSite());
    case 11:return c.Priority();case 14:return static_cast<int32_t>(c.SourceScheme());case 15:return c.SourcePort();
    case 16:return static_cast<int32_t>(c.SourceType());case 17:return c.ValueSize().InBytes();
  }CHECK(false)<<field;
}
bool Feature(int64_t id){
  switch(id){
    case 0:return base::FeatureList::IsEnabled(features::kDisallowNonAsciiCookies);
    case 1:return base::FeatureList::IsEnabled(features::kCookieDomainRejectNonASCII);
    case 2:return base::FeatureList::IsEnabled(features::kCookieParseRejectEmptyNameAmbiguous);
    case 3:return base::FeatureList::IsEnabled(features::kTimeLimitedInsecureCookies);
    case 4:return base::FeatureList::IsEnabled(features::kCookieProcessBoundEncryptOnlyImportant);
    case 5:return features::kCookieProcessBoundEncryptHttpOnly.Get();
    case 6:return base::FeatureList::IsEnabled(features::kSameSiteDefaultChecksMethodRigorously);
    case 7:return base::FeatureList::IsEnabled(features::kShortLaxAllowUnsafeThreshold);
    case 8:return base::FeatureList::IsEnabled(features::kEnablePortBoundCookies);
    case 9:return base::FeatureList::IsEnabled(features::kEnableSchemeBoundCookies);
  }CHECK(false)<<id;
}
void Metric(CanonicalContext& context,const CookieFamilyCall& call){
  int64_t auxiliary=0;
  if(call.bytes.size==sizeof(auxiliary)) {
    // Rust supplies this initialized view for the synchronous callback.
    base::byte_span_from_ref(auxiliary).copy_from(UNSAFE_BUFFERS(base::span(call.bytes.data, call.bytes.size)));
  }
  const bool value=call.second!=0;
  switch(call.first){
    case 0:base::UmaHistogramBoolean("Cookie.NameOrValueHtab.Subsampled",value);return;
    case 1:base::UmaHistogramBoolean("Cookie.DomainHasNonASCII.Subsampled",value);return;
    case 2:base::UmaHistogramBoolean("Cookie.Parse.EmptyName",value);return;
    case 3:base::UmaHistogramBoolean("Cookie.Parse.EmptyNameAmbiguousValue",value);return;
    case 4:base::UmaHistogramEnumeration("Cookie.CookiePrefix.Subsampled",static_cast<CookiePrefix>(call.second));return;
    case 5:base::UmaHistogramBoolean("Cookie.IsPartitionedValid",value);return;
    case 6:base::UmaHistogramEnumeration("Cookie.SameSiteAttributeValue.Subsampled",static_cast<CookieSameSiteString>(call.second));return;
    case 7:{auto kind=static_cast<NamelessCookieLineParseType>(call.second);
      base::UmaHistogramEnumeration("Cookie.Parse.EmptyNameParseType",kind);
      if(auxiliary==static_cast<int32_t>(CookieSourceType::kHTTP))base::UmaHistogramEnumeration("Cookie.Parse.EmptyNameParseType.Http",kind);
      else if(auxiliary==static_cast<int32_t>(CookieSourceType::kScript))base::UmaHistogramEnumeration("Cookie.Parse.EmptyNameParseType.Script",kind);return;}
    case 8:base::UmaHistogramBoolean("Cookie.HasNonASCII.Name.Subsampled",value);return;
    case 9:base::UmaHistogramBoolean("Cookie.HasNonASCII.Value.Subsampled",value);return;
    case 10:base::UmaHistogramBoolean("Cookie.DoubleUnderscorePrefixedName.Subsampled",value);return;
    case 11:{constexpr std::array<std::string_view,9> names={"AndroidCookiesFetcherRestoreUtil","ChromeOsCookieSyncConversions","OauthMultiloginResult","","SqlitePersistentCookieStore","CookieManager","CookieManagerMojomTraits","RestrictedCookieManager","Tests"};
      CHECK_GE(call.second,0);CHECK_LT(static_cast<size_t>(call.second),std::size(names));
      base::UmaHistogramBoolean(base::StrCat({"Cookie.FromStorageWithValidLength.",names.at(call.second),".Subsampled"}),auxiliary!=0);return;}
    case 12:base::UmaHistogramEnumeration("Cookie.RequestSameSiteContext",static_cast<CookieOptions::SameSiteCookieContext::ContextType>(call.second),CookieOptions::SameSiteCookieContext::ContextType::COUNT);return;
    case 13:base::UmaHistogramEnumeration("Cookie.IncludedRequestEffectiveSameSite.Subsampled",static_cast<CookieEffectiveSameSite>(call.second),CookieEffectiveSameSite::COUNT);return;
    case 14:base::UmaHistogramEnumeration("Cookie.IncludedResponseEffectiveSameSite.Subsampled",static_cast<CookieEffectiveSameSite>(call.second),CookieEffectiveSameSite::COUNT);return;
    case 15:base::UmaHistogramEnumeration("Cookie.CrossSiteRedirectType.Read.Subsampled",static_cast<CookieOptions::SameSiteCookieContext::ContextMetadata::ContextRedirectTypeBug1221316>(call.second));return;
    case 16:base::UmaHistogramEnumeration("Cookie.CrossSiteRedirectType.Write.Subsampled",static_cast<CookieOptions::SameSiteCookieContext::ContextMetadata::ContextRedirectTypeBug1221316>(call.second));return;
    case 17:base::UmaHistogramEnumeration("Cookie.CrossSiteRedirectDowngradeChangesInclusion2.Read",static_cast<CookieSameSiteForMetrics>(call.second));return;
    case 18:base::UmaHistogramEnumeration("Cookie.CrossSiteRedirectDowngradeChangesInclusion2.Write.Subsampled",static_cast<CookieSameSiteForMetrics>(call.second));return;
  }CHECK(false)<<call.first;
}
}
const CanonicalCookie* CanonicalContext::Cookie(int64_t id) const {
  if(cookie_lookup)return &cookie_lookup(id);
  if(list_cookie)return &list_cookie(static_cast<size_t>(id));
  CHECK_GE(id,0);CHECK_LT(id,2);return fields[id].cookie;
}
const std::optional<CookiePartitionKey>& CanonicalContext::Partition(int64_t id) const {
  if(partition_lookup)return partition_lookup(id);
  CHECK_GT(id,0);const auto* cookie=Cookie(id-1);return cookie?cookie->PartitionKey():fields[id-1].partition;
}
CookieFamilyUrl CanonicalUrl(const GURL& url){
  if(url.is_valid())return Url(url);
  return {0,View(url.scheme()),View(url.host()),View(url.path()),false,false,false,false,false,url.EffectiveIntPort()};
}
void CanonicalNativeUnchecked(CanonicalContext& context,const CookieFamilyCall& call,CookieFamilyNativeResult& out){
  switch(call.operation){
    case 1100:out.number=base::ShouldRecordSubsampledMetric(kHistogramSampleProbability);return;
    case 1101:out.number=base::Time::Now().ToInternalValue();return;
    case 1102:out.number=(Time(call.first)+base::Microseconds(call.second)).ToInternalValue();return;
    case 1103:out.number=(Time(call.first)-Time(call.second)).InMicroseconds();return;
    case 1104:out.number=base::Seconds(call.first).InMicroseconds();return;
    case 1105:{url::RawCanonOutputT<char> path;url::Component component;url::CanonicalizePath(StringView(call.bytes),&path,&component);context.scratch=std::string(path.view().substr(component.begin,component.len));out.bytes=View(context.scratch);return;}
    case 1106:context.scratch=HttpUtil::TimeFormatHTTP(Time(call.first));out.bytes=View(context.scratch);return;
    case 1109:Metric(context,call);return;
    case 1110:DVLOG(cookie_util::kVlogSetCookies)<<"WARNING: Couldn't parse cookie";return;
    case 1111:
      switch(call.first){case 14:DVLOG(cookie_util::kVlogSetCookies)<<"Create() failed to get a valid cookie domain";return;
        case 15:DVLOG(cookie_util::kVlogSetCookies)<<"Create() failed because the cookie violated prefix rules.";return;}CHECK(false);
    case 1112:DCHECK(call.first);return;
    case 1114:NOTREACHED();
    case 1113:CHECK_IS_TEST();return;
    case 1120:{const auto id=call.first;const auto field=call.second;const auto* cookie=context.Cookie(id);
      if(field==12){const auto& partition=cookie?cookie->PartitionKey():context.fields[id].partition;out.number=partition? id+1:0;return;}
      if(field==13){const auto& partition=cookie?cookie->PartitionKey():context.fields[id].partition;out.number=partition&&partition->nonce().has_value();return;}
      out.number=cookie?Numeric(*cookie,field):context.fields[id].number[field];return;}
    case 1121:{if(call.first==-1){const auto attribute=call.second==2?context.parsed->Expires():context.parsed->MaxAge();out.bytes=View(attribute.value());return;}
      const auto* cookie=context.Cookie(call.first);
      if(!cookie){out.bytes=View(context.fields[call.first].bytes[call.second]);return;}
      switch(call.second){case 0:out.bytes=View(cookie->Name());return;case 1:context.scratch=cookie->Value();out.bytes=View(context.scratch);return;
        case 2:out.bytes=View(cookie->Domain());return;case 3:out.bytes=View(cookie->Path());return;}CHECK(false);}
    case 1122:out.number=Feature(call.first);return;
    case 1123:out.number=(call.first==2?context.parsed->Expires():context.parsed->MaxAge()).has_value();return;
    case 1125:out.number=context.Cookie(call.first)->IsSecure();return;
    case 1126:out.number=context.list_size;return;
    case 1127:out.number=context.Cookie(0)->CreationDate().ToTimeT();return;
    case 1130:
      if(call.first<4){context.output.bytes[call.first]=std::string(StringView(call.bytes));return;}
      context.output.number[call.first]=call.second;
      if(call.first==12){context.output.partition=call.second?context.Partition(call.second):std::nullopt;}return;
    case 1140:context.left_partition=&context.Partition(call.first).value();context.right_partition=&context.Partition(call.second).value();return;
    case 1141:out.number=CookieFamilyNativeAccess::Ancestor(call.first?*context.right_partition:*context.left_partition);return;
    case 1142:{const auto& left=*context.left_partition;const auto& right=*context.right_partition;
      switch(call.first){
        case 12:out.number=left.site()==right.site();return;case 13:out.number=Sign(left.site()<=>right.site());return;
        case 14:out.number=left.nonce().has_value();return;case 15:out.number=right.nonce().has_value();return;
        case 16:out.number=left.nonce().value()==right.nonce().value();return;
        case 17:out.number=Sign(left.nonce().value()<=>right.nonce().value());return;
      }CHECK(false)<<call.first;}
    default:{CookieFamilyNativeResult shared{};Native(&context.common,&call,&shared);out=shared;return;}
  }
}
void CanonicalNative(void* raw,const CookieFamilyCall* call,CookieFamilyNativeResult* out){
  auto& context=*static_cast<CanonicalContext*>(raw);
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try{CanonicalNativeUnchecked(context,*call,*out);}catch(...){context.exception=std::current_exception();out->failed=1;}
#else
  CanonicalNativeUnchecked(context,*call,*out);
#endif
}
CookieFamilyResult RunCanonical(uint32_t operation,const CookieFamilyRequest& request,CanonicalContext& context){
  CookieFamilyResult out{};cookie_family_call(operation,&request,&out,&context,CanonicalNative);
  auto exception=context.exception?context.exception:context.common.exception;
  if(exception){cookie_family_buffer_free(out.buffer);std::rethrow_exception(exception);}return out;
}
std::unique_ptr<CanonicalCookie> MaterializeCanonical(base::PassKey<CanonicalCookie> key,CanonicalContext& context){
  auto& out=context.output;const auto& n=out.number;
  return std::make_unique<CanonicalCookie>(key,std::move(out.bytes[0]),std::move(out.bytes[1]),std::move(out.bytes[2]),std::move(out.bytes[3]),
    Time(n[4]),Time(n[5]),Time(n[6]),Time(n[7]),n[8],n[9],static_cast<CookieSameSite>(n[10]),static_cast<CookiePriority>(n[11]),
    std::move(out.partition),static_cast<CookieSourceScheme>(n[14]),n[15],static_cast<CookieSourceType>(n[16]));
}
}  // namespace net::cookie_family
