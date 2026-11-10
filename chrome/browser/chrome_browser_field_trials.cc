// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_browser_field_trials.h"

#include <optional>
#include <string>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/features.h"
#include "base/functional/callback_helpers.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/metrics/chrome_browser_sampling_trials.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/metrics/chrome_metrics_service_client.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "components/feed/feed_feature_list.h"
#include "components/metrics/metrics_features.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/persistent_histograms.h"
#include "components/password_manager/core/browser/features/password_features.h"
#include "components/site_isolation/features.h"
#include "components/variations/feature_overrides.h"
#include "components/version_info/version_info.h"
#include "third_party/blink/public/common/features.h"



#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/signin/first_run_desktop_refresh_field_trial.h"
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_POSIX)
#include "base/check_deref.h"
#include "chrome/browser/metrics/client_id_backup_correctness_fix_field_trial.h"
#endif  // BUILDFLAG(IS_POSIX)

ChromeBrowserFieldTrials::ChromeBrowserFieldTrials(PrefService* local_state)
    : local_state_(local_state) {
  CHECK(local_state_, base::NotFatalUntil::M162);
}

ChromeBrowserFieldTrials::~ChromeBrowserFieldTrials() = default;

void ChromeBrowserFieldTrials::SetUpClientSideFieldTrials(
    bool has_seed,
    const variations::EntropyProviders& entropy_providers,
    base::FeatureList* feature_list) {
  // Only create the fallback trials if there isn't already a variations seed
  // being applied. This should occur during first run when first-run variations
  // isn't supported. It's assumed that, if there is a seed, then it either
  // contains the relevant studies, or is intentionally omitted, so no fallback
  // is needed. The exception is for sampling trials. Fallback trials are
  // created even if no variations seed was applied. This allows testing the
  // fallback code by intentionally omitting the sampling trial from a
  // variations seed.
  metrics::CreateFallbackSamplingTrialsIfNeeded(
      entropy_providers.default_entropy(), feature_list);
  metrics::CreateFallbackUkmSamplingTrialIfNeeded(
      entropy_providers.default_entropy(), feature_list);


#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  // This trial is client controlled on Mac and Linux because the first run
  // experience is shown on the very first run of Chrome. These platforms do not
  // support variations seed on the first run.
  if (first_run::IsChromeFirstRun()) {
    signin::CreateFirstRunDesktopRefreshFieldTrial(
        CHECK_DEREF(feature_list), entropy_providers.default_entropy());
  }
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_POSIX)
  metrics::CreateClientIdBackupCorrectnessFixFieldTrial(
      CHECK_DEREF(feature_list), entropy_providers.default_entropy());
#endif  // BUILDFLAG(IS_POSIX)
}

void ChromeBrowserFieldTrials::RegisterSyntheticTrials() {
}

void ChromeBrowserFieldTrials::RegisterFeatureOverrides(
    base::FeatureList* feature_list) {
  variations::FeatureOverrides feature_overrides(*feature_list);

}

void ChromeBrowserFieldTrials::EnableRuntimeMutableFeatures(
    base::FeatureList* feature_list) {
  variations::PlatformFieldTrials::EnableRuntimeMutableFeatures(feature_list);

  // Add calls to enable runtime-mutable features here.
  // Defined on desktop only (see password_features.h).
  feature_list->EnableRuntimeMutability(
      password_manager::features::kPasskeyUnlockPromo, base::DoNothing());
}
