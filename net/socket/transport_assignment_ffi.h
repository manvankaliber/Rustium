// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_TRANSPORT_ASSIGNMENT_FFI_H_
#define NET_SOCKET_TRANSPORT_ASSIGNMENT_FFI_H_

#include <stddef.h>

#include <limits>

#include "net/base/net_export.h"

namespace net {

inline constexpr size_t kTransportAssignmentNone =
    std::numeric_limits<size_t>::max();
inline constexpr size_t kTransportAssignmentFront =
    kTransportAssignmentNone - 1;
inline constexpr size_t kTransportAssignmentInvalid =
    kTransportAssignmentNone - 2;

// Test projections of the retained queue's production rank decisions.
// All other return values are ranks in the unbound request queue.
NET_EXPORT_PRIVATE size_t TransportAssignmentHole(size_t rank,
                                                 size_t request_count,
                                                 size_t assigned,
                                                 size_t unassigned);
NET_EXPORT_PRIVATE size_t TransportAssignmentRelease(size_t request_count,
                                                    size_t assigned,
                                                    size_t unassigned);

}  // namespace net

#endif  // NET_SOCKET_TRANSPORT_ASSIGNMENT_FFI_H_
