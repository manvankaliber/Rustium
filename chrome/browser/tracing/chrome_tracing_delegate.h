// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRACING_CHROME_TRACING_DELEGATE_H_
#define CHROME_BROWSER_TRACING_CHROME_TRACING_DELEGATE_H_

#include <optional>

#include "base/memory/raw_ref.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/public/browser/tracing_delegate.h"

#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"  // nogncheck crbug.com/40147906

class BrowserWindowInterface;
class GlobalBrowserCollection;
class PrefService;

namespace tracing {
class BackgroundTracingStateManager;
}

// These values are logged to UMA. Entries should not be renumbered and numeric
// values should never be reused. Please keep in sync with
// "TracingFinalizationDisallowedReason" in
// src/tools/metrics/histograms/enums.xml.
enum class TracingFinalizationDisallowedReason {
  kIncognitoLaunched = 0,
  //  kProfileNotLoaded = 1, Obsolete
  //  kCrashMetricsNotLoaded = 2, Obsolete
  //  kLastSessionCrashed = 3, Obsolete
  kMetricsReportingDisabled = 4,
  //  kTraceUploadedRecently = 5, Obsolete
  //  kLastTracingSessionDidNotEnd = 6, Obsolete as of Nov'2024.
  kMaxValue = kMetricsReportingDisabled
};

class ChromeTracingDelegate : public content::TracingDelegate,
                              public BrowserCollectionObserver
{
 public:
  // Whether system-wide performance trace collection using the external system
  // tracing service is enabled.
  static bool IsSystemWideTracingEnabled();

  explicit ChromeTracingDelegate(PrefService& local_state);
  ~ChromeTracingDelegate() override;

  // content::TracingDelegate implementation:
  bool IsRecordingAllowed(IsLocalScenario is_local_scenario,
                          base::TimeTicks session_start) const override;
  bool ShouldSaveUnuploadedTrace() const override;
  std::unique_ptr<tracing::BackgroundTracingStateManager> CreateStateManager()
      override;
  std::string RecordSerializedSystemProfileMetrics() const override;
  tracing::MetadataDataSource::BundleRecorder
  CreateSystemProfileMetadataRecorder() const override;
  tracing::MetadataDataSource::ChromeMetadataRecorder
  CreateChromeMetadataPacketRecorder() const override;

#if BUILDFLAG(IS_WIN)
  void GetSystemTracingState(
      base::OnceCallback<void(bool service_supported, bool service_enabled)>
          on_tracing_state) override;
  void EnableSystemTracing(
      base::OnceCallback<void(bool success)> on_complete) override;
  void DisableSystemTracing(
      base::OnceCallback<void(bool success)> on_complete) override;
#endif  // BUILDFLAG(IS_WIN)

 private:
  // BrowserCollectionObserver:
  void OnBrowserCreated(BrowserWindowInterface* browser) override;
  void OnBrowserClosed(BrowserWindowInterface* browser) override;

  void EnsureObservingBrowserCollection() const;

  const raw_ref<PrefService> local_state_;

  // Track the most recent OffTheRecord browser creation time. It's ok to update
  // to a newer timestamp when there are multiple OffTheRecord browsers, since
  // the newer timestamp is stricter.
  base::TimeTicks latest_incognito_launched_;

  mutable base::ScopedObservation<GlobalBrowserCollection,
                                  BrowserCollectionObserver>
      browser_collection_observation_{this};
};

#endif  // CHROME_BROWSER_TRACING_CHROME_TRACING_DELEGATE_H_
