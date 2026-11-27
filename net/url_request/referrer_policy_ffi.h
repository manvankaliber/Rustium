// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_REFERRER_POLICY_FFI_H_
#define NET_URL_REQUEST_REFERRER_POLICY_FFI_H_

#include <stddef.h>
#include <stdint.h>

// Borrows initialized bytes for this call only. Null is valid at zero length.
// Returns the persisted public enum value, or -1 for no recognized token.
extern "C" int32_t rustium_referrer_policy_parse(const uint8_t* data,
                                                 size_t len);

#endif  // NET_URL_REQUEST_REFERRER_POLICY_FFI_H_
