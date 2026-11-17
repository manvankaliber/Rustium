// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/websockets/websocket_frame_parser.h"

#include "net/websockets/websocket_frame_rust_ffi.h"

namespace net {
static_assert(kWebSocketNormalClosure == 1000);
static_assert(kWebSocketErrorProtocolError == 1002);
static_assert(kWebSocketErrorMessageTooBig == 1009);
namespace {
struct DecodeContext {
  // Synchronous stack callback context; no pointers escape Decode.
  RAW_PTR_EXCLUSION base::span<uint8_t> input;
  RAW_PTR_EXCLUSION std::vector<std::unique_ptr<WebSocketFrameChunk>>* output;
};
void EmitChunk(void* context,
               const WebSocketFrameRustHeader* h,
               bool final_chunk,
               size_t start,
               size_t size) {
  auto& c = *static_cast<DecodeContext*>(context);
  auto chunk = std::make_unique<WebSocketFrameChunk>();
  if (h) {
    chunk->header = std::make_unique<WebSocketFrameHeader>(h->opcode);
    auto& header = *chunk->header;
    header.final = h->final_bit;
    header.reserved1 = h->reserved1;
    header.reserved2 = h->reserved2;
    header.reserved3 = h->reserved3;
    header.masked = h->masked;
    header.payload_length = h->length;
    header.masking_key.key = {h->key[0], h->key[1], h->key[2], h->key[3]};
  }
  chunk->final_chunk = final_chunk;
  // The original exposes a default null span for empty chunks.
  if (size) {
    chunk->payload = base::as_writable_chars(c.input.subspan(start, size));
  }
  c.output->push_back(std::move(chunk));
}
}  // namespace
WebSocketFrameParser::WebSocketFrameParser()
    : rust_parser_(rustium_websocket_frame_parser_new()) {}
WebSocketFrameParser::~WebSocketFrameParser() {
  rustium_websocket_frame_parser_free(rust_parser_);
}
bool WebSocketFrameParser::Decode(
    base::span<uint8_t> data_span,
    std::vector<std::unique_ptr<WebSocketFrameChunk>>* frame_chunks) {
  DecodeContext context{data_span, frame_chunks};
  websocket_error_ = static_cast<WebSocketError>(rustium_websocket_frame_decode(
      rust_parser_, data_span.data(), data_span.size(), &context, EmitChunk));
  return websocket_error_ == kWebSocketNormalClosure;
}
}  // namespace net
