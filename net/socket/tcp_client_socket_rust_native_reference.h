// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_H_
#define NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_H_
#include "build/build_config.h"
#include "net/socket/tcp_client_socket_rust_native_reference_descriptor.h"
#if BUILDFLAG(IS_WIN)
#include "net/socket/tcp_client_socket_rust_native_reference_win.h"
namespace net {
using TCPClientSocketRustNativeReferenceSocket =
    TCPClientSocketRustNativeReferenceWin;
}
#elif BUILDFLAG(IS_POSIX)
#include "net/socket/tcp_client_socket_rust_native_reference_tcp_posix.h"
namespace net {
using TCPClientSocketRustNativeReferenceSocket =
    TCPClientSocketRustNativeReferenceTcpPosix;
}
#endif
#endif
