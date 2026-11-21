// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_FFI_H_
#define NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_FFI_H_
#include <stdint.h>
#include "net/base/net_export.h"
namespace net {
// Private component test/production bridge. -1 means retain prior selection.
NET_EXPORT_PRIVATE int32_t SelectStalledGroupBatch(uint64_t packed,
                                                  uint32_t count,
                                                  int32_t previous);
}  // namespace net
#endif  // NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_FFI_H_
