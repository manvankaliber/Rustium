// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/chrome_metrics_services_manager_client.h"

#include <map>
#include <string>

#include "base/check_op.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/metrics/chrome_metrics_service_client.h"
#include "chrome/browser/metrics/variations/chrome_variations_service_client.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/ui/browser_otr_state.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics/startup_visibility.h"
#include "components/prefs/pref_service.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/synthetic_trial_registry.h"
#include "components/variations/variations_associated_data.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"


#if BUILDFLAG(IS_WIN)
#include "base/win/registry.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/install_static/install_util.h"
#include "components/crash/core/app/crash_export_thunks.h"
#include "components/crash/core/app/crashpad.h"
#endif  // BUILDFLAG(IS_WIN)


namespace metrics {
namespace internal {

// Metrics reporting feature. This feature, along with user consent, controls if
// recording and reporting are enabled. If the feature is enabled, but no
// consent is given, then there will be no recording or reporting.
BASE_FEATURE(kMetricsReportingFeature,
             "MetricsReporting",
             base::FEATURE_ENABLED_BY_DEFAULT);


// Name of the variations param that defines the sampling rate.
const char kRateParamName[] = "sampling_rate_per_mille";

}  // namespace internal
}  // namespace metrics

namespace {

// Posts |GoogleUpdateSettings::StoreMetricsClientInfo| on blocking pool thread
// because it needs access to IO and cannot work from UI thread.
void PostStoreMetricsClientInfo(const metrics::ClientInfo& client_info) {
  // This must happen on the same sequence as the tasks to enable/disable
  // metrics reporting. Otherwise, this may run while disabling metrics
  // reporting if the user quickly enables and disables metrics reporting.
  GoogleUpdateSettings::CollectStatsConsentTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&GoogleUpdateSettings::StoreMetricsClientInfo,
                                client_info));
}


// Implementation of IsClientInSample() that takes a PrefService param.
bool IsClientInSampleImpl(PrefService* local_state) {
  // Test the MetricsReporting or PostFREFixMetricsReporting feature (depending
  // on the |kUsePostFREFixSamplingTrial| pref and platform) for all users to
  // ensure that the trial is reported. See the comment on
  // |kUsePostFREFixSamplingTrial| for more details on why there are two
  // different features.
  return base::FeatureList::IsEnabled(
      metrics::internal::kMetricsReportingFeature);
}


// Returns the name of a key under HKEY_CURRENT_USER that can be used to store
// backups of metrics data. Unused except on Windows.
std::wstring GetRegistryBackupKey() {
#if BUILDFLAG(IS_WIN)
  return install_static::GetRegistryPath().append(L"\\StabilityMetrics");
#else
  return std::wstring();
#endif
}

}  // namespace

class ChromeMetricsServicesManagerClient::ChromeEnabledStateProvider
    : public metrics::EnabledStateProvider {
 public:
  explicit ChromeEnabledStateProvider(PrefService* local_state)
      : local_state_(local_state) {}

  ChromeEnabledStateProvider(const ChromeEnabledStateProvider&) = delete;
  ChromeEnabledStateProvider& operator=(const ChromeEnabledStateProvider&) =
      delete;

  ~ChromeEnabledStateProvider() override = default;

  bool IsConsentGiven() const override {
    return ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled(
        local_state_);
  }

  bool IsReportingEnabled() const override {
    return metrics::EnabledStateProvider::IsReportingEnabled() &&
           IsClientInSampleImpl(local_state_);
  }

 private:
  const raw_ptr<PrefService> local_state_;
};

ChromeMetricsServicesManagerClient::ChromeMetricsServicesManagerClient(
    PrefService* local_state)
    : enabled_state_provider_(
          std::make_unique<ChromeEnabledStateProvider>(local_state)),
      local_state_(local_state) {
  CHECK(local_state, base::NotFatalUntil::M161);
}

ChromeMetricsServicesManagerClient::~ChromeMetricsServicesManagerClient() =
    default;

metrics::MetricsStateManager*
ChromeMetricsServicesManagerClient::GetMetricsStateManagerForTesting() {
  return GetMetricsStateManager();
}

// static
bool ChromeMetricsServicesManagerClient::IsClientInSampleForMetrics() {
  return IsClientInSampleImpl(g_browser_process->local_state());
}

