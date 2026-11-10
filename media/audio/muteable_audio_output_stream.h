// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_MUTEABLE_AUDIO_OUTPUT_STREAM_H_
#define MEDIA_AUDIO_MUTEABLE_AUDIO_OUTPUT_STREAM_H_

#include "media/audio/audio_io.h"

namespace media {

class MEDIA_EXPORT MuteableAudioOutputStream : public AudioOutputStream {
 public:
  // Volume control coming from hardware. It overrides volume when it's
  // true. Otherwise, use SetVolume(double volume) for scaling.
  virtual void SetMute(bool muted) = 0;
};

}  // namespace media

#endif  // MEDIA_AUDIO_MUTEABLE_AUDIO_OUTPUT_STREAM_H_
