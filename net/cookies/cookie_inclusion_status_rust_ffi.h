// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_INCLUSION_STATUS_RUST_FFI_H_
#define NET_COOKIES_COOKIE_INCLUSION_STATUS_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>
// This is a call-scoped value, never the layout of CookieInclusionStatus.
struct CookieInclusionStatusRustState {
  uint64_t exclusions;
  uint64_t warnings;
  int32_t exemption;
  uint32_t invalid;
};
static_assert(std::is_standard_layout_v<CookieInclusionStatusRustState>);
static_assert(sizeof(CookieInclusionStatusRustState) == 24);
static_assert(offsetof(CookieInclusionStatusRustState, exemption) == 16);
// IDs are shared with the safe Rust query/apply dispatch. Invalid query IDs
// return false; invalid transition IDs report `invalid` without CHECKing in
// Rust.
namespace cookie_inclusion_status_rust {
enum Query : uint32_t {
  kInclude = 0,
  kHasExclusion = 1,
  kOnlyExclusion = 2,
  kRecordDowngrade = 3,
  kWarn = 4,
  kHasWarning = 5,
  kUserPreferencesOrTPCD = 6,
  kExactExclusions = 7,
  kExactWarnings = 8,
};
enum Transition : uint32_t {
  kAddExclusion = 0,
  kRemoveExclusion = 1,
  kRemoveExclusions = 2,
  kSetExemption = 3,
  kClearSameSite = 4,
  kClearThirdParty = 5,
  kAddWarning = 6,
  kRemoveWarning = 7,
  kReset = 8,
  kWithoutExclusions = 9,
};
}  // namespace cookie_inclusion_status_rust
extern "C" bool cookie_inclusion_status_rust_query(
    CookieInclusionStatusRustState,
    uint32_t,
    int32_t,
    uint64_t);
extern "C" bool cookie_inclusion_status_rust_same(
    CookieInclusionStatusRustState,
    CookieInclusionStatusRustState);
extern "C" CookieInclusionStatusRustState cookie_inclusion_status_rust_apply(
    CookieInclusionStatusRustState,
    uint32_t,
    int32_t,
    uint64_t);
extern "C" CookieInclusionStatusRustState
cookie_inclusion_status_rust_components(uint64_t, uint64_t, int32_t);
// `output` is initialized and exclusive for capacity bytes; null/empty sizes
// only.
extern "C" size_t cookie_inclusion_status_rust_debug(
    CookieInclusionStatusRustState,
    uint8_t* output,
    size_t capacity);
#endif
