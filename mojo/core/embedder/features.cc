// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/core/embedder/features.h"

#include "build/build_config.h"

namespace mojo {
namespace core {

#if BUILDFLAG(IS_LINUX)
BASE_FEATURE(kMojoUseEventFd, base::FEATURE_DISABLED_BY_DEFAULT);
#endif

#if BUILDFLAG(IS_LINUX)
const base::FeatureParam<int> kMojoUseEventFdPages{&kMojoUseEventFd,
                                                   "MojoUseEventFdPages", 4};
const char kSuppressEventfdUpgradeForWebview[] =
    "suppress-eventfd-upgrade-for-webview";
#endif
        // BUILDFLAG(IS_ANDROID)

BASE_FEATURE(kMojoIpczMemV2, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kMojoFixGeometricBufferGrowth, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kMojoFixGeometricBufferGrowth2, base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_WIN)
// If enabled, then only handles of types Section, File, Directory and
// DxgkSharedResource are allowed to traverse a process boundary to an untrusted
// process via mojo.
BASE_FEATURE(kMojoHandleTypeProtections, base::FEATURE_ENABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_WIN)

}  // namespace core
}  // namespace mojo
