// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/platform_features.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"

namespace media {

bool IsVp9kSVCHWDecodingEnabled() {
#if BUILDFLAG(IS_WIN)
  // TODO(crbug.com/40286220): Experiment to enable on Windows.
  return base::FeatureList::IsEnabled(media::kD3D11Vp9kSVCHWDecoding);
#else
  return false;
#endif
}

}  // namespace media
