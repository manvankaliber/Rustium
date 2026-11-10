// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILES_IMAGE_DECODE_CACHE_UTILS_CC_
#define CC_TILES_IMAGE_DECODE_CACHE_UTILS_CC_

#include "cc/tiles/image_decode_cache_utils.h"

#include "base/byte_size.h"
#include "base/system/sys_info.h"
#include "build/build_config.h"


namespace cc {

// static
size_t ImageDecodeCacheUtils::GetWorkingSetBytesForImageDecode(
    bool for_renderer) {
  // Note: Android WebView does not use the increased working set budget on
  // Desktop Android and uses `kDefaultWorkingSet` (see
  // `layer_tree_settings.cc`).
  base::ByteSize decoded_image_working_set_budget = kDefaultWorkingSet;
  constexpr bool use_desktop_limits = true;

  if (use_desktop_limits && for_renderer) {
    const bool using_low_memory_policy = base::SysInfo::IsLowEndDevice();
    // If there's over 4GB of RAM, increase the working set size to 256MB for
    // both gpu and software.
    constexpr base::ByteSize kImageDecodeMemoryThreshold = base::GiB(4);
    if (using_low_memory_policy) {
      decoded_image_working_set_budget = base::MiB(32);
    } else if (base::SysInfo::AmountOfTotalPhysicalMemory() >=
               kImageDecodeMemoryThreshold) {
      decoded_image_working_set_budget = base::MiB(256);
    }
  }
  return decoded_image_working_set_budget.InBytes();
}

}  // namespace cc

#endif  // CC_TILES_IMAGE_DECODE_CACHE_UTILS_CC_
