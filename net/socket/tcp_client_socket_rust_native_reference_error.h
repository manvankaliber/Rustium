// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_ERROR_H_
#define NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_ERROR_H_
#include "base/component_export.h"
#include "base/logging.h"
namespace net::tcp_client_socket_rust_native_reference {
enum Error {
  OK = 0,
#define NET_ERROR(label, value) ERR_##label = value,
#include "net/socket/tcp_client_socket_rust_native_reference_error_list.h"
#undef NET_ERROR
  ERR_CERT_BEGIN = ERR_CERT_COMMON_NAME_INVALID,
};
COMPONENT_EXPORT(TCP_CLIENT_SOCKET_RUST_REFERENCE)
Error MapSystemError(logging::SystemErrorCode os_error);
}  // namespace net::tcp_client_socket_rust_native_reference
#endif
