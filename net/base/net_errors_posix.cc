// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/net_errors.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <type_traits>

#include "base/logging.h"
#include "base/posix/safe_strerror.h"
#include "build/build_config.h"
#include "net/base/net_error_rust_ffi.h"
namespace net {
namespace {
constexpr net_error_rust::PosixConstants kConstants = {
#define NET_ERROR_RUST_CONSTANT(name) name,
#include "net/base/net_error_rust_posix_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
};
}  // namespace
Error MapSystemError(logging::SystemErrorCode os_error) {
  static_assert(std::is_signed_v<logging::SystemErrorCode>);
  static_assert(sizeof(logging::SystemErrorCode) == sizeof(int32_t));
  if (os_error != 0) {
    DVLOG(2) << "Error " << os_error << ": "
             << logging::SystemErrorCodeToString(os_error);
  }
  const int32_t result = net_error_rust_posix(os_error, &kConstants);
  if (result == net_error_rust::kUnknown) {
    LOG(WARNING) << "Unknown error " << base::safe_strerror(os_error) << " ("
                 << os_error << ") mapped to net::ERR_FAILED";
    return ERR_FAILED;
  }
  return static_cast<Error>(result);
}
}  // namespace net
