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

#include "base/check.h"
#include "net/spdy/http2_priority_dependencies.h"
#include "net/spdy/http2_priority_rust_original.h"

namespace net {
namespace {
constexpr std::array<const char*, 10> kCases = {
    "create_fifo",        "create_bands",    "destroy_hit", "destroy_miss",
    "lifetime",           "update_reparent", "update_noop", "update_missing",
    "update_same_parent", "duplicate_create"};
volatile uint64_t sink;

// Untimed specification fixture: a single flat stable priority order, with
// linear scans and no shared Rust arena or frozen C++ lists/map. Checks the
// complete transcript of each workload independently, before any clocks.
template <class Queue>
class CheckedQueue {
 public:
  void OnStreamCreation(uint32_t id,
                        uint8_t priority,
                        uint32_t* parent,
                        int* weight,
                        bool* exclusive) {
    const auto existing = Find(id);
    if (existing != order_.end()) {
      const auto saved_parent = *parent;
      const auto saved_weight = *weight;
      const auto saved_exclusive = *exclusive;
      queue_.OnStreamCreation(id, priority, parent, weight, exclusive);
      CHECK(*parent == saved_parent && *weight == saved_weight &&
            *exclusive == saved_exclusive);
      return;
    }
    const auto insertion = Bound(priority);
    const auto expected_parent =
        insertion == order_.begin() ? 0 : std::prev(insertion)->first;
    queue_.OnStreamCreation(id, priority, parent, weight, exclusive);
    CHECK(*parent == expected_parent && *weight == Weight(priority) &&
          *exclusive);
    order_.insert(insertion, {id, priority});
  }

  void OnStreamDestruction(uint32_t id) {
    const auto it = Find(id);
    if (it != order_.end()) {
      order_.erase(it);
    }
    queue_.OnStreamDestruction(id);
  }

  auto OnStreamUpdate(uint32_t id, uint8_t priority) {
    const auto current = Find(id);
    std::vector<Http2PriorityDependencies::DependencyUpdate> expected;
    if (current != order_.end() && current->second != priority) {
      const auto old_parent = current == order_.begin()
                                  ? std::nullopt
                                  : std::optional(std::prev(current)->first);
      const auto insertion = Bound(priority);
      auto new_parent = insertion == order_.begin()
                            ? std::nullopt
                            : std::optional(std::prev(insertion)->first);
      if (new_parent == id) {
        new_parent = old_parent;
      }
      if (old_parent != new_parent) {
        const auto child = std::next(current);
        if (child != order_.end()) {
          expected.push_back({child->first, old_parent.value_or(0),
                              Weight(child->second), true});
        }
        expected.push_back(
            {id, new_parent.value_or(0), Weight(priority), true});
      }
      order_.erase(current);
      order_.insert(Bound(priority), {id, priority});
    }
    auto actual = queue_.OnStreamUpdate(id, priority);
    CHECK(actual.size() == expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      CHECK(actual[i].id == expected[i].id &&
            actual[i].parent_stream_id == expected[i].parent_stream_id &&
            actual[i].weight == expected[i].weight &&
            actual[i].exclusive == expected[i].exclusive);
    }
    return actual;
  }

