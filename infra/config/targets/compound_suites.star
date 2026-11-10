# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compound suite declarations

Compound suites are a collection of basic suites that can be referenced by a
builder in //testing/buildbot/waterfalls.pyl. Suites also define a bundle
containing the same tests as the suite, so they can be used wherever a bundle is
expected.

The legacy_ prefix denotes the ability for compound suites to be referenced in
//testing/buildbot. Once a suite is no longer referenced via //testing/buildbot,
targets.bundle can be used for grouping tests in a more flexible manner (mixing
test types and/or compile targets and arbitrary nesting). Named bundles are
defined in ./bundles.star.
"""

load("@chromium-luci//targets.star", "targets")

targets.legacy_compound_suite(
    name = "chrome_linux_isolated_script_tests",
    basic_suites = [
        "chrome_isolated_script_tests",
        "chrome_private_code_test_isolated_scripts",
    ],
)

targets.legacy_compound_suite(
    name = "chromium_linux_blink_rel_isolated_scripts",
    basic_suites = [
        "chromium_blink_isolated_scripts",
        "linux_specific_chromium_isolated_scripts",
        "chromium_web_tests_high_dpi_isolated_scripts",
    ],
)

targets.legacy_compound_suite(
    name = "devtools_gtests",
    basic_suites = [
        "devtools_browser_tests_suite",
    ],
)

# BEGIN composition test suites used by the GPU bots

# Legacy copy of the gpu_all_linux_release_telemetry_tests bundle.
# TODO(crbug.com/554055689): Remove this once V8 is updated to use Starlark test
# specs.
targets.legacy_compound_suite(
    name = "legacy_gpu_all_linux_release_telemetry_tests",
    basic_suites = [
        "gpu_passthrough_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "legacy_info_collection_tests",
        "legacy_trace_test",
        "legacy_vulkan_pixel_skia_gold_test",
        "legacy_webcodecs_tests",
        "legacy_webgl_conformance_tests",
        "legacy_webgl2_conformance_gl_passthrough_tests",
        # TODO(crbug.com/541312843): Add `webrtc_tests` to make this actually
        # identical to gpu_all_linux_release_telemetry_tests.
    ],
)

targets.legacy_compound_suite(
    name = "gpu_fyi_only_mac_release_telemetry_tests",
    basic_suites = [
        "gpu_metal_passthrough_graphite_telemetry_tests",
        "gpu_webcodecs_metal_passthrough_graphite_telemetry_test",
        "gpu_webrtc_metal_passthrough_graphite_telemetry_test",
        "gpu_webgl2_conformance_metal_passthrough_graphite_telemetry_tests",
        "gpu_webgl_conformance_metal_passthrough_graphite_telemetry_tests",
        "gpu_webgl_conformance_swangle_passthrough_representative_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "legacy_info_collection_tests",
        "legacy_trace_test",
    ],
)

# This is functionally equivalent to the gpu_all_win_release_telemetry_tests
# bundle, but is defined separately since V8 needs to use legacy suites for now.
targets.legacy_compound_suite(
    name = "gpu_v8_win_release_telemetry_tests",
    basic_suites = [
        "gpu_passthrough_telemetry_tests",
        "gpu_webgl2_conformance_d3d11_passthrough_telemetry_tests",
        "gpu_webgl_conformance_d3d11_passthrough_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "legacy_context_lost_passthrough_graphite_tests",
        "legacy_expected_color_pixel_passthrough_graphite_test",
        "legacy_info_collection_tests",
        "legacy_pixel_skia_gold_passthrough_graphite_test",
        "legacy_screenshot_sync_passthrough_graphite_tests",
        "legacy_trace_test",
        "legacy_webcodecs_tests",
        "legacy_webrtc_tests",
    ],
)
