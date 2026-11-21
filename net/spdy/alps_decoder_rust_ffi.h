// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_ALPS_DECODER_RUST_FFI_H_
#define NET_SPDY_ALPS_DECODER_RUST_FFI_H_
#include <cstddef>
#include <cstdint>

#include "base/memory/raw_ptr_exclusion.h"

struct AlpsRustSetting {
  uint32_t id;
  uint32_t value;
};
struct AlpsRustHeader {
  int32_t error;
  bool accept;
};
struct AlpsRustPair {
  size_t origin;
  size_t origin_len;
  size_t value;
  size_t value_len;
};
struct AlpsRustPayload {
  RAW_PTR_EXCLUSION void* owner;
  RAW_PTR_EXCLUSION const AlpsRustPair* pairs;
  size_t count;
  int32_t bypass;
};
// All bools are initialized C++ bools; integer error IDs match AlpsDecoder.
// Payload consumes initialized readable bytes synchronously. Its initialized
// pair array contains bounded offsets, is immutable, and lives until free.
// No Rust input borrow survives the call. Payload makes no native callbacks;
// header may query the feature primitive with only scalar values live.
extern "C" {
int32_t alps_decoder_rust_bypass(int32_t, int32_t);
int32_t alps_decoder_rust_finish(bool, bool, bool, bool, int32_t);
bool alps_decoder_rust_common_header(bool, uint8_t);
int32_t alps_decoder_rust_settings_start(int32_t);
AlpsRustSetting alps_decoder_rust_setting(uint32_t, uint32_t);
bool alps_decoder_rust_settings_ack();
AlpsRustHeader alps_decoder_rust_header(int32_t,
                                        bool (*)(),
                                        uint32_t,
                                        uint8_t,
                                        uint8_t);
AlpsRustPayload alps_decoder_rust_payload(const uint8_t*, size_t);
// The disjoint caller array may be uninitialized. An inline result has null
// owner and borrows this array; an overflow result owns its immutable array.
// Both paths parse fully before native strings/containers mutate. The returned
// bypass includes the previous bypass according to Rust's sticky-error policy.
AlpsRustPayload alps_decoder_rust_payload_into(const uint8_t*,
                                               size_t,
                                               AlpsRustPair*,
                                               size_t,
                                               int32_t);
void alps_decoder_rust_payload_free(void*);
}
#endif  // NET_SPDY_ALPS_DECODER_RUST_FFI_H_
