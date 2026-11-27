// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <optional>
#include <random>
#include <string_view>
#include <type_traits>

#include "base/time/time_override.h"
#include "net/spdy/http2_write_queue_rust_test_support.h"

namespace net::http2_write_queue_test {
namespace {
constexpr std::array<const char*, 10> kCases = {
    "frame_caps",         "empty_queries", "priority_fifo",
    "remove_stream",      "remove_after",  "change_priority",
    "clear_reentrant",    "destruction",   "remove_stream_hit",
    "change_priority_hit"};
volatile uint64_t sink;

template <class Queue>
uint64_t Run(size_t kind,
             size_t iterations,
             const std::vector<std::unique_ptr<SpdyStream>>& streams) {
  uint64_t sum = 0;
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    Queue queue;
    if (kind == 0) {
      for (int f = 0; f < 256; ++f) {
        if constexpr (std::is_same_v<Queue, SpdyWriteQueue>) {
          sum +=
              IsSpdyFrameTypeWriteCapped(static_cast<spdy::SpdyFrameType>(f));
        } else {
          sum += Http2OriginalIsCapped(static_cast<spdy::SpdyFrameType>(f));
        }
      }
      continue;
    }
    if (kind == 1) {
      for (int i = 0; i < 256; ++i) {
        sum += queue.IsEmpty();
      }
      continue;
    }
    for (int i = 0; i < 256; ++i) {
      const auto p = static_cast<RequestPriority>(i % NUM_PRIORITIES);
      Push(queue, p, i % 11, i,
           (i & 1) ? StreamWeakPtrFor<Queue>(*streams[p]) : base::WeakPtr<SpdyStream>());
    }
    if (kind == 3) {
      queue.RemovePendingWritesForStream(streams[MEDIUM].get());
    }
    if (kind == 4) {
      queue.RemovePendingWritesForStreamsAfter(4);
    }
    if (kind == 5) {
      queue.ChangePriorityOfWritesForStream(streams[MEDIUM].get(), MEDIUM,
                                            HIGHEST);
    }
    // Preserve the first eight historical cases, including their MEDIUM
    // stream scan misses. LOW has 43 attached writes in this fixture.
    if (kind == 8) {
      queue.RemovePendingWritesForStream(streams[LOW].get());
    }
    if (kind == 9) {
      queue.ChangePriorityOfWritesForStream(streams[LOW].get(), LOW, HIGHEST);
    }
    if (kind == 6) {
      Push(queue, HIGHEST, 6, 300, {}, [&] { Push(queue, LOW, 3, 301); });
      queue.Clear();
    }
    sum += queue.num_queued_capped_frames();
    if (kind != 7) {
      for (;;) {
        OutputFor<Queue> out;
        if (!out.Pop(queue)) {
          break;
        }
        sum += out.value() + static_cast<uint8_t>(out.frame);
      }
      sum += queue.num_queued_capped_frames();
    }
    // Queue/producers and all per-iteration storage are destroyed in the
    // sample.
  }
  return sum;
}

// Outside all measurement clocks, prove that the added hit workloads remove
// or move actual writes, and that FIFO order follows the documented queues.
// Check each implementation independently: paired checksums alone could agree
// even if neither fixture exercised the intended mutation.
template <class Queue>
std::array<std::vector<int>, 2> VerifyHitMutations(
    const std::vector<std::unique_ptr<SpdyStream>>& streams) {
  static_assert(LOW == 3 && HIGHEST == 5 && NUM_PRIORITIES == 6);
  std::array<std::vector<int>, 2> destruction_traces;
  for (bool move : {false, true}) {
    auto& destroyed = destruction_traces[move ? 1 : 0];
    std::vector<int> matching;
    Queue queue;
    for (int i = 0; i < 256; ++i) {
      const auto priority = static_cast<RequestPriority>(i % NUM_PRIORITIES);
      const bool attached = (i & 1) != 0;
      if (attached && priority == LOW) {
        matching.push_back(i);
      }
      Push(queue, priority, i % 11, i,
           attached ? StreamWeakPtrFor<Queue>(*streams[priority])
                    : base::WeakPtr<SpdyStream>(),
           [&, i] { destroyed.push_back(i); });
    }
    CHECK_EQ(matching.size(), size_t{43});
    if (move) {
      queue.ChangePriorityOfWritesForStream(streams[LOW].get(), LOW, HIGHEST);
      CHECK(destroyed.empty());
    } else {
      queue.RemovePendingWritesForStream(streams[LOW].get());
      // Removal promises the matching writes are gone, not ascending producer
      // destruction. Preserve the actual callback trace for paired comparison.
      auto removed = destroyed;
      std::sort(removed.begin(), removed.end());
      CHECK(removed == matching);
    }
    size_t drained = 0;
    size_t attached_low = 0;
    auto expect_next = [&](int id) {
      OutputFor<Queue> out;
      CHECK(out.Pop(queue));
      CHECK_EQ(out.value(), id);
      CHECK_EQ(static_cast<int>(out.frame), id % 11);
      const auto priority = static_cast<RequestPriority>(id % NUM_PRIORITIES);
      CHECK_EQ(out.stream.get(), (id & 1) ? streams[priority].get() : nullptr);
      attached_low += out.stream.get() == streams[LOW].get();
      ++drained;
    };
    for (int priority = HIGHEST; priority >= MINIMUM_PRIORITY; --priority) {
      for (int id = priority; id < 256; id += NUM_PRIORITIES) {
        if (priority != LOW || !(id & 1)) {
          expect_next(id);
        }
      }
      // Moved writes follow the pre-existing HIGHEST queue, in original FIFO.
      if (move && priority == HIGHEST) {
        for (int id : matching) {
          expect_next(id);
        }
      }
    }
    CHECK_EQ(drained, move ? size_t{256} : size_t{213});
    CHECK_EQ(attached_low, move ? size_t{43} : size_t{0});
    OutputFor<Queue> extra;
    CHECK(!extra.Pop(queue));
    CHECK(queue.IsEmpty());
    CHECK_EQ(queue.num_queued_capped_frames(), 0);
    CHECK_EQ(destroyed.size(), size_t{256});
  }
  return destruction_traces;
}

template <class Queue>
double Measure(size_t kind,
               size_t iterations,
               const std::vector<std::unique_ptr<SpdyStream>>& streams) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = Run<Queue>(kind, iterations, streams);
  const auto stop = std::chrono::steady_clock::now();
  sink = result;
  return std::chrono::duration<double, std::nano>(stop - start).count() /
         iterations;
}
}  // namespace
}  // namespace net::http2_write_queue_test

