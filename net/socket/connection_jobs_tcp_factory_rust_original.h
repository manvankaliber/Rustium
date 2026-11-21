// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECTION_JOBS_TCP_FACTORY_RUST_ORIGINAL_H_
#define NET_SOCKET_CONNECTION_JOBS_TCP_FACTORY_RUST_ORIGINAL_H_

#include "base/memory/raw_ref.h"
#include "base/component_export.h"
#include "net/socket/client_socket_factory.h"

namespace net {

// Reference-only TCP provider injection through the existing native factory
// interface. Production factories/custom overrides are never replaced globally.
// Non-TCP provider must outlive this factory; returned sockets keep their normal
// own lifetime and do not retain this adapter.
class COMPONENT_EXPORT(CONNECTION_JOBS_TCP_FACTORY_ORIGINAL)
    ConnectionJobsTcpFactoryRustOriginal final : public ClientSocketFactory {
 public:
  explicit ConnectionJobsTcpFactoryRustOriginal(
      ClientSocketFactory& non_tcp_provider);
  ~ConnectionJobsTcpFactoryRustOriginal() override;

  ConnectionJobsTcpFactoryRustOriginal(
      const ConnectionJobsTcpFactoryRustOriginal&) = delete;
  ConnectionJobsTcpFactoryRustOriginal& operator=(
      const ConnectionJobsTcpFactoryRustOriginal&) = delete;

  std::unique_ptr<DatagramClientSocket> CreateDatagramClientSocket(
      DatagramSocket::BindType bind_type,
      handles::NetworkHandle target_network,
      NetLog* net_log,
      const NetLogSource& source) override;

  std::unique_ptr<TransportClientSocket> CreateTransportClientSocket(
      const AddressList& addresses,
      handles::NetworkHandle target_network,
      std::unique_ptr<SocketPerformanceWatcher> socket_performance_watcher,
      NetworkQualityEstimator* network_quality_estimator,
      NetLog* net_log,
      const NetLogSource& source) override;

  std::unique_ptr<SSLClientSocket> CreateSSLClientSocket(
      SSLClientContext* context,
      std::unique_ptr<StreamSocket> stream_socket,
      const HostPortPair& host_and_port,
      const SSLConfig& ssl_config) override;

 private:
  // Explicit provider reference, not a fallback to public TCP creation.
  raw_ref<ClientSocketFactory> non_tcp_provider_;
};

}  // namespace net
#endif  // NET_SOCKET_CONNECTION_JOBS_TCP_FACTORY_RUST_ORIGINAL_H_