#if BUILDFLAG(IS_WIN)
// static
bool ChromeMetricsServicesManagerClient::IsClientInSampleForCrashes() {

  // If this is a Windows client, or if this is an Android client that went
  // through the FRE before the FRE fix was deployed, then this client uses
  // the MetricsReportingFeature and its "disable_crashes" parameter to control
  // whether the client is in-sample for crash reporting.

  // If reporting isn't enabled at all, then we can return early.
  if (!base::FeatureList::IsEnabled(
          metrics::internal::kMetricsReportingFeature)) {
    return false;
  }

  const bool crashes_are_disabled = base::GetFieldTrialParamByFeatureAsBool(
      metrics::internal::kMetricsReportingFeature, "disable_crashes", false);
  return !crashes_are_disabled;
}
#endif

// static
bool ChromeMetricsServicesManagerClient::GetSamplingRatePerMille(int* rate) {
  const base::Feature& feature = metrics::internal::kMetricsReportingFeature;
  std::string rate_str = base::GetFieldTrialParamValueByFeature(
      feature, metrics::internal::kRateParamName);
  if (rate_str.empty()) {
    return false;
  }

  if (!base::StringToInt(rate_str, rate) || *rate > 1000) {
    return false;
  }

  return true;
}


std::unique_ptr<variations::VariationsService>
ChromeMetricsServicesManagerClient::CreateVariationsService() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return variations::VariationsService::Create(
      std::make_unique<ChromeVariationsServiceClient>(), local_state_,
      GetMetricsStateManager(), switches::kDisableBackgroundNetworking,
      base::BindOnce(&content::GetNetworkConnectionTracker));
}

std::unique_ptr<metrics::MetricsServiceClient>
ChromeMetricsServicesManagerClient::CreateMetricsServiceClient(
    variations::SyntheticTrialRegistry* synthetic_trial_registry) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return ChromeMetricsServiceClient::Create(GetMetricsStateManager(),
                                            synthetic_trial_registry);
}

metrics::MetricsStateManager*
ChromeMetricsServicesManagerClient::GetMetricsStateManager() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!metrics_state_manager_) {
    base::FilePath user_data_dir;
    base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);

    metrics::StartupVisibility startup_visibility;
    startup_visibility = metrics::StartupVisibility::kForeground;

    metrics_state_manager_ = metrics::MetricsStateManager::Create(
        local_state_, enabled_state_provider_.get(), GetRegistryBackupKey(),
        user_data_dir, startup_visibility,
        {
            .default_entropy_provider_type =
                metrics::EntropyProviderType::kDefault,
            .force_benchmarking_mode =
                base::CommandLine::ForCurrentProcess()->HasSwitch(
                    switches::kEnableGpuBenchmarking),
        },
        base::BindRepeating(&PostStoreMetricsClientInfo),
        base::BindRepeating(&GoogleUpdateSettings::LoadMetricsClientInfo));
  }
  return metrics_state_manager_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
ChromeMetricsServicesManagerClient::GetURLLoaderFactory() {
  return g_browser_process->system_network_context_manager()
      ->GetSharedURLLoaderFactory();
}

const metrics::EnabledStateProvider&
ChromeMetricsServicesManagerClient::GetEnabledStateProvider() {
  return *enabled_state_provider_;
}

bool ChromeMetricsServicesManagerClient::IsOffTheRecordSessionActive() {
  return ::IsOffTheRecordSessionActive();
}

void ChromeMetricsServicesManagerClient::OnFieldTrialsInitialized() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
#if BUILDFLAG(IS_WIN)
  // Set the registry value so that Crashpad will have the sampling state now
  // and for subsequent runs. Note that Crashpad uses *both* the registry value
  // and the value sent from SetUploadConsent() below.
  // We use IsClientInSampleForCrashes() which checks the feature for whether
  // crash reports should be sent.
  install_static::SetCollectStatsInSample(IsClientInSampleForCrashes());
#endif  // BUILDFLAG(IS_WIN)
}

#if BUILDFLAG(IS_WIN)
void ChromeMetricsServicesManagerClient::UpdateRunningServices(
    bool may_record_and_upload) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Crashpad only tracks consent-to-upload, so tell it this if metrics services
  // are both recording and uploading. The "MetricsReportingEnabled" enterprise
  // policy takes precedence over any value provided here. Furthermore, the
  // "disable_crashes" param of the "MetricsReporting" feature will suppress
  // crash reporting for this client when set; see IsClientInSampleForCrashes.
  if (base::FeatureList::IsEnabled(
          metrics_services_manager::kOffloadUpdateRunningServicesWin)) {
    GoogleUpdateSettings::CollectStatsConsentTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&SetUploadConsent_ExportThunk, may_record_and_upload));
    return;
  }

  SetUploadConsent_ExportThunk(may_record_and_upload);
}
#endif  // BUILDFLAG(IS_WIN)
