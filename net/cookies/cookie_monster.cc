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

#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_family_monster_native.h"

#include <algorithm>
#include <array>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/containers/to_vector.h"
#include "base/dcheck_is_on.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "base/trace_event/trace_event.h"
#include "net/base/features.h"
#include "net/base/isolation_info.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/schemeful_site.h"
#include "net/base/url_util.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_access_params.h"
#include "net/cookies/cookie_base.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_monster_change_dispatcher.h"
#include "net/cookies/cookie_monster_netlog_params.h"
#include "net/cookies/cookie_partition_key.h"
#include "net/cookies/cookie_partition_key_collection.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "net/cookies/unique_cookie_key.h"
#include "net/http/http_util.h"
#include "net/log/net_log.h"
#include "net/log/net_log_values.h"
#include "url/origin.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"
#include "url/url_constants.h"

using base::Time;
using base::TimeTicks;
using TimeRange = net::CookieDeletionInfo::TimeRange;

// In steady state, most cookie requests can be satisfied by the in memory
// cookie monster store. If the cookie request cannot be satisfied by the in
// memory store, the relevant cookies must be fetched from the persistent
// store. The task is queued in CookieMonster::tasks_pending_ if it requires
// all cookies to be loaded from the backend, or tasks_pending_for_key_ if it
// only requires all cookies associated with an eTLD+1.
//
// On the browser critical paths (e.g. for loading initial web pages in a
// session restore) it may take too long to wait for the full load. If a cookie
// request is for a specific URL, DoCookieCallbackForURL is called, which
// triggers a priority load if the key is not loaded yet by calling
// PersistentCookieStore::LoadCookiesForKey. The request is queued in
// CookieMonster::tasks_pending_for_key_ and executed upon receiving
// notification of key load completion via CookieMonster::OnKeyLoaded(). If
// multiple requests for the same eTLD+1 are received before key load
// completion, only the first request calls
// PersistentCookieStore::LoadCookiesForKey, all subsequent requests are queued
// in CookieMonster::tasks_pending_for_key_ and executed upon receiving
// notification of key load completion triggered by the first request for the
// same eTLD+1.





namespace net {

// See comments at declaration of these variables in cookie_monster.h
// for details.
const size_t CookieMonster::kDomainMaxCookies = 180;
const size_t CookieMonster::kDomainPurgeCookies = 30;
const size_t CookieMonster::kMaxCookies = 3300;
const size_t CookieMonster::kPurgeCookies = 300;

const size_t CookieMonster::kMaxDomainPurgedKeys = 100;

const size_t CookieMonster::kPerPartitionDomainMaxCookieBytes = 10240;
const size_t CookieMonster::kPerPartitionDomainMaxCookies = 180;

const size_t CookieMonster::kDomainCookiesQuotaLow = 30;
const size_t CookieMonster::kDomainCookiesQuotaMedium = 50;
const size_t CookieMonster::kDomainCookiesQuotaHigh =
    kDomainMaxCookies - kDomainPurgeCookies - kDomainCookiesQuotaLow -
    kDomainCookiesQuotaMedium;

const int CookieMonster::kSafeFromGlobalPurgeDays = 30;

namespace {
int64_t CookieMonsterRustScalar(uint32_t operation,int64_t first=0,int64_t second=0) {
  CookieMonsterRustNative context(static_cast<CookieMonster*>(nullptr));CookieFamilyRequest request{};
  request.scalar[0]=first;request.scalar[1]=second;return context.Run(operation,request).scalar;
}
bool CookieMonsterRustBool(uint32_t operation,int64_t first,int64_t second=0) {return CookieMonsterRustScalar(operation,first,second)!=0;}
}  // namespace


CookieMonster::CookieMonster(scoped_refptr<PersistentCookieStore> store,
                             NetLog* net_log,
                             std::unique_ptr<PrefDelegate> pref_delegate)
    : CookieMonster(base::PassKey<CookieMonster>(),
                    std::move(store),
                    base::Microseconds(CookieMonsterRustScalar(2070)),
                    net_log,
                    std::move(pref_delegate)) {}

CookieMonster::CookieMonster(base::PassKey<CookieMonster>,
                             scoped_refptr<PersistentCookieStore> store,
                             base::TimeDelta last_access_threshold,
                             NetLog* net_log,
                             std::unique_ptr<PrefDelegate> pref_delegate)
    : change_dispatcher_(this),
      net_log_(NetLogWithSource::Make(net_log, NetLogSourceType::COOKIE_STORE)),
      pref_delegate_(std::move(pref_delegate)),
      store_(std::move(store)),
      last_access_threshold_(last_access_threshold),
      last_statistic_record_time_(base::Time::Now()) {
  cookieable_schemes_ = GetDefaultCookieableSchemes();
  net_log_.BeginEvent(NetLogEventType::COOKIE_STORE_ALIVE, [&] {
    return NetLogCookieMonsterConstructorParams(store_ != nullptr);
  });
}

// static
std::unique_ptr<CookieMonster> CookieMonster::CreateForTesting(
    scoped_refptr<PersistentCookieStore> store,
    base::TimeDelta last_access_threshold,
    NetLog* net_log,
    std::unique_ptr<PrefDelegate> pref_delegate) {
  return std::make_unique<CookieMonster>(
      base::PassKey<CookieMonster>(), std::move(store), last_access_threshold,
      net_log, std::move(pref_delegate));
}

// Asynchronous CookieMonster API

void CookieMonster::FlushStore(base::OnceClosure callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.closure=std::move(callback);
  context.Run(2000,request);
}

void CookieMonster::SetForceKeepSessionState() {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  context.Run(2001,request);
}

void CookieMonster::SetAllCookiesAsync(const CookieList& list,
                                       SetCookiesCallback callback) {
  DoCookieCallback(base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::SetAllCookies, base::Unretained(this), list,
      std::move(callback)));
}

