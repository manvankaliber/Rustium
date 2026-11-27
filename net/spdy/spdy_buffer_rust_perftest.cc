// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/test/bind.h"
#include "net/base/io_buffer.h"
#include "net/spdy/spdy_buffer.h"
#include "net/spdy/spdy_buffer_rust_original.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"

namespace net::spdy_buffer_perf {
namespace {
constexpr std::array<const char*, 12> kCases = {
    "raw_small",        "raw_large",      "owned_frame",
    "remaining_views",  "consume_chunks", "callback_one",
    "callback_eight",   "discard_partial", "discard_full",
    "nested_consume",   "callback_registration", "io_view_lifetime"};
volatile uint64_t sink;

// Every input is prepared outside clocks. Buffer/frame allocations, copies,
// destruction, callbacks and IOBuffer views remain inside their sample.
struct Inputs {
  std::vector<uint8_t> small = std::vector<uint8_t>(32);
  std::vector<uint8_t> normal = std::vector<uint8_t>(1024);
  std::vector<uint8_t> large = std::vector<uint8_t>(32768);
  Inputs() {
    for (auto* input : {&small, &normal, &large}) {
      size_t index = 0;
      for (auto& byte : *input) {
        byte = static_cast<uint8_t>(19 + index++ * 31);
      }
    }
  }
};

template <class Buffer>
uint64_t RunOne(size_t kind, const Inputs& inputs, bool verify) {
  const auto data = base::span(kind == 0 ? inputs.small
                               : kind == 1 ? inputs.large
                                           : inputs.normal);
  uint64_t sum = 0;
  size_t callbacks = 0;
  size_t callback_bytes = 0;
  size_t consume_calls = 0;
  size_t discard_calls = 0;
  scoped_refptr<IOBuffer> held;
  if (kind == 2) {
    auto bytes = std::make_unique<char[]>(data.size());
    base::span<char> output =
        // SAFETY: bytes owns data.size() initialized char objects.
        UNSAFE_BUFFERS(base::span(bytes.get(), data.size()));
    base::as_writable_byte_span(output).copy_from(data);
    Buffer buffer(std::make_unique<spdy::SpdySerializedFrame>(
        std::move(bytes), data.size()));
    sum += buffer.GetRemainingSize() + buffer.GetRemaining().front();
    if (verify) {
      CHECK(buffer.GetRemaining() == data);
    }
    return sum;
  }
  {
    Buffer buffer(data);
    sum += buffer.GetRemainingSize() + buffer.GetRemaining().front();
    if (kind == 3) {
      for (size_t i = 0; i < 128; ++i) {
        sum += buffer.GetRemainingSize() + buffer.GetRemaining().front();
      }
    } else if (kind == 4) {
      for (size_t i = 0; i < 16; ++i) {
        buffer.Consume(64);
        sum += buffer.GetRemainingSize();
      }
      if (verify) {
        CHECK_EQ(buffer.GetRemainingSize(), 0u);
      }
    } else if (kind >= 5 && kind <= 8) {
      const size_t count = kind == 6 ? 8 : 1;
      for (size_t i = 0; i < count; ++i) {
        buffer.AddConsumeCallback(base::BindLambdaForTesting(
            [&](size_t bytes, typename Buffer::ConsumeSource source) {
              ++callbacks;
              callback_bytes += bytes;
              consume_calls += source == Buffer::CONSUME;
              discard_calls += source == Buffer::DISCARD;
              sum = sum * 131 + bytes + static_cast<int>(source);
            }));
      }
      if (kind == 5 || kind == 6) {
        buffer.Consume(data.size());
      } else if (kind == 7) {
        buffer.Consume(256);
      }
    } else if (kind == 9) {
      bool entered = false;
      buffer.AddConsumeCallback(base::BindLambdaForTesting(
          [&](size_t bytes, typename Buffer::ConsumeSource source) {
            ++callbacks;
            callback_bytes += bytes;
            sum = sum * 131 + bytes + static_cast<int>(source);
            if (!entered) {
              entered = true;
              buffer.Consume(data.size() - 1);
            }
          }));
      buffer.Consume(1);
      if (verify) {
        CHECK_EQ(buffer.GetRemainingSize(), 0u);
        CHECK_EQ(callbacks, 2u);
        CHECK_EQ(callback_bytes, data.size());
      }
    } else if (kind == 10) {
      bool added = false;
      buffer.AddConsumeCallback(base::BindLambdaForTesting(
          [&](size_t bytes, typename Buffer::ConsumeSource source) {
            ++callbacks;
            callback_bytes += bytes;
            sum = sum * 131 + bytes + static_cast<int>(source);
            if (!added) {
              added = true;
              buffer.AddConsumeCallback(base::BindLambdaForTesting(
                  [&](size_t amount, typename Buffer::ConsumeSource why) {
                    ++callbacks;
                    callback_bytes += amount;
                    sum = sum * 131 + amount + static_cast<int>(why);
                  }));
            }
          }));
      buffer.Consume(1);
      if (verify) {
        CHECK_EQ(callbacks, 1u);
      }
      buffer.Consume(data.size() - 1);
      if (verify) {
        CHECK_EQ(callbacks, 3u);
        CHECK_EQ(callback_bytes, data.size() * 2 - 1);
      }
    } else if (kind == 11) {
      buffer.Consume(256);
      held = buffer.GetIOBufferForRemainingData();
      buffer.Consume(data.size() - 256);
      if (verify) {
        CHECK_EQ(buffer.GetRemainingSize(), 0u);
      }
    }
  }
  if (kind == 11) {
    sum += held->span().size() + held->span().front();
    if (verify) {
      CHECK(held->span() == data.subspan(256u));
    }
  }
  if (verify && kind >= 5 && kind <= 8) {
    const size_t count = kind == 6 ? 8 : 1;
    CHECK_EQ(callback_bytes, data.size() * count);
    CHECK_EQ(callbacks, kind == 7 ? 2u : count);
    CHECK_EQ(consume_calls, kind == 8 ? 0u : count);
    CHECK_EQ(discard_calls, kind == 7 || kind == 8 ? 1u : 0u);
  }
  return sum;
}

template <class Buffer>
uint64_t Run(size_t kind, size_t iterations, const Inputs& inputs, bool verify) {
  uint64_t sum = 0;
  for (size_t i = 0; i < iterations; ++i) {
    sum = sum * 131 + RunOne<Buffer>(kind, inputs, verify);
  }
  return sum;
}

template <class Buffer>
std::pair<double, uint64_t> Measure(size_t kind,
                                    size_t iterations,
                                    const Inputs& inputs) {
  const auto start = std::chrono::steady_clock::now();
  const uint64_t result = Run<Buffer>(kind, iterations, inputs, false);
  const auto stop = std::chrono::steady_clock::now();
  sink = result;
  return {std::chrono::duration<double, std::nano>(stop - start).count() /
              iterations,
          result};
}
}  // namespace
}  // namespace net::spdy_buffer_perf

