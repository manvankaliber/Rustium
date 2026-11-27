// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_WEBSOCKETS_WEBSOCKET_FRAME_PARSER_H_
#define NET_WEBSOCKETS_WEBSOCKET_FRAME_PARSER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/containers/span.h"
#include "net/base/net_export.h"
#include "net/websockets/websocket_errors.h"
#include "net/websockets/websocket_frame.h"

namespace net {
struct WebSocketFrameChunk;
struct WebSocketFrameHeader;

// Parses WebSocket frames from byte stream.
//
// Specification of WebSocket frame format is available at
// <http://tools.ietf.org/html/rfc6455#section-5>.
// This class does *NOT* unmask frame payload.
class NET_EXPORT WebSocketFrameParser {
 public:
  WebSocketFrameParser();

  WebSocketFrameParser(const WebSocketFrameParser&) = delete;
  WebSocketFrameParser& operator=(const WebSocketFrameParser&) = delete;

  ~WebSocketFrameParser();

  // Decodes the given byte stream and stores parsed WebSocket frames in
  // |frame_chunks|.
  // Each WebSocketFrameChunk's payload is a subspan of data_span.
  // Thus callers must take care of its lifecycle.
  //
  // The contents of data_span are mutable, allowing the payload in
  // WebSocketFrameChunk to be mutable so that callers can perform unmasking
  // or other in-place modifications of the payload.
  //
  // If the parser encounters invalid payload length format, Decode() fails
  // and returns false. Once Decode() has failed, the parser refuses to decode
  // any more data and future invocations of Decode() will simply return false.
  //
  // Payload data of parsed WebSocket frames may be incomplete; see comments in
  // websocket_frame.h for more details.
  bool Decode(base::span<uint8_t> data_span,
              std::vector<std::unique_ptr<WebSocketFrameChunk>>* frame_chunks);

  // Returns kWebSocketNormalClosure if the parser has not failed to decode
  // WebSocket frames. Otherwise returns WebSocketError which is defined in
  // websocket_errors.h. We can convert net::WebSocketError to net::Error by
  // using WebSocketErrorToNetError().
  WebSocketError websocket_error() const { return websocket_error_; }

 private:
  // Rust owns all incremental parsing state. C++ owns public chunk objects
  // and caller-backed mutable payload spans, never parser policy.
  // Opaque Rust allocation is outside PartitionAlloc.
  RAW_PTR_EXCLUSION void* rust_parser_ = nullptr;
  WebSocketError websocket_error_ = kWebSocketNormalClosure;
};

}  // namespace net

#endif  // NET_WEBSOCKETS_WEBSOCKET_FRAME_PARSER_H_
