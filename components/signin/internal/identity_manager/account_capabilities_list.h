// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file intentionally does not have header guards, it's included
// inside a macro to generate a list of constants. The following line silences a
// presubmit and Tricium warning that would otherwise be triggered by this:
// no-include-guard-because-multiply-included
// NOLINT(build/header_guard)

// This is the list of account capabilities identifiers and their values. For
// the constant declarations, include the file
// "account_capabilities_constants.h".

// Here we define the values using a macro ACCOUNT_CAPABILITY, so it can be
// expanded differently in some places. The macro has the following signature:
// ACCOUNT_CAPABILITY(cpp_label, java_label, name).

// Here we rely on build_config.h to not introduce any symbols that can be
// textually inserted in the code. If that changes in the future, this header
// should be removed from this file and included transitively instead.
#include "build/build_config.h"

// The server-side documentation and definition for a given capability can be
// found at go/capability-alias (eg. go/capability-alias/ge2dinbnmnqxa).

// clang-format off
// keep-sorted start newline_separated=yes sticky_prefixes=#if,BUILDFLAG group_prefixes=#endif
// clang-format on



ACCOUNT_CAPABILITY(kCanFetchFamilyMemberInfoCapabilityName,
                   CAN_FETCH_FAMILY_MEMBER_INFO_CAPABILITY_NAME,
                   "accountcapabilities/ge2dinbnmnqxa")

ACCOUNT_CAPABILITY(kCanHaveEmailAddressDisplayedCapabilityName,
                   CAN_HAVE_EMAIL_ADDRESS_DISPLAYED_CAPABILITY_NAME,
                   "accountcapabilities/haytqlldmfya")

ACCOUNT_CAPABILITY(kCanMakeChromeSearchEngineChoiceScreenChoice,
                   CAN_MAKE_CHROME_SEARCH_ENGINE_CHOICE_SCREEN_CHOICE,
                   "accountcapabilities/ge4tenznmnqxa")

ACCOUNT_CAPABILITY(kCanOverrideAccountInfoCapabilityName,
                   CAN_OVERRIDE_ACCOUNT_INFO_CAPABILITY_NAME,
                   "accountcapabilities/gmydknbnmnqxa")

ACCOUNT_CAPABILITY(
    kCanShowHistorySyncOptInsWithoutMinorModeRestrictionsCapabilityName,
    CAN_SHOW_HISTORY_SYNC_OPT_INS_WITHOUT_MINOR_MODE_RESTRICTIONS_CAPABILITY_NAME,
    "accountcapabilities/gi2tklldmfya")


#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
ACCOUNT_CAPABILITY(kCanSubmitFeedbackInChromeCapabilityName,
                   CAN_SUBMIT_FEEDBACK_IN_CHROME_CAPABILITY_NAME,
                   "accountcapabilities/gizdqmrnmnqxa")
#endif



ACCOUNT_CAPABILITY(kCanUseDevToolsGenerativeAiFeaturesCapabilityName,
                   CAN_USE_DEVTOOLS_GENERATIVE_AI_FEATURES_CAPABILITY_NAME,
                   "accountcapabilities/geztenjnmnqxa")

ACCOUNT_CAPABILITY(kCanUseEduFeaturesCapabilityName,
                   CAN_USE_EDU_FEATURES_CAPABILITY_NAME,
                   "accountcapabilities/gezdsmbnmnqxa")

ACCOUNT_CAPABILITY(kCanUseGeminiInChromeCapabilityName,
                   CAN_USE_GEMINI_IN_CHROME_CAPABILITY_NAME,
                   "accountcapabilities/giytmnrnmnqxa")



ACCOUNT_CAPABILITY(kCanUseMantaServiceName,
                   CAN_USE_MANTA_SERVICE_NAME,
                   "accountcapabilities/geytcnbnmnqxa")

