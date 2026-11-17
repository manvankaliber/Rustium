// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_HOST_RESOLVER_CACHE_RUST_FFI_H_
#define NET_DNS_HOST_RESOLVER_CACHE_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
#include "base/memory/raw_ptr_exclusion.h"
struct HostResolverCacheRustValues { uint64_t a; uint64_t b; int64_t x; int64_t y; };
struct HostResolverCacheRustRequest {
  HostResolverCacheRustValues values;
  RAW_PTR_EXCLUSION const uint8_t* data;
  size_t length;
};
enum class HostResolverCacheRustOp : uint32_t {
  kSize = 1,
  kCapacity = 2,
  kGeneration = 3,
  kSetGeneration = 4,
  kCanonicalize = 5,
  kCompleteCanonical = 6,
  kOpenRange = 7,
  kStepBack = 8,
  kQuery = 9,
  kEntry = 10,
  kRetain = 11,
  kRetainedCount = 12,
  kCloseMatching = 13,
  kTickNow = 14,
  kWallNow = 15,
  kAssertTimed = 16,
  kExpiration = 17,
  kDifference = 18,
  kMagnitude = 19,
  kSelectResult = 20,
  kPreparePending = 21,
  kDropPending = 22,
  kEraseRetained = 23,
  kInsertPending = 24,
  kBeginEntries = 25,
  kCursorValid = 26,
  kCursorNext = 27,
  kEraseCursor = 28,
  kKeepBest = 29,
  kEraseBest = 30,
  kCheckBest = 31,
  kCheckCapacity = 32,
  kListReset = 33,
  kDictReset = 34,
  kDictInt = 35,
  kDictBool = 36,
  kNakValue = 37,
  kNakDebugValue = 38,
  kDictNak = 39,
  kResultValue = 40,
  kDictResult = 41,
  kNetwork = 42,
  kDictNetwork = 43,
  kListAppend = 44,
  kLoggingReset = 45,
  kCapacityInt = 46,
  kLoggingInt = 47,
  kLoggingEntries = 48,
  kRestoreList = 49,
  kRestoreDict = 50,
  kRestoreNakField = 51,
  kRestoreNak = 52,
  kRestoreSource = 53,
  kRestoreSecure = 54,
  kRestoreResult = 55,
  kRestorePending = 56,
  kRestoreReset = 57,
};
using HostResolverCacheRustCallback = HostResolverCacheRustValues (*)(void*, uint32_t, HostResolverCacheRustRequest);
using HostResolverCacheRustNakCompare = int32_t (*)(const void*);
extern "C" {
HostResolverCacheRustValues host_resolver_cache_rust_run(void*, HostResolverCacheRustCallback, uint32_t method, HostResolverCacheRustValues args);
bool host_resolver_cache_rust_capacity_valid(size_t);
bool host_resolver_cache_rust_at_capacity(size_t size, size_t capacity);
bool host_resolver_cache_rust_stale_result(bool generation, bool expired);
bool host_resolver_cache_rust_key_less(int32_t domain_compare, const void*, HostResolverCacheRustNakCompare, int64_t left_network, int64_t right_network);
}
static_assert(sizeof(HostResolverCacheRustValues) == 32);
#endif  // NET_DNS_HOST_RESOLVER_CACHE_RUST_FFI_H_
