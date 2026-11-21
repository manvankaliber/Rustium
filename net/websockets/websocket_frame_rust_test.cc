// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/base/io_buffer.h"
#include "net/websockets/websocket_frame_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
TEST(WebSocketFrameRust, AllHeaderBytesAndFragmentedStreams) {
  for (unsigned first = 0; first < 256; ++first) {
    for (unsigned second = 0; second < 256; ++second) {
      std::array<uint8_t, 24> bytes{};
      bytes[0] = first;
      bytes[1] = second;
      net::websocket_frame_rust_test::Public(bytes, first * 256 + second);
    }
  }
}
TEST(WebSocketFrameRust, AliasesAndAlignment) {
  net::websocket_frame_rust_test::Aliases();
}
TEST(WebSocketFrameRust, HeaderOutputMayBeUninitialized) {
  namespace ref = net::websocket_frame_rust_reference;
  for (uint64_t length : {0ULL, 125ULL, 126ULL, 65535ULL, 65536ULL}) {
    for (bool masked : {false, true}) {
      net::WebSocketFrameHeader header(2);
      ref::WebSocketFrameHeader original(2);
      header.payload_length = original.payload_length = length;
      header.masked = original.masked = masked;
      net::WebSocketMaskingKey key{{1, 2, 3, 4}};
      ref::WebSocketMaskingKey original_key{{1, 2, 3, 4}};
      // This is the production stream allocation pattern. Neither the header
      // destination nor its unwritten payload tail has been initialized.
      auto output = base::MakeRefCounted<net::IOBufferWithSize>(65536);
      std::array<uint8_t, 14> expected{};
      const int size = net::WriteWebSocketFrameHeader(
          header, masked ? &key : nullptr, output->span());
      const int original_size = ref::WriteWebSocketFrameHeader(
          original, masked ? &original_key : nullptr, expected);
      ASSERT_EQ(size, original_size);
      ASSERT_GT(size, 0);
      // Only the successfully written header bytes are initialized/readable.
      EXPECT_EQ(output->span().first(static_cast<size_t>(size)),
                base::span(expected).first(static_cast<size_t>(size)));
    }
  }
}
TEST(WebSocketFrameRust, LengthErrorsAndMultipleFrames) {
  for (uint64_t length :
       {0ULL, 125ULL, 126ULL, 65535ULL, 65536ULL, 0x7fffffffULL, 0x80000000ULL,
        0x7fffffffffffffffULL, 0x8000000000000000ULL, 0xffffffffffffffffULL}) {
    std::vector<uint8_t> data = {0x81, 0, 0x82, 0x7f};
    for (int i = 7; i >= 0; --i) {
      data.push_back(static_cast<uint8_t>(length >> (i * 8)));
    }
    data.insert(data.end(), 40, 17);
    for (uint64_t seed = 0; seed < 100; ++seed) {
      net::websocket_frame_rust_test::Public(data, seed);
    }
  }
}
TEST(WebSocketFrameRust, HeaderObjectOutputAliases) {
  namespace ref = net::websocket_frame_rust_reference;
  for (uint64_t length : {0ULL, 1ULL, 125ULL, 126ULL, 65536ULL}) {
    for (bool masked : {false, true}) {
      net::WebSocketFrameHeader h(15);
      ref::WebSocketFrameHeader r(15);
      h.payload_length = r.payload_length = length;
      h.masked = r.masked = masked;
      net::WebSocketMaskingKey k{{1, 2, 3, 4}};
      ref::WebSocketMaskingKey l{{1, 2, 3, 4}};
      auto a = base::byte_span_from_ref(base::allow_nonunique_obj, h)
                   .subspan(offsetof(net::WebSocketFrameHeader, masking_key));
      auto b = base::byte_span_from_ref(base::allow_nonunique_obj, r)
                   .subspan(offsetof(ref::WebSocketFrameHeader, masking_key));
      CHECK_EQ(net::WriteWebSocketFrameHeader(h, nullptr, a.first(0u)),
               ref::WriteWebSocketFrameHeader(r, nullptr, b.first(0u)));
      CHECK_EQ(net::WriteWebSocketFrameHeader(h, masked ? &k : nullptr, a),
               ref::WriteWebSocketFrameHeader(r, masked ? &l : nullptr, b));
      net::websocket_frame_rust_test::EqualHeader(h, r);
    }
  }
  for (uint64_t length : {0ULL, 1ULL, 125ULL}) {
    net::WebSocketFrameHeader h(15);
    ref::WebSocketFrameHeader r(15);
    h.payload_length = r.payload_length = length;
    CHECK_EQ(net::WriteWebSocketFrameHeader(
                 h, nullptr, base::byte_span_from_ref(h.payload_length)),
             ref::WriteWebSocketFrameHeader(
                 r, nullptr, base::byte_span_from_ref(r.payload_length)));
    net::websocket_frame_rust_test::EqualHeader(h, r);
  }
}
TEST(WebSocketFrameRust, CloseFramePolicy) {
  for (const std::vector<uint8_t>& bytes :
       std::vector<std::vector<uint8_t>>{{},
                                         {1},
                                         {3, 237},
                                         {3, 238},
                                         {3, 247},
                                         {3, 232},
                                         {3, 232, 0xef, 0xbf, 0xbf},
                                         {3, 232, 0xf0, 0x9f, 0x98, 0x80},
                                         {3, 232, 0xed, 0xa0, 0x80},
                                         {3, 232, 0xc0, 0x80},
                                         {3, 232, 0xf4, 0x90, 0x80, 0x80},
                                         {3, 232, 0xe2}}) {
    net::websocket_frame_rust_test::Public(bytes, 0);
  }
}
