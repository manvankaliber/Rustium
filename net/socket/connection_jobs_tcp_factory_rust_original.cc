// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_tcp_factory_rust_original.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "net/socket/datagram_client_socket.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_client_socket_rust_reference.h"

namespace net {

static_assert(std::is_base_of_v<TransportClientSocket,
                              TCPClientSocketRustReference>);

ConnectionJobsTcpFactoryRustOriginal::ConnectionJobsTcpFactoryRustOriginal(
    ClientSocketFactory& non_tcp_provider)
    : non_tcp_provider_(non_tcp_provider) {}

ConnectionJobsTcpFactoryRustOriginal::~ConnectionJobsTcpFactoryRustOriginal() =
    default;

std::unique_ptr<DatagramClientSocket>
ConnectionJobsTcpFactoryRustOriginal::CreateDatagramClientSocket(
    DatagramSocket::BindType bind_type,
    handles::NetworkHandle target_network,
    NetLog* net_log,
    const NetLogSource& source) {
  return non_tcp_provider_->CreateDatagramClientSocket(
      bind_type, target_network, net_log, source);
}

std::unique_ptr<TransportClientSocket>
ConnectionJobsTcpFactoryRustOriginal::CreateTransportClientSocket(
    const AddressList& addresses,
    handles::NetworkHandle target_network,
    std::unique_ptr<SocketPerformanceWatcher> socket_performance_watcher,
    NetworkQualityEstimator* network_quality_estimator,
    NetLog* net_log,
    const NetLogSource& source) {
  // Exact original default transport-factory expression/argument order with
  // only TCPClientSocket replaced by the independently frozen original type.
  return std::make_unique<TCPClientSocketRustReference>(
      addresses, std::move(socket_performance_watcher),
      network_quality_estimator, net_log, source, target_network);
}

std::unique_ptr<SSLClientSocket>
ConnectionJobsTcpFactoryRustOriginal::CreateSSLClientSocket(
    SSLClientContext* context,
    std::unique_ptr<StreamSocket> stream_socket,
    const HostPortPair& host_and_port,
    const SSLConfig& ssl_config) {
  // Deliberate shared native TLS provider: not an original whole-TLS claim.
  return non_tcp_provider_->CreateSSLClientSocket(
      context, std::move(stream_socket), host_and_port, ssl_config);
}

}  // namespace net
