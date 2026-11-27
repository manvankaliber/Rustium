// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_finch_features.h"

#include <string_view>

#include "base/byte_size.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/synchronization/atomic_flag.h"
#include "build/build_config.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/config/gpu_switches.h"
#include "ui/gl/gl_features.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/gl_utils.h"


namespace features {
namespace {

SkiaGraphiteFeatureParams g_skia_graphite_feature_params;

base::AtomicFlag& GetGraphiteParamsInitFlag() {
  static base::NoDestructor<base::AtomicFlag> flag;
  return *flag;
}

void InitSkiaGraphiteFeatureParams(const base::Feature* feature) {
  if (GetGraphiteParamsInitFlag().IsSet()) {
    return;
  }

  if (!feature) {
    g_skia_graphite_feature_params = SkiaGraphiteFeatureParams();
    GetGraphiteParamsInitFlag().Set();
    return;
  }

  g_skia_graphite_feature_params.dawn_skip_validation =
      base::FeatureParam<bool>(
          feature, "dawn_skip_validation",
          g_skia_graphite_feature_params.dawn_skip_validation)
          .Get();
  g_skia_graphite_feature_params.dawn_backend_validation =
      base::FeatureParam<bool>(
          feature, "dawn_backend_validation",
          g_skia_graphite_feature_params.dawn_backend_validation)
          .Get();
  g_skia_graphite_feature_params.dawn_backend_debug_labels =
      base::FeatureParam<bool>(
          feature, "dawn_backend_debug_labels",
          g_skia_graphite_feature_params.dawn_backend_debug_labels)
          .Get();
  g_skia_graphite_feature_params.dawn_enable_auto_map =
      base::FeatureParam<bool>(
          feature, "dawn_enable_auto_map",
          g_skia_graphite_feature_params.dawn_enable_auto_map)
          .Get();
  g_skia_graphite_feature_params.max_pending_recordings =
      base::FeatureParam<int>(
          feature, "max_pending_recordings",
          g_skia_graphite_feature_params.max_pending_recordings)
          .Get();
  g_skia_graphite_feature_params.max_time_between_submits =
      base::FeatureParam<base::TimeDelta>(
          feature, "max_time_between_submits",
          g_skia_graphite_feature_params.max_time_between_submits)
          .Get();
  g_skia_graphite_feature_params.enable_deferred_submit =
      base::FeatureParam<bool>(
          feature, "enable_deferred_submit",
          g_skia_graphite_feature_params.enable_deferred_submit)
          .Get();
  g_skia_graphite_feature_params.enable_msaa_on_newer_intel =
      base::FeatureParam<bool>(
          feature, "enable_msaa_on_newer_intel",
          g_skia_graphite_feature_params.enable_msaa_on_newer_intel)
          .Get();
#if BUILDFLAG(IS_WIN)
  g_skia_graphite_feature_params.dawn_dumpwc_d3d_errors =
      base::FeatureParam<bool>(
          feature, "dawn_dumpwc_d3d_errors",
          g_skia_graphite_feature_params.dawn_dumpwc_d3d_errors)
          .Get();
  g_skia_graphite_feature_params.dawn_disable_d3d_shader_optimizations =
      base::FeatureParam<bool>(
          feature, "dawn_disable_d3d_shader_optimizations",
          g_skia_graphite_feature_params.dawn_disable_d3d_shader_optimizations)
          .Get();
  g_skia_graphite_feature_params.dawn_d3d11_delay_flush =
      base::FeatureParam<bool>(
          feature, "dawn_d3d11_delay_flush",
          g_skia_graphite_feature_params.dawn_d3d11_delay_flush)
          .Get();
  g_skia_graphite_feature_params.flush_d3d11_tile_raster_commands_to_driver =
      base::FeatureParam<bool>(
          feature, "flush_d3d11_tile_raster_commands_to_driver",
          g_skia_graphite_feature_params
              .flush_d3d11_tile_raster_commands_to_driver)
          .Get();
  g_skia_graphite_feature_params.triple_buffered_dcomp_root_surface =
      base::FeatureParam<bool>(
          feature, "triple_buffered_dcomp_root_surface",
          g_skia_graphite_feature_params.triple_buffered_dcomp_root_surface)
          .Get();
#endif

  GetGraphiteParamsInitFlag().Set();
}


}  // namespace

// More aggressive behavior for the shader cache: increase size, and do not
// purge as much in case of memory pressure.
BASE_FEATURE(kAggressiveShaderCacheLimits, base::FEATURE_DISABLED_BY_DEFAULT);


// When enabled, gives GpuChannel/Host its own dedicated Mojo pipe instead
// of associating with an unused IPC::Channel.
BASE_FEATURE(kRemoveGPULegacyIPC, base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_LINUX)
// Feature flag to control whether SharedImageStub sequence uses high priority
// on ChromeOS and Linux. Enabled by default.
BASE_FEATURE(kSharedImageStubHighPriority, base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Enable GPU Rasterization by default. This can still be overridden by
// --enable-gpu-rasterization or --disable-gpu-rasterization.
// DefaultEnableGpuRasterization has launched on Mac, Windows, ChromeOS,
// Android and Linux.
BASE_FEATURE(kDefaultEnableGpuRasterization,
#if BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_WIN) || BUILDFLAG(USE_WEBGPU_ON_VULKAN_VIA_GL_INTEROP)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

// Enables the use of MSAA in skia on Ice Lake and later intel architectures.

BASE_FEATURE(kEnableMSAAOnNewIntelGPUs,
             base::FEATURE_DISABLED_BY_DEFAULT
);

#if BUILDFLAG(IS_WIN)
BASE_FEATURE(kNoUndamagedOverlayPromotion, base::FEATURE_DISABLED_BY_DEFAULT);
#endif

#if BUILDFLAG(IS_MAC)
// If enabled, the TASK_CATEGORY_POLICY value of the GPU process will be
// adjusted to match the one from the browser process every time it changes.
BASE_FEATURE(kAdjustGpuProcessPriority, base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// When enabled, Grshader disk cache will be cleared on startup if any cache
// entry prefix does not match with the current prefix. prefix is made up of
// various parameters like chrome version, driver version etc.
BASE_FEATURE(kClearGrShaderDiskCacheOnInvalidPrefix,
             base::FEATURE_DISABLED_BY_DEFAULT);

// When enabled, the GPU process initializes through GpuInit2. GpuInit2 is where
// GPU startup optimizations are being developed.
BASE_FEATURE(kGpuInitOptimization, base::FEATURE_DISABLED_BY_DEFAULT);

// When enabled, Chrome will use the shader disk cache. This feature provides a
// kill-switch for working around issues with the disk cache and assessing the
// performance value of the disk cache. The --disable-gpu-shader-disk-cache flag
// overrides this feature and forces the disk cache to be disabled.
BASE_FEATURE(kGpuShaderDiskCache, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsShaderDiskCacheEnabled(const base::CommandLine* command_line) {
  if (command_line->HasSwitch(switches::kDisableGpuShaderDiskCache)) {
    return false;
  }

  return base::FeatureList::IsEnabled(kGpuShaderDiskCache);
}

// Force enable WebGPU interop when enabled. When disabled the webgpu interop
// mechanism will default to auto detection in 'GetWebGPUOnVulkanViaGLInterop'
// function.
BASE_FEATURE(kForceEnableWebGpuInterop, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kEnableDrDc,
#if BUILDFLAG(IS_MAC)
             // DrDC will not be running if Graphite is disabled on Mac.
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             // NOT SUPPORTED. DO NOT ENABLE!
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

// Enable WebGPU on gpu service side only. This is used with origin trial and
// enabled by default on supported platforms.
#if BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_WIN) || BUILDFLAG(USE_WEBGPU_ON_VULKAN_VIA_GL_INTEROP)
#define WEBGPU_ENABLED base::FEATURE_ENABLED_BY_DEFAULT
#else
#define WEBGPU_ENABLED base::FEATURE_DISABLED_BY_DEFAULT
#endif
BASE_FEATURE(kWebGPUService, WEBGPU_ENABLED);
BASE_FEATURE(kWebGPUBlobCache, WEBGPU_ENABLED);
#undef WEBGPU_ENABLED

// Feature enforces WebGPU security in Android Advanced Protection Mode.
// Disable feature by default for Finch testing.
BASE_FEATURE(kAAPMBlocksWebGPU, base::FEATURE_ENABLED_BY_DEFAULT);

// List of Dawn toggles for WebGPU, delimited by ,
// The FeatureParam may be overridden via Finch config, or via the command line
// For example:
//   --enable-field-trial-config \
//   --force-fieldtrial-params=WebGPU.Enabled:DisabledToggles/toggle1%2Ctoggle2
// Note that the comma should be URL-encoded.
const base::FeatureParam<std::string> kWebGPUDisabledToggles{
    &kWebGPUService, "DisabledToggles", ""};
const base::FeatureParam<std::string> kWebGPUEnabledToggles{
    &kWebGPUService, "EnabledToggles", ""};
// List of WebGPU feature names, delimited by ,
// The FeatureParam may be overridden via Finch config, or via the command line
// For example:
//   --enable-field-trial-config \
//   --force-fieldtrial-params=WebGPU.Enabled:UnsafeFeatures/timestamp-query%2Cshader-f16
// Note that the comma should be URL-encoded.
const base::FeatureParam<std::string> kWebGPUUnsafeFeatures{
    &kWebGPUService, "UnsafeFeatures", ""};
// List of WGSL feature names, delimited by ,
// The FeatureParam may be overridden via Finch config, or via the command line
// For example:
//   --enable-field-trial-config \
//   --force-fieldtrial-params=WebGPU.Enabled:UnsafeWGSLFeatures/feature_1%2Cfeature_2
// Note that the comma should be URL-encoded.
const base::FeatureParam<std::string> kWGSLUnsafeFeatures{
    &kWebGPUService, "UnsafeWGSLFeatures", ""};

BASE_FEATURE(kWebGPUEnableRangeAnalysisForRobustness,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kWebGPUUseSpirv14, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kWebGPUUseHLSL2021, base::FEATURE_ENABLED_BY_DEFAULT);

// Enables the use of dedicated shared memory transfer buffers for mapped WebGPU
// buffers. Only takes effect when explicitly enabled (e.g. via
// --enable-features=WebGPUUseDedicatedTransferBuffer).
BASE_FEATURE(kWebGPUUseDedicatedTransferBuffer,
             base::FEATURE_DISABLED_BY_DEFAULT);


// Enable Skia Graphite with the platform's default Dawn backend.
// Note: This can be overridden by --enable-skia-graphite and
// --disable-skia-graphite which take precedence over the feature flag, and the
// Dawn backend can be overridden with the --skia-graphite-dawn-backend flag.
BASE_FEATURE(kSkiaGraphite,
#if BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_WIN)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

// Controls Skia Graphite specifically for Intel GPUs on Windows.
// On Windows, the status of Graphite on Intel GPUs won't be controlled
// by the standard SkiaGraphite feature, but by this feature flag
// instead. This feature only works if `kLateGraphiteFeatureCheck` is
// also enabled.
BASE_FEATURE(kSkiaGraphiteWinIntel,
             base::FEATURE_DISABLED_BY_DEFAULT);

// Allows CompoundImageBacking to allocate backings during runtime if a
// compatible backing to serve clients requested usage is not already present.
BASE_FEATURE(kUseDynamicBackingAllocations, base::FEATURE_ENABLED_BY_DEFAULT);

// When enabled, this feature allows ClientSharedImage to store and use a
// scoped_refptr to SharedImageInterface, instead of the raw_ptr as used in
// SharedImageInterfaceHolder.
BASE_FEATURE(kUseStrongRefToSharedImageInterface,
             base::FEATURE_ENABLED_BY_DEFAULT
);

// When enabled, this feature lets ClientSharedImage handle all SyncToken
// management (i.e. generation, waiting and storing) internally. All SyncTokens
// that clients obtain from ClientSharedImage will be empty in this situation.
BASE_FEATURE(kUseAutomaticSyncTokenManagement,
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enable atlasing of small paths on Skia Graphite. Only meaningful if
// SkiaGraphite is also enabled.
BASE_FEATURE(kSkiaGraphiteSmallPathAtlas, base::FEATURE_DISABLED_BY_DEFAULT);

// When enabled, the Graphite feature check (including blocklist) is deferred to
// the GPU process rather than evaluated in the browser process.
BASE_FEATURE(kLateGraphiteFeatureCheck,
#if BUILDFLAG(IS_WIN)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

// Enable Skia Graphite's Pipeline precompilation feature.
// Note: This is only meaningful when Skia Graphite is enabled but can then also
// be overridden by
// --enable-skia-graphite-precompilation and
// --disable-skia-graphite-precompilation.
BASE_FEATURE(kSkiaGraphitePrecompilation, base::FEATURE_DISABLED_BY_DEFAULT);

// Whether to use PersistentCache for Skia Graphite's pipeline cache.
BASE_FEATURE(kSkiaGraphiteUsePersistentCache,
#if BUILDFLAG(IS_WIN)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

bool SkiaGraphiteUsesPersistentCache() {
  return base::FeatureList::IsEnabled(kSkiaGraphiteUsePersistentCache);
}

// Enables switching Graphite from the sort-based draw ordering to the
// layer-based system.
BASE_FEATURE(kSkiaGraphiteDrawListLayer, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kConditionallySkipGpuChannelFlush,
// To enable on ChromeOS, test failures must be investigated
// (crrev.com/c/5435673).
             base::FEATURE_ENABLED_BY_DEFAULT
);

const SkiaGraphiteFeatureParams& GetSkiaGraphiteFeatureParams() {
  DCHECK(GetGraphiteParamsInitFlag().IsSet());
  return g_skia_graphite_feature_params;
}

void InitSkiaGraphiteDefaultParamsForTesting() {
  InitSkiaGraphiteFeatureParams(nullptr);
}

const base::FeatureParam<int> kSkiaGraphiteMinPathSizeForMsaa{
    &kSkiaGraphiteSmallPathAtlas, "min_path_size_for_msaa", 0};

// Whether to use the GpuPersistentCache for caching GPU process shader blobs.
// Usage for Graphite is controlled independently with
// kSkiaGraphiteDawnUsePersistentCache.
BASE_FEATURE(kGpuPersistentCache,
#if BUILDFLAG(IS_WIN)
             base::FEATURE_DISABLED_BY_DEFAULT);
#else
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif

BASE_FEATURE(kGpuPersistentCacheMetadata, base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<int> kGpuPersistentCacheMetadataPreloadCount{
    &kGpuPersistentCacheMetadata, "preload_count", 50};

BASE_FEATURE(kGpuPersistentCacheAsyncInitialize,
             base::FEATURE_DISABLED_BY_DEFAULT);

// Use a 100-command limit before forcing context switch per command buffer
// instead of 20.
BASE_FEATURE(kIncreasedCmdBufferParseSlice, base::FEATURE_DISABLED_BY_DEFAULT);

// Prune transfer cache entries not accessed recently. This also turns off
// similar logic in cc::GpuImageDecodeCache which is the largest (often single)
// client of transfer cache.
BASE_FEATURE(kPruneOldTransferCacheEntries, base::FEATURE_ENABLED_BY_DEFAULT);

// On platforms with delegated compositing, try to release overlays later, when
// no new frames are swapped.
BASE_FEATURE(kDeferredOverlaysRelease,
             "DeferredOverlayRelease",
             base::FEATURE_ENABLED_BY_DEFAULT);

// This feature allows enabling specific entries in
// software_rendering_list.json, via experimentation. The entries must have
// test_group property and test_group feature parameter should be set in the
// experiment for the entries that need to be enabled.
BASE_FEATURE(kGPUBlockListTestGroup, base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<int> kGPUBlockListTestGroupId{&kGPUBlockListTestGroup,
                                                       "test_group", 0};

// This feature allows enabling specific entries in gpu_driver_bug_list.json,
// via experimentation. The entries must have test_group property and
// test_group feature parameter should be set in the experiment for the entries
// that need to be enabled.
BASE_FEATURE(kGPUDriverBugListTestGroup, base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<int> kGPUDriverBugListTestGroupId{
    &kGPUDriverBugListTestGroup, "test_group", 0};

#if BUILDFLAG(IS_LINUX)
bool IsForceEnableWebGpuInterop() {
  return base::FeatureList::IsEnabled(kForceEnableWebGpuInterop);
}
#endif

bool IsUsingThreadSafeMediaForWebView() {
  return false;
}

// Note that DrDc is also disabled on some of the gpus (crbug.com/1354201).
// Thread safe media will still be used on those gpus which should be fine for
// now as the lock shouldn't have much overhead and is limited to only few gpus.
// This should be fixed/updated later to account for disabled gpus.
bool NeedThreadSafeAndroidMedia() {
  // If GpuFeatureInfo is available, replace ShouldEnableDrDc() with
  // IsDrDcEnabled(gpu_feature_info) which is set after checking drdc
  // workarounds;
  return ShouldEnableDrDc() || IsUsingThreadSafeMediaForWebView();
}

namespace {
bool IsSkiaGraphiteSupportedByDevice() {
#if BUILDFLAG(IS_APPLE)
  return true;
#elif BUILDFLAG(IS_WIN) && defined(ARCH_CPU_ARM64)
  // Graphite on Windows ARM requires further research.
  return false;
#elif BUILDFLAG(IS_WIN)
  return true;
#else
  // Disallow Graphite from being enabled via the base::Feature on
  // not-yet-supported platforms to avoid users experiencing undefined
  // behavior, including behavior that might prevent them from being able to
  // return to chrome://flags to disable the feature.
  if (base::FeatureList::IsEnabled(features::kSkiaGraphite)) {
    LOG(ERROR) << "Enabling Graphite on a not-yet-supported platform is "
                  "disallowed for safety";
  }
  return false;
#endif
}
}  // namespace

bool IsSkiaGraphiteEnabled(const base::CommandLine* command_line) {
  // Force disabling graphite if --disable-skia-graphite flag is specified.
  if (command_line->HasSwitch(switches::kDisableSkiaGraphite)) {
    return false;
  }

  // Force Graphite on if --enable-skia-graphite flag is specified.
  if (command_line->HasSwitch(switches::kEnableSkiaGraphite)) {
    // The flag enable-skia-graphite is for testing purpose, so set the params
    // to default values.
    InitSkiaGraphiteFeatureParams(nullptr);
    return true;
  }

  if (!IsSkiaGraphiteSupportedByDevice()) {
    // Return early before checking "SkiaGraphite" feature so that devices
    // which don't support graphite are not included in the finch study.
    return false;
  }

  if (base::FeatureList::IsEnabled(kSkiaGraphite)) {
    InitSkiaGraphiteFeatureParams(&kSkiaGraphite);
    return true;
  }
  return false;
}

bool IsSkiaGraphiteWinIntelEnabled() {
  if (base::FeatureList::IsEnabled(kSkiaGraphiteWinIntel)) {
    InitSkiaGraphiteFeatureParams(&kSkiaGraphiteWinIntel);
    return true;
  }
  return false;
}

bool IsDrDcEnabled(const gpu::GpuFeatureInfo& gpu_feature_info) {
  return gpu_feature_info.status_values
             [gpu::GPU_FEATURE_TYPE_DIRECT_RENDERING_DISPLAY_COMPOSITOR] ==
         gpu::kGpuFeatureStatusEnabled;
}

bool ShouldEnableDrDc() {

  return base::FeatureList::IsEnabled(kEnableDrDc);
}

bool IsSkiaGraphitePrecompilationEnabled(
    const base::CommandLine* command_line) {
  // Force disabling Graphite Precompilation if
  // --disable-skia-graphite-precompilation flag is specified.
  if (command_line->HasSwitch(switches::kDisableSkiaGraphitePrecompilation)) {
    return false;
  }

  // Force Graphite Precompilation on if --enable-skia-graphite-precompilation
  // flag is specified.
  if (command_line->HasSwitch(switches::kEnableSkiaGraphitePrecompilation)) {
    return true;
  }

  return base::FeatureList::IsEnabled(features::kSkiaGraphitePrecompilation);
}

bool EnablePruneOldTransferCacheEntries() {
  return base::FeatureList::IsEnabled(kPruneOldTransferCacheEntries);
}

bool IsLegacyIpcDisabled() {
  return base::FeatureList::IsEnabled(kRemoveGPULegacyIPC);
}


// When this flag is enabled, stops using gpu::SyncPointOrderData for sync point
// validation, uses gpu::TaskGraph instead.
// Graph-based validation doesn't require sync point releases are submitted to
// the scheduler prior to their corresponding waits. Therefore it allows to
// remove the synchronous flush done by VerifySyncTokens().
//
// TODO(b/324276400): Work in progress.
BASE_FEATURE(kSyncPointGraphValidation, base::FEATURE_DISABLED_BY_DEFAULT);

bool IsSyncPointGraphValidationEnabled() {
  return base::FeatureList::IsEnabled(kSyncPointGraphValidation);
}

BASE_FEATURE(kANGLEPerContextBlobCache, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kANGLETrimMemory, base::FEATURE_ENABLED_BY_DEFAULT);

// Controls whether WebGL contexts trim their ANGLE memory caches when idle.
BASE_FEATURE(kANGLETrimMemoryOnIdle, base::FEATURE_DISABLED_BY_DEFAULT);

// Support thread safety for graphite::context by sharing the same
// graphite::context as well as its wrapper class GraphiteSharedContext between
// GpuMain and CompositorGpuThread. Note: When this feature is disabled,
// each thread creates its own graphite::context and the context wrapper.
BASE_FEATURE(kGraphiteContextIsThreadSafe,
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsGraphiteContextThreadSafe() {
  return base::FeatureList::IsEnabled(features::kGraphiteContextIsThreadSafe);
}

BASE_FEATURE(kWebGPUCompatibilityMode, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kWebGLDeleteBuffersInBackground,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kWebGPUAndroidOpenGLES, base::FEATURE_ENABLED_BY_DEFAULT);

#if BUILDFLAG(IS_WIN)
BASE_FEATURE(kWebGPUQualcommWindows, base::FEATURE_ENABLED_BY_DEFAULT);
#endif

// Enables runtime configuration of the GPU watchdog timeout via
// experimentation.
BASE_FEATURE(kConfigurableGPUWatchdogTimeout,
             base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<int> kConfigurableGPUWatchdogTimeoutSeconds{
    &kConfigurableGPUWatchdogTimeout, "watchdog_timeout_seconds", 30};

// Enables the optimization where GPU channels are sent to renderer processes
// early when the renderer process is being initialized, instead of waiting
// for the renderer to request the GPU channel to the browser process.
BASE_FEATURE(kSendGPUChannelEarly, base::FEATURE_DISABLED_BY_DEFAULT);
// If true, only enable the early GPU channel optimization for topchrome WebUI
// renderers.
const base::FeatureParam<bool> kSendGPUChannelEarlyTopChromeOnly{
    &kSendGPUChannelEarly, "for_topchrome_webui_only", false};

// Enables Ganesh/Vulkan MSAA support on Imagination (PowerVR) GPUs.
BASE_FEATURE(kVulkanImaginationMSAA, base::FEATURE_DISABLED_BY_DEFAULT);

// Kill switch for enforcing exclusive write access.
BASE_FEATURE(kWrappedSkiaBackingExclusiveAccess,
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
