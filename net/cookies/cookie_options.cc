// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Brought to you by number 42.

#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_family_native.h"

#include <tuple>

#include "net/cookies/cookie_util.h"

namespace net {
namespace cf=cookie_family;

CookieOptions::SameSiteCookieContext
CookieOptions::SameSiteCookieContext::MakeInclusive() {
  CookieFamilyRequest request{};return cf::Materialize(cf::Run(40,request).context);
}

CookieOptions::SameSiteCookieContext
CookieOptions::SameSiteCookieContext::MakeInclusiveForSet() {
  CookieFamilyRequest request{};request.scalar[0]=1;return cf::Materialize(cf::Run(40,request).context);
}

CookieOptions::SameSiteCookieContext::ContextType
CookieOptions::SameSiteCookieContext::GetContextForCookieInclusion() const {
  CookieFamilyRequest request{};request.context=cf::Snapshot(*this);return static_cast<ContextType>(cf::Run(41,request).scalar);
}

const CookieOptions::SameSiteCookieContext::ContextMetadata&
CookieOptions::SameSiteCookieContext::GetMetadataForCurrentSchemefulMode()
    const {
  return schemeful_metadata();
}

void CookieOptions::SameSiteCookieContext::SetContextTypesForTesting(
    ContextType context_type,
    ContextType schemeful_context_type) {
  context_ = context_type;
  schemeful_context_ = schemeful_context_type;
}

bool CookieOptions::SameSiteCookieContext::CompleteEquivalenceForTesting(
    const SameSiteCookieContext& other) const {
  CookieFamilyRequest request{};request.context=cf::Snapshot(*this);request.scalar[0]=1;
  cf::NativeContext native;native.other_context=&other;return cf::Run(42,request,&native).scalar;
}

bool operator==(const CookieOptions::SameSiteCookieContext& lhs,
                const CookieOptions::SameSiteCookieContext& rhs) {
  CookieFamilyRequest request{};request.context=cf::Snapshot(lhs);
  cf::NativeContext native;native.other_context=&rhs;return cf::Run(42,request,&native).scalar;
}

// Keep default values in sync with
// services/network/public/mojom/cookie_manager.mojom.
CookieOptions::CookieOptions()
    : same_site_cookie_context_(SameSiteCookieContext(
          SameSiteCookieContext::ContextType::CROSS_SITE)) {}

CookieOptions::CookieOptions(const CookieOptions& other) = default;
CookieOptions::CookieOptions(CookieOptions&& other) = default;
CookieOptions::~CookieOptions() = default;

CookieOptions& CookieOptions::operator=(const CookieOptions&) = default;
CookieOptions& CookieOptions::operator=(CookieOptions&&) = default;

// static
CookieOptions CookieOptions::MakeAllInclusive() {
  CookieFamilyRequest request{};const auto value=cf::Run(43,request).options;CookieOptions out;
  out.exclude_httponly_=value.exclude_httponly;out.same_site_cookie_context_=cf::Materialize(value.context);
  out.update_access_time_=value.update_access_time;out.return_excluded_cookies_=value.return_excluded;return out;
}

}  // namespace net
