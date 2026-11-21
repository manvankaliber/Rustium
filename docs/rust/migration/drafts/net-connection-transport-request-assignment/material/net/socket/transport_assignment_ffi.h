// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_TRANSPORT_ASSIGNMENT_FFI_H_
#define NET_SOCKET_TRANSPORT_ASSIGNMENT_FFI_H_
#include <stdint.h>
#include "net/base/net_export.h"
namespace net {
NET_EXPORT_PRIVATE int32_t TransportAssignmentPrefix(uint32_t bits, uint32_t count);
NET_EXPORT_PRIVATE bool TransportAssignmentUseFront(bool has_unassigned);
}  // namespace net
#endif  // NET_SOCKET_TRANSPORT_ASSIGNMENT_FFI_H_
