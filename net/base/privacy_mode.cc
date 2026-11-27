// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/privacy_mode.h"

#include "base/notreached.h"
#include "net/base/net_diagnostics_debug_formatting_ffi.h"

namespace net {

const char* PrivacyModeToDebugString(PrivacyMode privacy_mode) {
  const char* result =
      net_diagnostics_privacy_mode(static_cast<int>(privacy_mode));
  if (!result) {
    NOTREACHED();
  }
  return result;
}

}  // namespace net
