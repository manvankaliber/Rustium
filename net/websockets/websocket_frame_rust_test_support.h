// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_WEBSOCKETS_WEBSOCKET_FRAME_RUST_TEST_SUPPORT_H_
#define NET_WEBSOCKETS_WEBSOCKET_FRAME_RUST_TEST_SUPPORT_H_
#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "base/check.h"
#include "net/websockets/websocket_frame.h"
#include "net/websockets/websocket_frame_parser.h"
#include "net/websockets/websocket_frame_rust_reference.h"
#include "net/websockets/websocket_frame_rust_reference_parser.h"

namespace net::websocket_frame_rust_test {
namespace ref = websocket_frame_rust_reference;
template <typename H, typename R>
void EqualHeader(const H& h, const R& r) {
  CHECK_EQ(h.final, r.final);
  CHECK_EQ(h.reserved1, r.reserved1);
  CHECK_EQ(h.reserved2, r.reserved2);
  CHECK_EQ(h.reserved3, r.reserved3);
  CHECK_EQ(h.opcode, r.opcode);
  CHECK_EQ(h.masked, r.masked);
  CHECK(h.masking_key.key == r.masking_key.key);
  CHECK_EQ(h.payload_length, r.payload_length);
}
inline void Stream(base::span<const uint8_t> input, uint64_t seed) {
  WebSocketFrameParser p;
  ref::WebSocketFrameParser q;
  std::vector<uint8_t> a(input.begin(), input.end()), b = a;
  std::vector<std::unique_ptr<WebSocketFrameChunk>> x;
  std::vector<std::unique_ptr<ref::WebSocketFrameChunk>> y;
  size_t pos = 0;
  do {
    seed = seed * 6364136223846793005ULL + 1;
    size_t count = std::min(input.size() - pos, size_t(1 + (seed >> 32) % 33));
    auto av = base::span(a).subspan(pos, count);
    auto bv = base::span(b).subspan(pos, count);
    CHECK_EQ(p.Decode({}, &x), q.Decode({}, &y));
    CHECK_EQ(p.Decode(av, &x), q.Decode(bv, &y));
    CHECK_EQ(p.websocket_error(), q.websocket_error());
    CHECK_EQ(x.size(), y.size());
    for (size_t i = 0; i < x.size(); ++i) {
      CHECK_EQ(bool(x[i]->header), bool(y[i]->header));
      if (x[i]->header) {
        EqualHeader(*x[i]->header, *y[i]->header);
      }
      CHECK_EQ(x[i]->final_chunk, y[i]->final_chunk);
      CHECK_EQ(x[i]->payload.size(), y[i]->payload.size());
      if (x[i]->payload.empty()) {
        CHECK_EQ(x[i]->payload.data(), nullptr);
        CHECK_EQ(y[i]->payload.data(), nullptr);
      } else {
        CHECK_EQ(reinterpret_cast<uint8_t*>(x[i]->payload.data()) - a.data(),
                 reinterpret_cast<uint8_t*>(y[i]->payload.data()) - b.data());
        CHECK(std::equal(x[i]->payload.begin(), x[i]->payload.end(),
                         y[i]->payload.begin()));
        // Public spans remain live mutable aliases after subsequent Decode
        // calls.
        x[i]->payload[0] ^= 17;
        y[i]->payload[0] ^= 17;
      }
    }
    CHECK(a == b);
    pos += count;
  } while (pos < input.size());
  // Failure stays sticky even on empty input; output is append-only.
  std::array<uint8_t, 2> zero{};
  CHECK_EQ(p.Decode(zero, &x), q.Decode(zero, &y));
  CHECK_EQ(p.Decode({}, &x), q.Decode({}, &y));
  CHECK_EQ(x.size(), y.size());
}
inline void Public(base::span<const uint8_t> input, uint64_t seed) {
  Stream(input, seed);
  auto c = ParseCloseFrame(base::as_chars(input));
  auto d = ref::ParseCloseFrame(base::as_chars(input));
  CHECK_EQ(c.code, d.code);
  CHECK(c.reason == d.reason);
  CHECK(c.error == d.error);
  if (!c.reason.empty() && !c.error) {
    CHECK_EQ(c.reason.data(), d.reason.data());
  }
  WebSocketFrameHeader h(int(seed & 15));
  ref::WebSocketFrameHeader r(int(seed & 15));
  h.final = r.final = seed & 16;
  h.reserved1 = r.reserved1 = seed & 32;
  h.reserved2 = r.reserved2 = seed & 64;
  h.reserved3 = r.reserved3 = seed & 128;
  h.masked = r.masked = seed & 256;
  constexpr uint64_t sizes[] = {
      0,        1, 125, 126, 65535, 65536, INT32_MAX, uint64_t(INT32_MAX) + 1,
      INT64_MAX};
  h.payload_length = r.payload_length = sizes[(seed >> 10) % std::size(sizes)];
  WebSocketMaskingKey k{{1, 2, 3, 4}};
  ref::WebSocketMaskingKey l{{1, 2, 3, 4}};
  if (input.size() >= 4) {
    std::copy_n(input.begin(), 4, k.key.begin());
    l.key = k.key;
  }
  h.masking_key = k;
  r.masking_key = l;
  EqualHeader(*h.Clone(), *r.Clone());
  CHECK_EQ(GetWebSocketFrameHeaderSize(h), ref::GetWebSocketFrameHeaderSize(r));
  std::array<uint8_t, 24> a{}, b{};
  size_t cap = (seed >> 16) % 24;
  CHECK_EQ(WriteWebSocketFrameHeader(h, h.masked ? &k : nullptr,
                                     base::span(a).first(cap)),
           ref::WriteWebSocketFrameHeader(r, r.masked ? &l : nullptr,
                                          base::span(b).first(cap)));
  CHECK(a == b);
  std::vector<uint8_t> x(input.begin(), input.end()), y = x;
  MaskWebSocketFramePayload(k, seed, x);
  ref::MaskWebSocketFramePayload(l, seed, y);
  CHECK(x == y);
  MaskWebSocketFramePayload(k, seed, x);
  CHECK(std::equal(x.begin(), x.end(), input.begin()));
  for (int op = -1; op <= 16; ++op) {
    CHECK_EQ(WebSocketFrameHeader::IsKnownDataOpCode(op),
             ref::WebSocketFrameHeader::IsKnownDataOpCode(op));
    CHECK_EQ(WebSocketFrameHeader::IsKnownControlOpCode(op),
             ref::WebSocketFrameHeader::IsKnownControlOpCode(op));
    CHECK_EQ(WebSocketFrameHeader::IsReservedDataOpCode(op),
             ref::WebSocketFrameHeader::IsReservedDataOpCode(op));
    CHECK_EQ(WebSocketFrameHeader::IsReservedControlOpCode(op),
             ref::WebSocketFrameHeader::IsReservedControlOpCode(op));
  }
}
inline void Aliases() {
  for (size_t size :
       {0, 1, 3, 4, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 256, 65536}) {
    for (size_t alignment = 0; alignment < 16; ++alignment) {
      for (size_t keypos :
           {size_t(0), size_t(16), size_t(19), size_t(32), size + 30}) {
        std::vector<uint8_t> a(size + 64, 0x53), b = a;
        auto* k = std::construct_at(
            reinterpret_cast<WebSocketMaskingKey*>(
                base::span(a)
                    .subspan(keypos, sizeof(WebSocketMaskingKey))
                    .data()),
            WebSocketMaskingKey{{1, 3, 7, 13}});
        auto* l = std::construct_at(
            reinterpret_cast<ref::WebSocketMaskingKey*>(
                base::span(b)
                    .subspan(keypos, sizeof(ref::WebSocketMaskingKey))
                    .data()),
            ref::WebSocketMaskingKey{{1, 3, 7, 13}});
        uint64_t phase = UINT64_MAX - alignment;
        MaskWebSocketFramePayload(*k, phase,
                                  base::span(a).subspan(16 + alignment, size));
        ref::MaskWebSocketFramePayload(
            *l, phase, base::span(b).subspan(16 + alignment, size));
        CHECK(a == b);
      }
    }
  }
  for (uint64_t length : {0ULL, 126ULL, 65536ULL}) {
    for (size_t keypos = 0; keypos < 20; ++keypos) {
      WebSocketFrameHeader h(15);
      ref::WebSocketFrameHeader r(15);
      h.masked = r.masked = true;
      h.payload_length = r.payload_length = length;
      std::array<uint8_t, 32> a{}, b{};
      auto* k = std::construct_at(
          reinterpret_cast<WebSocketMaskingKey*>(
              base::span(a)
                  .subspan(keypos, sizeof(WebSocketMaskingKey))
                  .data()),
          WebSocketMaskingKey{{1, 3, 7, 13}});
      auto* l = std::construct_at(
          reinterpret_cast<ref::WebSocketMaskingKey*>(
              base::span(b)
                  .subspan(keypos, sizeof(ref::WebSocketMaskingKey))
                  .data()),
          ref::WebSocketMaskingKey{{1, 3, 7, 13}});
      CHECK_EQ(WriteWebSocketFrameHeader(h, k, a),
               ref::WriteWebSocketFrameHeader(r, l, b));
      CHECK(a == b);
    }
  }
}
}  // namespace net::websocket_frame_rust_test
#endif
