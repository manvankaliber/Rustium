// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <optional>
#include <utility>

#include "net/cert/cert_verifier.h"
#include "net/quic/quic_http_utils.h"
#include "net/quic/quic_proxy_client_socket_test_fixture.h"
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"

namespace net::test {

namespace {

class ReadyProxySessionPool : public QuicSessionPool {
 public:
  ReadyProxySessionPool(
      HttpNetworkSession& dependencies,
      url::SchemeHostPort proxy,
      std::unique_ptr<QuicChromiumClientSession::Handle> handle)
      : QuicSessionPool(nullptr,
                        nullptr,
                        dependencies.ssl_config_service(),
                        nullptr,
                        dependencies.http_server_properties(),
                        dependencies.cert_verifier(),
                        dependencies.context().transport_security_state,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr,
                        dependencies.context().quic_context),
        proxy_(std::move(proxy)),
        handle_(std::move(handle)) {}

  int RequestSession(
      const QuicSessionKey& key,
      url::SchemeHostPort destination,
      quic::ParsedQuicVersion version,
      const std::optional<NetworkTrafficAnnotationTag>,
      MultiplexedSessionCreationInitiator,
      std::optional<ConnectionManagementConfig>,
      const HttpUserAgentSettings*,
      RequestPriority priority,
      bool use_dns_aliases,
      int cert_verify_flags,
      const GURL& url,
      const NetLogWithSource&,
      QuicSessionRequest* request) override {
    ++requests_;
    EXPECT_EQ(destination, proxy_);
    EXPECT_EQ(url, proxy_.GetURL());
    EXPECT_EQ(version, quic::ParsedQuicVersion::RFCv1());
    EXPECT_EQ(key.proxy_chain(), ProxyChain::Direct());
    EXPECT_EQ(priority, DEFAULT_PRIORITY);
    EXPECT_FALSE(use_dns_aliases);
    EXPECT_NE(cert_verify_flags & CertVerifier::VERIFY_DISABLE_NETWORK_FETCHES,
              0);
    if (!handle_) {
      ADD_FAILURE() << "Unexpected additional QUIC session request";
      return ERR_UNEXPECTED;
    }
    request->SetSession(std::move(handle_));
    return OK;
  }

  void CancelRequest(QuicSessionRequest*) override {
    ADD_FAILURE() << "The session was already acquired";
  }

  int requests() const { return requests_; }

 private:
  const url::SchemeHostPort proxy_;
  std::unique_ptr<QuicChromiumClientSession::Handle> handle_;
  int requests_ = 0;
};

class QuicProxyConnectJobSuccessTest : public QuicProxyClientSocketTest {
 protected:
  template <class Types>
  void AssertSuccessfulJob() {
    // Fresh native session and packet script for each implementation. The job
    // must acquire the session handle and open its own tunnel stream.
    int packet_number = 1;
    mock_quic_data_.AddWrite(SYNCHRONOUS,
                            ConstructSettingsPacket(packet_number++));
    mock_quic_data_.AddWrite(
        SYNCHRONOUS,
        client_maker_.MakePriorityPacket(
            packet_number++, client_data_stream_id1_,
            ConvertRequestPriorityToQuicPriority(DEFAULT_PRIORITY)));
    mock_quic_data_.AddWrite(
        SYNCHRONOUS,
        ConstructConnectRequestPacket(packet_number++, std::nullopt,
                                      DEFAULT_PRIORITY));
    mock_quic_data_.AddRead(ASYNC,
                           ConstructServerConnectReplyPacket(1, !kFin));
    mock_quic_data_.AddReadPauseForever();
    mock_quic_data_.AddWrite(
        SYNCHRONOUS, ConstructAckAndRstPacket(packet_number++,
                                             quic::QUIC_STREAM_CANCELLED, 1, 1));
    InitializeSession(/*create_stream=*/false);
    ASSERT_TRUE(session_handle_);
    ASSERT_FALSE(stream_handle_);
    session_->StartReading();

    SpdySessionDependencies dependencies;
    dependencies.host_resolver = std::make_unique<MockHostResolver>(
        MockHostResolverBase::RuleResolver::GetLocalhostResult());
    dependencies.http_user_agent_settings =
        std::make_unique<StaticHttpUserAgentSettings>("*", user_agent_);
    Types::PrepareAuth(dependencies);
    auto context_session =
        SpdySessionDependencies::SpdyCreateSession(&dependencies);
    auto http2_context = Types::CreateHttp2Context(*context_session);
    const quic::ParsedQuicVersionVector supported_versions{version_};
    ReadyProxySessionPool pool(*context_session, proxy_endpoint_,
                               std::move(session_handle_));
    auto common = Types::CommonFrom(
        context_session->CreateCommonConnectJobParams(), http2_context);
    common.quic_session_pool = &pool;
    common.quic_supported_versions = &supported_versions;

    const ProxyChain chain(ProxyServer::SCHEME_QUIC,
                           HostPortPair::FromSchemeHostPort(proxy_endpoint_));
    SSLConfig ssl;
    ssl.disable_cert_verification_network_fetches = true;
    auto params = base::MakeRefCounted<typename Types::ProxyParams>(
        ssl, HostPortPair::FromSchemeHostPort(destination_endpoint_), chain,
        /*proxy_chain_index=*/0, /*tunnel=*/true, TRAFFIC_ANNOTATION_FOR_TESTS,
        NetworkAnonymizationKey(), SecureDnsPolicy::kAllow,
        handles::kInvalidNetworkHandle);

    connection_jobs_job_workloads::Delegate<Types> delegate;
    typename Types::Proxy job(DEFAULT_PRIORITY, SocketTag(), &common,
                              std::move(params), &delegate, nullptr);
    ASSERT_EQ(job.Connect(), ERR_IO_PENDING);
    RunUntilIdle();
    ASSERT_EQ(delegate.result, OK);
    EXPECT_EQ(delegate.completions, 1u);
    EXPECT_EQ(delegate.auth_challenges, 0u);
    EXPECT_EQ(pool.requests(), 1);
    auto tunnel = job.PassSocket();
    ASSERT_TRUE(tunnel);
    EXPECT_TRUE(tunnel->IsConnected());
    EXPECT_EQ(tunnel->GetNegotiatedProtocol(), NextProto::kProtoUnknown);
    tunnel.reset();
    RunUntilIdle();
    EXPECT_EQ(delegate.completions, 1u);
  }
};

TEST_P(QuicProxyConnectJobSuccessTest, OriginalOpensTunnel) {
  AssertSuccessfulJob<connection_jobs_job_workloads::Original>();
}

TEST_P(QuicProxyConnectJobSuccessTest, ProductionOpensTunnel) {
  AssertSuccessfulJob<connection_jobs_job_workloads::Production>();
}

INSTANTIATE_TEST_SUITE_P(RfcV1,
                         QuicProxyConnectJobSuccessTest,
                         ::testing::Values(quic::ParsedQuicVersion::RFCv1()));

}  // namespace

}  // namespace net::test
