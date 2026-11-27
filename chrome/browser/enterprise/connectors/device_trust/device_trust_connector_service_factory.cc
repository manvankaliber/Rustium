// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/enterprise/connectors/device_trust/device_trust_connector_service_factory.h"

#include "base/no_destructor.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/enterprise/device_trust/core/device_trust_connector_service.h"
#include "components/keyed_service/core/keyed_service.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/enterprise/connectors/device_trust/browser/signing_key_policy_observer.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "components/enterprise/browser/controller/chrome_browser_cloud_management_controller.h"
#include "components/enterprise/device_trust/core/device_trust_key_manager.h"



namespace enterprise_connectors {

namespace {

// Returns whether the browser-level policy should initialize the browser-wide
// device trust key manager.
bool ShouldObserveSigningKeyPolicy() {
  // `system_network_context_manager` can be null in unit test environments
  // (e.g. `TestingBrowserProcess`) where the network service and Chrome Browser
  // Cloud Management (CBCM) are not initialized. In that case, avoid
  // initializing the device trust key manager and observer.
  return g_browser_process->system_network_context_manager();
}

}  // namespace

// static
DeviceTrustConnectorServiceFactory*
DeviceTrustConnectorServiceFactory::GetInstance() {
  static base::NoDestructor<DeviceTrustConnectorServiceFactory> instance;
  return instance.get();
}

// static
DeviceTrustConnectorService* DeviceTrustConnectorServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<DeviceTrustConnectorService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

bool DeviceTrustConnectorServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool DeviceTrustConnectorServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

DeviceTrustConnectorServiceFactory::DeviceTrustConnectorServiceFactory()
    : ProfileKeyedServiceFactory(
          "DeviceTrustConnectorService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // TODO(crbug.com/41488885): Check if this service is needed for
              // Ash Internals.
              .WithAshInternals(ProfileSelection::kOwnInstance)
              .WithIsolatedMode(ProfileSelection::kOwnInstance)
              .Build()) {}

DeviceTrustConnectorServiceFactory::~DeviceTrustConnectorServiceFactory() =
    default;

std::unique_ptr<KeyedService>
DeviceTrustConnectorServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  // Disallow service for Incognito except for the sign-in profile of ChromeOS
  // (on the login screen) or Enterprise Isolated Mode profile.
  if (context->IsOffTheRecord()) {
    bool unsupported_profile = true;

    if (profile->IsEnterpriseIsolatedModeProfile()) {
      unsupported_profile = false;
    }

    if (unsupported_profile) {
      return nullptr;
    }
  }

  std::unique_ptr<DeviceTrustConnectorService> service =
      std::make_unique<DeviceTrustConnectorService>(profile->GetPrefs());

  if (ShouldObserveSigningKeyPolicy()) {
    auto* key_manager = g_browser_process->browser_policy_connector()
                            ->chrome_browser_cloud_management_controller()
                            ->GetDeviceTrustKeyManager();
    service->AddObserver(
        std::make_unique<SigningKeyPolicyObserver>(key_manager));
  }

  return service;
}

}  // namespace enterprise_connectors
