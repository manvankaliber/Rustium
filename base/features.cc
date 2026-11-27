// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/features.h"

#include <atomic>

#include "base/debug/stack_trace.h"
#include "base/files/file_path.h"
#include "base/message_loop/message_pump_wakeup_counter.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/task/sequence_manager/sequence_manager_impl.h"
#include "base/task/thread_pool/job_task_source.h"
#include "base/threading/platform_thread.h"
#include "build/blink_buildflags.h"
#include "build/build_config.h"
#include "build/buildflag.h"

#if BUILDFLAG(IS_LINUX)
#include "base/message_loop/message_pump_epoll.h"
#endif

#if BUILDFLAG(IS_APPLE)
#include "base/files/file.h"
#include "base/message_loop/message_pump_apple.h"
#include "base/synchronization/condition_variable.h"

#include "base/message_loop/message_pump_kqueue.h"

#endif


#if BUILDFLAG(IS_WIN)
#include "base/task/sequence_manager/thread_controller_power_monitor.h"
#endif

namespace base::features {

namespace {

// State of the `kI18nOptimizations` feature, to be updated after the feature
// list is available. All operations use `std::memory_order_relaxed` because
// there are no dependent memory accesses.
std::atomic_bool g_i18n_optimizations{true};

}  // namespace

// Alphabetical:

// Use Rust base64 (simd) for base::Base64Encode() and
// base::Base64EncodeAppend().
BASE_FEATURE(kBase64SimdBase64Encode, base::FEATURE_DISABLED_BY_DEFAULT);

// Controls caching within BASE_FEATURE_PARAM(). This is feature-controlled
// so that ScopedFeatureList can disable it to turn off caching.
BASE_FEATURE(kFeatureParamWithCache, FEATURE_ENABLED_BY_DEFAULT);

// Whether a fast implementation of FilePath::IsParent is used. This feature
// exists to ensure that the fast implementation can be disabled quickly if
// issues are found with it.
BASE_FEATURE(kFastFilePathIsParent, FEATURE_ENABLED_BY_DEFAULT);

// Enables residency tagging in the heap profiler.
BASE_FEATURE(kHeapProfilerIncludeResidency, FEATURE_DISABLED_BY_DEFAULT);

// Enables optimizations in //base/i18n (e.g. fast-pathing TimeFormatAsIso8601
// and avoiding ICU resource bundle lookups for TimeZone::GMT/Unknown).
BASE_FEATURE(kI18nOptimizations, FEATURE_ENABLED_BY_DEFAULT);

// Use non default low memory device threshold.
// Value should be given via |LowMemoryDeviceThresholdMB|.
// Updated Desktop default threshold to match the Android 2021 definition.
#define LOW_MEMORY_DEVICE_THRESHOLD_MB 2048
BASE_FEATURE(kLowEndMemoryExperiment, FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE_PARAM(int,
                   kLowMemoryDeviceThresholdMB,
                   &kLowEndMemoryExperiment,
                   LOW_MEMORY_DEVICE_THRESHOLD_MB);

// Controls whether lock acquisition times are recorded and reported by a
// given thread.
BASE_FEATURE(kRecordLockAcquisitionTime, FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE_PARAM(std::string,
                   kRecordLockAcquisitionTimeAllowedThreads,
                   &kRecordLockAcquisitionTime,
                   "RecordLockAcquisitionTimeAllowedThreads",
                   "CrBrowserMain,CrRendererMain,CrGpuMain,"
                   "VizCompositor,Compositor,BrowserIO,NetworkService,"
                   "GPU.ForegroundWorker,GPU.BackgroundWorker,"
                   "Renderer.ForegroundWorker,Renderer.BackgroundWorker,"
                   "Browser.ForegroundWorker,Browser.BackgroundWorker");

// Apply base::ScopedBestEffortExecutionFence to registered task queues as well
// as the thread pool.
BASE_FEATURE(kScopedBestEffortExecutionFenceForTaskQueue,
             base::FEATURE_DISABLED_BY_DEFAULT);

// Whether to restrict the max gap between the frame pointer and the stack end
// for stack scanning. If the gap is beyond the given gap threshold, the stack
// end is treated as unreliable. Stack scanning stops when that happens.
// This feature is only in effect when BUILDFLAG(CAN_UNWIND_WITH_FRAME_POINTERS)
// is on and `TraceStackFramePointers` would run stack scanning. Default gap
// threshold is an absurdly large 100MB.
// The feature is enabled by default on ChromeOS where crashes caused by
// unreliable stack end are found. See https://crbug.com/402542102
BASE_FEATURE(kStackScanMaxFramePointerToStackEndGap,
             FEATURE_DISABLED_BY_DEFAULT
);
BASE_FEATURE_PARAM(int,
                   kStackScanMaxFramePointerToStackEndGapThresholdMB,
                   &kStackScanMaxFramePointerToStackEndGap,
                   "StackScanMaxFramePointerToStackEndGapThresholdMB",
                   100);



// When enabled, GetTerminationStatus() returns
// TERMINATION_STATUS_EVICTED_FOR_MEMORY for processes terminated due to commit
// failures. Otherwise, it returns TERMINATION_STATUS_OOM.
BASE_FEATURE(kUseTerminationStatusMemoryExhaustion, FEATURE_ENABLED_BY_DEFAULT);

// Optimize text decoding by using FindFirstNonASCII to find and copy ASCII
// content.
BASE_FEATURE(kUtfConversionAsciiFastPath, FEATURE_ENABLED_BY_DEFAULT);

#if BUILDFLAG(IS_WIN)
// When enabled, use ABOVE_NORMAL_PRIORITY_CLASS for Priority::kUserBlocking on
// Windows.
BASE_FEATURE(kUserBlockingAboveNormalPriority, FEATURE_DISABLED_BY_DEFAULT);

// When enabled, retries CreateFileMapping on a commit limit failure (OOM).
// If retrying fails, the function returns failure as usual and reports the
// last error code.
BASE_FEATURE(kRetryCreateFileMappingOnCommitLimit, FEATURE_DISABLED_BY_DEFAULT);

// Prevents base::DeletePathRecursively on Windows from traversing NTFS reparse
// points (such as directory junctions). This protects against TOCTOU
// vulnerabilities and prevents deleting files outside the target directory.
BASE_FEATURE(kPreventReparsePointTraversal, FEATURE_ENABLED_BY_DEFAULT);

// When enabled, FilePersistentMemoryAllocator calls ::FlushFileBuffers() on the
// underlying file handle during synchronous flushes to commit the cache buffers
// to physical storage.
BASE_FEATURE(kFlushFileBuffersOnSyncFlush, FEATURE_DISABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_POSIX)
// If enabled, threads acquiring a base::Lock will try to acquire it in user
// space. The `kSpinCount` parameter represents the maximum number of pause
// instructions (yields) that will be executed with an exponential backoff
// before blocking in the kernel.
BASE_FEATURE(kBaseLockTrySpin, FEATURE_DISABLED_BY_DEFAULT);
#if defined(ARCH_CPU_X86_FAMILY)
BASE_FEATURE_PARAM(int, kSpinCountX86, &kBaseLockTrySpin, "spin_count_x86", 0);
#elif defined(ARCH_CPU_ARM_FAMILY)
BASE_FEATURE_PARAM(int, kSpinCountArm, &kBaseLockTrySpin, "spin_count_arm", 0);
#endif  // defined(ARCH_CPU_X86_FAMILY)
#endif  // BUILDFLAG(IS_POSIX)

bool IsI18nOptimizationsEnabled() {
  return g_i18n_optimizations.load(std::memory_order_relaxed);
}

void Init() {
  g_i18n_optimizations.store(FeatureList::IsEnabled(kI18nOptimizations),
                             std::memory_order_relaxed);
  strings_internal::InitializeUtfStringConversionsFeatures();
#if BUILDFLAG(IS_POSIX)
  base::Lock::InitializeFeatures();
#endif  // BUILDFLAG(IS_POSIX)

  sequence_manager::internal::SequenceManagerImpl::InitializeFeatures();
  sequence_manager::internal::ThreadController::InitializeFeatures();
  base::internal::JobTaskSource::InitializeFeatures();

  debug::StackTrace::InitializeFeatures();
  FilePath::InitializeFeatures();
  MessagePumpWakeupCounter::InitializeFeatures();

#if BUILDFLAG(IS_LINUX)
  MessagePumpEpoll::InitializeFeatures();
#endif

#if BUILDFLAG(IS_APPLE)
  PlatformThread::InitializeFeatures();
#endif

#if BUILDFLAG(IS_APPLE)
  MessagePumpCFRunLoopBase::InitializeFeatures();

// Kqueue is not used for ios blink.
  MessagePumpKqueue::InitializeFeatures();

#endif


#if BUILDFLAG(IS_WIN)
  sequence_manager::internal::ThreadControllerPowerMonitor::
      InitializeFeatures();
#endif
}

}  // namespace base::features
