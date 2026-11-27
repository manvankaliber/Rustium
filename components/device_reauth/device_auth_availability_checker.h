// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DEVICE_REAUTH_DEVICE_AUTH_AVAILABILITY_CHECKER_H_
#define COMPONENTS_DEVICE_REAUTH_DEVICE_AUTH_AVAILABILITY_CHECKER_H_

#include "build/build_config.h"
namespace device_reauth {


// This interface encapsulates operations related to checking biometric and
// screen lock availability. It's intended to be used prior to sharing the
// user's credentials with a website, either via form filling or the Credential
// Management API.
class DeviceAuthAvailabilityChecker {
 public:
  DeviceAuthAvailabilityChecker() = default;
  DeviceAuthAvailabilityChecker(const DeviceAuthAvailabilityChecker&) = delete;
  DeviceAuthAvailabilityChecker& operator=(
      const DeviceAuthAvailabilityChecker&) = delete;
  virtual ~DeviceAuthAvailabilityChecker() = default;

  // Returns whether biometrics are available for a given device.
  virtual bool CanAuthenticateWithBiometrics() = 0;

  // Returns whether biometrics or screen lock are available for a given device.
  virtual bool CanAuthenticateWithBiometricOrScreenLock() = 0;

};

}  // namespace device_reauth

#endif  // COMPONENTS_DEVICE_REAUTH_DEVICE_AUTH_AVAILABILITY_CHECKER_H_