int main(int argc, char** argv) {
  using namespace net;
  using namespace net::http2_write_queue_test;
  const bool verify =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) {
    // SAFETY: the diagnostic is a static NUL-terminated literal.
    UNSAFE_BUFFERS(
        std::fputs("Specify --verify-only (no timing) or --timing\n", stderr));
    return 2;
  }
  // Stream construction is an excluded fixture service. In verification mode,
  // supply constant timestamps as well: neither setup nor Run consults clocks.
  std::optional<base::subtle::ScopedTimeClockOverrides> constant_time;
  if (verify) {
    constant_time.emplace([] { return base::Time(); },
                          [] { return base::TimeTicks(); },
                          [] { return base::ThreadTicks(); });
  }
  std::vector<std::unique_ptr<SpdyStream>> streams;
  for (int p = 0; p < NUM_PRIORITIES; ++p) {
    streams.push_back(MakeStream(static_cast<RequestPriority>(p), p * 2));
  }
  CHECK(VerifyHitMutations<Http2WriteQueueOriginal>(streams) ==
        VerifyHitMutations<SpdyWriteQueue>(streams));
  std::mt19937 random(20261113);
  constexpr size_t iterations = 256;
  for (size_t kind = 0; kind < kCases.size(); ++kind) {
    CHECK_EQ(Run<Http2WriteQueueOriginal>(kind, 8, streams),
             Run<SpdyWriteQueue>(kind, 8, streams));
    if (verify) {
      continue;  // No steady_clock or other timing calls on this path.
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double baseline, rust;
      if (rust_first) {
        rust = Measure<SpdyWriteQueue>(kind, iterations, streams);
        baseline = Measure<Http2WriteQueueOriginal>(kind, iterations, streams);
      } else {
        baseline = Measure<Http2WriteQueueOriginal>(kind, iterations, streams);
        rust = Measure<SpdyWriteQueue>(kind, iterations, streams);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          kCases[kind], repeat, baseline, rust, iterations,
          rust_first ? "true" : "false"));
    }
  }
}
