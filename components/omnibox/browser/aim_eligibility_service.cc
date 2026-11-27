// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/aim_eligibility_service.h"

#include "components/omnibox/browser/omnibox_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

bool AimEligibilityService::GenericKillSwitchFeatureCheck(
    const AimEligibilityService* aim_eligibility_service,
    const base::Feature& feature) {
  return false;
}

void AimEligibilityService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  // Retained for existing policy mappings; they cannot enable AI Mode.
  registry->RegisterIntegerPref(omnibox::kAIModeSettings, 1);
  registry->RegisterIntegerPref(omnibox::kThirdPartyAiChatSettings, 1);
}

bool AimEligibilityService::IsAimAllowedByPolicy(const PrefService* prefs) {
  return false;
}

bool AimEligibilityService::IsAimAllowedByThirdPartyPolicy(
    const PrefService* prefs) {
  return false;
}

std::string AimEligibilityService::EligibilityResponseSourceToString(
    EligibilityResponseSource source) {
  return "Removed";
}

AimEligibilityService::ServerEligibilityRequestMode
AimEligibilityService::GetServerEligibilityRequestMode() {
  return ServerEligibilityRequestMode::kLegacyGet;
}

AimEligibilityService::AimEligibilityService(
    PrefService& pref_service,
    TemplateURLService* template_url_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    signin::IdentityManager* identity_manager,
    const std::string& locale,
    Configuration configuration) {}

AimEligibilityService::~AimEligibilityService() {}

std::string AimEligibilityService::GetLocale() const {
  return GetLocaleImpl();
}

bool AimEligibilityService::IsLanguage(const std::string& language) const {
  return false;
}

base::CallbackListSubscription
AimEligibilityService::RegisterEligibilityChangedCallback(
    base::RepeatingClosure callback) {
  return {};
}

bool AimEligibilityService::IsServerEligibilityEnabled() const {
  return false;
}

bool AimEligibilityService::IsAimAllowedByDse() const {
  return false;
}

bool AimEligibilityService::IsAimAllowedByFeatureAndPolicy() const {
  return false;
}

bool AimEligibilityService::IsAimAllowedByThirdPartyPolicy() const {
  return false;
}

bool AimEligibilityService::IsAimLocallyEligible() const {
  return false;
}

bool AimEligibilityService::IsAimEligible() const {
  return false;
}

bool AimEligibilityService::IsPdfUploadEligible() const {
  return false;
}

bool AimEligibilityService::IsDeepSearchEligible() const {
  return false;
}

bool AimEligibilityService::IsCreateImagesEligible() const {
  return false;
}

bool AimEligibilityService::IsCanvasEligible() const {
  return false;
}

bool AimEligibilityService::IsCobrowseServerEligible() const {
  return false;
}

bool AimEligibilityService::IsCobrowseEligible() const {
  return false;
}

bool AimEligibilityService::IsFuseboxEligible() const {
  return false;
}

bool AimEligibilityService::IsCsbEligible() const {
  return false;
}

bool AimEligibilityService::IsAimUrl(
    const GURL& url,
    std::optional<contextual_tasks::HostOverride> host_override) const {
  return false;
}

bool AimEligibilityService::IsAimHost(
    const GURL& url,
    std::optional<contextual_tasks::HostOverride> host_override) const {
  return false;
}

bool AimEligibilityService::HasAimUrlParams(const GURL& url) const {
  return false;
}

bool AimEligibilityService::HasNoCobrowseParams(const GURL& url) const {
  return false;
}

const omnibox::AimEligibilityResponse&
AimEligibilityService::GetMostRecentResponse() const {
  return empty_response_;
}

AimEligibilityService::EligibilityResponseSource
AimEligibilityService::GetMostRecentResponseSource() const {
  return EligibilityResponseSource::kDefault;
}

const omnibox::SearchboxConfig* AimEligibilityService::GetSearchboxConfig()
    const {
  return nullptr;
}

const omnibox::LhsRailConfig* AimEligibilityService::GetLhsRailConfig() const {
  return nullptr;
}

AimEligibilityService::AuthenticationMethod
AimEligibilityService::GetMostRecentResponseAuthMethod() const {
  return AuthenticationMethod::kNone;
}

void AimEligibilityService::StartServerEligibilityRequestForDebugging() {}

void AimEligibilityService::FetchEligibility(RequestSource source) {}

bool AimEligibilityService::SetEligibilityResponseForDebugging(
    const std::string& base64_encoded_response) {
  return false;
}
