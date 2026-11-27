// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_WEBSOCKETS_WEBSOCKET_FRAME_RUST_FFI_H_
#define NET_WEBSOCKETS_WEBSOCKET_FRAME_RUST_FFI_H_
#include <stddef.h>
#include <stdint.h>
struct WebSocketFrameRustHeader {
  uint64_t length;
  uint8_t key[4];
  int32_t opcode;
  uint8_t final_bit;
  uint8_t reserved1;
  uint8_t reserved2;
  uint8_t reserved3;
  uint8_t masked;
};
static_assert(sizeof(WebSocketFrameRustHeader) == 24);
static_assert(alignof(WebSocketFrameRustHeader) == alignof(uint64_t));
extern "C" {
bool rustium_websocket_frame_opcode(uint8_t kind, int32_t opcode);
size_t rustium_websocket_frame_header_size(uint64_t length, bool masked);
int32_t rustium_websocket_frame_write(WebSocketFrameRustHeader header,
                                      const uint8_t* key,
                                      uint8_t* out,
                                      size_t len);
int32_t rustium_websocket_frame_write_live(
    WebSocketFrameRustHeader initial,
    const void* context,
    WebSocketFrameRustHeader (*read)(const void*),
    const uint8_t* key,
    uint8_t* out,
    size_t len);
void rustium_websocket_frame_mask(const uint8_t* key,
                                  uint64_t offset,
                                  uint8_t* data,
                                  size_t len,
                                  size_t packed_width);
uint32_t rustium_websocket_frame_close(const uint8_t* data, size_t len);
void* rustium_websocket_frame_parser_new();
void rustium_websocket_frame_parser_free(void* parser);
uint16_t rustium_websocket_frame_decode(
    void* parser,
    const uint8_t* data,
    size_t len,
    void* context,
    void (*emit)(void*, const WebSocketFrameRustHeader*, bool, size_t, size_t));
}
#endif
