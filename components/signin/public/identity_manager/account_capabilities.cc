// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/public/identity_manager/account_capabilities.h"

#include <array>
#include <string>
#include <vector>

#include "base/containers/heap_array.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "build/build_config.h"
#include "components/signin/internal/identity_manager/account_capabilities_constants.h"
#include "components/signin/public/identity_manager/tribool.h"


AccountCapabilities::AccountCapabilities() = default;
AccountCapabilities::~AccountCapabilities() = default;
AccountCapabilities::AccountCapabilities(const AccountCapabilities& other) =
    default;
AccountCapabilities::AccountCapabilities(AccountCapabilities&& other) noexcept =
    default;
AccountCapabilities& AccountCapabilities::operator=(
    const AccountCapabilities& other) = default;
AccountCapabilities& AccountCapabilities::operator=(
    AccountCapabilities&& other) noexcept = default;

// static
base::span<const std::string_view>
AccountCapabilities::GetSupportedAccountCapabilityNames() {
  static constexpr auto kSupportedAccountCapabilityNames =
      std::to_array<std::string_view>({
#define ACCOUNT_CAPABILITY(cpp_label, java_label, value) cpp_label,
#include "components/signin/internal/identity_manager/account_capabilities_list.h"
#undef ACCOUNT_CAPABILITY
      });
  return kSupportedAccountCapabilityNames;
}

// static
std::string AccountCapabilities::GetCapabilityDisplayName(
    std::string_view name) {
#define ACCOUNT_CAPABILITY(cpp_label, java_label, value) \
  if (name == value) {                                   \
    return #cpp_label;                                   \
  }
#include "components/signin/internal/identity_manager/account_capabilities_list.h"
#undef ACCOUNT_CAPABILITY
  NOTREACHED() << "Unknown capability: " << name;
}

bool AccountCapabilities::AreAnyCapabilitiesKnown() const {
  for (std::string_view capability_name :
       GetSupportedAccountCapabilityNames()) {
    if (GetCapabilityByName(capability_name) != signin::Tribool::kUnknown) {
      return true;
    }
  }
  return false;
}

bool AccountCapabilities::AreAllCapabilitiesKnown() const {
  for (std::string_view capability_name :
       GetSupportedAccountCapabilityNames()) {
    if (GetCapabilityByName(capability_name) == signin::Tribool::kUnknown) {
      return false;
    }
  }
  return true;
}

signin::Tribool AccountCapabilities::GetCapabilityByName(
    std::string_view name) const {
  if (auto it = capabilities_overrides_.find(name);
      it != capabilities_overrides_.end()) {
    return it->second;
  }
  return GetFetchedCapabilityByName(name);
}

signin::Tribool AccountCapabilities::GetFetchedCapabilityByName(
    std::string_view name) const {
  const auto iterator = capabilities_map_.find(name);
  if (iterator == capabilities_map_.end()) {
    return signin::Tribool::kUnknown;
  }
  return iterator->second ? signin::Tribool::kTrue : signin::Tribool::kFalse;
}

const base::flat_map<std::string, signin::Tribool>&
AccountCapabilities::GetCapabilityOverrides() const {
  return capabilities_overrides_;
}

// clang-format off
// keep-sorted start newline_separated=yes sticky_prefixes=#if,BUILDFLAG group_prefixes=AccountCapabilities,#endif block=yes
// clang-format on
signin::Tribool AccountCapabilities::can_fetch_family_member_info() const {
  return GetCapabilityByName(kCanFetchFamilyMemberInfoCapabilityName);
}

signin::Tribool AccountCapabilities::can_have_email_address_displayed() const {
  return GetCapabilityByName(kCanHaveEmailAddressDisplayedCapabilityName);
}

signin::Tribool
AccountCapabilities::can_make_chrome_search_engine_choice_screen_choice()
    const {
  return GetCapabilityByName(kCanMakeChromeSearchEngineChoiceScreenChoice);
}

signin::Tribool
AccountCapabilities::can_override_account_info() const {
  return GetCapabilityByName(kCanOverrideAccountInfoCapabilityName);
}

signin::Tribool AccountCapabilities::
    can_show_history_sync_opt_ins_without_minor_mode_restrictions() const {
  return GetCapabilityByName(
      kCanShowHistorySyncOptInsWithoutMinorModeRestrictionsCapabilityName);
}


#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
signin::Tribool AccountCapabilities::can_submit_feedback() const {
  return GetCapabilityByName(kCanSubmitFeedbackInChromeCapabilityName);
}
#endif



signin::Tribool AccountCapabilities::can_use_devtools_generative_ai_features()
    const {
  return GetCapabilityByName(kCanUseDevToolsGenerativeAiFeaturesCapabilityName);
}

signin::Tribool AccountCapabilities::can_use_edu_features() const {
  return GetCapabilityByName(kCanUseEduFeaturesCapabilityName);
}

