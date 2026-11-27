// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <chrono>
#include <cstdio>
#include <ratio>
#include <string_view>

#include "net/spdy/http2_read_queue_rust_test_support.h"

namespace net::http2_read_queue_test {
namespace {
constexpr std::array<const char*, 7> kCases = {
    "empty_queries", "empty_dequeue", "single_full", "fragmented_fifo",
    "partial_reads", "clear",         "destruction"};
volatile uint64_t sink;

template <class Queue>
uint64_t Run(size_t kind, size_t iterations, bool verify = false) {
  uint64_t checksum = 0;
  for (size_t i = 0; i < iterations; ++i) {
    // Independent native buffer instances per implementation. Byte payloads
    // change on each iteration; callback bytes/source participate in checksum.
    std::vector<Event> events;
    size_t expected_bytes = 0;
    {
      Queue queue;
      if (kind == 0) {
        for (size_t j = 0; j < 256; ++j) {
          checksum += queue.IsEmpty() + queue.GetTotalSize();
        }
        continue;
      }
      if (kind != 1) {
        const size_t count = kind == 2 ? 1 : 32;
        for (size_t j = 0; j < count; ++j) {
          std::vector<uint8_t> data(64 + ((i + j) % 17));
          for (size_t k = 0; k < data.size(); ++k) {
            data[k] = static_cast<uint8_t>(i + j + k);
          }
          expected_bytes += data.size();
          Push(queue, j, data, events);
        }
      }
      checksum += queue.GetTotalSize() + queue.IsEmpty();
      if (kind == 5) {
        queue.Clear();
        queue.Clear();
        checksum += queue.IsEmpty() + queue.GetTotalSize();
      } else if (kind != 6) {
        std::vector<uint8_t> out(kind == 4 ? 7 : 4096);
        for (;;) {
          size_t copied = queue.Dequeue(out);
          checksum += copied + queue.GetTotalSize() + queue.IsEmpty();
          for (uint8_t byte : base::span(out).first(copied)) {
            checksum = checksum * 131 + byte;
          }
          if (!copied) {
            break;
          }
        }
      }
    }
    if (verify) {
      size_t callback_bytes = 0;
      size_t consumes = 0;
      for (const auto& event : events) {
        callback_bytes += event.bytes;
        consumes += event.source == SpdyBuffer::CONSUME;
      }
      CHECK_EQ(callback_bytes, expected_bytes);
      CHECK_EQ(kind == 1   ? size_t{0}
               : kind == 2 ? size_t{1}
                           : size_t{32},
               kind == 4 ? events.size() - consumes : events.size());
      CHECK_EQ(consumes != 0, kind == 4);
    }
    for (const auto& event : events) {
      checksum = checksum * 131 + event.id + event.bytes + event.source;
    }
  }
  return checksum;
}

template <class Queue>
std::pair<double, uint64_t> Measure(size_t kind, size_t iterations) {
  auto start = std::chrono::steady_clock::now();
  auto checksum = Run<Queue>(kind, iterations);
  auto stop = std::chrono::steady_clock::now();
  sink = checksum;
  return {std::chrono::duration<double, std::nano>(stop - start).count() /
              iterations,
          checksum};
}
}  // namespace
}  // namespace net::http2_read_queue_test

int main(int argc, char** argv) {
  using namespace net;
  using namespace net::http2_read_queue_test;
  bool verify =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  bool timing =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) {
    UNSAFE_BUFFERS(std::fputs("Use --verify-only or --timing\n", stderr));
    return 2;
  }
  CHECK(VerifyFixture<Http2ReadQueueOriginal>() ==
        VerifyFixture<SpdyReadQueue>());
  Differential(3919, 4096);
  std::mt19937 random(20261113);
  constexpr size_t iterations = 256;
  // All fixture checks and changing-input paired checksums precede ANY clocks.
  for (size_t kind = 0; kind < kCases.size(); ++kind) {
    uint64_t expected = Run<Http2ReadQueueOriginal>(kind, iterations, true);
    uint64_t actual = Run<SpdyReadQueue>(kind, iterations, true);
    CHECK_EQ(expected, actual);
    if (kind == 0 || kind == 1) {
      CHECK_EQ(actual, iterations * (kind == 0 ? 256u : 2u));
    }
  }
  if (verify) {
    UNSAFE_BUFFERS(
        std::puts("Verified seven public-API workloads; no timing."));
    return 0;
  }
  for (size_t kind = 0; kind < kCases.size(); ++kind) {
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      bool rust_first = (random() & 1) != 0;
      std::pair<double, uint64_t> baseline, rust;
      if (rust_first) {
        rust = Measure<SpdyReadQueue>(kind, iterations);
        baseline = Measure<Http2ReadQueueOriginal>(kind, iterations);
      } else {
        baseline = Measure<Http2ReadQueueOriginal>(kind, iterations);
        rust = Measure<SpdyReadQueue>(kind, iterations);
      }
      CHECK_EQ(baseline.second, rust.second);
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,"
                      "\"rust_ns\":%.9f,\"iterations\":%zu,\"rust_first\":%s,"
                      "\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
                      kCases[kind], repeat, baseline.first, rust.first,
                      iterations, rust_first ? "true" : "false",
                      static_cast<unsigned long long>(baseline.second),
                      static_cast<unsigned long long>(rust.second)));
    }
  }
}