void CookieMonster::SetCanonicalCookieAsync(
    std::unique_ptr<CanonicalCookie> cookie,
    const GURL& source_url,
    const CookieOptions& options,
    SetCookiesCallback callback,
    std::optional<CookieAccessResult> cookie_access_result) {
  if constexpr (DCHECK_IS_ON()) {
    CanonicalCookie::CanonicalizationResult result = cookie->IsCanonical();
    DCHECK(result) << result;
  }

  std::string domain = cookie->Domain();
  DoCookieCallbackForHostOrDomain(
      base::BindOnce(
          // base::Unretained is safe as DoCookieCallbackForHostOrDomain stores
          // the callback on |*this|, so the callback will not outlive
          // the object.
          &CookieMonster::SetCanonicalCookie, base::Unretained(this),
          std::move(cookie), source_url, options, std::move(callback),
          std::move(cookie_access_result)),
      domain);
}

void CookieMonster::SetUnsafeCanonicalCookieForTestAsync(
    std::unique_ptr<CanonicalCookie> cookie,
    SetCookiesCallback callback) {
  CanonicalCookie::CanonicalizationResult result = cookie->IsCanonical();
  CHECK(result) << result;

  std::string domain = cookie->Domain();
  DoCookieCallbackForHostOrDomain(
      base::BindOnce(
          // base::Unretained is safe as DoCookieCallbackForHostOrDomain stores
          // the callback on |*this|, so the callback will not outlive
          // the object.
          &CookieMonster::SetUnsafeCanonicalCookieForTest,
          base::Unretained(this), std::move(cookie), std::move(callback)),
      domain);
}

void CookieMonster::GetCookieListWithOptionsAsync(
    const GURL& url,
    const CookieOptions& options,
    const CookiePartitionKeyCollection& cookie_partition_key_collection,
    GetCookieListCallback callback) {
  TRACE_EVENT("net", "CookieMonster::GetCookieListWithOptionsAsync");

  DoCookieCallbackForURL(
      base::BindOnce(
          // base::Unretained is safe as DoCookieCallbackForURL stores
          // the callback on |*this|, so the callback will not outlive
          // the object.
          &CookieMonster::GetCookieListWithOptions, base::Unretained(this), url,
          options, cookie_partition_key_collection, std::move(callback)),
      url);
}

void CookieMonster::GetAllCookiesAsync(GetAllCookiesCallback callback) {
  DoCookieCallback(base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::GetAllCookies, base::Unretained(this),
      std::move(callback)));
}

