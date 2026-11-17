// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_BACKOFF_RUST_FFI_H_
#define NET_BASE_BACKOFF_RUST_FFI_H_
#include <stdint.h>
struct BackoffRustPolicy {
  int32_t ignore;
  int64_t initial_ms;
  double factor;
  double jitter;
  int64_t maximum_ms;
  int64_t lifetime_ms;
  uint8_t always_initial;
};
struct BackoffRustEntry {
  int32_t failures;
  int64_t release;
};
extern "C" {
BackoffRustEntry backoff_rust_reset();
BackoffRustEntry backoff_rust_custom(BackoffRustEntry, int64_t);
BackoffRustEntry backoff_rust_inform(BackoffRustEntry,
                                     BackoffRustPolicy,
                                     uint8_t,
                                     const void*);
uint8_t backoff_rust_reject(BackoffRustEntry, const void*);
int64_t backoff_rust_until(BackoffRustEntry, const void*);
uint8_t backoff_rust_discard(BackoffRustEntry, BackoffRustPolicy, const void*);
int64_t backoff_rust_release_for_duration(int64_t,
                                          BackoffRustPolicy,
                                          const void*);
}
struct BackoffRustDecoded {
  int32_t failures;
  int64_t duration;
  int64_t absolute;
  uint8_t valid;
};
struct BackoffRustPair {
  int64_t first;
  int64_t second;
};
struct BackoffRustParsed {
  int64_t value;
  uint8_t valid;
};
extern "C" {
BackoffRustPair backoff_rust_serialize_times(int64_t, int64_t, int64_t);
BackoffRustDecoded backoff_rust_decode(const void*);
BackoffRustParsed backoff_rust_restore_duration(int64_t, int64_t, int64_t);
uint8_t backoff_rust_finite_release(int64_t);
}

#endif  // NET_BASE_BACKOFF_RUST_FFI_H_