ACCOUNT_CAPABILITY(kCanUseModelExecutionFeaturesName,
                   CAN_USE_MODEL_EXECUTION_FEATURES_NAME,
                   "accountcapabilities/gezdcnbnmnqxa")

ACCOUNT_CAPABILITY(kCanUseSpeakerLabelInRecorderApp,
                   CAN_USE_SPEAKER_LABEL_IN_RECORDER_APP,
                   "accountcapabilities/ge2tknznmnqxa")

ACCOUNT_CAPABILITY(kIsAllowedForMachineLearningCapabilityName,
                   IS_ALLOWED_FOR_MACHINE_LEARNING_CAPABILITY_NAME,
                   "accountcapabilities/g42tslldmfya")

ACCOUNT_CAPABILITY(kIsOptedInToParentalSupervisionCapabilityName,
                   IS_OPTED_IN_TO_PARENTAL_SUPERVISION_CAPABILITY_NAME,
                   "accountcapabilities/guzdslldmfya")

ACCOUNT_CAPABILITY(
    kIsSubjectToAccountLevelEnterprisePoliciesCapabilityName,
    IS_SUBJECT_TO_ACCOUNT_LEVEL_ENTERPRISE_POLICIES_CAPABILITY_NAME,
    "accountcapabilities/ge4tgnznmnqxa")

ACCOUNT_CAPABILITY(kIsSubjectToEnterprisePoliciesCapabilityName,
                   IS_SUBJECT_TO_ENTERPRISE_POLICIES_CAPABILITY_NAME,
                   "accountcapabilities/g44tilldmfya")

ACCOUNT_CAPABILITY(kIsSubjectToParentalControlsCapabilityName,
                   IS_SUBJECT_TO_PARENTAL_CONTROLS_CAPABILITY_NAME,
                   "accountcapabilities/guydolldmfya")

ACCOUNT_CAPABILITY(kIsSubjectToParentalControlsViaBundleCapabilityName,
                   IS_SUBJECT_TO_PARENTAL_CONTROLS_VIA_BUNDLE_CAPABILITY_NAME,
                   "accountcapabilities/gm2dgmjnmnqxa")

ACCOUNT_CAPABILITY(kIsSubjectToUniversalOptOutCapabilityName,
                   IS_SUBJECT_TO_UNIVERSAL_OPT_OUT_CAPABILITY_NAME,
                   "accountcapabilities/gmydemrnmnqxa")


ACCOUNT_CAPABILITY(
    kMustShowEquallyWeightedButtonsForPrivatePassesInChromeAutofillCapabilityName,
    MUST_SHOW_EQUALLY_WEIGHTED_BUTTONS_FOR_PRIVATE_PASSES_IN_CHROME_AUTOFILL_CAPABILITY_NAME,
    "accountcapabilities/gm3denjnmnqxa")


ACCOUNT_CAPABILITY(kShouldBeAddressedInFeminineGrammaticalGender,
                   SHOULD_BE_ADDRESSED_IN_FEMININE_GRAMMATICAL_GENDER,
                   "accountcapabilities/ge3dsnbnmnqxa")

ACCOUNT_CAPABILITY(kShouldBeAddressedInMasculineGrammaticalGender,
                   SHOULD_BE_ADDRESSED_IN_MASCULINE_GRAMMATICAL_GENDER,
                   "accountcapabilities/ge3dsnjnmnqxa")

ACCOUNT_CAPABILITY(kShouldBeAddressedInNeuterGrammaticalGender,
                   SHOULD_BE_ADDRESSED_IN_NEUTER_GRAMMATICAL_GENDER,
                   "accountcapabilities/ge3dsnrnmnqxa")

ACCOUNT_CAPABILITY(
    kSupportsWalletPrivatePassesInAutofillCapabilityName,
    SUPPORTS_WALLET_PRIVATE_PASSES_IN_AUTOFILL_NAME,
    "accountcapabilities/gi3dknrnmnqxa"
)

// keep-sorted end