 private:
  static int Weight(uint8_t p) {
    constexpr std::array<int, 8> weights = {256, 220, 183, 147, 110, 74, 37, 1};
    return weights[p];
  }
  auto Find(uint32_t id) {
    return std::find_if(order_.begin(), order_.end(),
                        [id](const auto& entry) { return entry.first == id; });
  }
  auto Bound(uint8_t priority) {
    return std::upper_bound(
        order_.begin(), order_.end(), priority,
        [](uint8_t p, const auto& entry) { return p < entry.second; });
  }
  Queue queue_;
  std::vector<std::pair<uint32_t, uint8_t>> order_;
};

uint64_t Mix(uint64_t sum, uint64_t value) {
  return (sum ^ value) * 0x100000001b3ull;
}

template <class Queue>
uint64_t Create(Queue& queue, uint32_t id, uint8_t p, uint64_t sum) {
  uint32_t parent = 0xfedcba98;
  int weight = -77;
  bool exclusive = false;
  queue.OnStreamCreation(id, p, &parent, &weight, &exclusive);
  return Mix(Mix(Mix(sum, parent), static_cast<uint32_t>(weight)), exclusive);
}

template <class Queue>
uint64_t Update(Queue& queue, uint32_t id, uint8_t p, uint64_t sum) {
  auto updates = queue.OnStreamUpdate(id, p);
  sum = Mix(sum, updates.size());
  for (const auto& update : updates) {
    sum = Mix(
        Mix(Mix(Mix(sum, update.id), update.parent_stream_id), update.weight),
        update.exclusive);
  }
  return sum;
}

// Every sample includes construction/destruction, initial creation of the
// fixture, public API + FFI, output hashing and vector allocation. No isolated
// operation timing is implied. IDs/priorities vary with iteration.
template <class Queue>
uint64_t Run(size_t kind, size_t iterations) {
  uint64_t sum = 0xcbf29ce484222325ull;
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    Queue queue;
    const uint32_t base = static_cast<uint32_t>(iteration * 1024);
    if (kind == 7) {
      for (uint32_t i = 0; i < 128; ++i) {
        sum = Update(queue, base + i, i % 8, sum);
      }
      continue;
    }
    if (kind == 8) {
      // One stream changes bands without changing its parent; every result is
      // intentionally empty. Do not label this as reparenting or no-op
      // priority.
      sum = Create(queue, base, iteration % 8, sum);
      for (uint32_t i = 0; i < 128; ++i) {
        sum = Update(queue, base, (i + iteration) % 8, sum);
      }
      continue;
    }
    for (uint32_t i = 0; i < 128; ++i) {
      const uint8_t p = kind == 0 ? 3 : (i + iteration) % 8;
      sum = Create(queue, base + i, p, sum);
    }
    for (uint32_t i = 0; i < 128; ++i) {
      switch (kind) {
        case 2:
          queue.OnStreamDestruction(base + ((i * 37) % 128));
          break;
        case 3:
          queue.OnStreamDestruction(base + 128 + i);
          break;
        case 5:
          sum = Update(queue, base + ((i * 37) % 128), (i + iteration + 5) % 8,
                       sum);
          break;
        case 6:
          sum = Update(queue, base + i, (i + iteration) % 8, sum);
          break;
        case 9:
          sum = Create(queue, base + i, (i + iteration + 1) % 8, sum);
          break;
      }
    }
    // Probe post-action state: destruction hit must leave an empty tree;
    // misses, FIFO order, update movement, and duplicates affect this checksum.
    if (kind != 4) {
      for (uint8_t p = 0; p < 8; ++p) {
        sum = Create(queue, base + 256 + p, p, sum);
      }
    }
  }
  return sum;
}

// Type-independent semantic fixture checks run separately for each class
// before clocks. Paired checksums are additional validation, not their oracle.
template <class Queue>
void VerifySemantics() {
  Queue queue;
  auto create = [&](uint32_t id, uint8_t p, uint32_t expected_parent) {
    uint32_t parent;
    int weight;
    bool exclusive;
    queue.OnStreamCreation(id, p, &parent, &weight, &exclusive);
    CHECK(parent == expected_parent);
    constexpr std::array<int, 8> weights = {256, 220, 183, 147, 110, 74, 37, 1};
    CHECK(weight == weights[p]);
    CHECK(exclusive);
  };
  CHECK(queue.OnStreamUpdate(999, 255).empty());
  queue.OnStreamDestruction(999);
  create(1, 0, 0);
  create(2, 1, 1);
  create(3, 7, 2);
  auto updates = queue.OnStreamUpdate(1, 1);
  CHECK(updates.size() == 2);
  CHECK(updates[0].id == 2 && updates[0].parent_stream_id == 0 &&
        updates[0].weight == 220 && updates[0].exclusive);
  CHECK(updates[1].id == 1 && updates[1].parent_stream_id == 2 &&
        updates[1].weight == 220 && updates[1].exclusive);
  CHECK(queue.OnStreamUpdate(1, 1).empty());
  // Duplicate creation preserves all sentinels and priority order.
  uint32_t parent = 123;
  int weight = -55;
  bool exclusive = false;
  queue.OnStreamCreation(1, 7, &parent, &weight, &exclusive);
  CHECK(parent == 123 && weight == -55 && !exclusive);
  create(4, 1, 1);
  queue.OnStreamDestruction(4);
  create(5, 1, 1);
  queue.OnStreamDestruction(2);
  queue.OnStreamDestruction(1);
  queue.OnStreamDestruction(3);
  queue.OnStreamDestruction(5);
  create(6, 7, 0);
  CHECK(queue.OnStreamUpdate(6, 0).empty());
}

template <class Queue>
double Measure(size_t kind, size_t iterations) {
  const auto start = std::chrono::steady_clock::now();
  const uint64_t result = Run<Queue>(kind, iterations);
  const auto stop = std::chrono::steady_clock::now();
  sink = result;
  return std::chrono::duration<double, std::nano>(stop - start).count() /
         iterations;
}
}  // namespace
}  // namespace net

int main(int argc, char** argv) {
  using namespace net;
  const bool verify =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) {
    UNSAFE_BUFFERS(std::fputs("Specify --verify-only or --timing\n", stderr));
    return 2;
  }
  VerifySemantics<Http2PriorityDependencies>();
  VerifySemantics<Http2PriorityRustOriginal>();
  std::mt19937 random(20261113);
  constexpr size_t iterations = 256;
  for (size_t kind = 0; kind < kCases.size(); ++kind) {
    const auto baseline = Run<Http2PriorityRustOriginal>(kind, 32);
    const auto rust = Run<Http2PriorityDependencies>(kind, 32);
    CHECK(baseline == rust);
    CHECK(baseline == Run<CheckedQueue<Http2PriorityRustOriginal>>(kind, 32));
    CHECK(rust == Run<CheckedQueue<Http2PriorityDependencies>>(kind, 32));
    if (verify) {
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"verified\":true,\"iterations\":32,"
                      "\"checksum\":\"%016llx\"}\n",
                      kCases[kind], static_cast<unsigned long long>(rust)));
      continue;  // This path never enters Measure or reads any timing clock.
    }
    uint64_t expected_checksum =
        Run<Http2PriorityRustOriginal>(kind, iterations);
    CHECK(expected_checksum ==
          Run<Http2PriorityDependencies>(kind, iterations));
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double baseline_ns;
      double rust_ns;
      if (rust_first) {
        rust_ns = Measure<Http2PriorityDependencies>(kind, iterations);
        CHECK(sink == expected_checksum);
        baseline_ns = Measure<Http2PriorityRustOriginal>(kind, iterations);
      } else {
        baseline_ns = Measure<Http2PriorityRustOriginal>(kind, iterations);
        CHECK(sink == expected_checksum);
        rust_ns = Measure<Http2PriorityDependencies>(kind, iterations);
      }
      CHECK(sink == expected_checksum);
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,"
                      "\"rust_ns\":%.9f,\"iterations\":%zu,\"rust_first\":%s,"
                      "\"checksum\":\"%016llx\"}\n",
                      kCases[kind], repeat, baseline_ns, rust_ns, iterations,
                      rust_first ? "true" : "false",
                      static_cast<unsigned long long>(expected_checksum)));
    }
  }
}
