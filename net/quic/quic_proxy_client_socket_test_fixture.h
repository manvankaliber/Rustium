// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_QUIC_PROXY_CLIENT_SOCKET_TEST_FIXTURE_H_
#define NET_QUIC_QUIC_PROXY_CLIENT_SOCKET_TEST_FIXTURE_H_

#include <memory>
#include <optional>

#include "base/containers/span.h"
#include "net/http/http_request_headers.h"
#include "net/quic/quic_proxy_client_socket.h"
#include "net/quic/quic_proxy_client_socket_test_base.h"

namespace net::test {

class QuicProxyClientSocketTest : public QuicProxyClientSocketTestBase {
 public:
  QuicProxyClientSocketTest();
  ~QuicProxyClientSocketTest() override;

  void TearDown() override;
  void InitializeClientSocket() override;

  void PopulateConnectRequestIR(
      quiche::HttpHeaderBlock* block,
      std::optional<const HttpRequestHeaders> extra_headers) override;

  void AssertConnectSucceeds() override;
  void AssertConnectFails(int result) override;

  void AssertWriteReturns(base::span<const char> data, int rv) override;
  void AssertSyncWriteSucceeds(base::span<const char> data) override;

  void AssertSyncReadEquals(base::span<const char> data) override;
  void AssertAsyncReadEquals(base::span<const char> data) override;

  void AssertReadStarts(base::span<const char> data) override;
  void AssertReadReturns(base::span<const char> data) override;

 protected:
  std::unique_ptr<QuicProxyClientSocket> sock_;
};

}  // namespace net::test

#endif  // NET_QUIC_QUIC_PROXY_CLIENT_SOCKET_TEST_FIXTURE_H_
