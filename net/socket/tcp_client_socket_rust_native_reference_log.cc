// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/tcp_client_socket_rust_native_reference_log.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/values.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_endpoint.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_with_source.h"

namespace net {

base::DictValue TCPClientSocketRustNativeReferenceLogSocketErrorParams(
    int net_error,
    int os_error) {
  return base::DictValue()
      .Set("net_error", net_error)
      .Set("os_error", os_error);
}

void TCPClientSocketRustNativeReferenceLogSocketError(
    const NetLogWithSource& net_log,
    NetLogEventType type,
    int net_error,
    int os_error) {
  net_log.AddEvent(type, [&] {
    return TCPClientSocketRustNativeReferenceLogSocketErrorParams(net_error,
                                                                  os_error);
  });
}

base::DictValue TCPClientSocketRustNativeReferenceLogHostPortPair(
    const HostPortPair* host_and_port) {
  return base::DictValue().Set("host_and_port", host_and_port->ToString());
}

base::DictValue TCPClientSocketRustNativeReferenceLogIPEndPoint(
    const IPEndPoint* address) {
  return base::DictValue().Set("address", address->ToString());
}

base::DictValue TCPClientSocketRustNativeReferenceLogAddressPair(
    const net::IPEndPoint& local_address,
    const net::IPEndPoint& remote_address) {
  return base::DictValue()
      .Set("local_address", local_address.ToString())
      .Set("remote_address", remote_address.ToString());
}

}  // namespace net
