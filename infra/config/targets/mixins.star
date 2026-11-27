# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Mixin declarations

Mixins are used to define common properties that can be applied to multiple
tests.
"""

load("@chromium-luci//targets.star", "targets")

targets.mixin(
    name = "amd_radeon_rx_5500_xt",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "1002:7340",
        },
    ),
)

targets.mixin(
    name = "arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
        },
    ),
)

targets.mixin(
    name = "blink_tests_write_run_histories",
    args = [
        "--write-run-histories-to=${ISOLATED_OUTDIR}/run_histories.json",
    ],
)

targets.mixin(
    name = "chrome-finch-swarming-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chrome.tests.finch",
        },
    ),
)

targets.mixin(
    name = "chrome-intelligence-swarming-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chrome.tests.intelligence",
        },
    ),
)

targets.mixin(
    name = "chrome-swarming-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chrome.tests",
        },
    ),
)

targets.mixin(
    name = "chrome-tester-service-account",
    swarming = targets.swarming(
        service_account = "chrome-tester@chops-service-accounts.iam.gserviceaccount.com",
    ),
)

targets.mixin(
    name = "chromium_tests_pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chromium.tests",
        },
    ),
)

targets.mixin(
    name = "chromium-tester-dev-service-account",
    swarming = targets.swarming(
        service_account = "chromium-tester-dev@chops-service-accounts.iam.gserviceaccount.com",
    ),
)

targets.mixin(
    name = "chromium-tester-service-account",
    swarming = targets.swarming(
        service_account = "chromium-tester@chops-service-accounts.iam.gserviceaccount.com",
    ),
)

# Used for invert CQ tests selection. Adding ci_only: False to
# test_suite_exceptions.pyl to select tests that are allowed on CQ builders.
targets.mixin(
    name = "ci_only",
    ci_only = True,
)

targets.mixin(
    name = "experiments",
    experiment_percentage = 100,
)

targets.mixin(
    name = "chromium-tests-oslogin",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chromium.tests.oslogin",
        },
    ),
)

targets.mixin(
    name = "dawn_end2end_gpu_test",
    args = [
        "--use-gpu-in-tests",
        "--exclusive-device-type-preference=discrete,integrated",
        # Dawn test retries deliberately disabled to prevent flakiness.
        "--test-launcher-retry-limit=0",
        # Reduces size of stdout of a batch crbug.com/1456415
        "--test-launcher-batch-limit=512",
    ],
)

targets.mixin(
    name = "disable_field_trial_config_for_earl_grey",
    args = [
        "--extra-app-args=--disable-field-trial-config",
    ],
)

targets.mixin(
    name = "docker",
    swarming = targets.swarming(
        dimensions = {
            "inside_docker": "1",
        },
    ),
)

# Used to pin tests that exhibit timing sensitivities on faster machine types
# (e.g. e4-standard-8) to e2-standard-8.
targets.mixin(
    name = "e2-standard-8",
    swarming = targets.swarming(
        dimensions = {
            "machine_type": "e2-standard-8",
        },
    ),
)

# In //testing/buildbot, what test type a test is expanded as depends on the
# test_suites key that the builder puts it under, so gtests included under the
# isolated_scripts suite type would get expanded as isolated scripts. In
# starlark, tests know what type they are and their type determines how they are
# expanded, which allows tests of different types to in the same bundle. This
# mixin enables using gtests as isolated script tests.
targets.mixin(
    name = "expand-as-isolated-script",
    expand_as_isolated_script = True,
)

targets.mixin(
    name = "finch-chromium-swarming-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chromium.tests.finch",
        },
    ),
)

targets.mixin(
    name = "upload_inv_extended_properties",
    resultdb = targets.resultdb(
        enable = True,
        inv_extended_properties_dir = "${ISOLATED_OUTDIR}/invocations",
    ),
)

targets.mixin(
    name = "gce",
    swarming = targets.swarming(
        dimensions = {
            "gce": "1",
        },
    ),
)

targets.mixin(
    name = "non-gce",
    swarming = targets.swarming(
        dimensions = {
            "gce": "0",
        },
    ),
)

targets.mixin(
    name = "gpu_gtest_common_args",
)

targets.mixin(
    # TODO(crbug.com/554055689): Remove this mixin and use an anonymous mixin
    # in the test definition once V8 uses Starlark test specs.
    name = "legacy_gpu_vulkan_pixel_skia_gold_test_args",
    args = [
        "--extra-browser-args=--use-vulkan=native --disable-vulkan-fallback-to-gl-for-testing --enable-features=Vulkan --use-gl=angle --use-angle=gl --use-cmd-decoder=passthrough",
    ],
)

targets.mixin(
    # TODO(crbug.com/554055689): Remove this mixin and use an anonymous mixin
    # in the test definition once V8 uses Starlark test specs.
    name = "legacy_gpu_webgl_conformance_shards",
    swarming = targets.swarming(
        shards = 2,
    ),
)

targets.mixin(
    # TODO(crbug.com/554055689): Remove this mixin and use an anonymous mixin
    # in the test definition once V8 uses Starlark test specs.
    name = "legacy_gpu_webgl2_conformance_shards",
    swarming = targets.swarming(
        shards = 5,
    ),
)

targets.mixin(
    name = "gpu_integration_test_common_args",
    args = [
        targets.magic_args.GPU_PARALLEL_JOBS,
    ],
)

targets.mixin(
    name = "gpu_integration_test_expected_color_args",
    args = [
        "--dont-restore-color-profile-after-test",
        "--test-machine-name",
        "${buildername}",
    ],
)

targets.mixin(
    name = "gpu_integration_test_pixel_args",
    args = [
        "--dont-restore-color-profile-after-test",
        "--test-machine-name",
        "${buildername}",
    ],
)

targets.mixin(
    name = "gpu_integration_test_screenshot_sync_args",
    args = [
        "--dont-restore-color-profile-after-test",
    ],
)

targets.mixin(
    name = "gpu_integration_test_webgl1_args",
    args = [
        targets.magic_args.GPU_WEBGL_RUNTIME_FILE,
    ],
)

targets.mixin(
    name = "gpu_integration_test_webgl2_args",
    args = [
        "--webgl-conformance-version=2.0.1",
        targets.magic_args.GPU_WEBGL_RUNTIME_FILE,
    ],
)

targets.mixin(
    name = "gpu_linux_gce_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "none",
            "os": "Ubuntu-22.04",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "gpu_win_gce_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "none",
            "os": "Windows-10-19045",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "gpu_amd_rx_5500_xt_linux_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "display_server": "x11",
            "gpu": "1002:7340-25.2.8",
            "os": "Ubuntu-24.04",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "gpu_amd_rx_9070_xt_linux_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "display_server": "x11",
            "gpu": "1002:7550-25.2.8",
            "os": "Ubuntu-24.04",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "gpu_amd_rx_9070_xt_win_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "1002:7550-32.0.31007.5012",
            "os": "Windows-11",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "gpu_nvidia_rtx_5080_linux_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "display_server": "x11",
            "gpu": "10de:2c02-595.58.03",
            "os": "Ubuntu-24.04",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "gpu_nvidia_rtx_5080_win_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "10de:2c02-32.0.15.9597",
            "os": "Windows-11",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "gpu-swarming-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "gpu_enable_metal_debug_layers",
    args = [
        "--enable-metal-debug-layers",
    ],
)

targets.mixin(
    name = "gpu_force_angle_d3d11",
    args = [
        "--extra-browser-args=--use-angle=d3d11",
    ],
)

targets.mixin(
    name = "gpu_force_angle_gles",
    args = [
        "--extra-browser-args=--use-angle=gles",
    ],
)

targets.mixin(
    name = "gpu_force_angle_gl",
    args = [
        "--extra-browser-args=--use-angle=gl",
    ],
)

targets.mixin(
    name = "gpu_force_angle_metal",
    args = [
        "--extra-browser-args=--use-angle=metal",
    ],
)

targets.mixin(
    name = "gpu_force_angle_swiftshader",
    args = [
        "--extra-browser-args=--use-angle=swiftshader",
    ],
)

targets.mixin(
    name = "gpu_force_angle_vulkan",
    args = [
        "--extra-browser-args=--use-angle=vulkan",
    ],
)

targets.mixin(
    name = "gpu_force_command_decoder_validating",
    args = [
        "--extra-browser-args=--use-cmd-decoder=validating",
    ],
)

targets.mixin(
    name = "gpu_force_command_decoder_passthrough",
    args = [
        "--extra-browser-args=--use-cmd-decoder=passthrough --use-gl=angle",
    ],
)

# On dual-GPU devices this forces high performance GPU to be used.
targets.mixin(
    name = "gpu_force_high_performance_gpu",
    args = [
        "--extra-browser-args=--force_high_performance_gpu",
    ],
)

# On dual-GPU devices this forces high performance GPU to be used by WebGL
# while everything else uses the low performance GPU. ANGLE/Metal only.
targets.mixin(
    name = "gpu_force_high_performance_gpu_for_webgl_metal",
    args = [
        "--extra-browser-args=--enable-features=EGLDualGPURendering,ForceHighPerformanceGPUForWebGL",
    ],
)

targets.mixin(
    name = "gpu_force_skia_ganesh",
    args = [
        "--extra-browser-args=--disable-skia-graphite",
    ],
)

targets.mixin(
    name = "gpu_force_skia_graphite",
    args = [
        "--extra-browser-args=--enable-skia-graphite",
    ],
)

targets.mixin(
    name = "gpu_force_webgl_on_webgpu",
    args = [
        # --enable-unsafe-webgpu bypasses the WebGPU adapter blocklist so the
        # suite can run on bots whose GPU/driver isn't allowlisted yet.
        "--extra-browser-args=--enable-features=WebGLOnWebGPU --enable-unsafe-webgpu",
    ],
    linux_args = [
        # Linux defaults to GL SharedImages, and webgpu_on_vk_via_gl_interop is
        # blocklisted on AMD and Intel < Gen12. Enable Vulkan so Dawn can get a
        # WebGPU adapter on all Linux GPU bots (Telemetry merges multiple
        # --enable-features flags).
        "--extra-browser-args=--enable-features=Vulkan",
    ],
)

# Use of this mixin signals to the recipe that the test uploads its results
# to result-sink and doesn't need to be wrapped by result_adapter.
targets.mixin(
    name = "has_native_resultdb_integration",
    resultdb = targets.resultdb(
        enable = True,
        # TODO(crbug.com/40740370): Remove the 'enable' field in favor of
        # 'has_native_resultdb_integration'.
        has_native_resultdb_integration = True,
    ),
)

targets.mixin(
    name = "integrity_high",
    swarming = targets.swarming(
        dimensions = {
            "integrity": "high",
        },
    ),
)

targets.mixin(
    name = "intel_uhd_630_or_770",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "8086:9bc5|8086:4680",
        },
    ),
)

targets.mixin(
    name = "isolate_profile_data",
    isolate_profile_data = True,
)

targets.mixin(
    name = "junit-swarming-emulator",
    swarming = targets.swarming(
        dimensions = {
            "cores": "8",
            "pool": "chromium.tests",
        },
    ),
)

targets.mixin(
    name = "limited_capacity_bot",
    # Some FYI bot configurations have a limited number of bots in the swarming
    # pool. Increase the default expiration_sec time from 1 hour to 6 hours to
    # prevent shards from timing out.
    swarming = targets.swarming(
        expiration_sec = 21600,
    ),
)

targets.mixin(
    name = "very_limited_capacity_bot",
    # Some FYI bot configurations have a very limited number of bots in the
    # swarming pool. Increase the default expiration_sec time from 1 hour to
    # 12 hours to prevent shards from timing out.
    swarming = targets.swarming(
        expiration_sec = 43200,
    ),
)

targets.mixin(
    name = "linux-focal",
    swarming = targets.swarming(
        dimensions = {
            "os": "Ubuntu-20.04",
        },
    ),
)

targets.mixin(
    name = "linux-jammy",
    swarming = targets.swarming(
        dimensions = {
            "os": "Ubuntu-22.04",
        },
    ),
)

targets.mixin(
    name = "linux-noble",
    swarming = targets.swarming(
        dimensions = {
            "os": "Ubuntu-24.04",
        },
    ),
)

targets.mixin(
    name = "linux-ubuntu",
    swarming = targets.swarming(
        dimensions = {
            "os": "Ubuntu",
        },
    ),
)

# Restricts Linux GPU High Tier AI WPT tasks to NVIDIA, AMD, or modern Intel GPUs
# (Intel UHD 770 / Iris Xe) to avoid execution stalls on low-end Intel UHD 630
# integrated graphics (lin-90-g582 and lin-91-g582).
targets.mixin(
    name = "linux_gpu_high_tier_ai_wpt_dimensions",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "10de|1002|8086:4680|8086:a780|8086:a7a0",
        },
    ),
)

# Restricts Linux GPU High Tier Gemma 4 AI WPT tasks to NVIDIA and AMD discrete
# GPUs to prevent VK_ERROR_DEVICE_LOST failures on Intel integrated graphics
# (see https://crbug.com/547936735).
targets.mixin(
    name = "linux_gpu_high_tier_gemma4_ai_wpt_dimensions",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "10de|1002",
        },
    ),
)

# Expands Windows GPU AI WPT tasks across both Windows 10 and Windows 11 x86-64
# discrete GPUs (NVIDIA, Intel Arc/UHD, and discrete AMD Radeon RX 5500 XT /
# 7600 / 7900 XTX / 9070 XT) while excluding integrated AMD APUs (1002:1638 and
# 1002:164e) that have < 4 GB dedicated VRAM and fail on-device GPU model
# preconditions.
targets.mixin(
    name = "win_gpu_ai_wpt_dimensions",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "10de|8086|1002:7340|1002:744c|1002:7480|1002:7550",
            "os": "Windows",
        },
    ),
)

# Restricts Windows GPU Gemma 4 AI WPT tasks to Windows 10 and Windows 11 x86-64
# NVIDIA discrete GPUs to prevent D3D12 vision compilation crashes on AMD
# integrated graphics (see https://crbug.com/547946848).
targets.mixin(
    name = "win_gpu_gemma4_ai_wpt_dimensions",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "10de",
            "os": "Windows",
        },
    ),
)

targets.mixin(
    name = "linux_amd_780m_experimental",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "1002:1900-25.2.2",
            "os": "Ubuntu-24.04",
            "display_attached": "1",
            "display_server": "x11",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "linux_amd_890m_experimental",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "1002:150e-25.0.7",
            "os": "Ubuntu-24.04",
            "display_attached": "1",
            "display_server": "x11",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "linux_amd_rx_5500_xt",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "1002:7340-25.2.8",
            "os": "Ubuntu-24.04",
            "display_attached": "1",
            "display_server": "x11",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_amd_rx_5500_xt_wayland_stable",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "1002:7340-25.2.8",
            "os": "Ubuntu-24.04",
            "display_attached": "1",
            "display_server": "wayland",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_amd_rx_7600_stable",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "1002:7480-25.0.7",
            "os": "Ubuntu-24.04",
            "display_attached": "1",
            "display_server": "x11",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_intel_uhd_630_experimental",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "8086:9bc5-23.2.1",
            "os": "Ubuntu-22.04.5",
            "display_attached": "1",
            "display_server": "x11",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_intel_uhd_630_stable",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "8086:9bc5-23.2.1",
            "os": "Ubuntu-22.04",
            "display_server": "x11",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_intel_uhd_770_stable",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "8086:4680-23.2.1",
            "os": "Ubuntu-22.04",
            "display_attached": "1",
            "display_server": "x11",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_intel_uhd_770_wayland_stable",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "8086:4680-26.0.8",
            "os": "Ubuntu-26.04",
            "display_attached": "1",
            "display_server": "wayland",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_nvidia_gtx_1660_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_server": "x11",
            "gpu": "10de:2184-595.104.02",
            "os": "Ubuntu-24.04",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_nvidia_gtx_1660_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_server": "x11",
            "gpu": "10de:2184-535.183.01|10de:2184-595.104.02",
            "os": "Ubuntu-22.04|Ubuntu-24.04",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_nvidia_rtx_4070_super_wayland_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "display_server": "wayland",
            "gpu": "10de:2783-595.91.07",
            "os": "Ubuntu-26.04",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "linux_vulkan",
    linux_args = [
        "--extra-browser-args=--enable-features=Vulkan",
    ],
)

targets.mixin(
    name = "mac_vm",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "Apple_(Virtual)",
            "os": "Mac",
            "pool": "chromium.tests.macvm",
        },
    ),
)

targets.mixin(
    name = "mac_26_vm_optional",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",  # fallback on bare metal if no VMs are available
            "os": "Mac-26",
        },
        optional_dimensions = {
            180: {
                "cpu": "Apple_(Virtual)",
            },
        },
    ),
)

targets.mixin(
    name = "mac_10.15",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-10.15",
        },
    ),
)

targets.mixin(
    name = "mac_11_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-11",
        },
    ),
)

targets.mixin(
    name = "mac_11_x64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-11",
        },
    ),
)

targets.mixin(
    name = "mac_12_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-12",
        },
    ),
)

targets.mixin(
    name = "mac_12_x64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-12",
        },
    ),
)

targets.mixin(
    name = "mac_13_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-13",
        },
    ),
)

targets.mixin(
    name = "mac_13_x64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-13",
        },
    ),
)

targets.mixin(
    name = "mac_14_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-14",
        },
    ),
)

targets.mixin(
    name = "mac_14_x64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-14",
        },
    ),
)

targets.mixin(
    name = "mac_15_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-15",
        },
    ),
)

targets.mixin(
    name = "mac_27_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-27",
        },
    ),
)

targets.mixin(
    name = "mac_26_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-26",
        },
    ),
)

targets.mixin(
    name = "mac_26_x64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-26",
        },
    ),
)

targets.mixin(
    name = "mac_15_x64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-15",
        },
    ),
)

targets.mixin(
    name = "mac_arm64_apple_m1_gpu_experimental",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "gpu": "apple:m1",
            "mac_model": "Macmini9,1",
            "os": "Mac-15.7",
            "pool": "chromium.tests",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_arm64_apple_m1_gpu_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "gpu": "apple:m1",
            "mac_model": "Macmini9,1",
            "os": "Mac-15.7",
            "pool": "chromium.tests",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_arm64_apple_m2_retina_gpu_experimental",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "gpu": "apple:m2",
            "mac_model": "Mac14,7",
            "os": "Mac-26.5.1",
            "pool": "chromium.tests.gpu",
            "display_attached": "1",
            "hidpi": "1",
        },
    ),
)

targets.mixin(
    name = "mac_arm64_apple_m2_retina_gpu_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "gpu": "apple:m2",
            "mac_model": "Mac14,7",
            "os": "Mac-26.5.1",
            "pool": "chromium.tests.gpu",
            "display_attached": "1",
            "hidpi": "1",
        },
    ),
)

targets.mixin(
    name = "mac_arm64_apple_m3_retina_gpu_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "gpu": "apple:m3",
            "mac_model": "Mac15,3",
            "os": "Mac-15.4.1",
            "pool": "chromium.tests.gpu",
            "display_attached": "1",
            "hidpi": "1",
        },
    ),
)

targets.mixin(
    name = "mac_beta_x64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "os": "Mac-26",
        },
    ),
)

# mac_default_arm64 is used as a prefered OS dimension for mac platform
# instead of any mac OS version. It selects the most representative
# dimension on Swarming.
targets.mixin(
    name = "mac_default_arm64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "arm64",
            "os": "Mac-26",
        },
    ),
)

targets.mixin(
    name = "mac_mini_intel_gpu_experimental",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "8086:3e9b",
            "os": "Mac-15.5",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_mini_intel_gpu_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "8086:3e9b",
            "os": "Mac-15.5",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_pro_amd_gpu",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "1002:679e",
            "os": "Mac-12.7",
            "pool": "chromium.tests.gpu",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_retina_amd_555x_gpu_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "1002:67ef",
            "hidpi": "1",
            "os": "Mac-14.4.1",
            "pool": "chromium.tests.gpu",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_retina_amd_gpu_experimental",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "1002:7340",
            "hidpi": "1",
            "os": "Mac-14.4.1",
            "pool": "chromium.tests.gpu",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_retina_amd_gpu_stable",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "1002:7340",
            "hidpi": "1",
            "os": "Mac-14.4.1",
            "pool": "chromium.tests.gpu",
            "display_attached": "1",
        },
    ),
)

targets.mixin(
    name = "mac_toolchain",
    swarming = targets.swarming(
        cipd_packages = [
            targets.cipd_package(
                package = "infra/tools/mac_toolchain/${platform}",
                location = ".",
                revision = "git_revision:bc2eb815d4eb1d66007cc1f19dcbe57f59a76b2d",
            ),
        ],
    ),
)

targets.mixin(
    name = "no_gpu",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "none",
        },
    ),
)

# Used to pin tests with large CAS inputs to non-SSD GCE bots (which have
# 300 GB disks) rather than SSD-backed e4 bots (which have 50 GB disks).
targets.mixin(
    name = "no_ssd",
    swarming = targets.swarming(
        dimensions = {
            "ssd": "0",
        },
    ),
)

targets.mixin(
    name = "no_xvfb_for_real_hardware",
    linux_args = [
        targets.magic_args.GPU_NO_XVFB_FOR_REAL_HARDWARE,
    ],
)

targets.mixin(
    name = "nvidia_geforce_gtx_1660",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "10de:2184",
        },
    ),
)

targets.mixin(
    name = "out_dir_arg",
    args = [
        "--out-dir",
        "${ISOLATED_OUTDIR}",
    ],
)

targets.mixin(
    name = "puppet_production",
    swarming = targets.swarming(
        dimensions = {
            "puppet_env": "production",
        },
    ),
)

targets.mixin(
    name = "record_failed_tests",
    args = [
        "--record-video",
        "failed_only",
    ],
)

targets.mixin(
    name = "skia_gold_test",
    args = [
        "--git-revision=${got_revision}",
        # BREAK GLASS IN CASE OF EMERGENCY
        # Uncommenting this argument will bypass all interactions with Skia
        # Gold in any tests that use it. This is meant as a temporary
        # emergency stop in case of a Gold outage that's affecting the bots.
        # "--bypass-skia-gold-functionality",
    ],
    precommit_args = [
        "--gerrit-issue=${patch_issue}",
        "--gerrit-patchset=${patch_set}",
        "--buildbucket-id=${buildbucket_build_id}",
    ],
)

targets.mixin(
    # Tests that reach out to Skia's gold instance slowdown substantially when
    # ran on GCE bots without external IPs. By explicitly targeting bots with
    # external IPs in such tests, we can roll out internal IPs more broadly
    # without affecting these Skia gold tests.
    name = "skia_gold_test_on_linux_gce",
    swarming = targets.swarming(
        dimensions = {
            "gce_has_external_ip": "1",
        },
    ),
)

targets.mixin(
    name = "swarming_containment_auto",
    swarming = targets.swarming(
        containment_type = "AUTO",
    ),
)

targets.mixin(
    name = "timeout_15m",
    swarming = targets.swarming(
        hard_timeout_sec = 900,
        io_timeout_sec = 900,
    ),
)

targets.mixin(
    name = "timeout_30m",
    swarming = targets.swarming(
        hard_timeout_sec = 1800,
        io_timeout_sec = 1800,
    ),
)

targets.mixin(
    name = "updater-default-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chromium.tests",
        },
    ),
)

targets.mixin(
    name = "updater-tests-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chromium.tests.updater",
        },
    ),
)

targets.mixin(
    name = "updater-win-uac-pool",
    swarming = targets.swarming(
        dimensions = {
            "pool": "chromium.win.uac",
        },
    ),
)

targets.mixin(
    name = "use_isolated_scripts_api",
    use_isolated_scripts_api = True,
)

targets.mixin(
    name = "vaapi_unittest_args",
    args = [
        "--stop-ui",
        "--gtest_filter=\"VaapiTest.*\"",
    ],
)

targets.mixin(
    name = "vaapi_unittest_libfake_args",
    args = [
        # Tell libva to do dummy encoding/decoding. For more info, see:
        # https://github.com/intel/libva/blob/v2.14-branch/va/va_fool.c#L52
        "--env-var",
        "LIBVA_DRIVERS_PATH",
        "./",
        "--env-var",
        "LIBVA_DRIVER_NAME",
        "libfake",
    ],
)

# Common options for gathering code coverage or other FYI data. Run all tests
# exactly once, and don't fail the suite for unexpected results.
targets.mixin(
    name = "web-test-coverage",
    args = [
        "--skipped=ignore",
        "--no-retry-failures",
        "--no-expectations",
    ],
)

targets.mixin(
    name = "web-test-leak",
    args = [
        "--additional-expectations",
        "../../third_party/blink/web_tests/LeakExpectations",
        "--enable-leak-detection",
    ],
)

targets.mixin(
    name = "webgpu_cts",
    args = [
        # crbug.com/953991 Ensure WebGPU is ready before running tests
        "--initialize-webgpu-adapter-at-startup-timeout-ms=60000",
    ],
    linux_args = [
        "--no-xvfb",
        "--additional-driver-flag=--enable-features=Vulkan",
    ],
    merge = targets.merge(
        script = "//third_party/blink/tools/merge_web_test_results.py",
        args = [
            "--verbose",
        ],
    ),
    resultdb = targets.resultdb(
        enable = True,
    ),
)

targets.mixin(
    name = "webgpu_telemetry_cts",
    args = [
        "--extra-browser-args=--force_high_performance_gpu",
        "--use-webgpu-power-preference=default-high-performance",
    ],
)

targets.mixin(
    name = "win10",
    swarming = targets.swarming(
        dimensions = {
            "os": "Windows-10-19045",
        },
    ),
)

targets.mixin(
    name = "win10-any",
    swarming = targets.swarming(
        dimensions = {
            "os": "Windows-10",
        },
    ),
)

targets.mixin(
    name = "win11_amd_780m_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "1002:1900-32.0.21025.10016",
            "os": "Windows-11-26100",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "win11_amd_890m_experimental",
    swarming = targets.swarming(
        dimensions = {
            "gpu": "1002:150e-32.0.21025.10016",
            "os": "Windows-11-26100",
            "display_attached": "1",
            "pool": "chromium.tests.gpu.experimental",
        },
    ),
)

targets.mixin(
    name = "win11_amd_rx_5500_xt_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "1002:7340-32.0.21037.1004",
            "os": "Windows-11-26100",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win11_amd_rx_5500_xt_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "1002:7340-32.0.21037.1004",
            "os": "Windows-11-26100",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win11_amd_rx_7600_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "1002:7480-32.0.12033.1030",
            "os": "Windows-11-26100",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win11_qualcomm_snapdragon_x_elite_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "screen_scaling_percent": "100",
            "cpu": "arm64",
            "gpu": "qcom:0c36-31.0.121.1",
            "os": "Windows-11-26100",
            "pool": "chromium.tests.gpu",
        },
    ),
)

# TODO(crbug.com/479147014): Remove this mixin once all uses have switched to
# gpu_win_gce_stable.
targets.mixin(
    name = "win10_gce_gpu_pool",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
            "gpu": "none",
            "os": "Windows-10",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win10_intel_uhd_630_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "8086:9bc5-31.0.101.2127",
            "os": "Windows-10",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win10_intel_uhd_630_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "8086:9bc5-31.0.101.2127",
            "os": "Windows-10",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win10_intel_uhd_770_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "8086:4680-31.0.101.5333",
            "os": "Windows-10-19045",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win10_nvidia_gtx_1660_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "10de:2184-32.0.16.1074",
            "os": "Windows-11-26200",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win10_nvidia_gtx_1660_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "10de:2184-31.0.15.4601|10de:2184-32.0.16.1074",
            "os": "Windows-10-19045|Windows-11-26200",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win11_nvidia_rtx_4070_super_experimental",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "10de:2783-32.0.15.8129",
            "os": "Windows-11",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win11_nvidia_rtx_4070_super_stable",
    swarming = targets.swarming(
        dimensions = {
            "display_attached": "1",
            "gpu": "10de:2783-32.0.15.8129",
            "os": "Windows-11",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "win11",
    swarming = targets.swarming(
        dimensions = {
            "os": "Windows-11-22631",
        },
    ),
)

targets.mixin(
    name = "win11-any",
    swarming = targets.swarming(
        dimensions = {
            "os": "Windows-11",
        },
    ),
)

targets.mixin(
    name = "win-arm64",
    swarming = targets.swarming(
        dimensions = {
            # Certain tests require 100 percent screen scaling, and all devices
            # should be configured for this.
            "screen_scaling_percent": "100",
            "cpu": "arm64",
            "os": "Windows-11",
        },
        # win-arm64 is a limited pool with ~100 bots that can be easily
        # overloaded by few builds. Increase the expiration_sec to 2h to prevent
        # shards from timing out.
        expiration_sec = 7200,
    ),
)

# Shards the slower x64 bot to 8 shards (overriding the default of 4 shards).
# Since these bots run sequentially to avoid resource starvation, they
# take longer to complete the test suite, requiring more shards to keep
# the total run time within the builder's limit.
targets.mixin(
    name = "x64_ai_wpt_shards",
    swarming = targets.swarming(
        shards = 8,
    ),
)

targets.mixin(
    name = "x86-64",
    swarming = targets.swarming(
        dimensions = {
            "cpu": "x86-64",
        },
    ),
)

targets.mixin(
    name = "xcode_16_main",
    args = [
        "--xcode-build-version",
        "16c5032a",
    ],
    swarming = targets.swarming(
        named_caches = [
        ],
    ),
)

targets.mixin(
    name = "xcode_27_beta",
    args = [
        "--xcode-build-version",
        "27c5020n",
    ],
    swarming = targets.swarming(
        named_caches = [
        ],
    ),
)

targets.mixin(
    name = "xcode_27_main",
    args = [
        "--xcode-build-version",
        "27a266a",
    ],
    swarming = targets.swarming(
        named_caches = [
        ],
    ),
)
