// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/cookies/cookie_inclusion_status.h"

#include <array>

#include "net/cookies/cookie_inclusion_status_rust_ffi.h"

namespace net {
namespace {
using Status = CookieInclusionStatus;
using namespace cookie_inclusion_status_rust;
CookieInclusionStatusRustState Snapshot(const Status& s) {
  return {s.exclusion_reasons().ToEnumBitmask(),
          s.warning_reasons().ToEnumBitmask(),
          static_cast<int32_t>(s.exemption_reason()), 0};
}
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_UNKNOWN_ERROR) == 0);
static_assert(static_cast<int>(Status::ExclusionReason::EXCLUDE_HTTP_ONLY) ==
              1);
static_assert(static_cast<int>(Status::ExclusionReason::EXCLUDE_SECURE_ONLY) ==
              2);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_DOMAIN_MISMATCH) == 3);
static_assert(static_cast<int>(Status::ExclusionReason::EXCLUDE_NOT_ON_PATH) ==
              4);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_SAMESITE_STRICT) == 5);
static_assert(static_cast<int>(Status::ExclusionReason::EXCLUDE_SAMESITE_LAX) ==
              6);
static_assert(
    static_cast<int>(
        Status::ExclusionReason::EXCLUDE_SAMESITE_UNSPECIFIED_TREATED_AS_LAX) ==
    7);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_SAMESITE_NONE_INSECURE) ==
    8);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_USER_PREFERENCES) == 9);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_FAILURE_TO_STORE) == 10);
static_assert(static_cast<int>(
                  Status::ExclusionReason::EXCLUDE_NONCOOKIEABLE_SCHEME) == 11);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_OVERWRITE_SECURE) == 12);
static_assert(static_cast<int>(
                  Status::ExclusionReason::EXCLUDE_OVERWRITE_HTTP_ONLY) == 13);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_INVALID_DOMAIN) == 14);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_INVALID_PREFIX) == 15);
static_assert(static_cast<int>(
                  Status::ExclusionReason::EXCLUDE_INVALID_PARTITIONED) == 16);
static_assert(
    static_cast<int>(
        Status::ExclusionReason::EXCLUDE_NAME_VALUE_PAIR_EXCEEDS_MAX_SIZE) ==
    17);
static_assert(
    static_cast<int>(
        Status::ExclusionReason::EXCLUDE_ATTRIBUTE_VALUE_EXCEEDS_MAX_SIZE) ==
    18);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_DOMAIN_NON_ASCII) == 19);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_PORT_MISMATCH) == 20);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_SCHEME_MISMATCH) == 21);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_SHADOWING_DOMAIN) == 22);
static_assert(static_cast<int>(
                  Status::ExclusionReason::EXCLUDE_DISALLOWED_CHARACTER) == 23);
static_assert(static_cast<int>(
                  Status::ExclusionReason::EXCLUDE_THIRD_PARTY_PHASEOUT) == 24);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_NO_COOKIE_CONTENT) == 25);
static_assert(
    static_cast<int>(Status::ExclusionReason::EXCLUDE_ANONYMOUS_CONTEXT) == 26);
static_assert(static_cast<int>(Status::ExclusionReason::EXCLUDE_INVALID_PATH) ==
              27);
static_assert(static_cast<int>(
                  Status::ExclusionReason::EXCLUDE_AMBIGUOUS_SERIALIZATION) ==
              28);
static_assert(static_cast<int>(Status::ExclusionReason::EXCLUDE_INVALID_HOST) ==
              29);
static_assert(
    static_cast<int>(
        Status::WarningReason::WARN_SAMESITE_UNSPECIFIED_CROSS_SITE_CONTEXT) ==
    0);
static_assert(
    static_cast<int>(Status::WarningReason::WARN_SAMESITE_NONE_INSECURE) == 1);
static_assert(
    static_cast<int>(
        Status::WarningReason::WARN_SAMESITE_UNSPECIFIED_LAX_ALLOW_UNSAFE) ==
    2);
static_assert(
    static_cast<int>(
        Status::WarningReason::WARN_SECURE_ACCESS_GRANTED_NON_CRYPTOGRAPHIC) ==
    3);
static_assert(static_cast<int>(
                  Status::WarningReason::
                      WARN_CROSS_SITE_REDIRECT_DOWNGRADE_CHANGES_INCLUSION) ==
              4);
static_assert(
    static_cast<int>(
        Status::WarningReason::
            WARN_CROSS_SITE_REDIRECT_DOWNGRADE_CHANGES_INCLUSION_NO_INITIATOR) ==
    5);
static_assert(
    static_cast<int>(
        Status::WarningReason::WARN_ATTRIBUTE_VALUE_EXCEEDS_MAX_SIZE) == 6);
