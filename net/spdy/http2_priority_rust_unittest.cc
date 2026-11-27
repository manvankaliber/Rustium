// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <cstdint>
#include <limits>
#include <random>

#include "net/spdy/http2_priority_dependencies.h"
#include "net/spdy/http2_priority_rust_original.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

struct Output {
  spdy::SpdyStreamId parent = 0xfedcba98;
  int weight = -77;
  bool exclusive = false;
  bool operator==(const Output&) const = default;
};

template <class State>
Output Create(State& state, uint32_t id, uint8_t priority) {
  Output out;
  state.OnStreamCreation(id, priority, &out.parent, &out.weight,
                         &out.exclusive);
  return out;
}

template <class A, class B>
void CompareUpdates(const A& a, const B& b) {
  ASSERT_EQ(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a[i].id, b[i].id);
    EXPECT_EQ(a[i].parent_stream_id, b[i].parent_stream_id);
    EXPECT_EQ(a[i].weight, b[i].weight);
    EXPECT_EQ(a[i].exclusive, b[i].exclusive);
  }
}

TEST(Http2PriorityRustTest, AllWeightsAndBoundaryIds) {
  constexpr std::array<int, 8> weights = {256, 220, 183, 147, 110, 74, 37, 1};
  for (uint8_t p = 0; p < 8; ++p) {
    Http2PriorityDependencies rust;
    Http2PriorityRustOriginal cpp;
    for (uint32_t id : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
      const auto actual = Create(rust, id, p);
      EXPECT_EQ(actual, Create(cpp, id, p));
      EXPECT_EQ(actual.weight, weights[p]);
      EXPECT_TRUE(actual.exclusive);
    }
    for (uint32_t id : {0u, 1u, 0xffffffffu}) {
      for (uint8_t q = 0; q < 8; ++q) {
        CompareUpdates(rust.OnStreamUpdate(id, q), cpp.OnStreamUpdate(id, q));
      }
    }
  }
}

TEST(Http2PriorityRustTest, DefinedEarlyReturnsAndUninitializedOutputs) {
  Http2PriorityDependencies rust;
  Http2PriorityRustOriginal cpp;
  for (unsigned p = 0; p <= 255; ++p) {
    CompareUpdates(rust.OnStreamUpdate(42, p), cpp.OnStreamUpdate(42, p));
  }
  // Successful creation must initialize every output; no prior value required.
  uint32_t parent;
  int weight;
  bool exclusive;
  rust.OnStreamCreation(42, 0, &parent, &weight, &exclusive);
  EXPECT_EQ(parent, 0u);
  EXPECT_EQ(weight, 256);
  EXPECT_TRUE(exclusive);
  Create(cpp, 42, 0);
  for (unsigned p = 0; p <= 255; ++p) {
    EXPECT_EQ(Create(rust, 42, p), Output());
    EXPECT_EQ(Create(cpp, 42, p), Output());
    // Duplicate creations return before dereferencing even null outputs.
    rust.OnStreamCreation(42, p, nullptr, nullptr, nullptr);
    cpp.OnStreamCreation(42, p, nullptr, nullptr, nullptr);
  }
  rust.OnStreamDestruction(42);
  cpp.OnStreamDestruction(42);
  rust.OnStreamDestruction(42);
  cpp.OnStreamDestruction(42);
  EXPECT_EQ(Create(rust, 42, 7), Create(cpp, 42, 7));
}

template <class State>
std::array<uint32_t, 3> AliasedOutputs() {
  State state;
  std::array<uint32_t, 3> out;
  uint32_t both = 0;
  bool exclusive;
  // Corresponding signed/unsigned types may alias. Preserve the original
  // initial root write, weight write and conditional final parent write.
  state.OnStreamCreation(0, 1, &both, reinterpret_cast<int*>(&both),
                         &exclusive);
  out[0] = both;  // No parent: last write was weight.
  state.OnStreamCreation(1, 1, &both, reinterpret_cast<int*>(&both),
                         &exclusive);
  out[1] = both;  // Real parent with ID 0: last write was parent 0.
  state.OnStreamCreation(2, 1, &both, reinterpret_cast<int*>(&both),
                         &exclusive);
  out[2] = both;
  return out;
}

TEST(Http2PriorityRustTest, AliasedOutputs) {
  const auto actual = AliasedOutputs<Http2PriorityDependencies>();
  EXPECT_EQ(actual, AliasedOutputs<Http2PriorityRustOriginal>());
  EXPECT_EQ(actual, (std::array<uint32_t, 3>{220, 0, 1}));
}

TEST(Http2PriorityRustTest, CopyOwnsIndependentState) {
  Http2PriorityDependencies a;
  Create(a, 0, 0);
  Create(a, 9, 7);
  Http2PriorityDependencies b(a);
  Http2PriorityDependencies c;
  c = b;
  auto* alias = &c;
  c = *alias;
  a.OnStreamDestruction(9);
  EXPECT_EQ(Create(a, 10, 7).parent, 0u);
  EXPECT_EQ(Create(b, 10, 7).parent, 9u);
  EXPECT_EQ(Create(c, 10, 7).parent, 9u);
  // Dead-slot free list is cloned as well as live nodes.
  b.OnStreamDestruction(0);
  Http2PriorityDependencies d(b);
  EXPECT_EQ(Create(d, 11, 0).parent, 0u);
  EXPECT_EQ(Create(b, 11, 0).parent, 0u);
}

TEST(Http2PriorityRustTest, MillionStatefulDifferentialTransitions) {
  std::mt19937 random(0x82f013);
  size_t transitions = 0;
  for (size_t epoch = 0; epoch < 1024; ++epoch) {
    Http2PriorityDependencies rust;
    Http2PriorityRustOriginal cpp;
    for (size_t step = 0; step < 1024; ++step) {
      const uint32_t word = random();
      uint32_t id = (word >> 8) & 127;
      if (id >= 124) {
        id = std::numeric_limits<uint32_t>::max() - (id - 124);
      }
      const uint8_t priority = (word >> 24) & 7;
      switch (word & 3) {
        case 0:
          ASSERT_EQ(Create(rust, id, priority), Create(cpp, id, priority));
          break;
        case 1:
          rust.OnStreamDestruction(id);
          cpp.OnStreamDestruction(id);
          break;
        default:
          CompareUpdates(rust.OnStreamUpdate(id, priority),
                         cpp.OnStreamUpdate(id, priority));
      }
      ++transitions;
      // Observe each priority band's tail after mutation, then undo probes.
      // This exposes state even when the most recent update had no outputs.
      if (step % 64 == 0) {
        for (uint8_t p = 0; p < 8; ++p) {
          ASSERT_EQ(Create(rust, 0x80000000, p), Create(cpp, 0x80000000, p));
          rust.OnStreamDestruction(0x80000000);
          cpp.OnStreamDestruction(0x80000000);
        }
      }
    }
    // Both destructors exercise nonempty mixed-band state; no callbacks or
    // cancellable operations exist in this synchronous value-only API.
  }
  EXPECT_EQ(transitions, 1048576u);
}

}  // namespace
}  // namespace net
