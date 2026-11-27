// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/websockets/websocket_frame.h"

#include <stddef.h>

#include <ostream>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/rand_util.h"
#include "base/strings/string_view_util.h"
#include "build/build_config.h"
#include "net/base/net_errors.h"
#include "net/websockets/websocket_frame_rust_ffi.h"

namespace net {
static_assert(ERR_INVALID_ARGUMENT == -4);

bool WebSocketFrameHeader::IsKnownDataOpCode(OpCode opcode) {
  return rustium_websocket_frame_opcode(0, opcode);
}

bool WebSocketFrameHeader::IsKnownControlOpCode(OpCode opcode) {
  return rustium_websocket_frame_opcode(1, opcode);
}

bool WebSocketFrameHeader::IsReservedDataOpCode(OpCode opcode) {
  return rustium_websocket_frame_opcode(2, opcode);
}

bool WebSocketFrameHeader::IsReservedControlOpCode(OpCode opcode) {
  return rustium_websocket_frame_opcode(3, opcode);
}

std::unique_ptr<WebSocketFrameHeader> WebSocketFrameHeader::Clone() const {
  auto ret = std::make_unique<WebSocketFrameHeader>(opcode);
  ret->CopyFrom(*this);
  return ret;
}

void WebSocketFrameHeader::CopyFrom(const WebSocketFrameHeader& source) {
  final = source.final;
  reserved1 = source.reserved1;
  reserved2 = source.reserved2;
  reserved3 = source.reserved3;
  opcode = source.opcode;
  masked = source.masked;
  masking_key = source.masking_key;
  payload_length = source.payload_length;
}

WebSocketFrame::WebSocketFrame(WebSocketFrameHeader::OpCode opcode)
    : header(opcode) {}

WebSocketFrame::~WebSocketFrame() = default;

WebSocketFrameChunk::WebSocketFrameChunk() = default;

WebSocketFrameChunk::~WebSocketFrameChunk() = default;

namespace {
WebSocketFrameRustHeader ReadHeader(const void* context) {
  const auto& header = *static_cast<const WebSocketFrameHeader*>(context);
  WebSocketFrameRustHeader h = {};
  h.length = header.payload_length;
  h.opcode = header.opcode;
  h.final_bit = header.final;
  h.reserved1 = header.reserved1;
  h.reserved2 = header.reserved2;
  h.reserved3 = header.reserved3;
  h.masked = header.masked;
  return h;
}
// Aliased writes may overwrite flag fields that the original never reads
// again. Only read the live length/mask fields at subsequent writer stages.
WebSocketFrameRustHeader ReadLiveHeader(const void* context) {
  const auto& header = *static_cast<const WebSocketFrameHeader*>(context);
  WebSocketFrameRustHeader h = {};
  h.length = header.payload_length;
  h.masked = header.masked;
  return h;
}
}  // namespace

size_t GetWebSocketFrameHeaderSize(const WebSocketFrameHeader& header) {
  return rustium_websocket_frame_header_size(header.payload_length,
                                             header.masked);
}

int WriteWebSocketFrameHeader(const WebSocketFrameHeader& header,
                              const WebSocketMaskingKey* masking_key,
                              base::span<uint8_t> buffer) {
  DCHECK((header.opcode & 15) == header.opcode);
  DCHECK_LE(header.payload_length, static_cast<uint64_t>(INT64_MAX));
  // Preserve the original short-buffer failure before requiring a key.
  if (GetWebSocketFrameHeaderSize(header) <= buffer.size()) {
    DCHECK_EQ(header.masked, masking_key != nullptr);
  }
  const auto h = ReadHeader(&header);
  const uintptr_t begin = reinterpret_cast<uintptr_t>(buffer.data());
  const uintptr_t header_begin = reinterpret_cast<uintptr_t>(&header);
  if (begin < header_begin + sizeof(header) &&
      header_begin < begin + buffer.size()) {
    return rustium_websocket_frame_write_live(
        h, &header, ReadLiveHeader,
        masking_key ? masking_key->key.data() : nullptr, buffer.data(),
        buffer.size());
  }
  return rustium_websocket_frame_write(
      h, masking_key ? masking_key->key.data() : nullptr, buffer.data(),
      buffer.size());
}

WebSocketMaskingKey GenerateWebSocketMaskingKey() {
  // Masking keys should be generated from a cryptographically secure random
  // number generator, which means web application authors should not be able
  // to guess the next value of masking key.
  WebSocketMaskingKey masking_key;
  base::RandBytes(masking_key.key);
  return masking_key;
}

void MaskWebSocketFramePayload(const WebSocketMaskingKey& masking_key,
                               uint64_t frame_offset,
                               base::span<uint8_t> data) {
#if defined(COMPILER_GCC) && \
    (defined(ARCH_CPU_X86_FAMILY) || defined(ARCH_CPU_ARM_FAMILY))
  constexpr size_t kOriginalPackedWidth = 16;
#else
  constexpr size_t kOriginalPackedWidth = sizeof(size_t);
#endif
  rustium_websocket_frame_mask(masking_key.key.data(), frame_offset,
                               data.data(), data.size(), kOriginalPackedWidth);
}

ParseCloseFrameResult ParseCloseFrame(base::span<const char> payload) {
  uint32_t result = rustium_websocket_frame_close(
      base::as_byte_span(payload).data(), payload.size());
  const uint16_t code = static_cast<uint16_t>(result);
  switch (result >> 16) {
    case 1:
      return {code,
              {},
              "Received a broken close frame with an invalid size of 1 byte."};
    case 2:
      return {
          code,
          {},
          "Received a broken close frame containing a reserved status code."};
    case 3:
      return {code, "Invalid UTF-8 in Close frame",
              "Received a broken close frame containing invalid UTF-8."};
    case 4:
      return {code, base::as_string_view(payload.subspan(2u))};
    default:
      return {code, {}};
  }
}

}  // namespace net
