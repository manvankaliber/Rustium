// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_HTTP2_FAMILY_ORIGINAL_SECURE_DNS_POLICY_H_
#define NET_SPDY_HTTP2_FAMILY_ORIGINAL_SECURE_DNS_POLICY_H_

#include "net/dns/public/secure_dns_policy.h"

namespace net::http2_family_original {

// Reference-local frozen mapping; compiled only into the HTTP/2 reference.
// SecureDnsPolicy remains the shared public enum, not a policy implementation.
const char* SecureDnsPolicyToDebugString(SecureDnsPolicy secure_dns_policy);

}  // namespace net::http2_family_original

#endif  // NET_SPDY_HTTP2_FAMILY_ORIGINAL_SECURE_DNS_POLICY_H_
