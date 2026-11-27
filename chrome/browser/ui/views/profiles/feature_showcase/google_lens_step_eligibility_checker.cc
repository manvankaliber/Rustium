// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/feature_showcase/google_lens_step_eligibility_checker.h"

#include <utility>

#include "base/functional/callback.h"
#include "chrome/browser/profiles/profile.h"
GoogleLensStepEligibilityChecker::GoogleLensStepEligibilityChecker() = default;

GoogleLensStepEligibilityChecker::~GoogleLensStepEligibilityChecker() = default;

void GoogleLensStepEligibilityChecker::CheckEligibility(
    Profile& profile,
    base::OnceCallback<void(bool)> callback) {
  {
    std::move(callback).Run(false);
    return;
  }

}

std::string GoogleLensStepEligibilityChecker::GetStepIdentifier() const {
  return std::string(kFeatureShowcaseGoogleLensStepIdentifier);
}
