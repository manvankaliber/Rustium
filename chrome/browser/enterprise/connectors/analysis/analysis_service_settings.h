// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENTERPRISE_CONNECTORS_ANALYSIS_ANALYSIS_SERVICE_SETTINGS_H_
#define CHROME_BROWSER_ENTERPRISE_CONNECTORS_ANALYSIS_ANALYSIS_SERVICE_SETTINGS_H_

#include <memory>
#include <optional>
#include <string>

#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/enterprise/buildflags/buildflags.h"
#include "components/enterprise/connectors/core/analysis_service_settings_base.h"
#include "components/enterprise/connectors/core/analysis_settings.h"
#include "components/enterprise/connectors/core/common.h"
#include "components/enterprise/connectors/core/service_provider_config.h"
#include "components/url_matcher/url_matcher.h"


namespace storage {
class FileSystemURL;
}

namespace enterprise_connectors {

// The settings for an analysis service obtained from a connector policy.
class AnalysisServiceSettings : public AnalysisServiceSettingsBase {
 public:
  explicit AnalysisServiceSettings(
      const base::Value& settings_value,
      const ServiceProviderConfig& service_provider_config);
  AnalysisServiceSettings(const AnalysisServiceSettings&) = delete;
  AnalysisServiceSettings(AnalysisServiceSettings&&);
  AnalysisServiceSettings& operator=(const AnalysisServiceSettings&) = delete;
  AnalysisServiceSettings& operator=(AnalysisServiceSettings&&);
  ~AnalysisServiceSettings() override;

  using AnalysisServiceSettingsBase::GetAnalysisSettings;


 private:
};

}  // namespace enterprise_connectors

#endif  // CHROME_BROWSER_ENTERPRISE_CONNECTORS_ANALYSIS_ANALYSIS_SERVICE_SETTINGS_H_
