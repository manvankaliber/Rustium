// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/device_reauth/chrome_device_authenticator_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "components/device_reauth/device_auth_availability_checker.h"
#include "components/device_reauth/device_authenticator_common.h"
#include "components/password_manager/core/browser/features/password_features.h"

#if BUILDFLAG(IS_MAC)
#include "chrome/browser/device_reauth/mac/authenticator_mac.h"
#include "chrome/browser/device_reauth/mac/device_auth_availability_checker_mac.h"
#include "chrome/browser/device_reauth/mac/device_authenticator_mac.h"
#elif BUILDFLAG(IS_WIN)
#include "chrome/browser/device_reauth/win/authenticator_win.h"
#include "chrome/browser/device_reauth/win/device_auth_availability_checker_win.h"
#include "chrome/browser/device_reauth/win/device_authenticator_win.h"
#endif

using content::BrowserContext;
using device_reauth::DeviceAuthAvailabilityChecker;
using device_reauth::DeviceAuthenticator;

ChromeDeviceAuthenticatorFactory::ChromeDeviceAuthenticatorFactory()
    : ProfileKeyedServiceFactory(
          "ChromeDeviceAuthenticator",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .WithGuest(ProfileSelection::kOwnInstance)
              // TODO(crbug.com/41488885): Check if this service is needed for
              // Ash Internals.
              .WithAshInternals(ProfileSelection::kOwnInstance)
              .Build()) {}

ChromeDeviceAuthenticatorFactory::~ChromeDeviceAuthenticatorFactory() = default;

// static
ChromeDeviceAuthenticatorFactory*
ChromeDeviceAuthenticatorFactory::GetInstance() {
  static base::NoDestructor<ChromeDeviceAuthenticatorFactory> instance;
  return instance.get();
}

// static
std::unique_ptr<DeviceAuthAvailabilityChecker>
ChromeDeviceAuthenticatorFactory::CreateAvailabilityChecker() {
#if BUILDFLAG(IS_MAC)
  return std::make_unique<DeviceAuthAvailabilityCheckerMac>();
#elif BUILDFLAG(IS_WIN)
  return std::make_unique<DeviceAuthAvailabilityCheckerWin>();
#else
  static_assert(false);
#endif
}

// static
std::unique_ptr<DeviceAuthenticator>
ChromeDeviceAuthenticatorFactory::GetForProfile(
    Profile* profile,
    const gfx::NativeWindow window,
    const device_reauth::DeviceAuthParams& params) {
  return ChromeDeviceAuthenticatorFactory::GetForProfile(profile, params);
}

// static
std::unique_ptr<DeviceAuthenticator>
ChromeDeviceAuthenticatorFactory::GetForProfile(
    Profile* profile,
    const device_reauth::DeviceAuthParams& params) {
  DeviceAuthenticatorProxy* proxy = static_cast<DeviceAuthenticatorProxy*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));

  CHECK(proxy);

#if BUILDFLAG(IS_MAC)
  auto device_authenticator = std::make_unique<DeviceAuthenticatorMac>(
      std::make_unique<AuthenticatorMac>(), proxy, params);
#elif BUILDFLAG(IS_WIN)
  auto device_authenticator = std::make_unique<DeviceAuthenticatorWin>(
      std::make_unique<AuthenticatorWin>(), proxy, params);
#else
  static_assert(false);
#endif
  return std::move(device_authenticator);
}

std::unique_ptr<KeyedService>
ChromeDeviceAuthenticatorFactory::BuildServiceInstanceForBrowserContext(
    BrowserContext* context) const {
#if BUILDFLAG(IS_WIN)
  // Here we cache the biometric availability. If cached value is wrong (eg.
  // user disable biometrics while chrome is running) then standard password
  // prompt will appear.
  // When the flag is enabled, the caching happens 30 seconds after startup.
  if (!base::FeatureList::IsEnabled(
          password_manager::features::kDeviceReauthRefactor)) {
    DeviceAuthenticatorWin::CacheIfBiometricsAvailable(
        std::make_unique<AuthenticatorWin>().get());
  }
#endif

  return std::make_unique<DeviceAuthenticatorProxy>();
}