void CookieMonster::GetAllCookiesWithAccessSemanticsAsync(
    GetAllCookiesWithAccessSemanticsCallback callback) {
  DoCookieCallback(base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::GetAllCookies, base::Unretained(this),
      base::BindOnce(&CookieMonster::AttachAccessSemanticsListForCookieList,
                     base::Unretained(this), std::move(callback))));
}

void CookieMonster::DeleteCanonicalCookieAsync(const CanonicalCookie& cookie,
                                               DeleteCallback callback) {
  auto delete_callback = base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::DeleteCanonicalCookie, base::Unretained(this), cookie,
      std::move(callback));
  if (CookieMonsterRustBool(2060,base::FeatureList::IsEnabled(features::kCookieMonsterPerKeyDelete),true)) {
    // `DeleteCanonicalCookie` only inspects
    // `equal_range(GetKey(cookie.Domain()))`. Dispatching via
    // `DoCookieCallbackForHostOrDomain` instead of `DoCookieCallback` avoids
    // setting `seen_global_task_ = true`, which would otherwise disable per-key
    // loading and force all subsequent cookie reads and writes across all
    // domains to wait for the full SQLite store load.
    DoCookieCallbackForHostOrDomain(std::move(delete_callback),
                                    cookie.Domain());
    return;
  }
  DoCookieCallback(std::move(delete_callback));
}

void CookieMonster::DeleteAllCreatedInTimeRangeAsync(
    const TimeRange& creation_range,
    DeleteCallback callback) {
  DoCookieCallback(base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::DeleteAllCreatedInTimeRange, base::Unretained(this),
      creation_range, std::move(callback)));
}

void CookieMonster::DeleteAllMatchingInfoAsync(CookieDeletionInfo delete_info,
                                               DeleteCallback callback) {
  std::optional<std::string> key =
      delete_info.SingleMatchingRegistrableDomain();
  base::UmaHistogramBoolean(
      "Cookie.DeleteAllMatchingInfoAsync.HasSingleMatchingDomain",
      key.has_value());

  auto cookie_matcher =
      base::BindRepeating(&CookieMonster::MatchCookieDeletionInfo,
                          base::Unretained(this), std::move(delete_info));
  auto delete_callback = base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::DeleteMatchingCookies, base::Unretained(this),
      std::move(cookie_matcher), DELETE_COOKIE_EXPLICIT, std::move(callback));

  if (CookieMonsterRustBool(2060,base::FeatureList::IsEnabled(features::kCookieMonsterPerKeyDelete),key.has_value())) {
    DoCookieCallbackForHostOrDomain(std::move(delete_callback), *key);
    return;
  }
  DoCookieCallback(std::move(delete_callback));
}

void CookieMonster::DeleteSessionCookiesAsync(
    CookieStore::DeleteCallback callback) {
  auto session_cookie_matcher =
      base::BindRepeating([](const net::CanonicalCookie& cookie) {
        return CookieMonsterRustBool(2061,cookie.ExpiryDate().ToInternalValue());
      });
  DoCookieCallback(base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::DeleteMatchingCookies, base::Unretained(this),
      std::move(session_cookie_matcher), DELETE_COOKIE_EXPIRED,
      std::move(callback)));
}

void CookieMonster::DeleteMatchingCookiesAsync(
    CookieStore::DeletePredicate predicate,
    CookieStore::DeleteCallback callback) {
  DoCookieCallback(base::BindOnce(
      // base::Unretained is safe as DoCookieCallback stores
      // the callback on |*this|, so the callback will not outlive
      // the object.
      &CookieMonster::DeleteMatchingCookies, base::Unretained(this),
      std::move(predicate), DELETE_COOKIE_EXPLICIT, std::move(callback)));
}

void CookieMonster::SetCookieableSchemes(
    std::vector<std::string> schemes,
    SetCookieableSchemesCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.schemes=std::move(schemes);context.schemes_callback=std::move(callback);
  context.Run(2002,request);
}

// This function must be called before the CookieMonster is used.
void CookieMonster::SetPersistSessionCookies(bool persist_session_cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  request.scalar[0]=persist_session_cookies;
  context.Run(2003,request);
}