static_assert(static_cast<int>(Status::WarningReason::WARN_DOMAIN_NON_ASCII) ==
              7);
static_assert(static_cast<int>(Status::WarningReason::WARN_PORT_MISMATCH) == 8);
static_assert(static_cast<int>(Status::WarningReason::WARN_SCHEME_MISMATCH) ==
              9);
static_assert(
    static_cast<int>(Status::WarningReason::
                         WARN_TENTATIVELY_ALLOWING_SECURE_SOURCE_SCHEME) == 10);
static_assert(static_cast<int>(Status::WarningReason::WARN_SHADOWING_DOMAIN) ==
              11);
static_assert(
    static_cast<int>(Status::WarningReason::WARN_THIRD_PARTY_PHASEOUT) == 12);
static_assert(static_cast<int>(Status::WarningReason::WARN_INVALID_HOST) == 13);
static_assert(static_cast<int>(Status::ExemptionReason::kNone) == 0);
static_assert(static_cast<int>(Status::ExemptionReason::kUserSetting) == 1);
static_assert(static_cast<int>(Status::ExemptionReason::kEnterprisePolicy) ==
              2);
static_assert(static_cast<int>(Status::ExemptionReason::kStorageAccess) == 3);
static_assert(static_cast<int>(Status::ExemptionReason::kScheme) == 4);
static_assert(static_cast<int>(
                  Status::ExemptionReason::kSameSiteNoneCookiesInSandbox) == 5);
static_assert(Status::ExclusionReasonBitset::kValueCount == 30);
static_assert(Status::WarningReasonBitset::kValueCount == 14);
}  // namespace
CookieInclusionStatus::CookieInclusionStatus() = default;
CookieInclusionStatus::CookieInclusionStatus(
    const CookieInclusionStatus& other) = default;
CookieInclusionStatus& CookieInclusionStatus::operator=(
    const CookieInclusionStatus& other) = default;
