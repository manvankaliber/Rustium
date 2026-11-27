// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_PORT_POLICY_FFI_H_
#define NET_BASE_PORT_POLICY_FFI_H_
#include <cstdint>
#include <memory>
#include <string>

#include "third_party/rust/cxx/v1/cxx.h"
namespace net::port_policy_internal {
// Callbacks are synchronous and token bytes are borrowed only for the call.
bool port_feature_enabled(bool localhost);
rust::Vec<uint8_t> port_feature_param(uint8_t kind);
void port_invalid_token(uint8_t kind, rust::Slice<const uint8_t> token);
}  // namespace net::port_policy_internal
#endif  // NET_BASE_PORT_POLICY_FFI_H_
