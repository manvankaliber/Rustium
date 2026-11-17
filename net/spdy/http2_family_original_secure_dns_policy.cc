// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/http2_family_original_secure_dns_policy.h"

namespace net::http2_family_original {

const char* SecureDnsPolicyToDebugString(SecureDnsPolicy secure_dns_policy) {
  switch (secure_dns_policy) {
    case SecureDnsPolicy::kAllow:
      return "allow";
    case SecureDnsPolicy::kDisable:
      return "disable";
    case SecureDnsPolicy::kBootstrap:
      return "bootstrap";
  }
}

}  // namespace net::http2_family_original