signin::Tribool AccountCapabilities::can_use_gemini_in_chrome() const {
  return GetCapabilityByName(kCanUseGeminiInChromeCapabilityName);
}



signin::Tribool AccountCapabilities::can_use_manta_service() const {
  return GetCapabilityByName(kCanUseMantaServiceName);
}

signin::Tribool AccountCapabilities::can_use_model_execution_features() const {
  return GetCapabilityByName(kCanUseModelExecutionFeaturesName);
}

signin::Tribool AccountCapabilities::can_use_speaker_label_in_recorder_app()
    const {
  return GetCapabilityByName(kCanUseSpeakerLabelInRecorderApp);
}

signin::Tribool AccountCapabilities::is_allowed_for_machine_learning() const {
  return GetCapabilityByName(kIsAllowedForMachineLearningCapabilityName);
}

signin::Tribool AccountCapabilities::is_opted_in_to_parental_supervision()
    const {
  return GetCapabilityByName(kIsOptedInToParentalSupervisionCapabilityName);
}

signin::Tribool
AccountCapabilities::is_subject_to_account_level_enterprise_policies() const {
  return GetCapabilityByName(
      kIsSubjectToAccountLevelEnterprisePoliciesCapabilityName);
}

signin::Tribool AccountCapabilities::is_subject_to_enterprise_features() const {
  return GetCapabilityByName(kIsSubjectToEnterprisePoliciesCapabilityName);
}

signin::Tribool AccountCapabilities::is_subject_to_parental_controls() const {
  return GetCapabilityByName(kIsSubjectToParentalControlsCapabilityName);
}

signin::Tribool
AccountCapabilities::is_subject_to_parental_controls_via_bundle() const {
  return GetCapabilityByName(
      kIsSubjectToParentalControlsViaBundleCapabilityName);
}

signin::Tribool AccountCapabilities::is_subject_to_universal_opt_out() const {
  return GetCapabilityByName(kIsSubjectToUniversalOptOutCapabilityName);
}


signin::Tribool AccountCapabilities::
    must_show_equally_weighted_buttons_for_private_passes_in_chrome_autofill()
        const {
  return GetCapabilityByName(
      kMustShowEquallyWeightedButtonsForPrivatePassesInChromeAutofillCapabilityName);
}


signin::Tribool
AccountCapabilities::should_be_addressed_in_feminine_grammatical_gender()
    const {
  return GetCapabilityByName(kShouldBeAddressedInFeminineGrammaticalGender);
}

signin::Tribool
AccountCapabilities::should_be_addressed_in_masculine_grammatical_gender()
    const {
  return GetCapabilityByName(kShouldBeAddressedInMasculineGrammaticalGender);
}

signin::Tribool
AccountCapabilities::should_be_addressed_in_neuter_grammatical_gender() const {
  return GetCapabilityByName(kShouldBeAddressedInNeuterGrammaticalGender);
}

signin::Tribool
AccountCapabilities::supports_wallet_private_passes_in_autofill() const {
  return GetCapabilityByName(
      kSupportsWalletPrivatePassesInAutofillCapabilityName);
}

// keep-sorted end

bool AccountCapabilities::UpdateWith(const AccountCapabilities& other) {
  bool modified = false;

  for (std::string_view name : GetSupportedAccountCapabilityNames()) {
    signin::Tribool other_capability = other.GetFetchedCapabilityByName(name);
    signin::Tribool current_capability = GetFetchedCapabilityByName(name);
    if (other_capability != signin::Tribool::kUnknown &&
        other_capability != current_capability) {
      capabilities_map_[std::string(name)] =
          other_capability == signin::Tribool::kTrue;
      modified = true;
    }
  }

  for (const auto& [name, value] : other.capabilities_overrides_) {
    auto iterator = capabilities_overrides_.find(name);
    if (iterator == capabilities_overrides_.end() ||
        iterator->second != value) {
      capabilities_overrides_[name] = value;
      modified = true;
    }
  }

  return modified;
}

void AccountCapabilities::SetCapabilityOverride(
    std::string_view name,
    std::optional<signin::Tribool> value) {
  if (value.has_value()) {
    capabilities_overrides_[std::string(name)] = *value;
  } else {
    capabilities_overrides_.erase(std::string(name));
  }
}

bool AccountCapabilities::operator==(const AccountCapabilities& other) const {
  for (std::string_view name : GetSupportedAccountCapabilityNames()) {
    if (GetFetchedCapabilityByName(name) != other.GetFetchedCapabilityByName(name)) {
      return false;
    }
  }
  return capabilities_overrides_ == other.capabilities_overrides_;
}


AccountCapabilities::AccountCapabilities(
    base::flat_map<std::string, bool> capabilities)
    : capabilities_map_(std::move(capabilities)) {}
