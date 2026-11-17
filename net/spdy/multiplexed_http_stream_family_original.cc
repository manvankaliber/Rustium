// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/multiplexed_http_stream_family_original.h"

#include <utility>

#include "base/notreached.h"
#include "net/http/http_raw_request_headers.h"

namespace net {

MultiplexedHttpStreamOriginal::MultiplexedHttpStreamOriginal(
    std::unique_ptr<MultiplexedSessionHandleOriginal> session)
    : session_(std::move(session)) {}

MultiplexedHttpStreamOriginal::~MultiplexedHttpStreamOriginal() = default;

int MultiplexedHttpStreamOriginal::GetRemoteEndpoint(IPEndPoint* endpoint) {
  return session_->GetRemoteEndpoint(endpoint);
}

void MultiplexedHttpStreamOriginal::GetSSLInfo(SSLInfo* ssl_info) {
  // Refresh from the live session to pick up state that may have changed
  // after the initial cache (e.g., early_data_accepted is only known after
  // the TLS/QUIC handshake completes, but the cache is populated at stream
  // creation time before the handshake finishes).
  session_->SaveSSLInfo();
  session_->GetSSLInfo(ssl_info);
}

void MultiplexedHttpStreamOriginal::SaveSSLInfo() {
  session_->SaveSSLInfo();
}

void MultiplexedHttpStreamOriginal::Drain(HttpNetworkSession* session) {
  NOTREACHED();
}

std::unique_ptr<HttpStream> MultiplexedHttpStreamOriginal::RenewStreamForAuth() {
  return nullptr;
}

void MultiplexedHttpStreamOriginal::SetConnectionReused() {}

bool MultiplexedHttpStreamOriginal::CanReuseConnection() const {
  // Multiplexed streams aren't considered reusable.
  return false;
}

void MultiplexedHttpStreamOriginal::SetRequestHeadersCallback(
    RequestHeadersCallback callback) {
  request_headers_callback_ = std::move(callback);
}

void MultiplexedHttpStreamOriginal::DispatchRequestHeadersCallback(
    const quiche::HttpHeaderBlock& spdy_headers) {
  if (!request_headers_callback_)
    return;
  HttpRawRequestHeaders raw_headers;
  for (const auto& entry : spdy_headers) {
    raw_headers.Add(entry.first, entry.second);
  }
  request_headers_callback_.Run(std::move(raw_headers));
}

}  // namespace net