// static
std::vector<std::string> CookieMonster::GetDefaultCookieableSchemes() {
CookieMonsterRustNative context(static_cast<CookieMonster*>(nullptr));
  CookieFamilyRequest request{};

  context.Run(2004,request);
  return std::move(context.schemes);
}

CookieChangeDispatcher& CookieMonster::GetChangeDispatcher() {
  return change_dispatcher_;
}

CookieMonster::~CookieMonster() {
  CookieMonsterRustNative::BeforeDestruction(this);
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  context.Run(2005,request);
}

// static
bool CookieMonster::CookieSorter(const CanonicalCookie* cc1,
                                 const CanonicalCookie* cc2) {
CookieMonsterRustNative context(static_cast<CookieMonster*>(nullptr));
  CookieFamilyRequest request{};
  request.scalar[0]=context.Add(const_cast<CanonicalCookie*>(cc1));request.scalar[1]=context.Add(const_cast<CanonicalCookie*>(cc2));
  const auto result=context.Run(2006,request);
  return result.scalar;
}

void CookieMonster::GetAllCookies(GetAllCookiesCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.all_callback=std::move(callback);
  context.Run(2007,request);
}

void CookieMonster::AttachAccessSemanticsListForCookieList(
    GetAllCookiesWithAccessSemanticsCallback callback,
    const CookieList& cookie_list) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.input_list=&cookie_list;context.semantics_callback=std::move(callback);
  context.Run(2008,request);
}

void CookieMonster::GetCookieListWithOptions(
    const GURL& url,
    const CookieOptions& options,
    const CookiePartitionKeyCollection& cookie_partition_key_collection,
    GetCookieListCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.url=&url;context.domain=std::string(url.host());context.options=&options;context.collection=&cookie_partition_key_collection;context.list_callback=std::move(callback);
  context.Run(2009,request);
}

void CookieMonster::DeleteAllCreatedInTimeRange(const TimeRange& creation_range,
                                                DeleteCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.argument[40]=creation_range.start().ToInternalValue();context.argument[41]=creation_range.end().ToInternalValue();context.delete_callback=std::move(callback);
  context.Run(2010,request);
}

bool CookieMonster::MatchCookieDeletionInfo(
    const CookieDeletionInfo& delete_info,
    const net::CanonicalCookie& cookie) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.deletion_info=&delete_info;context.argument[43]=delete_info.url.has_value();context.Add(const_cast<CanonicalCookie*>(&cookie));context.domain=cookie.Domain();
  const auto result=context.Run(2011,request);
  return result.scalar;
}

void CookieMonster::DeleteCanonicalCookie(const CanonicalCookie& cookie,
                                          DeleteCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.argument[79]=context.Add(const_cast<CanonicalCookie*>(&cookie));context.delete_callback=std::move(callback);
  context.Run(2012,request);
}

void CookieMonster::DeleteMatchingCookies(DeletePredicate predicate,
                                          DeletionCause cause,
                                          DeleteCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.predicate=std::move(predicate);context.argument[42]=cause;context.delete_callback=std::move(callback);
  context.Run(2013,request);
}

void CookieMonster::MarkCookieStoreAsInitialized() {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  context.Run(2014,request);
}

void CookieMonster::FetchAllCookiesIfNecessary() {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  context.Run(2015,request);
}

void CookieMonster::FetchAllCookies() {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  context.Run(2016,request);
}

void CookieMonster::OnLoaded(
    TimeTicks beginning_time,
    std::vector<std::unique_ptr<CanonicalCookie>> cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.load_begin=beginning_time;context.mutable_loaded_list=&cookies;
  context.Run(2017,request);
}

void CookieMonster::OnKeyLoaded(
    const std::string& key,
    std::vector<std::unique_ptr<CanonicalCookie>> cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.key=key;context.domain=key;context.mutable_loaded_list=&cookies;
  context.Run(2018,request);
}

void CookieMonster::StoreLoadedCookies(
    std::vector<std::unique_ptr<CanonicalCookie>> cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.mutable_loaded_list=&cookies;
  context.Run(2019,request);
}

