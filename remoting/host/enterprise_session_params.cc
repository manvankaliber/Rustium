// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/enterprise_session_params.h"

#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/time/time.h"

namespace remoting {

namespace {
constexpr char kSuppressUserDialogs[] = "suppressUserDialogs";
constexpr char kSuppressNotifications[] = "suppressNotifications";
constexpr char kCurtainLocalUserSession[] = "curtainLocalUserSession";
constexpr char kTerminateUponInput[] = "terminateUponInput";
constexpr char kAllowTroubleshootingTools[] = "allowTroubleshootingTools";
constexpr char kShowTroubleshootingTools[] = "showTroubleshootingTools";
constexpr char kAllowReconnections[] = "allowReconnections";
constexpr char kAllowFileTransfer[] = "allowFileTransfer";
constexpr char kConnectionDialogRequired[] = "connectionDialogRequired";
constexpr char kConnectionAutoAcceptTimeout[] = "connectionAutoAcceptTimeout";
constexpr char kMaximumSessionDuration[] = "maximumSessionDuration";
constexpr char kAllowRemoteInput[] = "allowRemoteInput";
constexpr char kAllowClipboardSync[] = "allowClipboardSync";
constexpr char kChromeOsEnterpriseRequestOrigin[] =
    "chromeOsEnterpriseRequestOrigin";
constexpr char kRequestOriginClassManagement[] = "classManagement";
constexpr char kRequestOriginEnterpriseAdmin[] = "enterpriseAdmin";
constexpr char kRequestOriginUknown[] = "requestOriginUnknown";
constexpr char kChromeOsEnterpriseAudioPlayback[] =
    "chromeOsEnterpriseAudioPlayback";
constexpr char kAudioPlaybackLocalOnly[] = "localOnly";
constexpr char kAudioPlaybackRemoteAndLocal[] = "remoteAndLocal";
constexpr char kAudioPlaybackRemoteOnly[] = "remoteOnly";
constexpr char kAudioPlaybackUnknown[] = "audioPlaybackUnknown";

EnterpriseSessionRequestOrigin ConvertStringToChromeOsEnterpriseRequestOrigin(
    const std::string& request_origin) {
  if (request_origin == kRequestOriginClassManagement) {
    return EnterpriseSessionRequestOrigin::kClassManagement;
  }
  if (request_origin == kRequestOriginEnterpriseAdmin) {
    return EnterpriseSessionRequestOrigin::kEnterpriseAdmin;
  }
  if (request_origin == kRequestOriginUknown) {
    return EnterpriseSessionRequestOrigin::kUnknown;
  }
  NOTREACHED();
}

std::string ConvertChromeOsEnterpriseRequestOriginToString(
    EnterpriseSessionRequestOrigin request_origin) {
  switch (request_origin) {
    case EnterpriseSessionRequestOrigin::kClassManagement:
      return kRequestOriginClassManagement;
    case EnterpriseSessionRequestOrigin::kEnterpriseAdmin:
      return kRequestOriginEnterpriseAdmin;
    case EnterpriseSessionRequestOrigin::kUnknown:
      return kRequestOriginUknown;
  }
}

EnterpriseSessionRequestOrigin GetRequestOriginOrDefault(
    const std::string* input_request_origin,
    EnterpriseSessionRequestOrigin default_request_origin) {
  return input_request_origin ? ConvertStringToChromeOsEnterpriseRequestOrigin(
                                    *input_request_origin)
                              : default_request_origin;
}

EnterpriseSessionAudioPlayback ConvertStringToChromeOsEnterpriseAudioPlayback(
    const std::string& audio_playback) {
  if (audio_playback == kAudioPlaybackLocalOnly) {
    return EnterpriseSessionAudioPlayback::kLocalOnly;
  }
  if (audio_playback == kAudioPlaybackRemoteAndLocal) {
    return EnterpriseSessionAudioPlayback::kRemoteAndLocal;
  }
  if (audio_playback == kAudioPlaybackRemoteOnly) {
    return EnterpriseSessionAudioPlayback::kRemoteOnly;
  }
  if (audio_playback == kAudioPlaybackUnknown) {
    return EnterpriseSessionAudioPlayback::kUnknown;
  }
  NOTREACHED();
}

EnterpriseSessionAudioPlayback GetAudioPlaybackOrDefault(
    const std::string* input_audio_playback,
    EnterpriseSessionAudioPlayback default_audio_playback) {
  return input_audio_playback ? ConvertStringToChromeOsEnterpriseAudioPlayback(
                                    *input_audio_playback)
                              : default_audio_playback;
}
}  // namespace

EnterpriseSessionParams::EnterpriseSessionParams() = default;

EnterpriseSessionParams::EnterpriseSessionParams(
    const EnterpriseSessionParams& other) = default;
EnterpriseSessionParams& EnterpriseSessionParams::operator=(
    const EnterpriseSessionParams& other) = default;

EnterpriseSessionParams::~EnterpriseSessionParams() = default;

bool EnterpriseSessionParams::operator==(
    const EnterpriseSessionParams& other) const = default;

std::string ConvertEnterpriseSessionAudioPlaybackToString(
    EnterpriseSessionAudioPlayback audio_playback) {
  switch (audio_playback) {
    case EnterpriseSessionAudioPlayback::kLocalOnly:
      return kAudioPlaybackLocalOnly;
    case EnterpriseSessionAudioPlayback::kRemoteAndLocal:
      return kAudioPlaybackRemoteAndLocal;
    case EnterpriseSessionAudioPlayback::kRemoteOnly:
      return kAudioPlaybackRemoteOnly;
    case EnterpriseSessionAudioPlayback::kUnknown:
      return kAudioPlaybackUnknown;
  }
}

// static
EnterpriseSessionParams EnterpriseSessionParams::FromDict(
    const base::DictValue& dict) {
  EnterpriseSessionParams params;
  params.suppress_user_dialogs =
      dict.FindBool(kSuppressUserDialogs).value_or(false);
  params.suppress_notifications =
      dict.FindBool(kSuppressNotifications).value_or(false);
  params.terminate_upon_input =
      dict.FindBool(kTerminateUponInput).value_or(false);
  params.curtain_local_user_session =
      dict.FindBool(kCurtainLocalUserSession).value_or(false);
  params.maximum_session_duration =
      base::ValueToTimeDelta(dict.Find(kMaximumSessionDuration))
          .value_or(base::TimeDelta());
  params.allow_remote_input = dict.FindBool(kAllowRemoteInput).value_or(true);
  params.allow_clipboard_sync =
      dict.FindBool(kAllowClipboardSync).value_or(true);
  params.show_troubleshooting_tools =
      dict.FindBool(kShowTroubleshootingTools).value_or(false);
  params.allow_troubleshooting_tools =
      dict.FindBool(kAllowTroubleshootingTools).value_or(false);
  params.allow_reconnections =
      dict.FindBool(kAllowReconnections).value_or(false);
  params.allow_file_transfer =
      dict.FindBool(kAllowFileTransfer).value_or(false);
  params.connection_dialog_required =
      dict.FindBool(kConnectionDialogRequired).value_or(false);
  params.request_origin = GetRequestOriginOrDefault(
      dict.FindString(kChromeOsEnterpriseRequestOrigin),
      /* default_request_origin= */ EnterpriseSessionRequestOrigin::
          kEnterpriseAdmin);
  params.audio_playback = GetAudioPlaybackOrDefault(
      dict.FindString(kChromeOsEnterpriseAudioPlayback),
      /* default_audio_playback= */ EnterpriseSessionAudioPlayback::kLocalOnly);
  params.connection_auto_accept_timeout =
      base::ValueToTimeDelta(dict.Find(kConnectionAutoAcceptTimeout))
          .value_or(base::TimeDelta());
  return params;
}

base::DictValue EnterpriseSessionParams::ToDict() const {
  return base::DictValue()
      .Set(kSuppressUserDialogs, suppress_user_dialogs)
      .Set(kSuppressNotifications, suppress_notifications)
      .Set(kTerminateUponInput, terminate_upon_input)
      .Set(kCurtainLocalUserSession, curtain_local_user_session)
      .Set(kMaximumSessionDuration,
           base::TimeDeltaToValue(maximum_session_duration))
      .Set(kAllowRemoteInput, allow_remote_input)
      .Set(kAllowClipboardSync, allow_clipboard_sync)
      .Set(kShowTroubleshootingTools, show_troubleshooting_tools)
      .Set(kAllowTroubleshootingTools, allow_troubleshooting_tools)
      .Set(kAllowReconnections, allow_reconnections)
      .Set(kAllowFileTransfer, allow_file_transfer)
      .Set(kConnectionDialogRequired, connection_dialog_required)
      .Set(kChromeOsEnterpriseRequestOrigin,
           ConvertChromeOsEnterpriseRequestOriginToString(request_origin))
      .Set(kChromeOsEnterpriseAudioPlayback,
           ConvertEnterpriseSessionAudioPlaybackToString(audio_playback))
      .Set(kConnectionAutoAcceptTimeout,
           base::TimeDeltaToValue(connection_auto_accept_timeout));
}

}  // namespace remoting