int main(int argc, char** argv) {
  using namespace net;
  using namespace net::spdy_buffer_perf;
  const bool verify =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) {
    UNSAFE_BUFFERS(std::fputs("Use --verify-only or --timing\n", stderr));
    return 2;
  }
  const Inputs inputs;
  constexpr size_t iterations = 256;
  // Semantic checks for each implementation run before ANY measurement clock.
  for (size_t kind = 0; kind < kCases.size(); ++kind) {
    CHECK_EQ(Run<SpdyBufferOriginal>(kind, 8, inputs, true),
             Run<SpdyBuffer>(kind, 8, inputs, true));
  }
  if (verify) {
    UNSAFE_BUFFERS(std::puts("Verified 12 paired SpdyBuffer workloads; no timing."));
    return 0;
  }
  std::mt19937 random(20261113);
  for (size_t kind = 0; kind < kCases.size(); ++kind) {
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      std::pair<double, uint64_t> baseline, rust;
      if (rust_first) {
        rust = Measure<SpdyBuffer>(kind, iterations, inputs);
        baseline = Measure<SpdyBufferOriginal>(kind, iterations, inputs);
      } else {
        baseline = Measure<SpdyBufferOriginal>(kind, iterations, inputs);
        rust = Measure<SpdyBuffer>(kind, iterations, inputs);
      }
      CHECK_EQ(baseline.second, rust.second);
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,"
          "\"rust_ns\":%.9f,\"iterations\":%zu,\"rust_first\":%s,"
          "\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
          kCases[kind], repeat, baseline.first, rust.first, iterations,
          rust_first ? "true" : "false",
          static_cast<unsigned long long>(baseline.second),
          static_cast<unsigned long long>(rust.second)));
    }
  }
}