void CookieMonster::InvokeQueue() {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  context.Run(2020,request);
}

void CookieMonster::EnsureCookiesMapIsValid() {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  context.Run(2021,request);
}

// Our strategy to find duplicates is:
// (1) Build a map from cookie unique key to
//     {list of cookies with this signature, sorted by creation time}.
// (2) For each list with more than 1 entry, keep the cookie having the
//     most recent creation time, and delete the others.
//
void CookieMonster::TrimDuplicateCookiesForKey(
    const std::string& key,
    CookieMap::iterator begin,
    CookieMap::iterator end,
    std::optional<PartitionedCookieMap::iterator> cookie_partition_it) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.key=key;context.domain=key;context.input_range.emplace(begin,end);context.selected_partition=cookie_partition_it;context.input_map=cookie_partition_it?cookie_partition_it.value()->second.get():&cookies_;
  context.Run(2022,request);
}

std::vector<CanonicalCookie*>
CookieMonster::FindCookiesForRegistryControlledHost(
    const GURL& url,
    CookieMap* cookie_map,
    CookieMonster::PartitionedCookieMap::iterator* partition_it) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.url=&url;context.domain=std::string(url.host());context.input_map=cookie_map;if(partition_it)context.selected_partition=*partition_it;
  context.Run(2023,request);
  return std::move(context.pointers);
}

std::vector<CanonicalCookie*>
CookieMonster::FindPartitionedCookiesForRegistryControlledHost(
    const CookiePartitionKey& cookie_partition_key,
    const GURL& url) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.url=&url;context.domain=std::string(url.host());context.input_partition=&cookie_partition_key;
  context.Run(2024,request);
  return std::move(context.pointers);
}

void CookieMonster::FilterCookiesWithOptions(
    const GURL& url,
    const CookieOptions& options,
    const CookiePartitionKeyCollection& cookie_partition_key_collection,
    std::vector<CanonicalCookie*>& cookie_ptrs,
    CookieAccessResultList& included_cookies,
    CookieAccessResultList& excluded_cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.url=&url;context.domain=std::string(url.host());context.options=&options;context.collection=&cookie_partition_key_collection;context.input_pointers=&cookie_ptrs;context.included_target=&included_cookies;context.excluded_target=&excluded_cookies;
  context.Run(2025,request);
}

CookieMonster::CookieChangeObservability
CookieMonster::MaybeDeleteEquivalentCookieAndUpdateStatus(
    const std::string& key,
    const CanonicalCookie& cookie_being_set,
    bool allowed_to_set_secure_cookie,
    bool skip_httponly,
    bool already_expired,
    base::Time& creation_date_to_inherit,
    CookieInclusionStatus& status,
    std::optional<PartitionedCookieMap::iterator> cookie_partition_it) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.key=key;context.domain=cookie_being_set.Domain();const auto id=context.Add(const_cast<CanonicalCookie*>(&cookie_being_set));context.argument[79]=id;context.selected_partition=cookie_partition_it;context.inherited_creation=creation_date_to_inherit;context.access[0].status=status;context.status_reference=&status;request.scalar[0]=id;request.scalar[1]=allowed_to_set_secure_cookie;request.scalar[2]=skip_httponly;request.scalar[3]=already_expired;
  const auto result=context.Run(2026,request);
  creation_date_to_inherit=context.inherited_creation;status=context.access[0].status;return static_cast<CookieChangeObservability>(result.scalar);
}

// static
CookieChangeCause CookieMonster::ToCookieChangeCause(
    CookieChangeObservability observability) {
CookieMonsterRustNative context(static_cast<CookieMonster*>(nullptr));
  CookieFamilyRequest request{};
  request.scalar[0]=static_cast<int32_t>(observability);
  const auto result=context.Run(2027,request);
  return static_cast<CookieChangeCause>(result.scalar);
}

std::variant<CookieMonster::CookieMap::iterator,
             CookieMonster::PartitionedCookieMapIterators>
