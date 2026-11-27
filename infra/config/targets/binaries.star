# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Binary declarations

Binaries can be referenced by tests and define the label of the compile target
to be built as well as various aspects that the infrastructure needs to know in
order to run the binary.
"""

load("@chromium-luci//targets.star", "targets")

targets.binaries.console_test_launcher(
    name = "absl_hardening_tests",
    label = "//third_party/abseil-cpp:absl_hardening_tests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "accessibility_unittests",
    label = "//ui/accessibility:accessibility_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_egl_tests",
    label = "//third_party/angle/src/tests:angle_deqp_egl_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles2_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles2_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles31_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles3_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_gles2_tests",
    label = "//third_party/angle/src/tests:angle_deqp_khr_gles2_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_gles3_tests",
    label = "//third_party/angle/src/tests:angle_deqp_khr_gles3_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_gles31_tests",
    label = "//third_party/angle/src/tests:angle_deqp_khr_gles31_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_rotate180_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles3_rotate180_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_rotate270_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles3_rotate270_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_rotate90_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles3_rotate90_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_rotate180_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles31_rotate180_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_rotate270_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles31_rotate270_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_rotate90_tests",
    label = "//third_party/angle/src/tests:angle_deqp_gles31_rotate90_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "deqp",
)

targets.binaries.windowed_test_launcher(
    name = "angle_end2end_tests",
    label = "//third_party/angle/src/tests:angle_end2end_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "angle_unittests",
    label = "//third_party/angle/src/tests:angle_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "angle_white_box_tests",
    label = "//third_party/angle/src/tests:angle_white_box_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "aura_unittests",
    label = "//ui/aura:aura_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "base_junit_tests",
    label = "//base:base_junit_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "junit",
)

targets.binaries.script(
    name = "base_perftests",
    label = "//base:base_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "base_perftests",
        "--non-telemetry=true",
        "--test-launcher-print-test-stdio=always",
        "--test-launcher-jobs=1",
        "--test-launcher-retry-limit=0",
    ],
)

targets.binaries.console_test_launcher(
    name = "base_unittests",
    label = "//base:base_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "blink_common_unittests",
    label = "//third_party/blink/common:blink_common_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "blink_fuzzer_unittests",
    label = "//third_party/blink/renderer/platform:blink_fuzzer_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "blink_heap_unittests",
    label = "//third_party/blink/renderer/platform/heap:blink_heap_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "blink_platform_unittests",
    label = "//third_party/blink/renderer/platform:blink_platform_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "blink_python_tests",
    label = "//:blink_python_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "pyunit",
)

targets.binaries.script(
    name = "blink_pytype",
    label = "//third_party/blink/tools:blink_pytype",
    script = "//third_party/blink/tools/run_pytype.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "single",
)

targets.binaries.console_test_launcher(
    name = "blink_unittests",
    label = "//third_party/blink/renderer/controller:blink_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "blink_web_tests",
    label = "//:blink_web_tests",
    results_handler = "layout tests",
    args = [
        "--results-directory",
        "${ISOLATED_OUTDIR}",
    ],
    merge = targets.merge(
        script = "//third_party/blink/tools/merge_web_test_results.py",
        args = [
            "--verbose",
        ],
    ),
    module_scheme = "webtest",
)

targets.binaries.generated_script(
    name = "blink_wpt_tests",
    label = "//:blink_wpt_tests",
    results_handler = "layout tests",
    args = [
        "--results-directory",
        "${ISOLATED_OUTDIR}",
        "--test-launcher-filter-file=../../third_party/blink/web_tests/TestLists/content_shell.filter",
        "--inverted-test-launcher-filter-file=../../third_party/blink/web_tests/TestLists/chrome.filter",
    ],
    merge = targets.merge(
        script = "//third_party/blink/tools/merge_web_test_results.py",
        args = [
            "--verbose",
        ],
    ),
    module_scheme = "webtest",
)

targets.binaries.console_test_launcher(
    name = "boringssl_crypto_tests",
    label = "//third_party/boringssl:boringssl_crypto_tests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "boringssl_ssl_tests",
    label = "//third_party/boringssl:boringssl_ssl_tests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "browser_tests",
    label = "//chrome/test:browser_tests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "captured_sites_interactive_tests",
    label = "//chrome/test:captured_sites_interactive_tests",
    args = [
        "--disable-extensions",
    ],
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "capture_unittests",
    label = "//media/capture:capture_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "cast_unittests",
    label = "//media/cast:cast_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "cc_unittests",
    label = "//cc:cc_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "chrome_app_unittests",
    label = "//chrome/test:chrome_app_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "chrome_elf_unittests",
    label = "//chrome/chrome_elf:chrome_elf_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "chrome_ml_unittests",
    label = "//components/optimization_guide/internal:chrome_ml_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "chrome_private_code_test",
    label = "//chrome:chrome_private_code_test",
    module_scheme = "single",
)

targets.binaries.generated_script(
    name = "chrome_sizes",
    label = "//chrome/test:chrome_sizes",
    merge = targets.merge(
        script = "//tools/perf/process_perf_results.py",
        args = [
            "--upload-skia-json",
        ],
    ),
    module_scheme = "single",
)

targets.binaries.script(
    name = "chromedriver_py_tests",
    label = "//chrome/test/chromedriver:chromedriver_py_tests",
    script = "//testing/xvfb.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "../../testing/scripts/run_chromedriver_tests.py",
        "../../chrome/test/chromedriver/test/run_py_tests.py",
        "--chromedriver=chromedriver",
        "--log-path=${ISOLATED_OUTDIR}/chrome.chromedriver.log",
        "--browser-name=chrome",
    ],
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "pyunit",
)

targets.binaries.script(
    name = "chromedriver_py_tests_headless_shell",
    label = "//chrome/test/chromedriver:chromedriver_py_tests_headless_shell",
    script = "//testing/scripts/run_chromedriver_tests.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "../../chrome/test/chromedriver/test/run_py_tests.py",
        "--chromedriver=chromedriver",
        "--log-path=${ISOLATED_OUTDIR}/chrome-headless-shell.chromedriver.log",
        "--browser-name=chrome-headless-shell",
    ],
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "pyunit",
)

targets.binaries.generated_script(
    name = "chrome_wpt_tests",
    label = "//:chrome_wpt_tests",
    results_handler = "layout tests",
    args = [
        "--results-directory",
        "${ISOLATED_OUTDIR}",
    ],
    merge = targets.merge(
        script = "//third_party/blink/tools/merge_web_test_results.py",
        args = [
            "--verbose",
        ],
    ),
    module_scheme = "webtest",
)

targets.binaries.script(
    name = "chromedriver_replay_unittests",
    label = "//chrome/test/chromedriver:chromedriver_replay_unittests",
    script = "//chrome/test/chromedriver/log_replay/client_replay_unittest.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "single",
)

targets.binaries.windowed_test_launcher(
    name = "chromedriver_unittests",
    label = "//chrome/test/chromedriver:chromedriver_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "command_buffer_perftests",
    label = "//gpu:command_buffer_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "command_buffer_perftests",
        "--non-telemetry=true",
        "--adb-path",
    ],
    merge = targets.merge(
        script = "//tools/perf/process_perf_results.py",
        args = [
            "--smoke-test-mode",
        ],
    ),
    module_scheme = "single",
)

targets.binaries.windowed_test_launcher(
    name = "components_browsertests",
    label = "//components:components_browsertests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "components_junit_tests",
    label = "//components:components_junit_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "junit",
)

targets.binaries.script(
    name = "components_perftests",
    label = "//components:components_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "--xvfb",
        "--non-telemetry=true",
        "components_perftests",
    ],
    merge = targets.merge(
        script = "//tools/perf/process_perf_results.py",
        args = [
            "--smoke-test-mode",
        ],
    ),
    module_scheme = "flat",
)

targets.binaries.windowed_test_launcher(
    name = "components_unittests",
    label = "//components:components_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "compositor_unittests",
    label = "//ui/compositor:compositor_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "content_browsertests",
    label = "//content/test:content_browsertests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "content_shell_crash_test",
    label = "//content/shell:content_shell_crash_test",
    script = "//testing/scripts/content_shell_crash_test.py",
    resultdb = targets.resultdb(
        enable = True,
        result_format = "single",
    ),
    module_scheme = "single",
)

targets.binaries.windowed_test_launcher(
    name = "content_unittests",
    label = "//content/test:content_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "crashpad_tests",
    label = "//third_party/crashpad/crashpad:crashpad_tests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "cronet_python_unittests",
    label = "//components/cronet/testing:cronet_python_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "pyunit",
)

targets.binaries.generated_script(
    name = "crossbench_smoketests",
    label = "//chrome/test:crossbench_smoketests",
    skip_usage_check = True,
    args = [
        "../../third_party/crossbench/cb.py",
        "--benchmarks=speedometer_3.0",
        "--isolated-script-test-output=${ISOLATED_OUTDIR}",
        "--repeat=1",
        "--iterations=2",
        "--fast",
        "--fileserver",
        "--luci-chromium",
    ],
    module_scheme = "single",
)

targets.binaries.console_test_launcher(
    name = "crypto_unittests",
    label = "//crypto:crypto_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "dawn_end2end_tests",
    label = "//third_party/dawn/src/dawn/tests:dawn_end2end_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "dawn_perf_tests",
    label = "//third_party/dawn/src/dawn/tests:dawn_perf_tests",
    script = "//testing/scripts/run_performance_tests.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "dawn_perf_tests",
        "--non-telemetry=true",
        "--test-launcher-print-test-stdio=always",
        "--test-launcher-jobs=1",
        "--test-launcher-retry-limit=0",
    ],
    merge = targets.merge(
        script = "//tools/perf/process_perf_results.py",
        args = [
            # Does not upload to the perf dashboard
            "--smoke-test-mode",
        ],
    ),
    module_scheme = "single",
)

targets.binaries.windowed_test_launcher(
    name = "dbus_unittests",
    label = "//dbus:dbus_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "delayloads_unittests",
    label = "//chrome/test:delayloads_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "device_junit_tests",
    label = "//device:device_junit_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "junit",
)

targets.binaries.console_test_launcher(
    name = "device_unittests",
    label = "//device:device_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "device_realtarget_browsertests",
    label = "//chrome/test/device_realtarget:device_realtarget_browsertests",
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "devtools_api_tests",
    label = "//third_party/devtools-frontend/src/test/api:devtools_api_tests",
    skip_usage_check = True,
    module_scheme = "mocha",
)

targets.binaries.generated_script(
    name = "devtools_e2e_tests",
    label = "//third_party/devtools-frontend/src/test/e2e:devtools_e2e_tests",
    skip_usage_check = True,
    module_scheme = "mocha",
)

targets.binaries.generated_script(
    name = "devtools_node_unit_tests",
    label = "//third_party/devtools-frontend/src/test/unit:devtools_node_unit_tests",
    skip_usage_check = True,
    module_scheme = "mocha",
)

targets.binaries.generated_script(
    name = "devtools_unit_tests",
    label = "//third_party/devtools-frontend/src/test/unit:devtools_unit_tests",
    skip_usage_check = True,
    module_scheme = "mocha",
)

targets.binaries.console_test_launcher(
    name = "display_unittests",
    label = "//ui/display:display_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "elevated_tracing_service_unittests",
    label = "//chrome/windows_services/elevated_tracing_service:elevated_tracing_service_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "elevation_service_unittests",
    label = "//chrome/elevation_service:elevation_service_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "enterprise_companion_integration_tests",
    label = "//chrome/enterprise_companion:enterprise_companion_integration_tests",
    script = "//testing/scripts/run_telemetry_as_googletest.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "test_service/enterprise_companion_integration_tests_launcher.py",
        "--test-output-dir=${ISOLATED_OUTDIR}",
        "--test-launcher-bot-mode",
        "--gtest_shuffle",
    ],
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "enterprise_companion_tests",
    label = "//chrome/enterprise_companion:enterprise_companion_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = ["--gtest_shuffle"],
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "env_chromium_unittests",
    label = "//third_party/leveldatabase:env_chromium_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "events_unittests",
    label = "//ui/events:events_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "extensions_unittests",
    label = "//extensions:extensions_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "fake_libva_driver_unittest",
    label = "//media/gpu/vaapi/test/fake_libva_driver:fake_libva_driver_unittest",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "headless_shell_wpt",
    label = "//:headless_shell_wpt",
    results_handler = "layout tests",
    args = [
        "--results-directory",
        "${ISOLATED_OUTDIR}",
    ],
    merge = targets.merge(
        script = "//third_party/blink/tools/merge_web_test_results.py",
        args = [
            "--verbose",
        ],
    ),
    module_scheme = "webtest",
)

targets.binaries.console_test_launcher(
    name = "video_decode_accelerator_tests",
    label = "//media/gpu/test:video_decode_accelerator_tests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "video_encode_accelerator_tests",
    label = "//media/gpu/test:video_encode_accelerator_tests",
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "filesystem_service_unittests",
    label = "//components/services/filesystem:filesystem_service_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "flatbuffers_unittests",
    label = "//third_party/flatbuffers:flatbuffers_unittests",
    script = "//testing/scripts/run_flatbuffers_unittests.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    resultdb = targets.resultdb(
        enable = True,
        result_format = "single",
    ),
    module_scheme = "single",
)

targets.binaries.console_test_launcher(
    name = "fuzzing_unittests",
    label = "//testing/libfuzzer/tests:fuzzing_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "gcm_unit_tests",
    label = "//google_apis/gcm:gcm_unit_tests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "gcp_unittests",
    label = "//chrome/credential_provider/test:gcp_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "gfx_unittests",
    label = "//ui/gfx:gfx_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "gin_unittests",
    label = "//gin:gin_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "gl_tests",
    label = "//gpu:gl_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [],
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "gl_unittests",
    label = "//ui/gl:gl_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "gl_unittests_ozone",
    label = "//ui/gl:gl_unittests_ozone",
    label_type = "group",
    executable = "gl_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "gold_common_pytype",
    label = "//build:gold_common_pytype",
    script = "//build/skia_gold_common/run_pytype.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "single",
)

targets.binaries.console_test_launcher(
    name = "google_apis_unittests",
    label = "//google_apis:google_apis_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "gpu_perftests",
    label = "//gpu:gpu_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "gpu_perftests",
        "--non-telemetry=true",
        "--adb-path",
    ],
)

targets.binaries.script(
    name = "gpu_pytype",
    label = "//content/test:gpu_pytype",
    script = "//content/test/gpu/run_pytype.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "single",
)

targets.binaries.windowed_test_launcher(
    name = "gpu_unittests",
    label = "//gpu:gpu_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "grit_python_unittests",
    label = "//tools/grit:grit_python_unittests",
    script = "//testing/scripts/run_isolated_script_test.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "../../tools/grit/grit/test_suite_all.py",
    ],
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "pyunit",
)

targets.binaries.console_test_launcher(
    name = "gwp_asan_unittests",
    label = "//components/gwp_asan:gwp_asan_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "headless_browsertests",
    label = "//headless:headless_browsertests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "headless_unittests",
    label = "//headless:headless_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "highway_tests",
    label = "//third_party/highway:highway_tests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "install_static_unittests",
    label = "//chrome/install_static:install_static_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "installer_util_unittests",
    label = "//chrome/installer/util:installer_util_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "interactive_ui_tests",
    label = "//chrome/test:interactive_ui_tests",
    args = [
        "--snapshot-output-dir=${ISOLATED_OUTDIR}",
    ],
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "ipc_tests",
    label = "//ipc:ipc_tests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "latency_unittests",
    label = "//ui/latency:latency_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "leveldb_unittests",
    label = "//third_party/leveldatabase:leveldb_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "libjingle_xmpp_unittests",
    label = "//third_party/libjingle_xmpp:libjingle_xmpp_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "liburlpattern_unittests",
    label = "//third_party/liburlpattern:liburlpattern_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "load_library_perf_tests",
    label = "//chrome/test:load_library_perf_tests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "load_library_perf_tests",
        "--non-telemetry=true",
        "--test-launcher-print-test-stdio=always",
    ],
)

targets.binaries.generated_script(
    name = "mac_signing_tests",
    label = "//chrome/installer/mac:mac_signing_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "pyunit",
)

targets.binaries.script(
    name = "media_perftests",
    label = "//media:media_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "media_perftests",
        "--non-telemetry=true",
        "--single-process-tests",
        "--test-launcher-retry-limit=0",
        "--isolated-script-test-filter=*::-*_unoptimized::*_unaligned::*unoptimized_aligned",
    ],
)

targets.binaries.script(
    name = "media_router_e2e_tests",
    label = "//chrome/test/media_router:media_router_e2e_tests",
    script = "//chrome/test/media_router/internal/media_router_tests.py",
    args = [
        "--test_binary",
        "./interactive_ui_tests",
    ],
)

targets.binaries.windowed_test_launcher(
    name = "media_unittests",
    label = "//media:media_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "message_center_unittests",
    label = "//ui/message_center:message_center_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "midi_unittests",
    label = "//media/midi:midi_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "mini_installer_tests",
    label = "//chrome/test/mini_installer:mini_installer_tests",
    script = "//testing/scripts/run_isolated_script_test.py",
    args = [
        "../../chrome/test/mini_installer/run_mini_installer_tests.py",
        "--output-dir=${ISOLATED_OUTDIR}",
    ],
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "model_validation_tests",
    label = "//components/optimization_guide/internal/testing:model_validation_tests",
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "model_validation_tests_light",
    label = "//components/optimization_guide/internal/testing:model_validation_tests_light",
    module_scheme = "flat",
)

targets.binaries.script(
    name = "mojo_python_unittests",
    label = "//mojo/public/tools:mojo_python_unittests",
    script = "//testing/scripts/run_isolated_script_test.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "../../mojo/public/tools/run_all_python_unittests.py",
    ],
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "pyunit",
)

targets.binaries.console_test_launcher(
    name = "mojo_unittests",
    label = "//mojo:mojo_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "build_rust_tests",
    label = "//build/rust/tests:build_rust_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "flat",
)

targets.binaries.windowed_test_launcher(
    name = "native_theme_unittests",
    label = "//ui/native_theme:native_theme_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "net_perftests",
    label = "//net:net_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "net_perftests",
        "--non-telemetry=true",
    ],
)

targets.binaries.console_test_launcher(
    name = "net_unittests",
    label = "//net:net_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "notification_helper_unittests",
    label = "//chrome/notification_helper:notification_helper_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "ondevice_stability_tests",
    label = "//components/optimization_guide/internal/testing:ondevice_stability_tests",
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "ondevice_stability_tests_light",
    label = "//components/optimization_guide/internal/testing:ondevice_stability_tests_light",
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "ondevice_api_scrape_tests",
    label = "//components/optimization_guide/internal/testing:ondevice_api_scrape_tests",
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "litert_e2e_tests",
    label = "//components/optimization_guide/internal/testing:litert_e2e_tests",
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "litert_lm_advanced_main_legacy_tests",
    label = "//components/optimization_guide/internal/testing:litert_lm_advanced_main_legacy_tests",
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "chrome_ai_wpt_tests",
    label = "//components/optimization_guide/internal/testing:chrome_ai_wpt_tests",
    results_handler = "layout tests",
    args = [
        "--results-directory",
        "${ISOLATED_OUTDIR}",
    ],
    merge = targets.merge(
        script = "//third_party/blink/tools/merge_web_test_results.py",
        args = [
            "--verbose",
        ],
    ),
    module_scheme = "webtest",
)

targets.binaries.console_test_launcher(
    name = "openscreen_unittests",
    label = "//chrome/browser/media/router:openscreen_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "optimization_guide_gpu_unittests",
    label = "//components/optimization_guide/internal:optimization_guide_gpu_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "optimization_guide_unittests",
    label = "//components/optimization_guide/internal:optimization_guide_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "ozone_gl_unittests",
    label = "//ui/ozone/gl:ozone_gl_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "ozone_unittests",
    label = "//ui/ozone:ozone_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "ozone_x11_unittests",
    label = "//ui/ozone:ozone_x11_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "partition_alloc_perftests",
    label = "//base:partition_alloc_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,
    args = [
        "partition_alloc_perftests",
        "--non-telemetry=true",
        "--test-launcher-print-test-stdio=always",
        "--test-launcher-jobs=1",
        "--test-launcher-retry-limit=0",
    ],
)

targets.binaries.console_test_launcher(
    name = "pdf_unittests",
    label = "//pdf:pdf_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "perfetto_diff_tests",
    label = "//base/tracing:perfetto_diff_tests",
    skip_usage_check = True,
    module_scheme = "single",
)

targets.binaries.console_test_launcher(
    name = "perfetto_unittests",
    label = "//third_party/perfetto:perfetto_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "performance_browser_tests",
    label = "//chrome/test:performance_browser_tests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "browser_tests",
        "--non-telemetry=true",
        "--full-performance-run",
        "--test-launcher-jobs=1",
        "--test-launcher-retry-limit=0",
        "--test-launcher-print-test-stdio=always",
        # Allow the full performance runs to take up to 60 seconds (rather than
        # the default of 30 for normal CQ browser test runs).
        "--ui-test-action-timeout=60000",
        "--ui-test-action-max-timeout=60000",
        "--test-launcher-timeout=60000",
        "--gtest_filter=*/TabCapturePerformanceTest.*:*/CastV2PerformanceTest.*",
    ],
)

targets.binaries.generated_script(
    name = "performance_test_suite",
    label = "//chrome/test:performance_test_suite",
    merge = targets.merge(
        script = "//tools/perf/process_perf_results.py",
        args = [
            "--smoke-test-mode",
        ],
    ),
    module_scheme = "flat",
)

targets.binaries.script(
    name = "performance_web_engine_test_suite",
    label = "//content/test:performance_web_engine_test_suite",
    script = "//testing/scripts/run_performance_tests.py",
    args = [
        "--per-test-logs-dir",
    ],
    module_scheme = "single",
)

targets.binaries.script(
    name = "polymer_tools_python_unittests",
    label = "//tools/polymer:polymer_tools_python_unittests",
    script = "//testing/scripts/run_isolated_script_test.py",
    args = [
        "../../tools/polymer/run_polymer_tools_tests.py",
    ],
    module_scheme = "pyunit",
)

targets.binaries.generated_script(
    name = "webui_resources_tools_python_unittests",
    label = "//ui/webui/resources/tools:webui_resources_tools_python_unittests",
    # All references are in starlark.
    skip_usage_check = True,
    module_scheme = "pyunit",
)

targets.binaries.console_test_launcher(
    name = "power_sampler_unittests",
    label = "//tools/mac/power:power_sampler_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "printing_unittests",
    label = "//printing:printing_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "private_code_failure_test",
    label = "//build/private_code_test:private_code_failure_test",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "single",
)

targets.binaries.console_test_launcher(
    name = "pthreadpool_unittests",
    label = "//third_party/pthreadpool:pthreadpool_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "puffin_unittests",
    label = "//third_party/puffin:puffin_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "remoting_unittests",
    label = "//remoting:remoting_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "rlz_unittests",
    label = "//rlz:rlz_unittests",
    module_scheme = "gtest",
    skip_usage_check = True,
)

targets.binaries.console_test_launcher(
    name = "sandbox_linux_unittests",
    label = "//sandbox/linux:sandbox_linux_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "sandbox_unittests",
    label = "//sandbox:sandbox_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "sbox_integration_tests",
    label = "//sandbox/win:sbox_integration_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "sbox_unittests",
    label = "//sandbox/win:sbox_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "sbox_validation_tests",
    label = "//sandbox/win:sbox_validation_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "services_junit_tests",
    label = "//services:services_junit_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "junit",
)

targets.binaries.windowed_test_launcher(
    name = "services_unittests",
    label = "//services:services_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "setup_unittests",
    label = "//chrome/installer/setup:setup_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "shell_encryption_unittests",
    label = "//third_party/shell-encryption:shell_encryption_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "shell_dialogs_unittests",
    label = "//ui/shell_dialogs:shell_dialogs_unittests",
    # These tests are more like dialog interactive ui tests.
    args = [
        "--test-launcher-jobs=1",
    ],
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "skia_unittests",
    label = "//skia:skia_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "snapshot_unittests",
    label = "//ui/snapshot:snapshot_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "sql_unittests",
    label = "//sql:sql_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "storage_unittests",
    label = "//storage:storage_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "sync_integration_tests",
    label = "//chrome/test:sync_integration_tests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "sync_performance_tests",
    label = "//chrome/test:sync_performance_tests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "sync_performance_tests",
        "--non-telemetry=true",
        "--test-launcher-print-test-stdio=always",
        "--test-launcher-jobs=1",
        "--test-launcher-retry-limit=0",
    ],
)

targets.binaries.generated_script(
    name = "telemetry_gpu_integration_test",
    label = "//chrome/test:telemetry_gpu_integration_test",
)

targets.binaries.script(
    name = "telemetry_gpu_unittests",
    label = "//chrome/test:telemetry_gpu_unittests",
    script = "//testing/scripts/run_telemetry_as_googletest.py",
    args = [
        "../../content/test/gpu/run_unittests.py",
        "-v",
    ],
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "flat",
)

# This isolate is used by
# https://www.chromium.org/developers/cluster-telemetry
targets.binaries.script(
    name = "ct_telemetry_perf_tests_without_chrome",
    label = "//chrome/test:ct_telemetry_perf_tests_without_chrome",
    script = "//testing/scripts/run_performance_tests.py",
    args = [
        "../../tools/perf/run_benchmark",
    ],
)

targets.binaries.generated_script(
    name = "telemetry_perf_unittests",
    label = "//chrome/test:telemetry_perf_unittests",
    module_scheme = "pyunit",
)

targets.binaries.generated_script(
    name = "telemetry_unittests",
    label = "//chrome/test:telemetry_unittests",
    module_scheme = "pyunit",
)

targets.binaries.generated_script(
    name = "test_env_py_unittests",
    label = "//testing:test_env_py_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "single",
)

targets.binaries.script(
    name = "testing_pytype",
    label = "//testing:testing_pytype",
    script = "//testing/run_pytype.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "single",
)

targets.binaries.script(
    name = "tracing_perftests",
    label = "//components/tracing:tracing_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    skip_usage_check = True,  # Used by Pinpoint: crbug.com/1042778
    args = [
        "tracing_perftests",
        "--non-telemetry=true",
        "--test-launcher-print-test-stdio=always",
        "--adb-path",
    ],
)

targets.binaries.windowed_test_launcher(
    name = "ui_base_unittests",
    label = "//ui/base:ui_base_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "ui_junit_tests",
    label = "//ui:ui_junit_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "junit",
)

targets.binaries.windowed_test_launcher(
    name = "ui_touch_selection_unittests",
    label = "//ui/touch_selection:ui_touch_selection_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "unit_tests",
    label = "//chrome/test:unit_tests",
    module_scheme = "gtest",
)

# The test action timeouts for `updater_tests`, `updater_tests_system`, and
# `updater_tests_win_uac` are based on empirical observations of test
# runtimes, 2021-07. The launcher timeout was 90000 but then we increased
# the value to 180000 to work around an unfixable issue in the Windows
# COM runtime class activation crbug.com/1259178.
targets.binaries.console_test_launcher(
    name = "updater_tests",
    label = "//chrome/updater:updater_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "--gtest_shuffle",
        "--test-launcher-timeout=180000",
        "--ui-test-action-max-timeout=45000",
        "--ui-test-action-timeout=40000",
    ],
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "updater_fuzztests",
    label = "//chrome/updater:updater_fuzztests",
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "updater_tests_system",
    label = "//chrome/updater:updater_tests_system",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "--gtest_shuffle",
        "--test-launcher-timeout=180000",
        "--ui-test-action-max-timeout=45000",
        "--ui-test-action-timeout=40000",
        "--exclude-paths-from-win-defender",
    ],
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "updater_tests_win_uac",
    label = "//chrome/updater:updater_tests_win_uac",
    script = "//testing/scripts/run_telemetry_as_googletest.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "test_service/run_command_as_standard_user.py",
        "--command=updater_tests.exe",
        "--test-launcher-bot-mode",
        "--cfi-diag=0",
        "--gtest_shuffle",
        "--test-launcher-timeout=180000",
        "--ui-test-action-max-timeout=45000",
        "--ui-test-action-timeout=40000",
    ],
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "ui_unittests",
    label = "//ui/tests:ui_unittests",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "upload_trace_processor",
    label = "//tools/perf/core/perfetto_binary_roller:upload_trace_processor",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.binaries.console_test_launcher(
    name = "url_unittests",
    label = "//url:url_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "vaapi_unittest",
    label = "//media/gpu/vaapi:vaapi_unittest",
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "variations_desktop_smoke_tests",
    label = "//chrome/test/variations:variations_desktop_smoke_tests",
    module_scheme = "flat",
)

targets.binaries.generated_script(
    name = "variations_smoke_tests",
    label = "//chrome/test:variations_smoke_tests",
    resultdb = targets.resultdb(
        enable = True,
        result_format = "single",
    ),
    module_scheme = "single",
)

targets.binaries.generated_script(
    name = "views_examples_mac_signing_tests",
    label = "//chrome/views_examples/mac/signing:views_examples_mac_signing_tests",
    skip_usage_check = True,
    module_scheme = "pyunit",
)

targets.binaries.windowed_test_launcher(
    name = "views_examples_unittests",
    label = "//ui/views/examples:views_examples_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "views_perftests",
    label = "//ui/views:views_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "--xvfb",
        "--non-telemetry=true",
        "views_perftests",
    ],
    merge = targets.merge(
        script = "//tools/perf/process_perf_results.py",
        args = [
            "--smoke-test-mode",
        ],
    ),
    module_scheme = "single",
)

targets.binaries.windowed_test_launcher(
    name = "views_unittests",
    label = "//ui/views:views_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "gtk_unittests",
    label = "//ui/gtk:gtk_unittests",
    module_scheme = "gtest",
    skip_usage_check = True,
)

targets.binaries.windowed_test_launcher(
    name = "viz_unittests",
    label = "//components/viz:viz_unittests",
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "vr_common_perftests",
    label = "//chrome/browser/vr:vr_common_perftests",
    script = "//testing/scripts/run_performance_tests.py",
    args = [
        "vr_common_perftests",
        "--non-telemetry=true",
        "--adb-path",
    ],
)

targets.binaries.console_test_launcher(
    name = "vr_common_unittests",
    label = "//chrome/browser/vr:vr_common_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.script(
    name = "vr_perf_tests",
    label = "//tools/perf/contrib/vr_benchmarks:vr_perf_tests",
    script = "//testing/scripts/run_performance_tests.py",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "../../tools/perf/run_benchmark",
    ],
    merge = targets.merge(
        script = "//tools/perf/process_perf_results.py",
        args = [
            "--upload-skia-json",
        ],
    ),
    module_scheme = "flat",
)

targets.binaries.windowed_test_launcher(
    name = "vulkan_tests",
    label = "//gpu/vulkan:vulkan_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "webdriver_bidi_unittests",
    label = "//third_party/chromium-bidi:webdriver_bidi_unittests",
    skip_usage_check = True,
    module_scheme = "mocha",
)

targets.binaries.generated_script(
    name = "webdriver_bidi_e2e_tests",
    label = "//third_party/chromium-bidi:webdriver_bidi_e2e_tests",
    skip_usage_check = True,
)

targets.binaries.generated_script(
    name = "webgpu_blink_web_tests",
    label = "//:webgpu_blink_web_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "--results-directory",
        "${ISOLATED_OUTDIR}",
    ],
    module_scheme = "webtest",
)

targets.binaries.windowed_test_launcher(
    name = "wm_unittests",
    label = "//ui/wm:wm_unittests",
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "wtf_unittests",
    label = "//third_party/blink/renderer/platform/wtf:wtf_unittests",
    module_scheme = "gtest",
)

targets.binaries.windowed_test_launcher(
    name = "xr_browser_tests",
    label = "//chrome/test:xr_browser_tests",
    # We can't use the "script" type since we need to be run from the output
    # directory (or at least given the path). Thus, we need to tell mb.py to not
    # automatically append the .exe suffix on Windows.
    executable = "run_xr_browser_tests.py",
    executable_suffix = "",
    # All references have been moved to starlark
    skip_usage_check = True,
    args = [
        "--enable-gpu",
        "--test-launcher-bot-mode",
        "--test-launcher-jobs=1",
        "--test-launcher-retry-limit=0",
        "--enable-pixel-output-in-tests",
        "--enable-unsafe-swiftshader",
    ],
    module_scheme = "gtest",
)

targets.binaries.generated_script(
    name = "xvfb_py_unittests",
    label = "//testing:xvfb_py_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    resultdb = targets.resultdb(
        enable = True,
    ),
    module_scheme = "single",
)

targets.binaries.console_test_launcher(
    name = "zlib_unittests",
    label = "//third_party/zlib:zlib_unittests",
    args = [
        "--test-launcher-timeout=400000",
    ],
    module_scheme = "gtest",
)

targets.binaries.console_test_launcher(
    name = "zucchini_unittests",
    label = "//components/zucchini:zucchini_unittests",
    # All references have been moved to starlark
    skip_usage_check = True,
    module_scheme = "gtest",
)
