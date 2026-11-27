// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_INCLUSION_STATUS_RUST_TEST_SUPPORT_H_
#define NET_COOKIES_COOKIE_INCLUSION_STATUS_RUST_TEST_SUPPORT_H_
#include <cstdint>
#include <limits>
#include <sstream>
#include <type_traits>

#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_inclusion_status_rust_reference.h"
namespace net::cookie_inclusion_status_rust_test {
using Rust = CookieInclusionStatus;
using Original = CookieInclusionStatusRustOriginal;
static_assert(sizeof(Rust) == sizeof(Original));
static_assert(alignof(Rust) == alignof(Original));
static_assert(sizeof(Rust::ExclusionReasonBitset) ==
              sizeof(Original::ExclusionReasonBitset));
static_assert(sizeof(Rust::WarningReasonBitset) ==
              sizeof(Original::WarningReasonBitset));
static_assert(std::is_standard_layout_v<Rust> &&
              std::is_standard_layout_v<Original>);
inline uint64_t Random(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
template <typename T>
auto Components(uint64_t ex, uint64_t warn, int exemption) {
  return T::MakeFromComponents(
      T::ExclusionReasonBitset::FromEnumBitmask(ex),
      T::WarningReasonBitset::FromEnumBitmask(warn),
      static_cast<typename T::ExemptionReason>(exemption));
}
template <typename T>
void Step(T& s, unsigned op, int reason, uint64_t mask) {
  using E = typename T::ExclusionReason;
  using W = typename T::WarningReason;
  switch (op % 13) {
    case 0:
      s.AddExclusionReason(static_cast<E>(static_cast<unsigned>(reason) % 30));
      break;
    case 1:
      s.RemoveExclusionReason(static_cast<E>(reason));
      break;
    case 2:
      s.RemoveExclusionReasons(T::ExclusionReasonBitset::FromEnumBitmask(mask));
      break;
    case 3:
      s.MaybeSetExemptionReason(
          static_cast<typename T::ExemptionReason>(reason));
      break;
    case 4:
      s.MaybeClearSameSiteWarning();
      break;
    case 5:
      s.AddWarningReason(static_cast<W>(static_cast<unsigned>(reason) % 14));
      break;
    case 6:
      s.RemoveWarningReason(static_cast<W>(reason));
      break;
    case 7:
      s.ResetForTesting();
      break;
    case 8: {
      auto copy(s);
      s = copy;
      const T* alias = &s;
      s = *alias;
      break;
    }
    case 9: {
      auto rebuilt = Components<T>(mask, mask >> 16, reason);
      if (rebuilt) {
        s = *rebuilt;
      }
      break;
    }
    case 10:
      s.AddExclusionReason(static_cast<E>(24));
      break;
    case 11:
      s.AddWarningReason(static_cast<W>(12));
      break;
    case 12:
      s.RemoveExclusionReason(
          static_cast<E>(static_cast<unsigned>(reason) % 30));
      break;
  }
}
inline bool Equal(const Rust& a, const Original& b, bool strings = true) {
  if (a.exclusion_reasons().ToEnumBitmask() !=
          b.exclusion_reasons().ToEnumBitmask() ||
      a.warning_reasons().ToEnumBitmask() !=
          b.warning_reasons().ToEnumBitmask() ||
      static_cast<int>(a.exemption_reason()) !=
          static_cast<int>(b.exemption_reason()) ||
      a.IsInclude() != b.IsInclude() || a.ShouldWarn() != b.ShouldWarn() ||
      a.ShouldRecordDowngradeMetrics() != b.ShouldRecordDowngradeMetrics() ||
      a.ExcludedByUserPreferencesOrTPCD() !=
          b.ExcludedByUserPreferencesOrTPCD()) {
    return false;
  }
  for (int i :
       {-1, 0, 1, 5, 7, 8, 9, 24, 29, 30, 31, std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max()}) {
    if (a.HasExclusionReason(static_cast<Rust::ExclusionReason>(i)) !=
            b.HasExclusionReason(static_cast<Original::ExclusionReason>(i)) ||
        a.HasOnlyExclusionReason(static_cast<Rust::ExclusionReason>(i)) !=
            b.HasOnlyExclusionReason(
                static_cast<Original::ExclusionReason>(i)) ||
        a.HasWarningReason(static_cast<Rust::WarningReason>(i)) !=
            b.HasWarningReason(static_cast<Original::WarningReason>(i))) {
      return false;
    }
  }
  const Rust copy(a);
  const Original old_copy(b);
  if (!(copy == a) || !(old_copy == b) ||
      ((a == Rust()) != (b == Original()))) {
    return false;
  }
  // Exact-exclusion testing factory deliberately CHECKs for inconsistent 3PCD
  // combinations, so pass the already normalized live exclusion set here.
  if (!a.HasExactlyExclusionReasonsForTesting(a.exclusion_reasons()) ||
      !b.HasExactlyExclusionReasonsForTesting(b.exclusion_reasons()) ||
      !a.HasExactlyWarningReasonsForTesting(a.warning_reasons()) ||
      !b.HasExactlyWarningReasonsForTesting(b.warning_reasons())) {
    return false;
  }
  return !strings || a.GetDebugString() == b.GetDebugString();
}
inline bool DeserializeEqual(uint64_t ex, uint64_t warn, int exemption) {
  auto a = Components<Rust>(ex, warn, exemption);
  auto b = Components<Original>(ex, warn, exemption);
  return a.has_value() == b.has_value() && (!a || Equal(*a, *b));
}
}  // namespace net::cookie_inclusion_status_rust_test
#endif
