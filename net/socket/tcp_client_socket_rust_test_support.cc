// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/tcp_client_socket_rust_test_support.h"
#if BUILDFLAG(IS_WIN)
namespace net {
template class TCPClientSocketRustFakeIOBase<TCPSocketWin>;
template class TCPClientSocketRustFakeIOBase<
    TCPClientSocketRustNativeReferenceSocket>;
}  // namespace net
#endif
