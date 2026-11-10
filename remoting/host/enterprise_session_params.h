// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_ENTERPRISE_SESSION_PARAMS_H_
#define REMOTING_HOST_ENTERPRISE_SESSION_PARAMS_H_

#include <string>

#include "base/time/time.h"
#include "base/values.h"

namespace remoting {

// The caller who initiated the request.
enum class EnterpriseSessionRequestOrigin {
  kUnknown,
  kEnterpriseAdmin,
  kClassManagement,
};

// Where to route audio from the local host during the CRD session, where a
// remote client is viewing the local host's screen.
enum class EnterpriseSessionAudioPlayback {
  kUnknown,
  kLocalOnly,
  kRemoteAndLocal,
  kRemoteOnly,
};

// Converts a `EnterpriseSessionAudioPlayback` to a string, for logging.
std::string ConvertEnterpriseSessionAudioPlaybackToString(
    EnterpriseSessionAudioPlayback audio_playback);

// Parameters exchanged with enterprise remote desktop peers.
// These parameters are not exposed through the public Mojom APIs, for security
// reasons.
struct EnterpriseSessionParams {
  EnterpriseSessionParams();

  EnterpriseSessionParams(const EnterpriseSessionParams& other);
  EnterpriseSessionParams& operator=(const EnterpriseSessionParams& other);

  ~EnterpriseSessionParams();

  bool operator==(const EnterpriseSessionParams& other) const;

  // Helpers used to serialize/deserialize enterprise params.
  static EnterpriseSessionParams FromDict(const base::DictValue& dict);
  base::DictValue ToDict() const;

  // Local machine configuration.
  bool suppress_user_dialogs = false;
  bool suppress_notifications = false;
  bool terminate_upon_input = false;
  bool curtain_local_user_session = false;
  base::TimeDelta maximum_session_duration;
  bool allow_remote_input = true;
  bool allow_clipboard_sync = true;

  // Remote machine configuration.
  bool show_troubleshooting_tools = false;
  bool allow_troubleshooting_tools = false;
  bool allow_reconnections = false;
  bool allow_file_transfer = false;
  bool connection_dialog_required = false;
  EnterpriseSessionRequestOrigin request_origin =
      EnterpriseSessionRequestOrigin::kUnknown;
  EnterpriseSessionAudioPlayback audio_playback =
      EnterpriseSessionAudioPlayback::kUnknown;

  // Both local and remote machine configuration.
  base::TimeDelta connection_auto_accept_timeout;
};

}  // namespace remoting

#endif  // REMOTING_HOST_ENTERPRISE_SESSION_PARAMS_H_
