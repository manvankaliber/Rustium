// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/referrer_policy.h"

#include "net/url_request/referrer_policy_ffi.h"

namespace net {
static_assert(
    static_cast<int>(
        ReferrerPolicy::CLEAR_ON_TRANSITION_FROM_SECURE_TO_INSECURE) == 0);
static_assert(
    static_cast<int>(
        ReferrerPolicy::REDUCE_GRANULARITY_ON_TRANSITION_CROSS_ORIGIN) == 1);
static_assert(static_cast<int>(
                  ReferrerPolicy::ORIGIN_ONLY_ON_TRANSITION_CROSS_ORIGIN) == 2);
static_assert(static_cast<int>(ReferrerPolicy::NEVER_CLEAR) == 3);
static_assert(static_cast<int>(ReferrerPolicy::ORIGIN) == 4);
static_assert(
    static_cast<int>(ReferrerPolicy::CLEAR_ON_TRANSITION_CROSS_ORIGIN) == 5);
static_assert(
    static_cast<int>(
        ReferrerPolicy::ORIGIN_CLEAR_ON_TRANSITION_FROM_SECURE_TO_INSECURE) ==
    6);
static_assert(static_cast<int>(ReferrerPolicy::NO_REFERRER) == 7);

std::optional<ReferrerPolicy> ReferrerPolicyFromHeader(std::string_view input) {
  const int32_t policy = rustium_referrer_policy_parse(
      reinterpret_cast<const uint8_t*>(input.data()), input.size());
  if (policy < 0) {
    return std::nullopt;
  }
  return static_cast<ReferrerPolicy>(policy);
}
}  // namespace net
