// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_DESCRIPTOR_H_
#define NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_DESCRIPTOR_H_
#include "base/component_export.h"
#include "net/socket/socket_descriptor.h"
namespace net {
COMPONENT_EXPORT(TCP_CLIENT_SOCKET_RUST_REFERENCE)
SocketDescriptor TCPClientSocketRustNativeReferenceCreatePlatformSocket(
    int family,
    int type,
    int protocol);
}  // namespace net
#endif
