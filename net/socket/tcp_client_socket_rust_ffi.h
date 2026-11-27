// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_TCP_CLIENT_SOCKET_RUST_FFI_H_
#define NET_SOCKET_TCP_CLIENT_SOCKET_RUST_FFI_H_
#include <stdint.h>
namespace net::tcp_client_socket_rust {
// By-value snapshots only. No C++ ownership or native object layouts cross ABI.
struct State {
  int64_t index, phase, previous, received, used, suspended;
};
struct Reply {
  State state;
  int64_t value;
  uint64_t alive;
};
static_assert(sizeof(State) == 48);
static_assert(sizeof(Reply) == 64);
enum class Operation : uint32_t {
  kInit = 0,
  kBind = 1,
  kConnect = 2,
  kCompleteConnect = 3,
  kDisconnect = 4,
  kSuspend = 5,
  kRead = 6,
  kReadReady = 7,
  kWrite = 8,
  kCompleteRead = 9,
  kCompleteWrite = 10,
  kCancelRead = 11,
  kLocalAddress = 12,
  kOpen = 13,
  kTimeout = 14,
};
enum class Action : uint32_t {
  kSave = 0,
  kValid = 1,
  kCount = 2,
  kHasBind = 3,
  kOpen = 4,
  kBindInput = 5,
  kBindSaved = 6,
  kSaveBind = 7,
  kClearBind = 8,
  kClose = 9,
  kInvalidate = 10,
  kDefaults = 11,
  kHasNetwork = 12,
  kBindNetwork = 13,
  kPortAllowed = 14,
  kBefore = 15,
  kWatcher = 16,
  kHasBefore = 17,
  kNotify = 18,
  kStartAttempt = 19,
  kHasAttempt = 20,
  kEmit = 21,
  kStopTimer = 22,
  kTimeoutEnabled = 23,
  kHasRtt = 24,
  kMinTimeout = 26,
  kMaxTimeout = 27,
  kAdaptiveTimeout = 28,
  kStartTimer = 29,
  kConnect = 30,
  kLogStart = 31,
  kLogEnd = 32,
  kResetCallbacks = 33,
  kStoreConnect = 34,
  kRead = 35,
  kReadReady = 36,
  kWrite = 37,
  kStoreRead = 38,
  kStoreWrite = 39,
  kResetRead = 40,
  kCancelRead = 41,
  kTakeWrite = 42,
  kRunWrite = 43,
  kCompleteRead = 44,
  kReadPending = 45,
  kInvokeConnect = 46,
  kInvokeRead = 47,
  kInvokeWrite = 48,
  kLocalAddress = 49,
  kCopyBind = 50,
  kValidateConnect = 51,
};
using Service = Reply (*)(void*, State, uint32_t, int64_t);
// Invocation-local POD storage only; callbacks retain neither output pointer.
// False means owner deletion or invalidation and forbids further owner access.
using FastService = bool (*)(void*, State*, uint32_t, int64_t, int64_t*);
extern "C" int64_t tcp_client_socket_rust_run(State,
                                              uint32_t,
                                              int64_t,
                                              void*,
                                              Service);
extern "C" int64_t tcp_client_socket_rust_run_fast(State*,
                                                   uint32_t,
                                                   int64_t,
                                                   void*,
                                                   FastService);
}  // namespace net::tcp_client_socket_rust
#endif
