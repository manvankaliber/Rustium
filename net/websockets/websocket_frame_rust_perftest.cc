// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Build-only during implementation; parent coordinates every timing run.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <ratio>
#include <type_traits>

#include "net/websockets/websocket_frame_rust_test_support.h"
namespace {
namespace ref = net::websocket_frame_rust_reference;
struct Case {
  const char* key;
  int kind;
  size_t bytes;
  size_t alignment;
  size_t fragment;
  size_t iterations;
};
volatile uint64_t sink = 0;
std::vector<uint8_t> Input(const Case& c) {
  if (c.kind == 0) {
    std::vector<uint8_t> data(c.bytes + 32, 'a');
    if (c.bytes == 14) {
      base::span(data).copy_prefix_from(base::as_byte_span(
          base::span_with_nul_from_cstring("Short Payload")));
    }
    return data;
  }
  if (c.kind == 1) {
    return std::vector<uint8_t>(14, 0);
  }
  if (c.kind == 3) {
    return {3, 232, 'g', 'o', 'o', 'd', 'b', 'y', 'e'};
  }
  if (c.kind == 4) {
    return {0x81, 0, 0x82, 0x7f, 0x80, 0, 0, 0, 0, 0, 0, 0};
  }
  std::vector<uint8_t> v;
  net::WebSocketFrameHeader h(2);
  h.final = true;
  h.payload_length = c.bytes;
  v.resize(net::GetWebSocketFrameHeaderSize(h));
  net::WriteWebSocketFrameHeader(h, nullptr, v);
  v.resize(v.size() + c.bytes, 'a');
  return v;
}
template <bool Rust>
uint64_t Run(const Case& c, std::vector<uint8_t>& storage, size_t iterations) {
  using H = std::conditional_t<Rust, net::WebSocketFrameHeader,
                               ref::WebSocketFrameHeader>;
  using K = typename H::WebSocketMaskingKey;
  using P = std::conditional_t<Rust, net::WebSocketFrameParser,
                               ref::WebSocketFrameParser>;
  using C = std::conditional_t<Rust, net::WebSocketFrameChunk,
                               ref::WebSocketFrameChunk>;
  K key{{0xfe, 0xed, 0xbe, 0xef}};
  H header(2);
  header.final = true;
  header.masked = c.alignment != 0;
  header.payload_length = c.bytes;
  uint64_t result = 0;
  auto payload = base::span(storage);
  if (c.kind == 0) {
    // 16-byte aligned base, explicit offset for unaligned workloads. Allocation
    // and initialization stay outside the timer, as in the original perftest.
    size_t adjustment =
        (16 - (reinterpret_cast<uintptr_t>(payload.data()) % 16)) % 16;
    payload =
        payload.subspan((c.fragment ? 0 : adjustment) + c.alignment, c.bytes);
  }
  for (size_t i = 0; i < iterations; ++i) {
    if (c.kind == 0) {
      MaskWebSocketFramePayload(key, c.fragment ? i % c.bytes : 3, payload);
    } else if (c.kind == 1) {
      result += GetWebSocketFrameHeaderSize(header);
      result += WriteWebSocketFrameHeader(
          header, header.masked ? &key : nullptr, payload.first(c.fragment));
      result += payload[0];
    } else if (c.kind == 3) {
      if constexpr (Rust) {
        auto r = net::ParseCloseFrame(base::as_chars(payload));
        result += r.code + r.reason.size();
      } else {
        auto r = ref::ParseCloseFrame(base::as_chars(payload));
        result += r.code + r.reason.size();
      }
    } else {
      P parser;
      std::vector<std::unique_ptr<C>> chunks;
      size_t pos = 0;
      while (pos < payload.size()) {
        size_t count = std::min(c.fragment, payload.size() - pos);
        result += parser.Decode(payload.subspan(pos, count), &chunks);
        pos += count;
      }
      result += parser.websocket_error();
      for (const auto& chunk : chunks) {
        result += chunk->payload.size() + chunk->final_chunk;
        if (chunk->header) {
          result += chunk->header->payload_length;
        }
      }
      // Parser state, public headers, chunks, vector allocations and all
      // destruction are inside this iteration's timing boundary.
    }
  }
  if (c.kind == 0) {
    result += payload.front() + payload.back();
  }
  return result;
}
template <bool Rust>
double Measure(const Case& c) {
  auto input = Input(c);
  const auto start = std::chrono::steady_clock::now();
  sink = Run<Rust>(c, input, c.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         c.iterations;
}
}  // namespace
int main(int argc, char** argv) {
  const bool validate_only =
      argc > 1 &&
      std::string_view(UNSAFE_BUFFERS(argv[1])) == "--validate-only";
  const Case cases[] = {{"mask_short_14", 0, 14, 0, 1, 100000},
                        {"mask_long_65536", 0, 65536, 0, 1, 100000},
                        {"mask_31", 0, 31, 0, 1, 100000},
                        {"mask_aligned_65536", 0, 65536, 0, 0, 100000},
                        {"mask_unaligned_65536", 0, 65536, 1, 0, 100000},
                        {"header_tiny", 1, 0, 0, 14, 100000},
                        {"header_masked_large", 1, 65536, 1, 14, 100000},
                        {"header_short_error", 1, 65536, 1, 2, 100000},
                        {"close_valid_utf8", 3, 9, 0, 0, 100000},
                        {"parse_tiny", 2, 14, 0, 65536, 100000},
                        {"parse_large", 2, 65536, 0, 131072, 100000},
                        {"parse_fragmented", 2, 126, 0, 1, 10000},
                        {"parse_header_fragmented", 2, 65536, 0, 7, 100},
                        {"parse_error_after_frame", 4, 12, 0, 12, 100000}};
  std::mt19937 random(20261113);
  for (const auto& c : cases) {
    auto a = Input(c), b = a;
    CHECK_EQ(Run<false>(c, a, 128), Run<true>(c, b, 128));
    if (validate_only) {
      continue;
    }
    for (unsigned pair = 0; pair < 20; ++pair) {
      bool first = (random() & 1) != 0;
      double cpp, rust;
      if (first) {
        rust = Measure<true>(c);
        cpp = Measure<false>(c);
      } else {
        cpp = Measure<false>(c);
        rust = Measure<true>(c);
      }
      // SAFETY: key is a static NUL-terminated case literal.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"input_bytes\":%zu,\"iterations\":%zu,\"repeat\":%"
          "u,\"rust_first\":%s,\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          c.key, c.bytes, c.iterations, pair, first ? "true" : "false", cpp,
          rust));
      std::fflush(stdout);
    }
  }
  if (validate_only) {
    std::puts(
        "PASS: all 14 paired perftest workloads validated without timing");
  }
}