CookieMonster::InternalInsertCookie(const std::string& key,
                                    std::unique_ptr<CanonicalCookie> cc,
                                    bool sync_to_store,
                                    const CookieAccessResult& access_result,
                                    bool dispatch_change,
                                    CookieChangeObservability observability) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.key=key;const auto id=context.Own(std::move(cc));context.argument[79]=id;context.access[0]=access_result;request.scalar[0]=id;request.scalar[1]=sync_to_store;request.scalar[2]=dispatch_change;request.scalar[3]=static_cast<int32_t>(observability);
  context.Run(2028,request);
  CHECK(context.inserted);return std::move(*context.inserted);
}

bool CookieMonster::ShouldUpdatePersistentStore(CanonicalCookie& cc) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  request.scalar[0]=context.Add(&cc);
  const auto result=context.Run(2029,request);
  return result.scalar;
}

void CookieMonster::SetCanonicalCookie(
    std::unique_ptr<CanonicalCookie> cc,
    const GURL& source_url,
    const CookieOptions& options,
    SetCookiesCallback callback,
    std::optional<CookieAccessResult> cookie_access_result) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.url=&source_url;context.domain=cc->Domain();context.options=&options;context.argument[79]=context.Own(std::move(cc));context.set_callback=std::move(callback);context.prior_access=std::move(cookie_access_result);if(context.prior_access)context.access[1]=*context.prior_access;
  context.Run(2030,request);
}

void CookieMonster::SetAllCookies(CookieList list,
                                  SetCookiesCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.input_list=&list;context.set_callback=std::move(callback);
  context.Run(2031,request);
}

void CookieMonster::SetUnsafeCanonicalCookieForTest(
    std::unique_ptr<CanonicalCookie> cc,
    SetCookiesCallback callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.argument[79]=context.Own(std::move(cc));context.set_callback=std::move(callback);
  context.Run(2032,request);
}

void CookieMonster::InternalUpdateCookieAccessTime(CanonicalCookie& cc,
                                                   Time current) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  request.scalar[0]=context.Add(&cc);request.scalar[1]=current.ToInternalValue();
  context.Run(2033,request);
}

// InternalDeleteCookies must not invalidate iterators other than the one being
// deleted.
void CookieMonster::InternalDeleteCookie(CookieMap::iterator it,
                                         bool sync_to_store,
                                         DeletionCause deletion_cause) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  request.scalar[0]=context.Add(it,&cookies_,std::nullopt);request.scalar[1]=sync_to_store;request.scalar[2]=deletion_cause;
  context.Run(2034,request);
}

void CookieMonster::InternalDeletePartitionedCookie(
    PartitionedCookieMap::iterator partition_it,
    CookieMap::iterator cookie_it,
    bool sync_to_store,
    DeletionCause deletion_cause) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.selected_partition=partition_it;request.scalar[0]=context.Add(cookie_it,partition_it->second.get(),partition_it);request.scalar[1]=sync_to_store;request.scalar[2]=deletion_cause;
  context.Run(2035,request);
}

// Domain expiry behavior is unchanged by key/expiry scheme (the
// meaning of the key is different, but that's not visible to this routine).
size_t CookieMonster::GarbageCollect(Time current, const std::string& key) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.key=key;context.domain=key;request.scalar[0]=current.ToInternalValue();
  const auto result=context.Run(2036,request);
  return static_cast<size_t>(result.scalar);
}

size_t CookieMonster::GarbageCollectPartitionedCookies(
    base::Time current,
    const CookiePartitionKey& cookie_partition_key,
    const std::string& key) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.key=key;context.domain=key;context.input_partition=&cookie_partition_key;request.scalar[0]=current.ToInternalValue();
  const auto result=context.Run(2037,request);
  return static_cast<size_t>(result.scalar);
}

size_t CookieMonster::PurgeLeastRecentMatches(CookieItVector& cookies,
                                              CookiePriority priority,
                                              size_t to_protect,
                                              size_t purge_goal,
                                              bool protect_secure_cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.input_vector=&cookies;request.scalar[0]=priority;request.scalar[1]=to_protect;request.scalar[2]=purge_goal;request.scalar[3]=protect_secure_cookies;
  const auto result=context.Run(2038,request);
  return static_cast<size_t>(result.scalar);
}

