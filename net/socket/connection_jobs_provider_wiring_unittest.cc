// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <memory>
#include <type_traits>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/connection_jobs_tcp_factory_rust_original.h"
#include "net/socket/connection_jobs_upper_rust_test_support.h"
#include "net/socket/datagram_client_socket.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_client_socket_rust_reference.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::connection_jobs_provider_test {
namespace {
// Explicit witness for the provider cut. A custom provider is never replaced
// globally. The original adapter owns only TCP construction; SSL/datagram calls
// must forward their actual native values to this separate provider.
class NonTcpWitness final : public ClientSocketFactory {
 public:
  std::unique_ptr<DatagramClientSocket> CreateDatagramClientSocket(
      DatagramSocket::BindType bind, handles::NetworkHandle network,
      NetLog* log, const NetLogSource& source) override {
    ++datagram_calls;
    observed_bind = bind;
    observed_network = network;
    observed_log = log;
    observed_source = &source;
    return nullptr;
  }
  std::unique_ptr<TransportClientSocket> CreateTransportClientSocket(
      const AddressList&, handles::NetworkHandle,
      std::unique_ptr<SocketPerformanceWatcher>, NetworkQualityEstimator*,
      NetLog*, const NetLogSource&) override {
    ++transport_calls;
    return nullptr;
  }
  std::unique_ptr<SSLClientSocket> CreateSSLClientSocket(
      SSLClientContext* context, std::unique_ptr<StreamSocket> stream,
      const HostPortPair& host, const SSLConfig& config) override {
    ++ssl_calls;
    observed_context = context;
    observed_stream = std::move(stream);
    observed_host = &host;
    observed_config = &config;
    return nullptr;
  }
  unsigned transport_calls = 0;
  unsigned datagram_calls = 0;
  unsigned ssl_calls = 0;
  DatagramSocket::BindType observed_bind = DatagramSocket::DEFAULT_BIND;
  handles::NetworkHandle observed_network = handles::kInvalidNetworkHandle;
  raw_ptr<NetLog> observed_log = nullptr;
  raw_ptr<const NetLogSource> observed_source = nullptr;
  raw_ptr<SSLClientContext> observed_context = nullptr;
  std::unique_ptr<StreamSocket> observed_stream;
  raw_ptr<const HostPortPair> observed_host = nullptr;
  raw_ptr<const SSLConfig> observed_config = nullptr;
};

static_assert(std::is_base_of_v<ClientSocketFactory,
                              ConnectionJobsTcpFactoryRustOriginal>);
static_assert(std::is_base_of_v<TransportClientSocket,
                              TCPClientSocketRustReference>);

TEST(ConnectionJobsOriginalProviderTest, OriginalCommonParamsInjectTcpProvider) {
  base::test::TaskEnvironment environment{
      base::test::TaskEnvironment::MainThreadType::IO};
  NonTcpWitness witness;
  ConnectionJobsTcpFactoryRustOriginal provider(witness);
  auto params = connect_job_rust_test::MakeParams<ConnectJobRustOriginal>();
  params.client_socket_factory = &provider;
  const auto log = NetLogWithSource::Make(NetLog::Get(), NetLogSourceType::SOCKET);
  // Construction/query/destruction only. No Connect, Bind, socket read/write,
  // DNS or network request is made by this ordinary wiring test.
  const AddressList addresses(
      IPEndPoint(IPAddress::IPv4Localhost(), 80));
  auto socket = params.client_socket_factory->CreateTransportClientSocket(
      addresses, handles::kInvalidNetworkHandle, nullptr, nullptr,
      NetLog::Get(), log.source());
  ASSERT_NE(nullptr, socket);
  EXPECT_EQ(0u, witness.transport_calls);
  EXPECT_FALSE(socket->IsConnected());
  EXPECT_EQ(0, socket->GetTotalReceivedBytes());
  socket.reset();
  params.client_socket_factory = nullptr;
}

TEST(ConnectionJobsOriginalProviderTest, SslForwardsNativeViewsAndOwnership) {
  base::test::TaskEnvironment environment;
  connect_job_rust_test::Trace trace(false);
  NonTcpWitness witness;
  ConnectionJobsTcpFactoryRustOriginal provider(witness);
  const HostPortPair host("provider.example", 443);
  const SSLConfig config;
  auto stream = std::make_unique<connection_jobs_upper_test::FakeSocket>(trace, 7);
  StreamSocket* const identity = stream.get();
  const auto result = provider.CreateSSLClientSocket(
      nullptr, std::move(stream), host, config);
  EXPECT_EQ(nullptr, result);
  EXPECT_EQ(nullptr, stream);
  EXPECT_EQ(1u, witness.ssl_calls);
  EXPECT_EQ(0u, witness.transport_calls);
  EXPECT_EQ(identity, witness.observed_stream.get());
  EXPECT_EQ(&host, witness.observed_host);
  EXPECT_EQ(&config, witness.observed_config);
  EXPECT_EQ(nullptr, witness.observed_context);
  witness.observed_stream.reset();
}

TEST(ConnectionJobsOriginalProviderTest, DatagramProviderRemainsExplicit) {
  NonTcpWitness witness;
  ConnectionJobsTcpFactoryRustOriginal provider(witness);
  const auto log = NetLogWithSource::Make(NetLog::Get(), NetLogSourceType::SOCKET);
  const auto result = provider.CreateDatagramClientSocket(
      DatagramSocket::RANDOM_BIND, handles::NetworkHandle{37},
      NetLog::Get(), log.source());
  EXPECT_EQ(nullptr, result);
  EXPECT_EQ(1u, witness.datagram_calls);
  EXPECT_EQ(0u, witness.transport_calls);
  EXPECT_EQ(DatagramSocket::RANDOM_BIND, witness.observed_bind);
  EXPECT_EQ(handles::NetworkHandle{37}, witness.observed_network);
  EXPECT_EQ(NetLog::Get(), witness.observed_log);
  EXPECT_EQ(&log.source(), witness.observed_source);
}
}  // namespace
}  // namespace net::connection_jobs_provider_test
