// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/universal_optout/features.h"

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "build/build_config.h"


namespace universal_optout::features {

BASE_FEATURE(kUniversalOptOut, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kUniversalOptOutExtension, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kUniversalOptOutExtensionSilent,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kUniversalOptOutSettings, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kUniversalOptOutUpdateOnGeoLocationChanged,
             base::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<bool> kUniversalOptOutExtensionSilentControllerOnly{
    &kUniversalOptOutExtensionSilent, "controller-only", false};

bool IsUniversalOptOutExtensionSilentEnabled() {
  if (!base::FeatureList::IsEnabled(kUniversalOptOutExtensionSilent)) {
    return false;
  }
  if (base::FeatureList::IsEnabled(kUniversalOptOutSettings)) {
    return false;
  }
  return true;
}

bool IsUniversalOptOutEnabled() {
  return base::FeatureList::IsEnabled(kUniversalOptOut);
}

const base::FeatureParam<std::string> kTargetLocations{
    &kUniversalOptOut, "target_locations", "us-ca"};

const base::FeatureParam<bool> kForceEligible{&kUniversalOptOut,
                                              "force_eligible", false};

const base::FeatureParam<base::TimeDelta> kEligibilityWindow{
    &kUniversalOptOut, "eligibility_window", base::Days(30)};

const base::FeatureParam<base::TimeDelta> kTrailingEligibilityWindow{
    &kUniversalOptOut, "trailing_eligibility_window", base::Days(90)};

const base::FeatureParam<double> kEligibilityThresholdRatio{
    &kUniversalOptOut, "eligibility_threshold_ratio", 0.5};

const base::FeatureParam<double> kTrailingEligibilityThresholdRatio{
    &kUniversalOptOut, "trailing_eligibility_threshold_ratio", 0.5};

std::vector<std::string> GetTargetLocations() {
  return base::SplitString(kTargetLocations.Get(), ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

}  // namespace universal_optout::features