bool CookieInclusionStatus::operator==(
    const CookieInclusionStatus& other) const {
  return cookie_inclusion_status_rust_same(Snapshot(*this), Snapshot(other));
}
void CookieInclusionStatus::ApplyRustTransition(uint32_t operation,
                                                int32_t reason,
                                                uint64_t mask) {
  const auto result = cookie_inclusion_status_rust_apply(
      Snapshot(*this), operation, reason, mask);
  // Rust validates the enum; C++ retains the original fatal CHECK boundary.
  CHECK_EQ(result.invalid, 0u)
      << "Invalid CookieInclusionStatus reason " << reason;
  exclusion_reasons_ =
      ExclusionReasonBitset::FromEnumBitmask(result.exclusions);
  warning_reasons_ = WarningReasonBitset::FromEnumBitmask(result.warnings);
  exemption_reason_ = static_cast<ExemptionReason>(result.exemption);
}
bool CookieInclusionStatus::IsInclude() const {
  return cookie_inclusion_status_rust_query(Snapshot(*this), kInclude, 0, 0);
}
bool CookieInclusionStatus::HasExclusionReason(ExclusionReason reason) const {
  return cookie_inclusion_status_rust_query(Snapshot(*this), kHasExclusion,
                                            static_cast<int32_t>(reason), 0);
}
bool CookieInclusionStatus::HasOnlyExclusionReason(
    ExclusionReason reason) const {
  return cookie_inclusion_status_rust_query(Snapshot(*this), kOnlyExclusion,
                                            static_cast<int32_t>(reason), 0);
}
bool CookieInclusionStatus::ShouldRecordDowngradeMetrics() const {
  return cookie_inclusion_status_rust_query(Snapshot(*this), kRecordDowngrade,
                                            0, 0);
}
bool CookieInclusionStatus::ShouldWarn() const {
  return cookie_inclusion_status_rust_query(Snapshot(*this), kWarn, 0, 0);
}
bool CookieInclusionStatus::HasWarningReason(WarningReason reason) const {
  return cookie_inclusion_status_rust_query(Snapshot(*this), kHasWarning,
                                            static_cast<int32_t>(reason), 0);
}
bool CookieInclusionStatus::ExcludedByUserPreferencesOrTPCD() const {
  return cookie_inclusion_status_rust_query(Snapshot(*this),
                                            kUserPreferencesOrTPCD, 0, 0);
}
void CookieInclusionStatus::AddExclusionReason(ExclusionReason reason) {
  ApplyRustTransition(kAddExclusion, static_cast<int32_t>(reason), 0);
}
void CookieInclusionStatus::RemoveExclusionReason(ExclusionReason reason) {
  ApplyRustTransition(kRemoveExclusion, static_cast<int32_t>(reason), 0);
}
void CookieInclusionStatus::RemoveExclusionReasons(
    ExclusionReasonBitset reasons) {
  ApplyRustTransition(kRemoveExclusions, 0, reasons.ToEnumBitmask());
}
void CookieInclusionStatus::MaybeSetExemptionReason(ExemptionReason reason) {
  ApplyRustTransition(kSetExemption, static_cast<int32_t>(reason), 0);
}
void CookieInclusionStatus::MaybeClearSameSiteWarning() {
  ApplyRustTransition(kClearSameSite, 0, 0);
}
void CookieInclusionStatus::MaybeClearThirdPartyPhaseoutReason() {
  ApplyRustTransition(kClearThirdParty, 0, 0);
}
void CookieInclusionStatus::AddWarningReason(WarningReason reason) {
  ApplyRustTransition(kAddWarning, static_cast<int32_t>(reason), 0);
}
void CookieInclusionStatus::RemoveWarningReason(WarningReason reason) {
  ApplyRustTransition(kRemoveWarning, static_cast<int32_t>(reason), 0);
}
void CookieInclusionStatus::ResetForTesting() {
  ApplyRustTransition(kReset, 0, 0);
}
CookieInclusionStatus::ExclusionReasonBitset
CookieInclusionStatus::ExclusionReasonsWithout(
    ExclusionReasonBitset reasons) const {
  const auto result = cookie_inclusion_status_rust_apply(
      Snapshot(*this), kWithoutExclusions, 0, reasons.ToEnumBitmask());
  return ExclusionReasonBitset::FromEnumBitmask(result.exclusions);
}
std::string CookieInclusionStatus::GetDebugString() const {
  // Upper bound of all 44 names/separators plus exemption is below 4096.
  std::array<uint8_t, 4096> output{};
  const size_t length = cookie_inclusion_status_rust_debug(
      Snapshot(*this), output.data(), output.size());
  CHECK_LE(length, output.size());
  return std::string(reinterpret_cast<const char*>(output.data()), length);
}
bool CookieInclusionStatus::HasExactlyExclusionReasonsForTesting(
    ExclusionReasonBitset reasons) const {
  const auto expected = MakeFromReasonsForTesting(reasons);
  return cookie_inclusion_status_rust_query(
      Snapshot(*this), kExactExclusions, 0,
      expected.exclusion_reasons().ToEnumBitmask());
}
bool CookieInclusionStatus::HasExactlyWarningReasonsForTesting(
    WarningReasonBitset reasons) const {
  const auto expected = MakeFromReasonsForTesting({}, reasons);
  return cookie_inclusion_status_rust_query(
      Snapshot(*this), kExactWarnings, 0,
      expected.warning_reasons().ToEnumBitmask());
}
CookieInclusionStatus CookieInclusionStatus::MakeFromReasonsForTesting(
    ExclusionReasonBitset exclusions,
    WarningReasonBitset warnings,
    ExemptionReason exemption) {
  const auto result = cookie_inclusion_status_rust_components(
      exclusions.ToEnumBitmask(), warnings.ToEnumBitmask(),
      static_cast<int32_t>(exemption));
  CookieInclusionStatus status;
  status.exclusion_reasons_ =
      ExclusionReasonBitset::FromEnumBitmask(result.exclusions);
  status.warning_reasons_ =
      WarningReasonBitset::FromEnumBitmask(result.warnings);
  status.exemption_reason_ = static_cast<ExemptionReason>(result.exemption);
  for (auto reason : exclusions) {
    CHECK(status.HasExclusionReason(reason))
        << "Exemption " << static_cast<int>(reason) << " could not be applied";
  }
  CHECK_EQ(status.exclusion_reasons_.size(), exclusions.size());
  for (auto reason : warnings) {
    CHECK(status.HasWarningReason(reason))
        << "Warning " << static_cast<int>(reason) << " could not be applied";
  }
  CHECK_EQ(status.warning_reasons_.size(), warnings.size());
  CHECK_EQ(status.exemption_reason(), exemption)
      << "Exemption " << static_cast<int>(exemption) << " could not be applied";

  return status;
}
std::optional<CookieInclusionStatus> CookieInclusionStatus::MakeFromComponents(
    ExclusionReasonBitset exclusions,
    WarningReasonBitset warnings,
    ExemptionReason exemption) {
  const auto result = cookie_inclusion_status_rust_components(
      exclusions.ToEnumBitmask(), warnings.ToEnumBitmask(),
      static_cast<int32_t>(exemption));
  if (result.invalid) {
    return std::nullopt;
  }
  CookieInclusionStatus status;
  status.exclusion_reasons_ =
      ExclusionReasonBitset::FromEnumBitmask(result.exclusions);
  status.warning_reasons_ =
      WarningReasonBitset::FromEnumBitmask(result.warnings);
  status.exemption_reason_ = static_cast<ExemptionReason>(result.exemption);
  return status;
}
}  // namespace net
