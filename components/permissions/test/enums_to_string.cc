// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/permissions/test/enums_to_string.h"

#include "base/containers/fixed_flat_map.h"
#include "components/permissions/permission_request_enums.h"
#include "components/permissions/prediction_service/permission_ui_selector.h"
#include "components/permissions/request_type.h"
#include "third_party/blink/public/public_buildflags.h"

namespace test {
std::string_view ToString(
    permissions::PermissionUiSelector::QuietUiReason ui_reason) {
  using QuietUiReason = ::permissions::PermissionUiSelector::QuietUiReason;
  static constexpr auto map =
      base::MakeFixedFlatMap<QuietUiReason, std::string_view>(
          {{QuietUiReason::kEnabledInPrefs, "EnabledInPrefs"},
           {QuietUiReason::kTriggeredByCrowdDeny, "TriggeredByCrowdDeny"},
           {QuietUiReason::kServicePredictedVeryUnlikelyGrant,
            "ServicePredictedVeryUnlikelyGrant"},
           {QuietUiReason::kOnDevicePredictedVeryUnlikelyGrant,
            "OnDevicePredictedVeryUnlikelyGrant"},
           {QuietUiReason::kTriggeredDueToAbusiveRequests,
            "TriggeredDueToAbusiveRequests"},
           {QuietUiReason::kTriggeredDueToAbusiveContent,
            "TriggeredDueToAbusiveContent"},
           {QuietUiReason::kTriggeredDueToDisruptiveBehavior,
            "TriggeredDueToDisruptiveBehavior"}});

  auto it = map.find(ui_reason);
  return (it == map.end()) ? "Unknown" : it->second;
}

std::string_view ToString(permissions::RequestType request_type) {
  using RequestType = ::permissions::RequestType;

  static constexpr auto map =
      base::MakeFixedFlatMap<RequestType, std::string_view>({
          {RequestType::kAmbientLogin, "AmbientLogin"},
          {RequestType::kArSession, "ArSession"},
          {RequestType::kCameraPanTiltZoom, "CameraPanTiltZoom"},
          {RequestType::kCameraStream, "CameraStream"},
          {RequestType::kCapturedSurfaceControl, "CapturedSurfaceControl"},
          {RequestType::kClipboard, "Clipboard"},
          {RequestType::kDiskQuota, "DiskQuota"},
          {RequestType::kFileSystemAccess, "FileSystemAccess"},
          {RequestType::kGeolocation, "Geolocation"},
          {RequestType::kHandTracking, "HandTracking"},
          {RequestType::kIdentityProvider, "IdentityProvider"},
          {RequestType::kIdleDetection, "IdleDetection"},
          {RequestType::kLocalFonts, "LocalFonts"},
          {RequestType::kMicStream, "MicStream"},
          {RequestType::kMidiSysex, "MidiSysex"},
          {RequestType::kMultipleDownloads, "MultipleDownloads"},
          {RequestType::kNotifications, "Notifications"},
          {RequestType::kKeyboardLock, "KeyboardLock"},
          {RequestType::kPointerLock, "PointerLock"},
#if BUILDFLAG(IS_WIN)
          {RequestType::kProtectedMediaIdentifier, "ProtectedMediaIdentifier"},
#endif
          {RequestType::kRegisterProtocolHandler, "RegisterProtocolHandler"},
          {RequestType::kStorageAccess, "StorageAccess"},
          {RequestType::kVrSession, "VrSession"},
          {RequestType::kWebAppInstallation, "WebAppInstallation"},
          {RequestType::kWindowManagement, "WindowManagement"},
      });

  auto it = map.find(request_type);
  return (it == map.end()) ? "Unknown" : it->second;
}

std::string_view ToString(
    permissions::PermissionRequestRelevance request_relevance) {
  using RequestRelevance = ::permissions::PermissionRequestRelevance;
  static constexpr auto map =
      base::MakeFixedFlatMap<RequestRelevance, std::string_view>({
          {RequestRelevance::kVeryLow, "VeryLow"},
          {RequestRelevance::kLow, "Low"},
          {RequestRelevance::kMedium, "Medium"},
          {RequestRelevance::kHigh, "High"},
          {RequestRelevance::kVeryHigh, "VeryHigh"},
      });

  auto it = map.find(request_relevance);
  return (it == map.end()) ? "Unknown" : it->second;
}

}  // namespace test
