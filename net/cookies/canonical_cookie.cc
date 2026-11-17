// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Portions of this code based on Mozilla:
//   (netwerk/cookie/src/nsCookieService.cpp)
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Daniel Witte (dwitte@stanford.edu)
 *   Michiel van Leeuwen (mvl@exedo.nl)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_family_canonical_native.h"

#include <limits>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

#include "base/byte_size.h"
#include "base/check_is_test.h"
#include "base/compiler_specific.h"
#include "base/dcheck_is_on.h"
#include "base/feature_list.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/base/features.h"
#include "net/base/url_util.h"
#include "net/cookies/cookie_access_params.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "net/http/http_util.h"
#include "url/gurl.h"
#include "url/url_canon.h"
#include "url/url_util.h"

using base::Time;

namespace net {

namespace {
bool ShouldEncryptCookieValue(bool secure,bool httponly) {
  cookie_family::CanonicalContext context;CookieFamilyRequest request{};
  request.scalar[0]=secure;request.scalar[1]=httponly;
  return cookie_family::RunCanonical(1007,request,context).scalar;
}
}  // namespace

CookieAccessParams::CookieAccessParams(CookieAccessSemantics access_semantics,
                                       CookieScopeSemantics scope_semantics,
                                       bool delegate_treats_url_as_trustworthy)
    : access_semantics(access_semantics),
      scope_semantics(scope_semantics),
      delegate_treats_url_as_trustworthy(delegate_treats_url_as_trustworthy) {}

CanonicalCookie::CanonicalizationResult::CanonicalizationResult(
    base::PassKey<CanonicalCookie>,
    std::optional<CanonicalizationFailure> failure)
    : failure_(failure) {}

std::ostream& operator<<(
    std::ostream& os,
    const CanonicalCookie::CanonicalizationResult& result) {
  if (result) {
    os << "(ok)";
  } else {
    os << result.failure_.value();
  }
  return os;
}

CanonicalCookie::CanonicalCookie() = default;

CanonicalCookie::CanonicalCookie(const CanonicalCookie& other) = default;

CanonicalCookie::CanonicalCookie(CanonicalCookie&& other) = default;

CanonicalCookie& CanonicalCookie::operator=(const CanonicalCookie& other) =
    default;

CanonicalCookie& CanonicalCookie::operator=(CanonicalCookie&& other) = default;

CanonicalCookie::CanonicalCookie(
    base::PassKey<CanonicalCookie> pass_key,
    std::string name,
    std::string value,
    std::string domain,
    std::string path,
    base::Time creation,
    base::Time expiration,
    base::Time last_access,
    base::Time last_update,
    bool secure,
    bool httponly,
    CookieSameSite same_site,
    CookiePriority priority,
    std::optional<CookiePartitionKey> partition_key,
    CookieSourceScheme source_scheme,
    int source_port,
    CookieSourceType source_type)
    : CookieBase(std::move(name),
                 std::move(domain),
                 std::move(path),
                 creation,
                 secure,
                 httponly,
                 same_site,
                 std::move(partition_key),
                 source_scheme,
                 source_port),
      value_(crypto::ProcessBoundString(
          std::move(value),
          ShouldEncryptCookieValue(secure, httponly))),
      expiry_date_(expiration),
      last_access_date_(last_access),
      last_update_date_(last_update),
      priority_(priority),
      source_type_(source_type) {}

CanonicalCookie::~CanonicalCookie() = default;

// static
Time CanonicalCookie::ParseExpiration(const ParsedCookie& pc,
                                      Time current,
                                      Time server_time) {
cookie_family::CanonicalContext context;context.parsed=&pc;
  CookieFamilyRequest request{};request.scalar[0]=current.ToInternalValue();request.scalar[1]=server_time.ToInternalValue();
  return base::Time::FromInternalValue(cookie_family::RunCanonical(1003,request,context).scalar);
}

// static
base::Time CanonicalCookie::ValidateAndAdjustExpiryDate(
    base::Time expiry_date,
    base::Time creation_date,
    net::CookieSourceScheme scheme) {
cookie_family::CanonicalContext context;CookieFamilyRequest request{};
  request.scalar[0]=expiry_date.ToInternalValue();request.scalar[1]=creation_date.ToInternalValue();request.scalar[2]=static_cast<int32_t>(scheme);
  return base::Time::FromInternalValue(cookie_family::RunCanonical(1004,request,context).scalar);
}

// static
std::unique_ptr<CanonicalCookie> CanonicalCookie::Create(
    const GURL& url,
    std::string_view cookie_line,
    base::Time creation_time,
    std::optional<base::Time> server_time,
    std::optional<CookiePartitionKey> cookie_partition_key,
    CookieSourceType source_type,
    CookieInclusionStatus* status) {
CookieInclusionStatus blank_status;if(!status)status=&blank_status;*status=CookieInclusionStatus();
  cookie_family::CanonicalContext context;context.common.url=&url;context.fields[0].partition=std::move(cookie_partition_key);
  CookieFamilyRequest request{};request.first=cookie_family::View(cookie_line);
  if(url.is_valid())request.url=cookie_family::CanonicalUrl(url);request.scalar[0]=url.is_valid();request.scalar[1]=creation_time.ToInternalValue();
  request.scalar[2]=server_time.has_value();request.scalar[3]=server_time.value_or(base::Time()).ToInternalValue();request.scalar[4]=static_cast<int32_t>(source_type);
  const auto result=cookie_family::RunCanonical(1000,request,context);*status=cookie_family::Materialize(result.access.status);
  if(!result.scalar)return nullptr;
  return cookie_family::MaterializeCanonical(base::PassKey<CanonicalCookie>(),context);
}

// static
std::unique_ptr<CanonicalCookie> CanonicalCookie::CreateSanitizedCookie(
    const GURL& url,
    const std::string& name,
    const std::string& value,
    const std::string& domain,
    const std::string& path,
    base::Time creation_time,
    base::Time expiration_time,
    base::Time last_access_time,
    bool secure,
    bool http_only,
    CookieSameSite same_site,
    CookiePriority priority,
    std::optional<CookiePartitionKey> partition_key,
    CookieInclusionStatus* status) {
CookieInclusionStatus blank_status;if(!status)status=&blank_status;*status=CookieInclusionStatus();
  cookie_family::CanonicalContext context;
  auto& field=context.fields[0];
  field.bytes={{std::string(name),std::string(value),std::string(domain),std::string(path)}};
  field.number[4]=creation_time.ToInternalValue();field.number[5]=expiration_time.ToInternalValue();
  field.number[6]=last_access_time.ToInternalValue();field.number[7]=base::Time().ToInternalValue();
  field.number[8]=secure;field.number[9]=http_only;field.number[10]=static_cast<int32_t>(same_site);
  field.number[11]=static_cast<int32_t>(priority);field.partition=partition_key;
  field.number[14]=static_cast<int32_t>(CookieSourceScheme::kUnset);field.number[15]=url::PORT_UNSPECIFIED;
  field.number[16]=static_cast<int32_t>(CookieSourceType::kOther);field.number[17]=value.size();
  context.common.url=&url;
  CookieFamilyRequest request{};request.url=cookie_family::CanonicalUrl(url);request.scalar[0]=url.is_valid();
  const auto result=cookie_family::RunCanonical(1001,request,context);*status=cookie_family::Materialize(result.access.status);
  if(!result.scalar)return nullptr;
  auto output=cookie_family::MaterializeCanonical(base::PassKey<CanonicalCookie>(),context);
  if constexpr(DCHECK_IS_ON()){const auto canonical=output->IsCanonical();DCHECK(canonical)<<canonical;}
  return output;
}

// static
std::unique_ptr<CanonicalCookie> CanonicalCookie::FromStorage(
    std::string name,
    std::string value,
    std::string domain,
    std::string path,
    base::Time creation,
    base::Time expiration,
    base::Time last_access,
    base::Time last_update,
    bool secure,
    bool httponly,
    CookieSameSite same_site,
    CookiePriority priority,
    std::optional<CookiePartitionKey> partition_key,
    CookieSourceScheme source_scheme,
    int source_port,
    CookieSourceType source_type,
    CanonicalCookieFromStorageCallSite call_site) {
cookie_family::CanonicalContext context;
  auto& field=context.fields[0];
  field.bytes={{std::string(name),std::string(value),std::string(domain),std::string(path)}};
  field.number[4]=creation.ToInternalValue();field.number[5]=expiration.ToInternalValue();
  field.number[6]=last_access.ToInternalValue();field.number[7]=last_update.ToInternalValue();
  field.number[8]=secure;field.number[9]=httponly;field.number[10]=static_cast<int32_t>(same_site);
  field.number[11]=static_cast<int32_t>(priority);field.partition=partition_key;
  field.number[14]=static_cast<int32_t>(source_scheme);field.number[15]=source_port;
  field.number[16]=static_cast<int32_t>(source_type);field.number[17]=value.size();
  CookieFamilyRequest request{};request.scalar[0]=static_cast<int32_t>(call_site);
  const auto result=cookie_family::RunCanonical(1002,request,context);if(!result.scalar)return nullptr;
  return cookie_family::MaterializeCanonical(base::PassKey<CanonicalCookie>(),context);
}

// static
std::unique_ptr<CanonicalCookie> CanonicalCookie::CreateUnsafeCookieForTesting(
    const std::string& name,
    const std::string& value,
    const std::string& domain,
    const std::string& path,
    base::Time creation,
    base::Time expiration,
    base::Time last_access,
    base::Time last_update,
    bool secure,
    bool httponly,
    CookieSameSite same_site,
    CookiePriority priority,
    CookieSourceType source_type,
    std::optional<CookiePartitionKey> partition_key,
    CookieSourceScheme source_scheme,
    int source_port) {
  return std::make_unique<CanonicalCookie>(
      base::PassKey<CanonicalCookie>(), name, value, domain, path, creation,
      expiration, last_access, last_update, secure, httponly, same_site,
      priority, partition_key, source_scheme, source_port, source_type);
}

// static
std::unique_ptr<CanonicalCookie> CanonicalCookie::CreateForTesting(
    const GURL& url,
    std::string_view cookie_line,
    base::Time creation_time,
    CookieSourceType source_type,
    std::optional<base::Time> server_time,
    std::optional<CookiePartitionKey> cookie_partition_key,
    CookieInclusionStatus* status) {
  return CanonicalCookie::Create(url, cookie_line, creation_time, server_time,
                                 cookie_partition_key, source_type, status);
}

std::string CanonicalCookie::Value() const {
  if (!value_.has_value()) {
    return std::string();
  }
  return value_->value();
}

base::ByteSize CanonicalCookie::ValueSize() const {
  if (!value_.has_value()) {
    return base::ByteSize(0);
  }
  return base::ByteSize(value_->size());
}

bool CanonicalCookie::IsEquivalentForSecureCookieMatching(
    const CanonicalCookie& secure_cookie) const {
cookie_family::CanonicalContext context;context.fields[0].cookie=this;context.fields[1].cookie=&secure_cookie;
  CookieFamilyRequest request{};return cookie_family::RunCanonical(1009,request,context).scalar;
}

bool CanonicalCookie::IsProbablyEquivalentTo(
    const CanonicalCookie& other) const {
cookie_family::CanonicalContext context;context.fields[0].cookie=this;context.fields[1].cookie=&other;
  CookieFamilyRequest request{};return cookie_family::RunCanonical(1010,request,context).scalar;
}

bool CanonicalCookie::HasEquivalentDataMembers(
    const CanonicalCookie& other) const {
cookie_family::CanonicalContext context;context.fields[0].cookie=this;context.fields[1].cookie=&other;
  CookieFamilyRequest request{};return cookie_family::RunCanonical(1011,request,context).scalar;
}

bool CanonicalCookie::IsWebEquivalentTo(const CanonicalCookie& other) const {
cookie_family::CanonicalContext context;context.fields[0].cookie=this;context.fields[1].cookie=&other;
  CookieFamilyRequest request{};return cookie_family::RunCanonical(1012,request,context).scalar;
}

void CanonicalCookie::PostIncludeForRequestURL(
    const CookieAccessResult& access_result,
    const CookieOptions& options_used,
    CookieOptions::SameSiteCookieContext::ContextType
        cookie_inclusion_context_used) const {
cookie_family::CanonicalContext context;CookieFamilyRequest request{};request.access=cookie_family::Snapshot(access_result);
  request.scalar[0]=static_cast<int32_t>(cookie_inclusion_context_used);
  request.scalar[1]=static_cast<int32_t>(options_used.same_site_cookie_context().GetMetadataForCurrentSchemefulMode().redirect_type_bug_1221316);
  request.scalar[2]=static_cast<int32_t>(SameSite());cookie_family::RunCanonical(1017,request,context);
}

void CanonicalCookie::PostIsSetPermittedInContext(
    const CookieAccessResult& access_result,
    const CookieOptions& options_used) const {
cookie_family::CanonicalContext context;CookieFamilyRequest request{};request.access=cookie_family::Snapshot(access_result);
  request.scalar[0]=static_cast<int32_t>(options_used.same_site_cookie_context().GetMetadataForCurrentSchemefulMode().redirect_type_bug_1221316);
  request.scalar[1]=static_cast<int32_t>(SameSite());cookie_family::RunCanonical(1018,request,context);
}

base::TimeDelta CanonicalCookie::GetLaxAllowUnsafeThresholdAge() const {
cookie_family::CanonicalContext context;CookieFamilyRequest request{};
  return base::Microseconds(cookie_family::RunCanonical(1016,request,context).scalar);
}

std::string CanonicalCookie::DebugString() const {
cookie_family::CanonicalContext context;context.fields[0].cookie=this;CookieFamilyRequest request{};
  return cookie_family::Take(cookie_family::RunCanonical(1015,request,context).buffer);
}

CanonicalCookie::CanonicalizationResult CanonicalCookie::IsCanonical() const {
  return IsCanonical(Name(), Value(), Domain(), Path(), CreationDate(),
                     expiry_date_, last_access_date_, SecureAttribute(),
                     IsHttpOnly(), PartitionKey(), SourceScheme());
}

CanonicalCookie::CanonicalizationResult
CanonicalCookie::IsCanonicalForFromStorage() const {
  return IsCanonicalForFromStorage(
      Name(), Value(), Domain(), Path(), CreationDate(), last_access_date_,
      SecureAttribute(), IsHttpOnly(), PartitionKey());
}

// static
CanonicalCookie::CanonicalizationResult CanonicalCookie::IsCanonical(
    std::string_view name,
    std::string_view value,
    std::string_view domain,
    std::string_view path,
    base::Time creation_date,
    base::Time expiry_date,
    base::Time last_access_date,
    bool is_secure,
    bool is_http_only,
    const std::optional<CookiePartitionKey>& partition_key,
    net::CookieSourceScheme source_scheme) {
cookie_family::CanonicalContext context;
  auto& field=context.fields[0];
  field.bytes={{std::string(name),std::string(value),std::string(domain),std::string(path)}};
  field.number[4]=creation_date.ToInternalValue();field.number[5]=expiry_date.ToInternalValue();
  field.number[6]=last_access_date.ToInternalValue();field.number[7]=base::Time().ToInternalValue();
  field.number[8]=is_secure;field.number[9]=is_http_only;field.number[10]=static_cast<int32_t>(CookieSameSite::UNSPECIFIED);
  field.number[11]=static_cast<int32_t>(COOKIE_PRIORITY_DEFAULT);field.partition=partition_key;
  field.number[14]=static_cast<int32_t>(source_scheme);field.number[15]=url::PORT_UNSPECIFIED;
  field.number[16]=static_cast<int32_t>(CookieSourceType::kOther);field.number[17]=value.size();
  CookieFamilyRequest request{};const auto result=cookie_family::RunCanonical(1006,request,context);
  return result.scalar<0?Pass():Fail(static_cast<CanonicalizationFailure>(result.scalar));
}

// static
CanonicalCookie::CanonicalizationResult
CanonicalCookie::IsCanonicalForFromStorage(
    std::string_view name,
    std::string_view value,
    std::string_view domain,
    std::string_view path,
    base::Time creation_date,
    base::Time last_access_date,
    bool is_secure,
    bool is_http_only,
    const std::optional<CookiePartitionKey>& partition_key) {
cookie_family::CanonicalContext context;
  auto& field=context.fields[0];
  field.bytes={{std::string(name),std::string(value),std::string(domain),std::string(path)}};
  field.number[4]=creation_date.ToInternalValue();field.number[5]=base::Time().ToInternalValue();
  field.number[6]=last_access_date.ToInternalValue();field.number[7]=base::Time().ToInternalValue();
  field.number[8]=is_secure;field.number[9]=is_http_only;field.number[10]=static_cast<int32_t>(CookieSameSite::UNSPECIFIED);
  field.number[11]=static_cast<int32_t>(COOKIE_PRIORITY_DEFAULT);field.partition=partition_key;
  field.number[14]=static_cast<int32_t>(CookieSourceScheme::kUnset);field.number[15]=url::PORT_UNSPECIFIED;
  field.number[16]=static_cast<int32_t>(CookieSourceType::kOther);field.number[17]=value.size();
  CookieFamilyRequest request{};const auto result=cookie_family::RunCanonical(1005,request,context);
  return result.scalar<0?Pass():Fail(static_cast<CanonicalizationFailure>(result.scalar));
}

bool CanonicalCookie::IsEffectivelySameSiteNone(
    CookieAccessSemantics access_semantics) const {
cookie_family::CanonicalContext context;CookieFamilyRequest request{};
  request.scalar[0]=static_cast<int32_t>(GetEffectiveSameSite(access_semantics));
  return cookie_family::RunCanonical(1025,request,context).scalar;
}

CookieEffectiveSameSite CanonicalCookie::GetEffectiveSameSiteForTesting(
    CookieAccessSemantics access_semantics) const {
  return GetEffectiveSameSite(access_semantics);
}

// static
std::string CanonicalCookie::BuildCookieLine(const CookieList& cookies) {
cookie_family::CanonicalContext context;context.list_size=cookies.size();
  context.list_cookie=[&](size_t index)->const CanonicalCookie&{return cookies[index];};
  CookieFamilyRequest request{};return cookie_family::Take(cookie_family::RunCanonical(1013,request,context).buffer);
}

// static
std::string CanonicalCookie::BuildCookieLine(
    const CookieAccessResultList& cookie_access_result_list) {
cookie_family::CanonicalContext context;context.list_size=cookie_access_result_list.size();
  context.list_cookie=[&](size_t index)->const CanonicalCookie&{return cookie_access_result_list[index].cookie;};
  CookieFamilyRequest request{};return cookie_family::Take(cookie_family::RunCanonical(1013,request,context).buffer);
}

// static
std::string CanonicalCookie::BuildCookieAttributesLine(
    const CanonicalCookie& cookie) {
cookie_family::CanonicalContext context;context.fields[0].cookie=&cookie;CookieFamilyRequest request{};
  return cookie_family::Take(cookie_family::RunCanonical(1014,request,context).buffer);
}

// static
int CanonicalCookie::GetAndAdjustPortForTrustworthyUrls(
    const GURL& source_url,
    bool url_is_trustworthy) {
cookie_family::CanonicalContext context;context.common.url=&source_url;CookieFamilyRequest request{};
  request.url=cookie_family::CanonicalUrl(source_url);request.scalar[0]=url_is_trustworthy;
  return static_cast<int>(cookie_family::RunCanonical(1008,request,context).scalar);
}

// static
CanonicalCookie::CanonicalizationResult CanonicalCookie::Pass() {
  return CanonicalizationResult(base::PassKey<CanonicalCookie>(), std::nullopt);
}

// static
CanonicalCookie::CanonicalizationResult CanonicalCookie::Fail(
    CanonicalCookie::CanonicalizationFailure failure) {
  return CanonicalizationResult(base::PassKey<CanonicalCookie>(), failure);
}

CookieAndLineWithAccessResult::CookieAndLineWithAccessResult() = default;

CookieAndLineWithAccessResult::CookieAndLineWithAccessResult(
    std::optional<CanonicalCookie> cookie,
    std::string cookie_string,
    CookieAccessResult access_result)
    : cookie(std::move(cookie)),
      cookie_string(std::move(cookie_string)),
      access_result(access_result) {}

CookieAndLineWithAccessResult::CookieAndLineWithAccessResult(
    const CookieAndLineWithAccessResult&) = default;

CookieAndLineWithAccessResult& CookieAndLineWithAccessResult::operator=(
    const CookieAndLineWithAccessResult& cookie_and_line_with_access_result) =
    default;

CookieAndLineWithAccessResult::CookieAndLineWithAccessResult(
    CookieAndLineWithAccessResult&&) = default;

CookieAndLineWithAccessResult::~CookieAndLineWithAccessResult() = default;

std::ostream& operator<<(std::ostream& os,
                         CanonicalCookie::CanonicalizationFailure failure) {
cookie_family::CanonicalContext context;CookieFamilyRequest request{};
  request.scalar[0]=static_cast<int32_t>(failure);
  os << cookie_family::Take(cookie_family::RunCanonical(1026,request,context).buffer);
  return os;
}


bool CanonicalCookie::IsPersistent() const {
  cookie_family::CanonicalContext context;CookieFamilyRequest request{};request.scalar[0]=expiry_date_.ToInternalValue();
  return cookie_family::RunCanonical(1019,request,context).scalar;
}
bool CanonicalCookie::IsExpired(base::Time current) const {
  cookie_family::CanonicalContext context;CookieFamilyRequest request{};request.scalar[0]=expiry_date_.ToInternalValue();request.scalar[1]=current.ToInternalValue();
  return cookie_family::RunCanonical(1020,request,context).scalar;
}
bool CanonicalCookie::IsValueEqual(const CanonicalCookie& other) const {
  cookie_family::CanonicalContext context;context.fields[0].cookie=this;context.fields[1].cookie=&other;CookieFamilyRequest request{};
  return cookie_family::RunCanonical(1023,request,context).scalar;
}
bool CanonicalCookie::IsValueEqual(std::string_view value) const {
  cookie_family::CanonicalContext context;context.fields[0].cookie=this;CookieFamilyRequest request{};request.first=cookie_family::View(value);
  return cookie_family::RunCanonical(1024,request,context).scalar;
}
bool CanonicalCookie::IsEquivalent(const CanonicalCookie& other) const {return *this==other;}
bool operator==(const CanonicalCookie& left,const CanonicalCookie& right) {
  cookie_family::CanonicalContext context;context.fields[0].cookie=&left;context.fields[1].cookie=&right;CookieFamilyRequest request{};
  return cookie_family::RunCanonical(1021,request,context).scalar;
}
std::strong_ordering operator<=>(const CanonicalCookie& left,const CanonicalCookie& right) {
  cookie_family::CanonicalContext context;context.fields[0].cookie=&left;context.fields[1].cookie=&right;CookieFamilyRequest request{};
  return cookie_family::RunCanonical(1022,request,context).scalar<=>0;
}
}  // namespace net
