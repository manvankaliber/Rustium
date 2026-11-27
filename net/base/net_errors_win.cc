// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/net_errors.h"

#include <winsock2.h>

#include <type_traits>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "net/base/net_error_rust_ffi.h"
#include "net/base/net_error_rust_win_sdk_checks.h"
namespace net {
namespace {
#if DCHECK_IS_ON()
// Keep construction of the native log stream off disabled-verbose paths. The
// public wrapper retains DVLOG's exact gate and its ordering before mapping.
NOINLINE void RecordVerboseSystemError(uint32_t os_error) {
  VLOG_STREAM(2) << "Error " << os_error;
}
#endif
void WarnUnknownSystemError(uint32_t os_error) {
  LOG(WARNING) << "Unknown error " << os_error << " mapped to net::ERR_FAILED";
}
}  // namespace
Error MapSystemError(logging::SystemErrorCode os_error) {
  static_assert(std::is_unsigned_v<logging::SystemErrorCode>);
  static_assert(sizeof(logging::SystemErrorCode) == sizeof(uint32_t));
#if DCHECK_IS_ON()
  if (os_error != 0 && VLOG_IS_ON(2)) {
    RecordVerboseSystemError(os_error);
  }
#endif
  return static_cast<Error>(
      net_error_rust_windows_native(os_error, WarnUnknownSystemError));
}
}  // namespace net
