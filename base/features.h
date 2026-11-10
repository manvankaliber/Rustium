// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_FEATURES_H_
#define BASE_FEATURES_H_

#include "base/base_export.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"

namespace base::features {

// All features in alphabetical order. The features should be documented
// alongside the definition of their values in the .cc file.

// Alphabetical:
BASE_EXPORT BASE_DECLARE_FEATURE(kBase64SimdBase64Encode);

BASE_EXPORT BASE_DECLARE_FEATURE(kFeatureParamWithCache);

BASE_EXPORT BASE_DECLARE_FEATURE(kFastFilePathIsParent);

BASE_EXPORT BASE_DECLARE_FEATURE(kHeapProfilerIncludeResidency);

BASE_EXPORT BASE_DECLARE_FEATURE(kI18nOptimizations);

BASE_EXPORT BASE_DECLARE_FEATURE(kLowEndMemoryExperiment);

BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(int, kLowMemoryDeviceThresholdMB);

BASE_EXPORT BASE_DECLARE_FEATURE(kRecordLockAcquisitionTime);
BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(
    std::string,
    kRecordLockAcquisitionTimeAllowedThreads);

BASE_EXPORT BASE_DECLARE_FEATURE(kScopedBestEffortExecutionFenceForTaskQueue);

BASE_EXPORT BASE_DECLARE_FEATURE(kStackScanMaxFramePointerToStackEndGap);
BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(
    int,
    kStackScanMaxFramePointerToStackEndGapThresholdMB);



BASE_EXPORT BASE_DECLARE_FEATURE(kUseTerminationStatusMemoryExhaustion);

BASE_EXPORT BASE_DECLARE_FEATURE(kUtfConversionAsciiFastPath);

#if BUILDFLAG(IS_WIN)
BASE_EXPORT BASE_DECLARE_FEATURE(kUserBlockingAboveNormalPriority);
BASE_EXPORT BASE_DECLARE_FEATURE(kRetryCreateFileMappingOnCommitLimit);

BASE_EXPORT BASE_DECLARE_FEATURE(kPreventReparsePointTraversal);

// Controls whether FilePersistentMemoryAllocator calls ::FlushFileBuffers()
// on the underlying file handle for synchronous flushes.
BASE_EXPORT BASE_DECLARE_FEATURE(kFlushFileBuffersOnSyncFlush);
#endif

#if BUILDFLAG(IS_POSIX)
BASE_EXPORT BASE_DECLARE_FEATURE(kBaseLockTrySpin);
#if defined(ARCH_CPU_X86_FAMILY)
BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(int, kSpinCountX86);
#elif defined(ARCH_CPU_ARM_FAMILY)
BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(int, kSpinCountArm);
#endif  // defined(ARCH_CPU_X86_FAMILY)
#endif  // BUILDFLAG(IS_POSIX)

// Returns whether `kI18nOptimizations` is enabled. Reads experiment state from
// a global atomic updated in `Init()`, so it is safe to call before
// `FeatureList` initialization.
BASE_EXPORT bool IsI18nOptimizationsEnabled();

// Initializes global variables that depend on `FeatureList`. Must be invoked
// early on process startup, but after `FeatureList` initialization. Different
// parts of //base read experiment state from global variables instead of
// directly from `FeatureList` to avoid data races (default values are used
// before this function is called to initialize the global variables).
BASE_EXPORT void Init();

}  // namespace base::features

#endif  // BASE_FEATURES_H_
