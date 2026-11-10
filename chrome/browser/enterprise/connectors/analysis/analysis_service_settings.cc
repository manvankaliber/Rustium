// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/enterprise/connectors/analysis/analysis_service_settings.h"

#include "build/build_config.h"
#include "components/enterprise/buildflags/buildflags.h"
#include "components/enterprise/connectors/core/common.h"
#include "components/enterprise/connectors/core/service_provider_config.h"


namespace enterprise_connectors {

AnalysisServiceSettings::AnalysisServiceSettings(
    const base::Value& settings_value,
    const ServiceProviderConfig& service_provider_config)
    : AnalysisServiceSettingsBase(settings_value, service_provider_config) {
  if (!analysis_config_) {
    // Parsing in the base class failed
    return;
  }

}


AnalysisServiceSettings::AnalysisServiceSettings(AnalysisServiceSettings&&) =
    default;
AnalysisServiceSettings& AnalysisServiceSettings::operator=(
    AnalysisServiceSettings&&) = default;
AnalysisServiceSettings::~AnalysisServiceSettings() = default;

}  // namespace enterprise_connectors
