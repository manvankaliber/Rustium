// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_FAMILY_CANONICAL_NATIVE_H_
#define NET_COOKIES_COOKIE_FAMILY_CANONICAL_NATIVE_H_
#include "base/memory/raw_ptr.h"
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_family_native.h"
#include "net/cookies/parsed_cookie.h"
namespace net::cookie_family {
struct CanonicalFields {
  raw_ptr<const CanonicalCookie> cookie=nullptr;
  std::array<std::string,4> bytes;
  std::array<int64_t,18> number{};
  std::optional<CookiePartitionKey> partition;
};
struct CanonicalContext {
  NativeContext common;
  std::array<CanonicalFields,2> fields;
  CanonicalFields output;
  std::function<const CanonicalCookie&(size_t)> list_cookie;
  size_t list_size=0;
  raw_ptr<const ParsedCookie> parsed=nullptr;
  raw_ptr<const CookiePartitionKey> left_partition=nullptr;
  raw_ptr<const CookiePartitionKey> right_partition=nullptr;
  std::string scratch;
  std::exception_ptr exception;
  // Native callbacks write only initialized scratch/output. A host supplied
  // store context may override row resolution while retaining the same ABI.
  std::function<const CanonicalCookie&(int64_t)> cookie_lookup;
  std::function<const std::optional<CookiePartitionKey>&(int64_t)> partition_lookup;
  const CanonicalCookie* Cookie(int64_t) const;
  const std::optional<CookiePartitionKey>& Partition(int64_t) const;
};
void CanonicalNativeUnchecked(CanonicalContext&,const CookieFamilyCall&,CookieFamilyNativeResult&);
void CanonicalNative(void*,const CookieFamilyCall*,CookieFamilyNativeResult*);
CookieFamilyResult RunCanonical(uint32_t,const CookieFamilyRequest&,CanonicalContext&);
std::unique_ptr<CanonicalCookie> MaterializeCanonical(base::PassKey<CanonicalCookie>,CanonicalContext&);
// A valid-only URL snapshot avoids running native URL methods before the
// original Create invalid-URL gate. Invalid URL facts remain explicit.
CookieFamilyUrl CanonicalUrl(const GURL&);
}  // namespace net::cookie_family
#endif
