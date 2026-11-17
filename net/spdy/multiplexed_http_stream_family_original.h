// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_FAMILY_ORIGINAL_MULTIPLEXED_HTTP_STREAM_H_
#define NET_SPDY_FAMILY_ORIGINAL_MULTIPLEXED_HTTP_STREAM_H_

#include "base/component_export.h"
#include <memory>
#include <vector>

#include "net/http/http_stream.h"
#include "net/spdy/multiplexed_session_handle_family_original.h"
#include "net/third_party/quiche/src/quiche/common/http/http_header_block.h"

namespace net {

// Base class for SPDY and QUIC HttpStream subclasses.
class COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) MultiplexedHttpStreamOriginal : public HttpStream {
 public:
  explicit MultiplexedHttpStreamOriginal(
      std::unique_ptr<MultiplexedSessionHandleOriginal> session);
  ~MultiplexedHttpStreamOriginal() override;

  int GetRemoteEndpoint(IPEndPoint* endpoint) override;
  void GetSSLInfo(SSLInfo* ssl_info) override;
  void Drain(HttpNetworkSession* session) override;
  std::unique_ptr<HttpStream> RenewStreamForAuth() override;
  void SetConnectionReused() override;
  bool CanReuseConnection() const override;

  // Caches SSL info from the underlying session.
  void SaveSSLInfo();
  void SetRequestHeadersCallback(RequestHeadersCallback callback) override;

 protected:
  void DispatchRequestHeadersCallback(
      const quiche::HttpHeaderBlock& spdy_headers);

  MultiplexedSessionHandleOriginal* session() { return session_.get(); }
  const MultiplexedSessionHandleOriginal* session() const { return session_.get(); }

 private:
  const std::unique_ptr<MultiplexedSessionHandleOriginal> session_;
  RequestHeadersCallback request_headers_callback_;
};

}  // namespace net

#endif  // NET_SPDY_FAMILY_ORIGINAL_MULTIPLEXED_HTTP_STREAM_H_
