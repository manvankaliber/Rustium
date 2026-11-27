// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cookies/cookie_base.h"
#include "net/cookies/cookie_family_native.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/types/pass_key.h"
#include "net/base/features.h"
#include "net/cookies/cookie_access_params.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/ref_unique_cookie_key.h"
#include "net/cookies/unique_cookie_key.h"
#include "url/gurl.h"

namespace net {

namespace cf = cookie_family;

CookieAccessResult CookieBase::IncludeForRequestURL(
    const GURL& url,
    const CookieOptions& options,
    const CookieAccessParams& params) const {
  cf::NativeContext native; native.cookie=this; native.options=&options; native.params=&params;
  native.threshold=[](const CookieBase* cookie) { return CookieFamilyNativeAccess::AgeThreshold(*cookie); };
  CookieFamilyRequest request{}; request.url=cf::Url(url);
  const auto output=cf::Run(30,request,&native);
  CookieAccessResult result=cf::Materialize(output.access);
  native = {};
  PostIncludeForRequestURL(result,options,static_cast<CookieOptions::SameSiteCookieContext::ContextType>(output.scalar));
  return result;
}

CookieAccessResult CookieBase::IsSetPermittedInContext(
    const GURL& source_url,
    const CookieOptions& options,
    const CookieAccessParams& params,
    const std::vector<std::string>& cookieable_schemes,
    const std::optional<CookieAccessResult>& cookie_access_result) const {
  cf::NativeContext native; native.cookie=this; native.options=&options; native.params=&params;
  native.threshold=[](const CookieBase* cookie) { return CookieFamilyNativeAccess::AgeThreshold(*cookie); };
  CookieFamilyRequest request{}; request.url=cf::Url(source_url);
  native.cookieable_schemes=&cookieable_schemes;
  request.scalar[1]=cookie_access_result.has_value();
  if(cookie_access_result) request.access=cf::Snapshot(*cookie_access_result);
  CookieAccessResult result=cf::Materialize(cf::Run(31,request,&native).access);
  native = {};
  PostIsSetPermittedInContext(result,options);
  return result;
}

bool CookieBase::IsOnPath(const std::string_view url_path) const {
  return cookie_util::IsOnPath(path_, url_path);
}

bool CookieBase::IsDomainMatch(const std::string_view host) const {
  return cookie_util::IsDomainMatch(domain_, host);
}

bool CookieBase::IsSecure() const {
  CookieFamilyRequest request{};request.scalar[0]=secure_;request.scalar[2]=static_cast<int32_t>(source_scheme_);
  return cf::Run(33,request).scalar;
}

bool CookieBase::IsFirstPartyPartitioned() const {
  CookieFamilyRequest request{};request.scalar[0]=partition_key_.has_value();request.scalar[1]=partition_key_&&partition_key_->nonce().has_value();
  cf::NativeContext native;native.cookie=this;return cf::Run(34,request,&native).scalar;
}

bool CookieBase::IsThirdPartyPartitioned() const {
  CookieFamilyRequest request{};request.scalar[0]=partition_key_.has_value();request.scalar[1]=partition_key_&&partition_key_->nonce().has_value();
  cf::NativeContext native;native.cookie=this;return cf::Run(35,request,&native).scalar;
}

std::string CookieBase::DomainWithoutDot() const {
  return cookie_util::CookieDomainAsHost(domain_);
}

UniqueCookieKey CookieBase::StrictlyUniqueKey() const {
  CookieFamilyRequest request{};request.scalar[0]=1;const auto policy=cf::Run(36,request).key;
  CHECK_EQ(policy.kind,0);return UniqueCookieKey::Strict(base::PassKey<CookieBase>(),partition_key_,name_,domain_,path_,source_scheme_,source_port_);
}

UniqueCookieKey CookieBase::UniqueKey() const {
  CookieFamilyRequest request{};request.first=cf::View(domain_);
  const auto policy=cf::Run(36,request).key;
  const auto scheme=policy.scheme?std::make_optional(source_scheme_):std::nullopt;
  if(policy.kind==2) return UniqueCookieKey::Domain(base::PassKey<CookieBase>(),partition_key_,name_,domain_,path_,scheme);
  CHECK_EQ(policy.kind,1);const auto port=policy.port?std::make_optional(source_port_):std::nullopt;
  return UniqueCookieKey::Host(base::PassKey<CookieBase>(),partition_key_,name_,domain_,path_,scheme,port);
}

RefUniqueCookieKey CookieBase::RefUniqueKey() const {
  CookieFamilyRequest request{};request.first=cf::View(domain_);
  const auto policy=cf::Run(36,request).key;
  const auto scheme=policy.scheme?std::make_optional(source_scheme_):std::nullopt;
  if(policy.kind==2) return RefUniqueCookieKey::Domain(base::PassKey<CookieBase>(),partition_key_,name_,domain_,path_,scheme);
  CHECK_EQ(policy.kind,1);const auto port=policy.port?std::make_optional(source_port_):std::nullopt;
  return RefUniqueCookieKey::Host(base::PassKey<CookieBase>(),partition_key_,name_,domain_,path_,scheme,port);
}

UniqueCookieKey CookieBase::LegacyUniqueKey() const {
  CookieFamilyRequest request{};request.scalar[0]=2;const auto policy=cf::Run(36,request).key;
  CHECK_EQ(policy.kind,3);return UniqueCookieKey::Legacy(base::PassKey<CookieBase>(),partition_key_,name_,domain_,path_);
}

void CookieBase::SetSourcePort(int port) {
  source_port_ = ValidateAndAdjustSourcePort(port);
}

CookieBase::CookieBase() = default;

CookieBase::CookieBase(const CookieBase& other) = default;

CookieBase::CookieBase(CookieBase&& other) = default;

CookieBase& CookieBase::operator=(const CookieBase& other) = default;

CookieBase& CookieBase::operator=(CookieBase&& other) = default;

CookieBase::~CookieBase() = default;

CookieBase::CookieBase(std::string name,
                       std::string domain,
                       std::string path,
                       base::Time creation,
                       bool secure,
                       bool httponly,
                       CookieSameSite same_site,
                       std::optional<CookiePartitionKey> partition_key,
                       CookieSourceScheme source_scheme,
                       int source_port)
    : name_(std::move(name)),
      domain_(std::move(domain)),
      path_(std::move(path)),
      creation_date_(creation),
      secure_(secure),
      httponly_(httponly),
      same_site_(same_site),
      partition_key_(std::move(partition_key)),
      source_scheme_(source_scheme),
      source_port_(source_port) {}

CookieEffectiveSameSite CookieBase::GetEffectiveSameSite(
    CookieAccessSemantics access_semantics) const {
  CookieFamilyRequest request{};request.scalar[0]=static_cast<int32_t>(access_semantics);
  cf::NativeContext native;native.cookie=this;native.threshold=[](const CookieBase* cookie){return CookieFamilyNativeAccess::AgeThreshold(*cookie);};
  return static_cast<CookieEffectiveSameSite>(cf::Run(32,request,&native).scalar);
}

base::TimeDelta CookieBase::GetLaxAllowUnsafeThresholdAge() const {
  return base::TimeDelta::Min();
}

bool CookieBase::IsRecentlyCreated(base::TimeDelta age_threshold) const {
  CookieFamilyRequest request{};request.scalar[0]=age_threshold.InMicroseconds();
  cf::NativeContext native;native.cookie=this;return cf::Run(38,request,&native).scalar;
}

// static
int CookieBase::ValidateAndAdjustSourcePort(int port) {
  CookieFamilyRequest request{};request.scalar[0]=port;return static_cast<int>(cf::Run(37,request).scalar);
}

bool CookieBase::IsDomainCookie() const {
  CookieFamilyRequest request{};request.first=cf::View(domain_);return cf::Run(29,request).scalar;
}
bool CookieBase::IsHostCookie() const {
  CookieFamilyRequest request{};request.first=cf::View(domain_);return cf::Run(0,request).scalar;
}

}  // namespace net
