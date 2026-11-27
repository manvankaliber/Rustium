// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_WEBSOCKETS_WEBSOCKET_FRAME_RUST_REFERENCE_UTF8_H_
#define NET_WEBSOCKETS_WEBSOCKET_FRAME_RUST_REFERENCE_UTF8_H_
#include <string_view>
namespace net::websocket_frame_rust_reference {
bool IsStringUTF8AllowingNoncharacters(std::string_view str);
}
#endif
