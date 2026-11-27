// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/startup_data.h"

#include <string_view>

#include "base/files/file_path.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/metrics/chrome_feature_list_creator.h"
#include "chrome/browser/prefs/profile_pref_store_manager.h"
#include "chrome/common/channel_info.h"
#include "components/metrics/cpu_metrics_provider.h"
#include "components/metrics/delegating_provider.h"
#include "components/metrics/early_metrics_guard.h"
#include "components/metrics/early_safe_metrics_provider.h"
#include "components/metrics/entropy_state_provider.h"
#include "components/metrics/field_trials_provider.h"
#include "components/metrics/install_date_provider.h"
#include "components/metrics/metrics_log.h"
#include "components/metrics/persistent_system_profile.h"
#include "components/metrics/version_utils.h"
#include "third_party/metrics_proto/system_profile.pb.h"


StartupData::StartupData() = default;

StartupData::~StartupData() = default;

// TODO(martinkong): Remove this function and replace its usage with
// ChromeFeatureListCreator::GetInstance()
ChromeFeatureListCreator* StartupData::chrome_feature_list_creator() {
  return ChromeFeatureListCreator::GetInstance();
}

void StartupData::RecordCoreSystemProfile() {
  metrics::SystemProfileProto system_profile;
  metrics::MetricsLog::RecordCoreSystemProfile(
      metrics::GetVersionString(),
      metrics::AsProtobufChannel(chrome::GetChannel()),
      chrome::IsExtendedStableChannel(),
      chrome_feature_list_creator()->actual_locale(),
      metrics::GetAppPackageName(), &system_profile);

  {
    // ========================================================================
    // EARLY STARTUP METRICS PROVIDERS
    // ========================================================================
    // Providers registered here run extremely early during browser startup
    // to populate a fallback system profile. They are running on the main
    // thread before most browser services (like the UI thread,
    // g_browser_process, or the full MetricsService) are fully initialized.
    //
    // RESTRICTIONS - To remain "Early Safe", providers MUST NOT:
    // 1. Perform blocking disk I/O (this is strictly enforced below via
    //    base::ScopedDisallowBlocking, which will crash in debug builds).
    // 2. Query late-initialized globals or UI-thread bound services.
    // 3. Emit 1-per-log lifecycle or status UMA histograms (as they will be
    //    double-counted when the provider runs again during normal startup).
    // 4. Spin up background threads or perform heavy, blocking computations.
    //
    // If your provider needs to skip certain operations during this early
    // phase, you can check
    // `metrics::EarlyMetricsGuard::IsEarlyMetricsRecordingActive()` in your
    // ProvideSystemProfileMetrics() implementation to safely early-return.
    metrics::EarlyMetricsGuard guard;
    base::ScopedDisallowBlocking disallow_blocking;

    metrics::DelegatingProvider delegating_provider;

    // Helper to enforce at compile-time that only early-safe providers
    // are registered during early startup.
    auto register_early_provider =
        [&delegating_provider](
            std::unique_ptr<metrics::EarlySafeMetricsProvider> provider) {
          delegating_provider.RegisterMetricsProvider(std::move(provider));
        };

    // TODO(hanxi): Create SyntheticTrialRegistry and pass it to
    // |field_trial_provider|.
    register_early_provider(std::make_unique<variations::FieldTrialsProvider>(
        nullptr, std::string_view()));

    // Persists low entropy source values.
    register_early_provider(std::make_unique<metrics::EntropyStateProvider>(
        chrome_feature_list_creator()->local_state()));

    // Register CPUMetricsProvider for hardware details.
    register_early_provider(std::make_unique<metrics::CPUMetricsProvider>());

    // Register InstallDateProvider.
    register_early_provider(std::make_unique<metrics::InstallDateProvider>(
        chrome_feature_list_creator()->local_state()));

    delegating_provider.ProvideSystemProfileMetricsWithLogCreationTime(
        base::TimeTicks(), &system_profile);
  }

  metrics::GlobalPersistentSystemProfile::GetInstance()->SetSystemProfile(
      system_profile, /* complete */ false);
}
