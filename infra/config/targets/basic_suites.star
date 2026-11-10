# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Basic suite definitions

Basic suites are a collection of tests that can be referenced by a builder in
//testing/buildbot/waterfalls.pyl or by compound suites and matrix compound
suites (defined in ./compound_suites.star and ./matrix_compound_suites.star
respectively). Suites also define a bundle containing the same tests as the
suite, so they can be used wherever a bundle is expected.

The legacy_ prefix denotes the ability for basic suites to be referenced in
//testing/buildbot. Once a suite is no longer referenced via //testing/buildbot,
targets.bundle can be used for grouping tests in a more flexible manner (mixing
test types and/or compile targets and arbitrary nesting). Named bundles are
defined in ./bundles.star.
"""

load("@chromium-luci//targets.star", "targets")

targets.legacy_basic_suite(
    name = "blink_unittests_suite",
    tests = {
        "blink_unittests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "chrome_finch_smoke_tests",
    tests = {
        "variations_desktop_smoke_tests": targets.legacy_test_config(
        ),
    },
)

targets.legacy_basic_suite(
    name = "chrome_isolated_script_tests",
    tests = {
        "chrome_sizes": targets.legacy_test_config(),
        "variations_smoke_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "chrome_private_code_test_isolated_scripts",
    tests = {
        "chrome_private_code_test": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "chrome_sizes_suite",
    tests = {
        "chrome_sizes": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "chromium_web_tests_high_dpi_isolated_scripts",
    tests = {
        # high_dpi_blink_web_tests provides coverage for
        # running Layout Tests with forced device scale factor.
        "high_dpi_blink_web_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 1,
            ),
        ),
        # high_dpi_blink_wpt_tests provides coverage for
        # running Layout Tests with forced device scale factor.
        "high_dpi_blink_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 1,
            ),
        ),
        # high_dpi_headless_shell_wpt_tests provides coverage for
        # running WPTs with forced device scale factor.
        "high_dpi_headless_shell_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "chromium_blink_isolated_scripts",
    tests = {
        "blink_web_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 5,
            ),
        ),
        "blink_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "chrome_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 1,
            ),
        ),
        "headless_shell_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 18,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "clang_tot_gtests",
    tests = {
        "base_unittests": targets.legacy_test_config(),
        "highway_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "client_v8_chromium_gtests",
    tests = {
        "browser_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 10,
            ),
        ),
        "chrome_app_unittests": targets.legacy_test_config(),
        "chromedriver_unittests": targets.legacy_test_config(),
        "components_browsertests": targets.legacy_test_config(),
        "components_unittests": targets.legacy_test_config(),
        "compositor_unittests": targets.legacy_test_config(),
        "content_browsertests": targets.legacy_test_config(),
        "content_unittests": targets.legacy_test_config(),
        "device_unittests": targets.legacy_test_config(),
        "extensions_unittests": targets.legacy_test_config(),
        "gcm_unit_tests": targets.legacy_test_config(),
        "gin_unittests": targets.legacy_test_config(),
        "google_apis_unittests": targets.legacy_test_config(),
        "gpu_unittests": targets.legacy_test_config(),
        "headless_browsertests": targets.legacy_test_config(),
        "headless_unittests": targets.legacy_test_config(),
        "interactive_ui_tests": targets.legacy_test_config(),
        "net_unittests": targets.legacy_test_config(),
        "pdf_unittests": targets.legacy_test_config(),
        "remoting_unittests": targets.legacy_test_config(),
        "services_unittests": targets.legacy_test_config(),
        "sync_integration_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
        "unit_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "client_v8_chromium_isolated_scripts",
    tests = {
        "content_shell_crash_test": targets.legacy_test_config(),
        "telemetry_gpu_unittests": targets.legacy_test_config(
            swarming = targets.swarming(
                idempotent = False,  # https://crbug.com/549140
            ),
        ),
        "telemetry_perf_unittests": targets.legacy_test_config(
            args = [
                "--xvfb",
                # TODO(crbug.com/40129085): Remove this once Crashpad is the default.
                "--extra-browser-args=--enable-crashpad",
            ],
            swarming = targets.swarming(
                shards = 12,
                idempotent = False,  # https://crbug.com/549140
            ),
            resultdb = targets.resultdb(
                enable = True,
            ),
        ),
        "telemetry_unittests": targets.legacy_test_config(
            args = [
                "--jobs=1",
                # Disable GPU compositing, telemetry_unittests runs on VMs.
                # https://crbug.com/871955
                "--extra-browser-args=--disable-gpu",
            ],
            swarming = targets.swarming(
                shards = 4,
                idempotent = False,  # https://crbug.com/549140
            ),
            resultdb = targets.resultdb(
                enable = True,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "devtools_browser_tests_suite",
    tests = {
        "devtools_browser_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 1,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "devtools_web_isolated_scripts",
    tests = {
        "blink_web_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 5,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "finch_smoke_tests",
    tests = {
        # TODO(crbug.com/40189140): Change this to the actual finch smoke test
        # once it exists.
        "base_unittests": targets.legacy_test_config(),
    },
)

# BEGIN tests which run on the GPU bots

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_context_lost_passthrough_graphite_tests",
    tests = {
        "context_lost_passthrough_graphite_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_expected_color_pixel_passthrough_graphite_test",
    tests = {
        "expected_color_pixel_passthrough_graphite_test": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_info_collection_tests",
    tests = {
        "info_collection_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_pixel_skia_gold_passthrough_graphite_test",
    tests = {
        "pixel_skia_gold_passthrough_graphite_test": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_screenshot_sync_passthrough_graphite_tests",
    tests = {
        "screenshot_sync_passthrough_graphite_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_trace_test",
    tests = {
        "trace_test": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_vulkan_pixel_skia_gold_test",
    tests = {
        "vulkan_pixel_skia_gold_test": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_webcodecs_tests",
    tests = {
        "webcodecs_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_webgl_conformance_tests",
    tests = {
        "webgl_conformance_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_webgl2_conformance_gl_passthrough_tests",
    tests = {
        "webgl2_conformance_gl_passthrough_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    # TODO(crbug.com/554055689): Remove this thin wrapper and directly use the
    # underlying test once V8 is migrated to use Starlark test specs.
    name = "legacy_webrtc_tests",
    tests = {
        "webrtc_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "gpu_metal_passthrough_graphite_telemetry_tests",
    tests = {
        "context_lost_metal_passthrough_graphite_tests": targets.legacy_test_config(),
        "expected_color_pixel_metal_passthrough_graphite_test": targets.legacy_test_config(),
        "gpu_process_launch_tests": targets.legacy_test_config(),
        "hardware_accelerated_feature_tests": targets.legacy_test_config(),
        "pixel_skia_gold_metal_passthrough_graphite_test": targets.legacy_test_config(),
        "screenshot_sync_metal_passthrough_graphite_tests": targets.legacy_test_config(),
    },
)

# TODO(crbug.com/541312843): Migrate non-GPU uses of this to include tests
# directly and remove this.
targets.legacy_basic_suite(
    name = "gpu_passthrough_telemetry_tests",
    tests = {
        "context_lost_passthrough_tests": targets.legacy_test_config(),
        "expected_color_pixel_passthrough_test": targets.legacy_test_config(),
        "gpu_process_launch_tests": targets.legacy_test_config(),
        "hardware_accelerated_feature_tests": targets.legacy_test_config(),
        "pixel_skia_gold_passthrough_test": targets.legacy_test_config(),
        "screenshot_sync_passthrough_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "gpu_webcodecs_metal_passthrough_graphite_telemetry_test",
    tests = {
        "webcodecs_metal_passthrough_graphite_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "gpu_webrtc_metal_passthrough_graphite_telemetry_test",
    tests = {
        "webrtc_metal_passthrough_graphite_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "gpu_webgl2_conformance_d3d11_passthrough_telemetry_tests",
    tests = {
        "webgl2_conformance_d3d11_passthrough_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                # These tests currently take about an hour and fifteen minutes
                # to run. Split them into roughly 5-minute shards.
                shards = 20,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "gpu_webgl2_conformance_metal_passthrough_graphite_telemetry_tests",
    tests = {
        "webgl2_conformance_metal_passthrough_graphite_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 20,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "gpu_webgl_conformance_d3d11_passthrough_telemetry_tests",
    tests = {
        "webgl_conformance_d3d11_passthrough_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "gpu_webgl_conformance_metal_passthrough_graphite_telemetry_tests",
    tests = {
        "webgl_conformance_metal_passthrough_graphite_tests": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "gpu_webgl_conformance_swangle_passthrough_representative_telemetry_tests",
    tests = {
        "webgl_conformance_swangle_passthrough_tests": targets.legacy_test_config(
            args = [
                # On dual-GPU devices we want the high-performance GPU to be active
                "--extra-browser-args=--force_high_performance_gpu",
                # We are only interested in running a 'smoketest' to test swangle
                # integration, not the full conformance suite.
                "--test-filter=conformance/rendering/gl-drawelements.html",
            ],
            swarming = targets.swarming(
                shards = 1,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "linux_specific_chromium_isolated_scripts",
    tests = {
        # not_site_per_process_blink_web_tests provides coverage for
        # running Layout Tests without site-per-process.  This is the mode used
        # on Android and Android bots currently do not run the full set of
        # layout tests.  Running in this mode on linux compensates for lack of
        # direct Android coverage.
        "not_site_per_process_blink_web_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 8,
            ),
        ),
        # not_site_per_process_blink_wpt_tests provides coverage for
        # running WPTs without site-per-process.  This is the mode used
        # on Android and Android bots currently do not run the full set of
        # layout tests.  Running in this mode on linux compensates for lack of
        # direct Android coverage.
        "not_site_per_process_blink_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "not_site_per_process_headless_shell_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 11,
            ),
        ),
        "webdriver_wpt_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "model_validation_tests_light_suite",
    tests = {
        "model_validation_tests_light": targets.legacy_test_config(
            mixins = [
                "has_native_resultdb_integration",
            ],
            args = [
                "--out_dir=.",
            ],
            linux_args = [
                "--use-xvfb",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "model_validation_tests_suite",
    tests = {
        "model_validation_tests": targets.legacy_test_config(
            mixins = [
                "has_native_resultdb_integration",
            ],
            args = [
                "--out_dir=.",
            ],
            linux_args = [
                "--chromedriver",
                "chromedriver",
                "--binary",
                "chrome",
                "--use-xvfb",
            ],
            mac_args = [
                "--chromedriver",
                "chromedriver",
                "--binary",
                "Google Chrome.app/Contents/MacOS/Google Chrome",
            ],
            win_args = [
                "--chromedriver",
                "chromedriver.exe",
                "--binary",
                "Chrome.exe",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "ondevice_api_scrape_tests_suite",
    tests = {
        "ondevice_api_scrape_tests": targets.legacy_test_config(
            mixins = [
                "has_native_resultdb_integration",
            ],
            linux_args = [
                "--chromedriver",
                "chromedriver",
                "--binary",
                "chrome",
                "--no-xvfb",
            ],
            mac_args = [
                "--chromedriver",
                "chromedriver",
                "--binary",
                "Google Chrome.app/Contents/MacOS/Google Chrome",
            ],
            win_args = [
                "--chromedriver",
                "chromedriver.exe",
                "--binary",
                "Chrome.exe",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "ondevice_stability_tests_light_suite",
    tests = {
        "ondevice_stability_tests_light": targets.legacy_test_config(
            mixins = [
                "has_native_resultdb_integration",
            ],
            linux_args = [
                "--no-xvfb",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "ondevice_stability_tests_suite",
    tests = {
        "ondevice_stability_tests": targets.legacy_test_config(
            mixins = [
                "has_native_resultdb_integration",
            ],
            linux_args = [
                "--chromedriver",
                "chromedriver",
                "--binary",
                "chrome",
                "--no-xvfb",
            ],
            mac_args = [
                "--chromedriver",
                "chromedriver",
                "--binary",
                "Google Chrome.app/Contents/MacOS/Google Chrome",
            ],
            win_args = [
                "--chromedriver",
                "chromedriver.exe",
                "--binary",
                "Chrome.exe",
            ],
        ),
        "ondevice_stability_tests_light": targets.legacy_test_config(
            mixins = [
                "has_native_resultdb_integration",
            ],
            linux_args = [
                "--no-xvfb",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "litert_e2e_tests_gpu_suite",
    tests = {
        "litert_e2e_tests_gpu": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "litert_e2e_tests_cpu_suite",
    tests = {
        "litert_e2e_tests_cpu": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "litert_lm_advanced_main_legacy_tests_cpu_suite",
    tests = {
        "litert_lm_advanced_main_legacy_tests_cpu": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "litert_lm_advanced_main_legacy_tests_gpu_suite",
    tests = {
        "litert_lm_advanced_main_legacy_tests_gpu": targets.legacy_test_config(),
    },
)

targets.legacy_basic_suite(
    name = "openscreen_chromium_gtests",
    tests = {
        "cast_unittests": targets.legacy_test_config(),
        "components_unittests": targets.legacy_test_config(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "media_unittests": targets.legacy_test_config(),
        "openscreen_unittests": targets.legacy_test_config(),
    },
)

_CHROME_AI_WPT_TEST_CONFIG = targets.legacy_test_config(
    mixins = [
        "has_native_resultdb_integration",
        "blink_tests_write_run_histories",
    ],
    # Hardcoded '--child-processes=1' to enforce sequential execution by default
    # and prevent timeouts. The slower x64 builders are sharded to 8 shards
    # using the 'x64_ai_wpt_shards' mixin to compensate for sequential
    # execution.
    args = [
        "--release",
        "--timeout-multiplier=5",
        "--child-processes=1",
    ],
    mac_args = [
        "--driver-name",
        "Google Chrome",
    ],
    swarming = targets.swarming(
        shards = 4,
    ),
)

_CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG = targets.legacy_test_config(
    mixins = [
        "has_native_resultdb_integration",
        "blink_tests_write_run_histories",
    ],
    # Hardcoded '--child-processes=1' to enforce sequential execution by default
    # and prevent timeouts. The slower x64 builders are sharded to 8 shards
    # using the 'x64_ai_wpt_shards' mixin to compensate for sequential
    # execution.
    args = [
        "--release",
        "--timeout-multiplier=5",
        "--child-processes=1",
    ],
    # Lower minimum VRAM requirement to 5000 MB for Windows AI WPT runner bots
    # equipped with NVIDIA GeForce GTX 1660 GPUs (which report 5981 MB VRAM,
    # slightly below the default 6000 MB threshold).
    win_args = [
        "--exit-after-n-crashes-or-timeouts=2",
        "--additional-driver-flag=--enable-features=OnDeviceModelGpuAudioInput:on_device_model_audio_input_vram_min/5000",
    ],
    mac_args = [
        "--driver-name",
        "Google Chrome",
    ],
    swarming = targets.swarming(
        shards = 4,
    ),
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu": _CHROME_AI_WPT_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_high_tier": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_low_tier": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gemma4_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu_gemma4": _CHROME_AI_WPT_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_cpu_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_cpu_gemma4_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu_gemma4": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_high_tier_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_high_tier": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_low_tier_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_low_tier": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu_conversation": _CHROME_AI_WPT_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_conversation": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_conversation": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_cpu_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu_conversation": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_high_tier_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_conversation": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_low_tier_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_conversation": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gemma4_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu_gemma4_conversation": _CHROME_AI_WPT_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_conversation": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_conversation": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_cpu_gemma4_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_cpu_gemma4_conversation": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_conversation": _CHROME_AI_WPT_GPU_HIGH_TIER_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_conversation_suite",
    tests = {
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_conversation": _CHROME_AI_WPT_TEST_CONFIG,
    },
)

targets.legacy_basic_suite(
    name = "optimization_guide_nogpu_gtests",
    tests = {
        "chrome_ml_unittests": targets.legacy_test_config(
            linux_args = [
                "--use-xvfb",
            ],
        ),
        "optimization_guide_unittests": targets.legacy_test_config(
            linux_args = [
                "--use-xvfb",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "optimization_guide_gpu_gtests",
    tests = {
        "optimization_guide_gpu_unittests": targets.legacy_test_config(
            args = [
                "--ui-test-action-timeout=30000",
            ],
            linux_args = [
                "--use-xvfb",
            ],
        ),
        "optimization_guide_services_unittests": targets.legacy_test_config(
            args = [
                "--ui-test-action-timeout=30000",
            ],
            linux_args = [
                "--no-xvfb",
            ],
        ),
    },
)

# TODO: crbug.com/433525769 - When builders using this suite are all migrated to
# starlark, this should be combined with optimization_guide_gpu_gtests.
targets.legacy_basic_suite(
    name = "optimization_guide_gpu_isolated_scripts",
    tests = {
        "blink_wpt_tests": targets.legacy_test_config(
            args = [
                # Ensure that the platform-specific backends are disabled so
                # that TFLite is used.
                "--additional-driver-flag=--disable-features=WebNNCoreML,WebNNDirectML,WebNNOnnxRuntime",
                "--ignore-default-expectations",
                "--additional-expectations=../../third_party/blink/web_tests/OptimizationGuideExpectations",
                "--test-launcher-filter-file=../../third_party/blink/web_tests/TestLists/optimization_guide.filter",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "translatekit_script_tests",
    tests = {
        "chrome_wpt_tests": targets.legacy_test_config(
            args = [
                "-t",
                "Release",
                "external/wpt/ai/translator",
            ],
        ),
    },
)

targets.legacy_basic_suite(
    name = "webrtc_chromium_wpt_tests",
    tests = {
        "blink_wpt_tests": targets.legacy_test_config(
            args = [
                "-t",
                "Release",
                "external/wpt/webrtc",
                "external/wpt/webrtc-encoded-transform",
                "external/wpt/webrtc-extensions",
                "external/wpt/webrtc-priority",
                "external/wpt/webrtc-stats",
                "external/wpt/webrtc-svc",
            ],
        ),
        "headless_shell_wpt_tests": targets.legacy_test_config(
            args = [
                "-t",
                "Release",
                "external/wpt/webrtc",
                "external/wpt/webrtc-encoded-transform",
                "external/wpt/webrtc-extensions",
                "external/wpt/webrtc-priority",
                "external/wpt/webrtc-stats",
                "external/wpt/webrtc-svc",
            ],
            swarming = targets.swarming(
                shards = 4,
            ),
        ),
    },
)

targets.legacy_basic_suite(
    name = "win_specific_isolated_scripts",
    tests = {
        "mini_installer_tests": targets.legacy_test_config(
            swarming = targets.swarming(
                dimensions = {
                    "integrity": "high",
                },
            ),
        ),
        "polymer_tools_python_unittests": targets.legacy_test_config(
            experiment_percentage = 0,
        ),
    },
)
