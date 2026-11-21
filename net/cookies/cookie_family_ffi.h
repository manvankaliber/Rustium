// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_FAMILY_FFI_H_
#define NET_COOKIES_COOKIE_FAMILY_FFI_H_
#include "base/memory/raw_ptr_exclusion.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "net/cookies/cookie_inclusion_status_rust_ffi.h"
// C ABI byte views/buffers: Rust requires a literal pointer plus length.
struct CookieFamilyView { RAW_PTR_EXCLUSION const uint8_t* data; size_t size; };
struct CookieFamilyBuffer { RAW_PTR_EXCLUSION uint8_t* data; size_t size; size_t capacity; uint32_t present; };
struct CookieFamilyMetadata { int32_t downgrade; bool null_initiator; int32_t redirect; };
struct CookieFamilyContext {
  int32_t context; int32_t schemeful_context;
  CookieFamilyMetadata metadata; CookieFamilyMetadata schemeful_metadata;
};
struct CookieFamilyOptions {
  bool exclude_httponly; CookieFamilyContext context;
  bool update_access_time; bool return_excluded;
};
struct CookieFamilyParams { int32_t access_semantics; int32_t scope_semantics; bool delegate_trustworthy; };
struct CookieFamilyAccess {
  CookieInclusionStatusRustState status;
  int32_t effective_same_site; int32_t access_semantics; int32_t scope_semantics; bool allowed_secure;
};
struct CookieFamilyUrl {
  size_t identity; CookieFamilyView scheme; CookieFamilyView host; CookieFamilyView path;
  bool standard; bool is_ip; bool cryptographic; bool localhost; bool host_compliant;
  int32_t effective_port;
};
struct CookieFamilyCalendar {
  int32_t year; int32_t month; int32_t day_of_week; int32_t day;
  int32_t hour; int32_t minute; int32_t second; int32_t millisecond;
};
struct CookieFamilyKeyPolicy { int32_t kind; bool scheme; bool port; };
struct CookieFamilyRequest {
  CookieFamilyView first; CookieFamilyView second; CookieFamilyView third;
  CookieFamilyUrl url; int64_t scalar[8]; CookieFamilyContext context;
  CookieFamilyAccess access; CookieFamilyAccess other_access;
};
struct CookieFamilyResult {
  CookieFamilyBuffer buffer; int64_t scalar; CookieFamilyContext context;
  CookieFamilyAccess access; CookieFamilyOptions options; CookieFamilyKeyPolicy key;
};
struct CookieFamilyCall {
  uint32_t operation; int64_t first; int64_t second;
  CookieFamilyView bytes; CookieFamilyView bytes2; CookieFamilyCalendar calendar;
};
struct CookieFamilyNativeResult {
  int64_t number; int64_t number2; int64_t number3; int64_t number4;
  CookieFamilyView bytes; CookieFamilyView bytes2;
  CookieFamilyContext context; CookieFamilyOptions options; CookieFamilyParams params;
  uint32_t failed;
};
// Caller initializes all inputs/outputs. Native byte views remain live until
// the callback returns and are copied before another callback. Effects may
// reenter with a distinct call context; no reference to native mutable storage
// is kept by Rust. The request/result and callback output objects are disjoint.
using CookieFamilyNative = void (*)(void*, const CookieFamilyCall*, CookieFamilyNativeResult*);
extern "C" void cookie_family_call(uint32_t, const CookieFamilyRequest*, CookieFamilyResult*, void*, CookieFamilyNative);
extern "C" void cookie_family_buffer_free(CookieFamilyBuffer);
static_assert(std::is_standard_layout_v<CookieFamilyRequest>);
static_assert(std::is_standard_layout_v<CookieFamilyResult>);
static_assert(sizeof(CookieFamilyMetadata)==12);
static_assert(offsetof(CookieFamilyContext, metadata)==8);
static_assert(offsetof(CookieFamilyAccess, effective_same_site)==24);
static_assert(sizeof(CookieFamilyContext)==32);
static_assert(sizeof(CookieFamilyOptions)==40);
static_assert(sizeof(CookieFamilyParams)==12);
static_assert(sizeof(CookieFamilyAccess)==40);
static_assert(sizeof(CookieFamilyNativeResult)==120+4*sizeof(void*));
#endif
