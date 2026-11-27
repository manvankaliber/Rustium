// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_PARTITION_KEY_RUST_FFI_H_
#define NET_COOKIES_COOKIE_PARTITION_KEY_RUST_FFI_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct CookiePartitionKeyRustResult {
  int32_t status;
  int32_t ancestor;
  uint32_t site_source;
  uint32_t error;
  uint32_t warning_count;
};
static_assert(std::is_standard_layout_v<CookiePartitionKeyRustResult>);
static_assert(sizeof(CookiePartitionKeyRustResult) == 20);
static_assert(offsetof(CookiePartitionKeyRustResult, warning_count) == 16);

struct CookiePartitionKeyRustText {
  const uint8_t* data;
  size_t length;
};
static_assert(std::is_standard_layout_v<CookiePartitionKeyRustText>);
static_assert(offsetof(CookiePartitionKeyRustText, length) == sizeof(void*));

// Separate pieces preserve original optional-nonce then ancestor stream writes.
struct CookiePartitionKeyRustStreamSuffixes {
  CookiePartitionKeyRustText nonce;
  CookiePartitionKeyRustText ancestor;
};
static_assert(std::is_standard_layout_v<CookiePartitionKeyRustStreamSuffixes>);
static_assert(offsetof(CookiePartitionKeyRustStreamSuffixes, ancestor) ==
              sizeof(CookiePartitionKeyRustText));

namespace cookie_partition_key_rust {
enum Primitive : uint32_t {
  kSiteOpaque = 0,
  kSiteIsFile = 1,
  kSiteSerialize = 2,
  kSiteSerializeFileHost = 3,
  kSiteDeserialize = 4,
  kSerializedEqualsInput = 5,
  kNikPartition = 6,
  kNikNoncePresent = 7,
  kNikSelectTop = 8,
  kNikSelectFrame = 9,
  kSfcIsNull = 10,
  kSfcIsFirstParty = 11,
  kSiteEqual = 12,
  kSiteCompare = 13,
  kLeftNoncePresent = 14,
  kRightNoncePresent = 15,
  kNonceEqual = 16,
  kNonceCompare = 17,
};
enum Status : int32_t { kPresent = 0, kAbsent = 1, kError = 2 };
enum Query : uint32_t {
  kSerializeable = 0,
  kForbidsUnpartitioned = 1,
  kHasNonce = 2,
  kThirdParty = 3,
};
enum ParseMode : uint32_t {
  kInternalStrict = 0,
  kInternalLoose = 1,
  kStorage = 2,
  kUntrusted = 3,
};
}  // namespace cookie_partition_key_rust

// Synchronous, no exceptions/unwind/reentry. Native implementation owns and
// dereferences context, including initialized optional/string scratch. It may
// not retain call-scoped addresses. Rust never forms a reference to that state.
using CookiePartitionKeyRustPrimitive = int32_t (*)(void*, uint32_t);

extern "C" int32_t cookie_partition_key_rust_ancestor(bool cross_site);
extern "C" int32_t cookie_partition_key_rust_construct_ancestor(int32_t value);
extern "C" bool cookie_partition_key_rust_query(
    uint32_t operation, bool present, bool opaque, bool nonce, int32_t ancestor);
extern "C" CookiePartitionKeyRustResult cookie_partition_key_rust_serialize(
    bool present,
    bool nonce,
    int32_t ancestor,
    void* context,
    CookiePartitionKeyRustPrimitive primitive);
extern "C" CookiePartitionKeyRustResult cookie_partition_key_rust_parse(
    uint32_t mode,
    bool input_empty,
    int32_t ancestor,
    void* context,
    CookiePartitionKeyRustPrimitive primitive);
extern "C" CookiePartitionKeyRustResult cookie_partition_key_rust_from_nik(
    bool main_frame_navigation,
    void* context,
    CookiePartitionKeyRustPrimitive primitive);
extern "C" bool cookie_partition_key_rust_equals(
    int32_t left_ancestor,
    int32_t right_ancestor,
    void* context,
    CookiePartitionKeyRustPrimitive primitive);
extern "C" int32_t cookie_partition_key_rust_compare(
    int32_t left_ancestor,
    int32_t right_ancestor,
    void* context,
    CookiePartitionKeyRustPrimitive primitive);

// These immutable bytes belong to the Rust module, require no free, and live
// for its lifetime. Their exact length excludes any terminator.
extern "C" CookiePartitionKeyRustText cookie_partition_key_rust_message(
    uint32_t error);
extern "C" CookiePartitionKeyRustText cookie_partition_key_rust_serialized_suffix(
    bool cross_site);
extern "C" CookiePartitionKeyRustStreamSuffixes cookie_partition_key_rust_stream_suffix(
    bool nonce, int32_t ancestor);

#endif  // NET_COOKIES_COOKIE_PARTITION_KEY_RUST_FFI_H_
