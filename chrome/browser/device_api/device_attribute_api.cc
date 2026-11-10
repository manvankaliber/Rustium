// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/device_api/device_attribute_api.h"

#include "base/functional/callback.h"
#include "build/build_config.h"


using blink::mojom::DeviceAPIService;

namespace {

constexpr char kNotAffiliatedErrorMessage[] =
    "This web API is not allowed if the current profile is not affiliated.";

constexpr char kNoDeviceAttributesPermissionErrorMessage[] =
    "The current origin cannot use this web API because it was not granted the "
    "'device-attributes' permission.";

constexpr char kNotSupportedPlatformErrorMessage[] =
    "This web API is not supported on the current platform.";

}  // namespace

DeviceAttributeApiImpl::DeviceAttributeApiImpl() = default;
DeviceAttributeApiImpl::~DeviceAttributeApiImpl() = default;

void DeviceAttributeApiImpl::ReportNotAffiliatedError(
    NotificationCallback callback) {
  std::move(callback).Run(base::unexpected(kNotAffiliatedErrorMessage));
}

void DeviceAttributeApiImpl::ReportNotAllowedError(
    NotificationCallback callback) {
  std::move(callback).Run(
      base::unexpected(kNoDeviceAttributesPermissionErrorMessage));
}

void DeviceAttributeApiImpl::ReportNotSupportedPlatformError(
    NotificationCallback callback) {
  std::move(callback).Run(base::unexpected(kNotSupportedPlatformErrorMessage));
}

void DeviceAttributeApiImpl::GetDirectoryId(
    DeviceAPIService::GetDirectoryIdCallback callback) {
  ReportNotSupportedPlatformError(std::move(callback));
}

void DeviceAttributeApiImpl::GetHostname(
    DeviceAPIService::GetHostnameCallback callback) {
  ReportNotSupportedPlatformError(std::move(callback));
}

void DeviceAttributeApiImpl::GetSerialNumber(
    DeviceAPIService::GetSerialNumberCallback callback) {
  ReportNotSupportedPlatformError(std::move(callback));
}

void DeviceAttributeApiImpl::GetAnnotatedAssetId(
    DeviceAPIService::GetAnnotatedAssetIdCallback callback) {
  ReportNotSupportedPlatformError(std::move(callback));
}

void DeviceAttributeApiImpl::GetAnnotatedLocation(
    DeviceAPIService::GetAnnotatedLocationCallback callback) {
  ReportNotSupportedPlatformError(std::move(callback));
}