size_t CookieMonster::PurgeLeastRecentMatchesForOBC(
    CookieItList& cookies,
    CookiePriority priority,
    size_t to_protect,
    size_t purge_goal,
    bool delete_secure_cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.input_iterator_list=&cookies;request.scalar[0]=priority;request.scalar[1]=to_protect;request.scalar[2]=purge_goal;request.scalar[3]=delete_secure_cookies;
  const auto result=context.Run(2039,request);
  return static_cast<size_t>(result.scalar);
}

size_t CookieMonster::GarbageCollectExpired(Time current,
                                            const CookieMapItPair& itpair,
                                            CookieItVector* cookie_its) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.input_map=&cookies_;context.input_range=itpair;context.input_vector=cookie_its;request.scalar[0]=current.ToInternalValue();request.scalar[1]=cookie_its!=nullptr;
  const auto result=context.Run(2040,request);
  return static_cast<size_t>(result.scalar);
}

size_t CookieMonster::GarbageCollectExpiredPartitionedCookies(
    Time current,
    const PartitionedCookieMap::iterator& cookie_partition_it,
    const CookieMapItPair& itpair,
    CookieItVector* cookie_its) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.selected_partition=cookie_partition_it;context.input_map=cookie_partition_it->second.get();context.input_range=itpair;context.input_vector=cookie_its;request.scalar[0]=current.ToInternalValue();request.scalar[1]=cookie_its!=nullptr;
  const auto result=context.Run(2041,request);
  return static_cast<size_t>(result.scalar);
}

void CookieMonster::GarbageCollectAllExpiredPartitionedCookies(Time current) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  request.scalar[0]=current.ToInternalValue();
  context.Run(2042,request);
}

size_t CookieMonster::GarbageCollectDeleteRange(
    Time current,
    DeletionCause cause,
    CookieItVector::iterator it_begin,
    CookieItVector::iterator it_end) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.iterator_input.assign(it_begin,it_end);request.scalar[0]=cause;
  const auto result=context.Run(2043,request);
  return static_cast<size_t>(result.scalar);
}

size_t CookieMonster::GarbageCollectLeastRecentlyAccessed(
    base::Time current,
    base::Time safe_date,
    size_t purge_goal,
    CookieItVector cookie_its,
    base::Time& earliest_time) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.input_vector=&cookie_its;context.earliest_output=earliest_time;context.argument[72]=earliest_time.ToInternalValue();request.scalar[0]=safe_date.ToInternalValue();request.scalar[1]=purge_goal;
  const auto result=context.Run(2044,request);
  earliest_time=context.earliest_output;return static_cast<size_t>(result.scalar);
}

// A wrapper around registry_controlled_domains::GetDomainAndRegistry
// to make clear we're creating a key for our local map or for the persistent
// store's use. Here and in FindCookiesForRegistryControlledHost() are the only
// two places where we need to conditionalize based on key type.
//
// Note that this key algorithm explicitly ignores the scheme.  This is
// because when we're entering cookies into the map from the backing store,
// we in general won't have the scheme at that point.
// In practical terms, this means that file cookies will be stored
// in the map either by an empty string or by UNC name (and will be
// limited by kMaxCookiesPerHost), and extension cookies will be stored
// based on the single extension id, as the extension id won't have the
// form of a DNS host and hence GetKey() will return it unchanged.
//
// Arguably the right thing to do here is to make the key
// algorithm dependent on the scheme, and make sure that the scheme is
// available everywhere the key must be obtained (specfically at backing
// store load time).  This would require either changing the backing store
// database schema to include the scheme (far more trouble than it's worth), or
// separating out file cookies into their own CookieMonster instance and
// thus restricting each scheme to a single cookie monster (which might
// be worth it, but is still too much trouble to solve what is currently a
// non-problem).
//
// static
std::string CookieMonster::GetKey(std::string_view domain) {
CookieMonsterRustNative context(static_cast<CookieMonster*>(nullptr));
  CookieFamilyRequest request{};
  context.domain=std::string(domain);
  context.Run(2045,request);
  return std::move(context.key);
}

bool CookieMonster::HasCookieableScheme(const GURL& url) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.url=&url;
  const auto result=context.Run(2046,request);
  return result.scalar;
}

