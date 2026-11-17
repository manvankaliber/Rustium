// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "net/spdy/http2_priority_dependencies.h"
#include "net/spdy/http2_priority_rust_original.h"

namespace {

using Rust = net::Http2PriorityDependencies;
using Original = net::Http2PriorityRustOriginal;
using Id = spdy::SpdyStreamId;
static_assert(std::is_same_v<std::make_signed_t<Id>, int>);

uint8_t Byte(base::span<const uint8_t>& input) {
  return input.empty() ? 0 : input.take_first<1>()[0];
}

Id StreamId(uint8_t byte) {
  // A small repeatable ID space guides duplicate/delete/update sequences.
  // Keep the probe ID 0xfffffffe out of that space.
  constexpr std::array<Id, 4> kExtremes = {0x7fffffff, 0x80000000, 0xfffffffd,
                                           0xffffffff};
  const size_t index = byte % 36;
  return index < 32 ? static_cast<Id>(index) : kExtremes[index - 32];
}

struct Output {
  Id parent = 0xfedcba98;
  int weight = -77;
  bool exclusive = false;
  bool operator==(const Output&) const = default;
};

template <class State>
Output Create(State& state, Id id, uint8_t priority, bool alias) {
  Output out;
  if (alias) {
    // Corresponding signed/unsigned integer types may alias. The original's
    // final parent write (even parent ID zero) must overwrite its weight write.
    state.OnStreamCreation(id, priority, &out.parent,
                           reinterpret_cast<int*>(&out.parent), &out.exclusive);
  } else {
    state.OnStreamCreation(id, priority, &out.parent, &out.weight,
                           &out.exclusive);
  }
  return out;
}

template <class A, class B>
void CompareUpdates(const A& actual, const B& expected) {
  CHECK_EQ(actual.size(), expected.size());
  for (size_t i = 0; i < actual.size(); ++i) {
    CHECK_EQ(actual[i].id, expected[i].id);
    CHECK_EQ(actual[i].parent_stream_id, expected[i].parent_stream_id);
    CHECK_EQ(actual[i].weight, expected[i].weight);
    CHECK_EQ(actual[i].exclusive, expected[i].exclusive);
  }
}

struct Command {
  enum Kind { kCreate, kDestroy, kUpdate } kind;
  Id id;
  uint8_t priority;
};

struct Pair {
  std::unique_ptr<Rust> actual = std::make_unique<Rust>();
  std::unique_ptr<Original> expected = std::make_unique<Original>();
  std::vector<Command> history;
  std::set<Id> live;

  void Apply(Command command, bool alias, bool null_duplicate) {
    switch (command.kind) {
      case Command::kCreate:
        if (null_duplicate) {
          CHECK(live.contains(command.id));
          actual->OnStreamCreation(command.id, command.priority, nullptr,
                                   nullptr, nullptr);
          expected->OnStreamCreation(command.id, command.priority, nullptr,
                                     nullptr, nullptr);
        } else {
          CHECK(Create(*actual, command.id, command.priority, alias) ==
                Create(*expected, command.id, command.priority, alias));
        }
        live.insert(command.id);
        break;
      case Command::kDestroy:
        actual->OnStreamDestruction(command.id);
        expected->OnStreamDestruction(command.id);
        live.erase(command.id);
        break;
      case Command::kUpdate:
        CompareUpdates(actual->OnStreamUpdate(command.id, command.priority),
                       expected->OnStreamUpdate(command.id, command.priority));
        break;
    }
    history.push_back(command);
  }

  void CopyFrom(const Pair& source, bool assign) {
    if (assign) {
      *actual = *source.actual;  // Also exercises defined Rust self-assignment.
    } else {
      actual = std::make_unique<Rust>(*source.actual);
    }
    // Original's implicit copy copies map iterators into the SOURCE lists.
    // Do not use that broken copy as an oracle. Independently replay its exact
    // policy history into a fresh original, then continue mutating both pairs.
    const auto copied_history = source.history;
    const auto copied_live = source.live;
    expected = std::make_unique<Original>();
    for (const auto& command : copied_history) {
      switch (command.kind) {
        case Command::kCreate:
          Create(*expected, command.id, command.priority, false);
          break;
        case Command::kDestroy:
          expected->OnStreamDestruction(command.id);
          break;
        case Command::kUpdate:
          expected->OnStreamUpdate(command.id, command.priority);
          break;
      }
    }
    history = copied_history;
    live = copied_live;
  }

  void Probe() {
    // A temporary stream observes every band's tail even after silent updates.
    // It is always removed, leaving logical order unchanged in both engines.
    constexpr Id kProbe = 0xfffffffe;
    for (uint8_t priority = 0; priority <= spdy::kV3LowestPriority;
         ++priority) {
      CHECK(Create(*actual, kProbe, priority, false) ==
            Create(*expected, kProbe, priority, false));
      actual->OnStreamDestruction(kProbe);
      expected->OnStreamDestruction(kProbe);
    }
  }
};

void Fuzz(base::span<const uint8_t> input) {
  std::array<Pair, 3> states;
  for (size_t step = 0; step < 256 && !input.empty(); ++step) {
    const uint8_t operation = Byte(input);
    Pair& state = states[(operation / 8) % states.size()];
    const Id id = StreamId(Byte(input));
    const uint8_t priority = Byte(input);
    const uint8_t flags = Byte(input);
    switch (operation % 8) {
      case 0:
        state.Apply({Command::kCreate, id, static_cast<uint8_t>(priority % 8)},
                    flags & 1, false);
        break;
      case 1:
        state.Apply({Command::kDestroy, id, 0}, false, false);
        break;
      case 2:
        state.Apply({Command::kUpdate, id, static_cast<uint8_t>(priority % 8)},
                    false, false);
        break;
      case 3:
        // Both original early returns precede priority validation/output
        // dereferences. Raw 0..255 priorities are defined on these paths only.
        if (state.live.contains(id)) {
          state.Apply({Command::kCreate, id, priority}, flags & 1, flags & 2);
        } else {
          state.Apply({Command::kUpdate, id, priority}, false, false);
        }
        break;
      case 4:
      case 5:
        state.CopyFrom(states[flags % states.size()], operation % 8 == 5);
        break;
      case 6:
        state.actual = std::make_unique<Rust>();
        state.expected = std::make_unique<Original>();
        state.history.clear();
        state.live.clear();
        break;
      case 7:
        break;
    }
    // Check source and destination after copies to expose shared-state bugs.
    for (auto& pair : states) {
      pair.Probe();
    }
  }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies size initialized readable bytes for this call.
  // Operands are consumed through checked spans, never raw pointer arithmetic.
  const auto input = size == 0 ? base::span<const uint8_t>()
                               : UNSAFE_BUFFERS(base::span(data, size));
  Fuzz(input.first(std::min(input.size(), size_t{1024})));
  return 0;
}
