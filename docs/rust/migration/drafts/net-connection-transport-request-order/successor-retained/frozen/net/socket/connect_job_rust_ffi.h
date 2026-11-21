// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECT_JOB_RUST_FFI_H_
#define NET_SOCKET_CONNECT_JOB_RUST_FFI_H_
#include "net/base/net_export.h"
#include <cstdint>
namespace net::connect_job_rust {
enum class Operation : uint32_t {
  kInit=0, kDestroy=1, kConnect=2, kChangePriority=3, kPassSocket=4,
  kSetSocket=5, kNotifyCompletion=6, kNotifyProxyAuth=7, kResetTimer=8,
  kTimerIsRunning=9, kLogStart=10, kStopAndLogCompletion=11,
  kCompletionInsideTrace=12, kSetDoneClosure=13, kDefaultAttempts=14,
  kDefaultResolution=15, kDefaultCertificate=16, kDefaultEndpoint=17,
  kDefaultSslError=18, kDefaultStaleDns=19, kDefaultTimeoutHook=20,
};
enum class Action : uint32_t {
  kTopLevel=0, kHasDelegate=1, kDurationMicros=2, kStartTimer=3,
  kStopTimer=4, kStampStartNow=5, kBeginConnectEvent=6, kConnectInternal=7,
  kStoreFinalError=8, kStampEndNow=9, kEndConnectEvent=10, kTakeDelegate=11,
  kClearDelegate=12, kInvokeCompletionDelegate=13, kTraceCompletion=14,
  kInvokeProxyAuthDelegate=15, kHasInputSocket=16, kLogInputSocketSource=17,
  kHasDnsAliases=18, kMoveDnsAliases=19, kMoveInputSocket=20, kClearSocket=21,
  kTimedOutInternal=22, kTimedOutEvent=23, kBeginTopEvent=24, kEndTopEvent=25,
  kStorePriority=26, kChangePriorityInternal=27, kPassSocket=28,
  kInstallDoneClosure=29, kTimerRunning=30, kEmptyAttempts=31,
  kNoResolution=32, kNoCertificate=33, kNoEndpoint=34,
};
// Value storage is initialized Rust stack storage, valid only during the call.
// False observes receiver deletion; the produced scalar may still be usable.
using Service = bool (*)(void*, uint32_t, int64_t, int64_t*);
extern "C" int64_t connect_job_rust_run(uint32_t, int64_t, void*, Service);
extern "C" void connect_job_rust_timeout(void*, Service);
extern "C" int32_t connect_job_rust_done_error(uint32_t, int32_t);
extern "C" bool connect_job_rust_top_level(uint32_t);
}  // namespace net::connect_job_rust
#endif  // NET_SOCKET_CONNECT_JOB_RUST_FFI_H_
