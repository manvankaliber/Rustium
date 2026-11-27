// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Parent alone runs --timing in a quiet coordinated window. --verify-only
// never reads a clock and checks every frozen workload through real public
// APIs.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>
#include <vector>

#include "base/compiler_specific.h"
#include "net/cookies/cookie_inclusion_status_rust_test_support.h"
namespace {
using namespace net::cookie_inclusion_status_rust_test;
struct Case {
  const char* key;
  unsigned mode;
  uint64_t ex, warn;
  int exemption;
  size_t iterations;
};
volatile uint64_t sink = 0;
uint64_t Consume(std::string_view s) {
  uint64_t hash = 2166136261;
  for (unsigned char c : s) {
    hash = (hash ^ c) * 16777619;
  }
  return hash;
}
template <typename T>
NOINLINE uint64_t Run(const Case& c, size_t count) {
  using E = typename T::ExclusionReason;
  using W = typename T::WarningReason;
  using X = typename T::ExemptionReason;
  uint64_t sum = 0;
  for (size_t i = 0; i < count; ++i) {
    T s;
    if (c.mode == 3 || c.mode == 4) {
      auto components =
          Components<T>(c.ex ^ (c.mode == 4 ? uint64_t{i & 63} << 58 : 0),
                        c.warn, c.exemption);
      sum += components.has_value();
      if (components) {
        sum += components->exclusion_reasons().ToEnumBitmask() +
               components->warning_reasons().ToEnumBitmask();
      }
      continue;
    }
    s = *Components<T>(c.ex, c.warn, c.exemption);
    switch (c.mode) {
      case 0:
        sum += s.IsInclude() + s.ShouldWarn() +
               s.ShouldRecordDowngradeMetrics() +
               s.ExcludedByUserPreferencesOrTPCD();
        for (int reason : {-1, 0, 7, 9, 14, 24, 29, 30}) {
          sum += s.HasExclusionReason(static_cast<E>(reason)) +
                 s.HasOnlyExclusionReason(static_cast<E>(reason)) +
                 s.HasWarningReason(static_cast<W>(reason));
        }
        break;
      case 1:
        s.AddExclusionReason(static_cast<E>(24));
        s.AddExclusionReason(static_cast<E>(7));
        s.RemoveExclusionReason(static_cast<E>(7));
        s.MaybeClearSameSiteWarning();
        sum += s.warning_reasons().ToEnumBitmask();
        break;
      case 2:
        for (int reason = 0; reason < 14; ++reason) {
          s.AddWarningReason(static_cast<W>(reason));
        }
        for (int reason = 0; reason < 30; ++reason) {
          s.AddExclusionReason(static_cast<E>(reason));
        }
        s.RemoveExclusionReasons(
            T::ExclusionReasonBitset::FromEnumBitmask(0x15555555));
        for (int reason = -1; reason < 31; ++reason) {
          s.RemoveExclusionReason(static_cast<E>(reason));
        }
        for (int reason = -1; reason < 15; ++reason) {
          s.RemoveWarningReason(static_cast<W>(reason));
        }
        break;
      case 5: {
        const auto factory = T::MakeFromReasonsForTesting(
            s.exclusion_reasons(), s.warning_reasons(), s.exemption_reason());
        sum += s == factory;
        sum += s.HasExactlyExclusionReasonsForTesting(s.exclusion_reasons()) +
               s.HasExactlyWarningReasonsForTesting(s.warning_reasons());
        sum += s.HasExactlyExclusionReasonsForTesting({}) +
               s.HasExactlyWarningReasonsForTesting({});
        break;
      }
      case 6: {
        const T copy(s);
        T assigned;
        assigned = copy;
        const T* alias = &assigned;
        assigned = *alias;
        sum += assigned == s;
        assigned.ResetForTesting();
        sum += assigned.IsInclude() + assigned.ShouldWarn();
        break;
      }
      case 7:
        sum += Consume(s.GetDebugString());
        break;
      case 8: {
        std::ostringstream a, b;
        a << s;
        PrintTo(s, &b);
        sum += Consume(a.str()) + Consume(b.str());
        break;
      }
      case 9:
        for (int reason : {0, 1, 2, 3, 4, 5, -1, 6}) {
          s.MaybeSetExemptionReason(static_cast<X>(reason));
          sum += static_cast<int>(s.exemption_reason());
        }
        s.AddExclusionReason(static_cast<E>(24));
        s.MaybeSetExemptionReason(static_cast<X>(2));
        sum += static_cast<int>(s.exemption_reason());
        s.RemoveExclusionReason(static_cast<E>(24));
        s.MaybeSetExemptionReason(static_cast<X>(5));
        sum += static_cast<int>(s.exemption_reason());
        break;
    }
    sum += s.exclusion_reasons().ToEnumBitmask() +
           s.warning_reasons().ToEnumBitmask() +
           static_cast<int>(s.exemption_reason());
  }
  return sum;
}
template <typename T>
double Measure(const Case& c) {
  const auto start = std::chrono::steady_clock::now();
  sink = Run<T>(c, c.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         c.iterations;
}
}  // namespace
int main(int argc, char** argv) {
  const auto option = argc == 2 ? UNSAFE_BUFFERS(std::string_view(argv[1]))
                                : std::string_view();
  if (option != "--verify-only" && option != "--timing") {
    // SAFETY: the diagnostic is a static NUL-terminated literal.
    UNSAFE_BUFFERS(std::fputs(
        "Use --verify-only; --timing is parent-owned quiet-window work.\n",
        stderr));
    return 2;
  }
  const std::vector<Case> cases = {
      {"status_query_include", 0u, 0x0ULL, 0x0ULL, 0, 100000},
      {"status_query_sparse", 0u, 0x200ULL, 0x8ULL, 0, 100000},
      {"status_query_dense", 0u, 0x3effffffULL, 0x3fffULL, 0, 50000},
      {"status_warning_precedence", 1u, 0x0ULL, 0x3fffULL, 5, 50000},
      {"status_transition_all", 2u, 0x0ULL, 0x0ULL, 0, 10000},
      {"status_components_valid", 3u, 0x180ULL, 0x3fffULL, 0, 30000},
      {"status_components_rejected", 3u, 0x1000001ULL, 0x3fffULL, 5, 30000},
      {"status_bitmask_extremes", 4u, 0xffffffffffffffffULL,
       0xffffffffffffffffULL, 0, 30000},
      {"status_factory_comparison", 5u, 0x180ULL, 0x3fffULL, 0, 30000},
      {"status_copy_reset", 6u, 0x200ULL, 0x8ULL, 0, 100000},
      {"status_debug_dense", 7u, 0x3effffffULL, 0x3fffULL, 0, 10000},
      {"status_debug_exempt", 7u, 0x0ULL, 0x0ULL, 5, 30000},
      {"status_debug_phaseout", 7u, 0x1000000ULL, 0x1000ULL, 0, 30000},
      {"status_stream_print", 8u, 0x200ULL, 0x8ULL, 0, 10000},
      {"status_exemption_policy", 9u, 0x0ULL, 0x0ULL, 0, 50000},
  };
  std::mt19937 random(20261113);
  for (const auto& c : cases) {
    CHECK_EQ(Run<Original>(c, 257), Run<Rust>(c, 257));
    if (option == "--verify-only") {
      continue;
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool first = (random() & 1) != 0;
      double cpp, rust;
      if (first) {
        rust = Measure<Rust>(c);
        cpp = Measure<Original>(c);
      } else {
        cpp = Measure<Original>(c);
        rust = Measure<Rust>(c);
      }
      // SAFETY: case keys are static NUL-terminated literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"iterations\":%zu,\"repeat\":%u,\"rust_first\":%s,"
          "\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          c.key, c.iterations, repeat, first ? "true" : "false", cpp, rust));
      std::fflush(stdout);
    }
  }
  if (option == "--verify-only") {
    std::puts(
        "PASS: 15 frozen public-API paired workloads, checksums match; clocks "
        "not read");
  }
}
