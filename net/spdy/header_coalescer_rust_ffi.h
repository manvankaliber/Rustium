// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HEADER_COALESCER_RUST_FFI_H_
#define NET_SPDY_HEADER_COALESCER_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>

struct HeaderCoalescerRustState {
  size_t header_list_size;
  uint8_t headers_valid;
  uint8_t error_seen;
  uint8_t regular_header_seen;
};
enum class HeaderCoalescerRustAction : uint8_t {
  kIgnore,
  kAppend,
  kEmptyName,
  kPseudoAfterRegular,
  kInvalidName,
  kUppercaseName,
  kTooLarge,
  kInvalidValue,
  kReleaseValid,
  kReleaseInvalid
};
struct HeaderCoalescerRustDecision {
  HeaderCoalescerRustState state;
  HeaderCoalescerRustAction action;
  uint8_t invalid_byte;
};
static_assert(std::is_standard_layout_v<HeaderCoalescerRustState>);
static_assert(offsetof(HeaderCoalescerRustState, headers_valid) ==
              sizeof(size_t));
static_assert(offsetof(HeaderCoalescerRustDecision, action) ==
              sizeof(HeaderCoalescerRustState));
static_assert(sizeof(bool) == sizeof(uint8_t));
static_assert(std::is_same_v<uint8_t, unsigned char>);
extern "C" HeaderCoalescerRustDecision header_coalescer_rust_header(
    HeaderCoalescerRustState state,
    const uint8_t* key,
    size_t key_len,
    const uint8_t* value,
    size_t value_len,
    uint32_t maximum);
extern "C" HeaderCoalescerRustDecision header_coalescer_rust_release(
    HeaderCoalescerRustState state);
// Action in bits 0..7 and next native validity byte in bits 8..15.
extern "C" uint16_t header_coalescer_rust_release_fast(uint8_t headers_valid);
// Scalar result: action in bits 0..7, invalid byte in bits 8..15, and the
// post-log error latch in bit 16. The two distinct native state fields are
// updated in original observation order. Inputs may alias either field.
// The latched-error path accesses no pointer. Otherwise size is initialized,
// aligned/writable and regular points to a valid one-byte native bool.
extern "C" uint32_t header_coalescer_rust_header_fast(size_t* size,
                                                      uint8_t* regular,
                                                      uint8_t error_seen,
                                                      const uint8_t* key,
                                                      size_t key_len,
                                                      const uint8_t* value,
                                                      size_t value_len,
                                                      uint32_t maximum);
#endif  // NET_SPDY_HEADER_COALESCER_RUST_FFI_H_
