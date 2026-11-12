// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_REFERRER_POLICY_REFERENCE_H_
#define NET_URL_REQUEST_REFERRER_POLICY_REFERENCE_H_

#include "base/component_export.h"
#include "net/url_request/referrer_policy.h"

namespace net::referrer_policy_reference {
COMPONENT_EXPORT(REFERRER_POLICY_REFERENCE)
std::optional<ReferrerPolicy> ReferrerPolicyFromHeader(std::string_view input);
}  // namespace net::referrer_policy_reference

#endif  // NET_URL_REQUEST_REFERRER_POLICY_REFERENCE_H_
