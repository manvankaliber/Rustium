// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_QUIC_PROXY_CLIENT_SOCKET_TEST_FIXTURE_H_
#define NET_QUIC_QUIC_PROXY_CLIENT_SOCKET_TEST_FIXTURE_H_

#include <memory>
#include <optional>
#include <utility>

#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "net/base/host_port_pair.h"
#include "net/base/io_buffer.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "net/http/http_auth_controller.h"
#include "net/http/http_request_headers.h"
#include "net/log/net_log_with_source.h"
#include "net/quic/quic_proxy_client_socket.h"
#include "net/quic/quic_proxy_client_socket_test_base.h"
#include "net/test/gtest_util.h"

namespace net::test {

class QuicProxyClientSocketTest : public QuicProxyClientSocketTestBase {
 public:
  void TearDown() override {
    sock_.reset();
    EXPECT_TRUE(mock_quic_data_.AllReadDataConsumed());
    EXPECT_TRUE(mock_quic_data_.AllWriteDataConsumed());
  }

  void InitializeClientSocket() override {
    sock_ = std::make_unique<QuicProxyClientSocket>(
        std::move(stream_handle_), std::move(session_handle_),
        // TODO(crbug.com/40181080) Construct `ProxyChain` with plain
        // `proxy_endpoint_` once it supports `url::SchemeHostPort`.
        ProxyChain(ProxyServer::SCHEME_HTTPS,
                   HostPortPair::FromSchemeHostPort(proxy_endpoint_)),
        /*proxy_chain_index=*/0, user_agent_,
        // TODO(crbug.com/40181080) Construct `QuicProxyClientSocket` with plain
        // `proxy_endpoint_` once it supports `url::SchemeHostPort`.
        HostPortPair::FromSchemeHostPort(destination_endpoint_),
        NetLogWithSource::Make(NetLogSourceType::NONE),
        base::MakeRefCounted<HttpAuthController>(
            HttpAuth::AUTH_PROXY, proxy_endpoint_.GetURL(),
            NetworkAnonymizationKey(), &http_auth_cache_,
            http_auth_handler_factory_.get(), host_resolver_.get()),
        proxy_delegate_.get());

    session_->StartReading();
  }

  void PopulateConnectRequestIR(
      quiche::HttpHeaderBlock* block,
      std::optional<const HttpRequestHeaders> extra_headers) override {
    (*block)[":method"] = "CONNECT";
    (*block)[":authority"] =
        HostPortPair::FromSchemeHostPort(destination_endpoint_).ToString();
    (*block)["user-agent"] = kUserAgent;
    if (extra_headers) {
      HttpRequestHeaders::Iterator it(*extra_headers);
      while (it.GetNext()) {
        std::string name = base::ToLowerASCII(it.name());
        (*block)[name] = it.value();
      }
    }
  }

  void AssertConnectSucceeds() override {
    TestCompletionCallback callback;
    ASSERT_THAT(sock_->Connect(callback.callback()),
                test::IsError(ERR_IO_PENDING));
    ASSERT_THAT(callback.WaitForResult(), test::IsOk());
  }

  void AssertConnectFails(int result) override {
    TestCompletionCallback callback;
    ASSERT_THAT(sock_->Connect(callback.callback()),
                test::IsError(ERR_IO_PENDING));
    EXPECT_EQ(result, callback.WaitForResult());
  }

  void AssertWriteReturns(base::span<const char> data, int rv) override {
    auto buf = base::MakeRefCounted<IOBufferWithSize>(data.size());
    buf->span().copy_from(base::as_bytes(data));
    EXPECT_EQ(rv,
              sock_->Write(buf.get(), buf->size(), write_callback_.callback(),
                           TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  void AssertSyncWriteSucceeds(base::span<const char> data) override {
    AssertWriteReturns(data, base::checked_cast<int>(data.size()));
  }

  void AssertSyncReadEquals(base::span<const char> data) override {
    auto buf = base::MakeRefCounted<IOBufferWithSize>(data.size());
    EXPECT_EQ(data.size(),
              sock_->Read(buf.get(), data.size(), CompletionOnceCallback()));
    EXPECT_EQ(data, base::as_chars(buf->span()));
    ASSERT_TRUE(sock_->IsConnected());
  }

  void AssertAsyncReadEquals(base::span<const char> data) override {
    AssertReadStarts(data);

    ResumeAndRun();

    AssertReadReturns(data);
  }

  void AssertReadStarts(base::span<const char> data) override {
    // Issue the read, which will be completed asynchronously.
    read_buf_ = base::MakeRefCounted<IOBufferWithSize>(data.size());
    ASSERT_EQ(ERR_IO_PENDING, sock_->Read(read_buf_.get(), data.size(),
                                          read_callback_.callback()));
    EXPECT_TRUE(sock_->IsConnected());
  }

  void AssertReadReturns(base::span<const char> data) override {
    EXPECT_TRUE(sock_->IsConnected());

    // Now the read will return.
    EXPECT_EQ(data.size(), read_callback_.WaitForResult());
    EXPECT_EQ(data, base::as_chars(read_buf_->span()));
  }

 protected:
  std::unique_ptr<QuicProxyClientSocket> sock_;
};

}  // namespace net::test

#endif  // NET_QUIC_QUIC_PROXY_CLIENT_SOCKET_TEST_FIXTURE_H_