CookieAccessSemantics CookieMonster::GetAccessSemanticsForCookie(
    const CanonicalCookie& cookie) const {
CookieMonsterRustNative context(static_cast<const CookieMonster*>(this));
  CookieFamilyRequest request{};
  request.scalar[0]=context.Add(const_cast<CanonicalCookie*>(&cookie));
  const auto result=context.Run(2047,request);
  return static_cast<CookieAccessSemantics>(result.scalar);
}

CookieScopeSemantics CookieMonster::GetScopeSemanticsForCookieDomain(
    const std::string_view domain) const {
CookieMonsterRustNative context(static_cast<const CookieMonster*>(this));
  CookieFamilyRequest request{};
  context.domain=std::string(domain);request.scalar[0]=-1;
  const auto result=context.Run(2048,request);
  return static_cast<CookieScopeSemantics>(result.scalar);
}

CookieScopeSemantics CookieMonster::CheckAndActivateLegacyScopeBehavior(
    const std::string_view domain) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.domain=std::string(domain);
  const auto result=context.Run(2049,request);
  return static_cast<CookieScopeSemantics>(result.scalar);
}

void CookieMonster::DeleteAllAliasingCookies(const std::string& domain) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.domain=domain;context.key=domain;
  context.Run(2050,request);
}

void CookieMonster::UpdateMostRecentCookie(
    const CookieMapItPair& itpair,
    std::map<UniqueCookieKey, std::pair<base::Time, UniqueCookieKey>>&
        most_recent_cookies) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.input_range=itpair;context.input_map=&cookies_;context.recent_output=&most_recent_cookies;
  context.Run(2051,request);
}

// Test to see if stats should be recorded, and record them if so.
// The goal here is to get sampling for the average browser-hour of
// activity.  We won't take samples when the web isn't being surfed,
// and when the web is being surfed, we'll take samples about every
// kRecordStatisticsIntervalSeconds.
// last_statistic_record_time_ is initialized to Now() rather than null
// in the constructor so that we won't take statistics right after
// startup, to avoid bias from browsers that are started but not used.
void CookieMonster::RecordPeriodicStats(base::Time current_time) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  request.scalar[0]=current_time.ToInternalValue();
  context.Run(2052,request);
}

bool CookieMonster::DoRecordPeriodicStats() {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};

  const auto result=context.Run(2053,request);
  return result.scalar;
}

void CookieMonster::DoCookieCallback(base::OnceClosure callback) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.closure=std::move(callback);
  context.Run(2054,request);
}

void CookieMonster::OnPreconnect(const GURL& url) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.url=&url;
  context.Run(2055,request);
}

void CookieMonster::DoCookieCallbackForURL(base::OnceClosure callback,
                                           const GURL& url) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.closure=std::move(callback);context.domain=std::string(url.host());
  context.Run(2056,request);
}

void CookieMonster::DoCookieCallbackForHostOrDomain(
    base::OnceClosure callback,
    std::string_view host_or_domain) {
CookieMonsterRustNative context(this);
  CookieFamilyRequest request{};
  context.closure=std::move(callback);context.domain=std::string(host_or_domain);
  context.Run(2057,request);
}

CookieMonster::CookieSentToSamePort
CookieMonster::IsCookieSentToSamePortThatSetIt(
    const GURL& destination,
    int source_port,
    CookieSourceScheme source_scheme) {
CookieMonsterRustNative context(static_cast<CookieMonster*>(nullptr));
  CookieFamilyRequest request{};
  context.url=&destination;request.scalar[0]=source_port;request.scalar[2]=static_cast<int32_t>(source_scheme);
  const auto result=context.Run(2058,request);
  return static_cast<CookieSentToSamePort>(result.scalar);
}

std::optional<bool> CookieMonster::SiteHasCookieInOtherPartition(
    const net::SchemefulSite& site,
    const CookiePartitionKey& partition_key) const {
CookieMonsterRustNative context(static_cast<const CookieMonster*>(this));
  CookieFamilyRequest request{};
  context.domain=site.GetURL().GetHost();context.input_partition=&partition_key;
  const auto result=context.Run(2059,request);
  if(result.scalar<0)return std::nullopt;return result.scalar!=0;
}

}  // namespace net
