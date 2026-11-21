// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_TRANSPORT_REQUEST_ORDER_FFI_H_
#define NET_SOCKET_TRANSPORT_REQUEST_ORDER_FFI_H_

#include <stddef.h>
#include <stdint.h>

#include "net/base/net_export.h"

namespace net {

// Low bit denotes front placement; remaining bits give the insertion rank.
// SIZE_MAX indicates an unrepresentable rank.
NET_EXPORT_PRIVATE size_t TransportRequestInsertPlan(int32_t respect_limits,
                                                     size_t higher_count,
                                                     size_t same_count);
// 0 retains position; 1 removes and reinserts at the requested priority.
NET_EXPORT_PRIVATE int32_t TransportRequestReprioritize(int32_t current,
                                                       int32_t requested);

}  // namespace net

#endif  // NET_SOCKET_TRANSPORT_REQUEST_ORDER_FFI_H_
