// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>
#include <type_traits>

#include "base/check.h"
#include "base/time/tick_clock.h"
#include "net/base/backoff_entry.h"
#include "net/base/backoff_entry_serializer.h"
#include "net/base/backoff_rust_original_entry.h"
#include "net/base/backoff_rust_original_serializer.h"
namespace {
class Clock final : public base::TickClock {
 public:
  base::TimeTicks NowTicks() const override {
    return base::TimeTicks() + base::Seconds(500);
  }
};
struct Case {
  const char* key;
  unsigned kind;
};
volatile uint64_t sink;
template <bool rust>
uint64_t Run(const Case& c, size_t n) {
  using Entry = std::conditional_t<rust, net::BackoffEntry,
                                   backoff_rust_original::BackoffEntry>;
  using Serializer =
      std::conditional_t<rust, net::BackoffEntrySerializer,
                         backoff_rust_original::BackoffEntrySerializer>;
  typename Entry::Policy policy{
      c.kind == 0 ? 100 : 0, 100, 2.0, 0.0, 20000, 5000, false};
  Clock clock;
  const base::Time wall = base::Time::FromInternalValue(500000000);
  base::ListValue list;
  list.Append(2);
  list.Append(3);
  list.Append("1000000");
  list.Append("501000000");
  uint64_t sum = 0;
  for (size_t i = 0; i < n; ++i) {
    Entry entry(&policy, &clock);
    if (c.kind == 0 || c.kind == 1) {
      entry.InformOfRequest(false);
      sum += entry.GetReleaseTime().ToInternalValue();
    } else if (c.kind == 2) {
      entry.SetCustomReleaseTime(base::TimeTicks() + base::Seconds(505));
      entry.InformOfRequest(true);
      sum += entry.CanDiscard();
    } else if (c.kind == 3) {
      entry.SetCustomReleaseTime(base::TimeTicks() + base::Seconds(505));
      sum += Serializer::SerializeToList(entry, wall).size();
    } else {
      auto restored =
          Serializer::DeserializeFromList(list, &policy, &clock, wall);
      sum += restored ? restored->GetReleaseTime().ToInternalValue() : 0;
    }
  }
  return sum;
}
template <bool rust>
double Measure(const Case& c, size_t n) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = Run<rust>(c, n);
  const auto stop = std::chrono::steady_clock::now();
  sink = result;
  return std::chrono::duration<double, std::nano>(stop - start).count() / n;
}
}  // namespace
int main(int argc, char** argv) {
  constexpr std::array<Case, 5> cases = {{{"ignored_failure", 0},
                                          {"exponential_failure", 1},
                                          {"success_horizon_discard", 2},
                                          {"serialize", 3},
                                          {"deserialize_replay", 4}}};
  const bool verify = argc > 1 && std::string_view(argv[1]) == "--verify-only";
  constexpr size_t n = 20000;
  std::mt19937 rng(20261112);
  for (const auto& c : cases) {
    CHECK_EQ(Run<false>(c, 200), Run<true>(c, 200));
    if (verify) {
      continue;
    }
    for (unsigned i = 0; i < 20; ++i) {
      const bool first = (rng() & 1) != 0;
      double baseline, candidate;
      if (first) {
        candidate = Measure<true>(c, n);
        baseline = Measure<false>(c, n);
      } else {
        baseline = Measure<false>(c, n);
        candidate = Measure<true>(c, n);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          c.key, i, baseline, candidate, n, first ? "true" : "false"));
    }
  }
}
