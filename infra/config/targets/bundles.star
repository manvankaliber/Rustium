# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Bundle declarations

Bundles are groupings of tests and/or compile targets that can be referenced by
builders or other bundles. Bundles cannot be referenced in //testing/buildbot.
"""

load("@chromium-luci//targets.star", "targets")

targets.bundle(
    name = "ash_pixel_gtests",
    targets = [
    ],
)

# TODO(gbeaty) - Make the resultdb information for tests using the same binaries
# consistent and move the information onto the binaries

targets.bundle(
    name = "aura_gtests",
    targets = [
  "aura_unittests",
  "compositor_unittests",
  "wm_unittests",
],
)

# Run content_browsertests with BackForwardCache disabled
targets.bundle(
    name = "bfcache_generic_gtests",
    targets = [
        "bf_cache_content_browsertests",
    ],
    per_test_modifications = {
        "bf_cache_content_browsertests": targets.mixin(
            swarming = targets.swarming(
                shards = 20,
            ),
        ),
    },
)

# Run browser_tests with BackForwardCache disabled
targets.bundle(
    name = "bfcache_linux_gtests",
    targets = [
        "bfcache_generic_gtests",
        "bfcache_linux_specific_gtests",
    ],
)

targets.bundle(
    name = "bfcache_linux_specific_gtests",
    targets = [
        "bf_cache_browser_tests",
    ],
    per_test_modifications = {
        "bf_cache_browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 20,
            ),
        ),
    },
)

targets.bundle(
    name = "blink_web_tests_ppapi_isolated_scripts",
    targets = [
  "ppapi_blink_web_tests",
],
)

targets.bundle(
    name = "chrome_profile_generator_tests",
    targets = [
    ],
    per_test_modifications = {
    },
)

targets.bundle(
    name = "chromedriver_py_tests_isolated_scripts",
    targets = [
        "chromedriver_py_tests",
        "chromedriver_py_tests_headless_shell",
        "chromedriver_replay_unittests",
    ],
    per_test_modifications = {
        "chromedriver_py_tests": targets.mixin(
            args = [
                "--test-type=integration",
            ],
        ),
        "chromedriver_py_tests_headless_shell": targets.mixin(
            args = [
                "--test-type=integration",
            ],
        ),
    },
)

targets.bundle(
    name = "chromium_dbg_isolated_scripts",
    targets = [
        "desktop_chromium_isolated_scripts",
        "performance_smoke_test_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_dev_linux_gtests",
    targets = [
        "base_unittests",
        "browser_tests",
        "content_browsertests",
        "content_unittests",
        "interactive_ui_tests",
        "net_unittests",
        "unit_tests",
    ],
    per_test_modifications = {
        "base_unittests": targets.mixin(
            swarming = targets.swarming(
                dimensions = {
                    "cores": "8",
                },
            ),
        ),
        "browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 16,
                dimensions = {
                    "cores": "8",
                },
            ),
        ),
        "content_browsertests": targets.mixin(
            swarming = targets.swarming(
                shards = 5,
                dimensions = {
                    "cores": "8",
                },
            ),
        ),
        "content_unittests": targets.mixin(
            swarming = targets.swarming(
                dimensions = {
                    "cores": "2",
                },
            ),
        ),
        "interactive_ui_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 6,
                dimensions = {
                    "cores": "8",
                },
            ),
        ),
        "net_unittests": targets.mixin(
            swarming = targets.swarming(
                dimensions = {
                    "cores": "8",
                },
            ),
        ),
        "unit_tests": targets.mixin(
            swarming = targets.swarming(
                dimensions = {
                    "cores": "2",
                },
            ),
        ),
    },
)

targets.bundle(
    name = "chromium_dev_mac_gtests",
    targets = [
        "base_unittests",
        "content_unittests",
        "net_unittests",
        "unit_tests",
    ],
)

targets.bundle(
    name = "chromium_dev_win_gtests",
    targets = [
        "base_unittests",
        "content_browsertests",
        "content_unittests",
        "interactive_ui_tests",
        "net_unittests",
        "unit_tests",
    ],
    per_test_modifications = {
        "content_browsertests": targets.mixin(
            swarming = targets.swarming(
                shards = 4,
            ),
        ),
        "interactive_ui_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 4,
            ),
        ),
    },
)

targets.bundle(
    name = "chromium_gtests",
    targets = [
  "absl_hardening_tests",
  "angle_unittests",
  "base_unittests",
  "blink_common_unittests",
  "blink_heap_unittests",
  "blink_platform_unittests",
  "blink_unittests",
  "boringssl_crypto_tests",
  "boringssl_ssl_tests",
  "capture_unittests",
  "cast_unittests",
  "components_browsertests",
  "components_unittests",
  "content_browsertests",
  "content_unittests",
  "crashpad_tests",
  "crypto_unittests",
  "env_chromium_unittests",
  "events_unittests",
  "fuzzing_unittests",
  "gcm_unit_tests",
  "gin_unittests",
  "google_apis_unittests",
  "gpu_unittests",
  "gwp_asan_unittests",
  "ipc_tests",
  "latency_unittests",
  "leveldb_unittests",
  "libjingle_xmpp_unittests",
  "liburlpattern_unittests",
  "media_unittests",
  "midi_unittests",
  "mojo_unittests",
  "net_unittests",
  "perfetto_unittests",
  "puffin_unittests",
  "services_unittests",
  "shell_dialogs_unittests",
  "skia_unittests",
  "sql_unittests",
  "storage_unittests",
  "ui_base_unittests",
  "ui_touch_selection_unittests",
  "url_unittests",
  "wtf_unittests",
  "zlib_unittests",
],
    per_test_modifications = {
  "angle_unittests": targets.mixin(
    use_isolated_scripts_api = True,
  ),
  "blink_unittests": targets.mixin(
    swarming = targets.swarming(
      shards = 2,
    ),
  ),
  "capture_unittests": targets.mixin(
    args = [
      "--gtest_filter=-*UsingRealWebcam*",
    ],
  ),
  "components_unittests": targets.mixin(
    swarming = targets.swarming(
      shards = 3,
    ),
  ),
  "content_browsertests": targets.mixin(
    swarming = targets.swarming(
      shards = 8,
    ),
  ),
  "env_chromium_unittests": targets.mixin(
    ci_only = True,
  ),
  "leveldb_unittests": targets.mixin(
    ci_only = True,
  ),
},
)

targets.bundle(
    name = "chromium_gtests_for_devices_with_graphical_output",
    targets = [
  "cc_unittests",
  "device_unittests",
  "display_unittests",
  "gfx_unittests",
  "unit_tests",
  "viz_unittests",
],
    per_test_modifications = {
  "unit_tests": targets.mixin(
    swarming = targets.swarming(
      shards = 2,
    ),
  ),
},
)

targets.bundle(
    name = "chromium_gtests_for_linux_and_mac_only",
    targets = [
        "openscreen_unittests",
    ],
)

targets.bundle(
    name = "chromium_gtests_for_linux_dbus",
    targets = [
  "dbus_unittests",
],
)

targets.bundle(
    name = "chromium_gtests_for_linux_only",
    targets = [
        "gtk_unittests",
        "ozone_unittests",
        "ozone_x11_unittests",
    ],
    per_test_modifications = {
        "gtk_unittests": targets.mixin(
            experiment_percentage = 100,
        ),
    },
)

targets.bundle(
    name = "chromium_gtests_for_linux_wayland_mutter",
    targets = [
        "browser_tests",
        "content_browsertests",
        "interactive_ui_tests",
    ],
    per_test_modifications = {
        "browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 8,
            ),
        ),
        "content_browsertests": targets.mixin(
            swarming = targets.swarming(
                shards = 8,
            ),
        ),
    },
)

targets.bundle(
    name = "chromium_gtests_for_win_and_linux_only",
    targets = [
  "pthreadpool_unittests",
],
    per_test_modifications = {
  "pthreadpool_unittests": targets.mixin(
    ci_only = True,
  ),
},
)

targets.bundle(
    name = "chromium_gtests_for_win_and_mac",
    targets = [
        "rlz_unittests",
    ],
)

targets.bundle(
    name = "chromium_linux_and_gl_and_vulkan_gtests",
    targets = [
        "chromium_linux_and_gl_gtests",
        "gpu_fyi_vulkan_swiftshader_gtests",
    ],
)

targets.bundle(
    name = "chromium_linux_and_gl_gtests",
    targets = [
        "chromium_linux_gtests",
        "gl_gtests_passthrough",
    ],
)

targets.bundle(
    name = "chromium_linux_dbg_isolated_scripts",
    targets = [
        "desktop_chromium_isolated_scripts",
        "linux_specific_chromium_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_linux_gtests",
    targets = [
        "aura_gtests",
        "chromium_gtests",
        "chromium_gtests_for_devices_with_graphical_output",
        "chromium_gtests_for_linux_and_mac_only",
        "chromium_gtests_for_linux_only",
        "chromium_gtests_for_win_and_linux_only",
        "linux_flavor_specific_chromium_gtests",
        "linux_specific_xr_gtests",
        "vr_platform_specific_chromium_gtests",
        "chromium_gtests_for_linux_dbus",
        "desktop_headless_gtests",
        "desktop_chromium_gtests_no_nacl",
    ],
)

targets.bundle(
    name = "chromium_linux_rel_isolated_scripts",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "chromium_web_tests_high_dpi_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "linux_specific_chromium_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "pytype_tests",
        "telemetry_perf_unittests_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_linux_rel_isolated_scripts_code_coverage",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "chromium_web_tests_high_dpi_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "gpu_dawn_webgpu_blink_web_tests",
        "linux_specific_chromium_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "pytype_tests",
        "telemetry_perf_unittests_isolated_scripts_xvfb",
    ],
)

# Like chromium_linux_rel_isolated_scripts, but should only
# include test suites that aren't affected by things like extra GN args
# (e.g. is_debug) or OS versions (e.g. Mac-12 vs Mac-13). Note: use
# chromium_linux_rel_isolated_scripts if you're setting up a new builder.
# This group should only be used across ~3 builders.
targets.bundle(
    name = "chromium_linux_rel_isolated_scripts_once",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "desktop_once_isolated_scripts",
        "linux_specific_chromium_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "pytype_tests",
        "telemetry_perf_unittests_isolated_scripts",
        "chromium_web_tests_high_dpi_isolated_scripts",
        # TODO(crbug.com/40287410): Remove this once the BackgroundResourceFetch
        # feature launches.
        "chromium_web_tests_brfetch_isolated_scripts",
        "devtools_api_tests",
        "devtools_e2e_tests",
        "devtools_node_unit_tests",
        "devtools_unit_tests",
        "webdriver_bidi_e2e_tests",
        "webdriver_bidi_unittests",
    ],
)

targets.bundle(
    name = "chromium_linux_scripts",
    targets = [
        "check_network_annotations",
        "check_static_initializers",
        "checkdeps",
        "checkperms",
        "metrics_python_tests",
        "blink_lint",
    ],
)

targets.bundle(
    name = "chromium_mac_gtests",
    targets = [
        "chromium_gtests",
        "chromium_gtests_for_devices_with_graphical_output",
        "chromium_gtests_for_linux_and_mac_only",
        "mac_specific_chromium_gtests",
        "chromium_gtests_for_win_and_mac",
        "desktop_headless_gtests",
        "desktop_chromium_gtests_no_nacl",
    ],
)

# chromium_mac_gtests_no_nacl_once in the same way.
# TODO(crbug.com/303417958): This no_nacl suite is identical to the normal
# suite, since NaCl has been disabled on Mac. Replace this by the normal
# suite.
targets.bundle(
    name = "chromium_mac_gtests_no_nacl",
    targets = [
        "chromium_gtests",
        "chromium_gtests_for_devices_with_graphical_output",
        "chromium_gtests_for_linux_and_mac_only",
        "mac_specific_chromium_gtests",
        "desktop_headless_gtests",
        "desktop_chromium_gtests_no_nacl",
    ],
)

targets.bundle(
    name = "chromium_mac_osxbeta_rel_isolated_scripts",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "components_perftests_isolated_scripts",
        "desktop_chromium_mac_osxbeta_scripts",
        "mac_specific_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_mac_rel_isolated_scripts",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "components_perftests_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "mac_specific_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_mac_rel_isolated_scripts_code_coverage",
    # TODO(crbug.com/40249801): Enable gpu_dawn_webgpu_blink_web_tests
)

# Like chromium_mac_rel_isolated_scripts above, but should only
# include test suites that aren't affected by things like extra GN args
# (e.g. is_debug) or OS versions (e.g. Mac-12 vs Mac-13). Note: use
# chromium_mac_rel_isolated_scripts if you're setting up a new builder.
targets.bundle(
    name = "chromium_mac_rel_isolated_scripts_once",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "components_perftests_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "desktop_once_isolated_scripts",
        "mac_specific_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_mac_scripts",
    targets = [
        "check_static_initializers",
        "metrics_python_tests",
        "blink_lint",
    ],
)

# Multiscreen tests for desktop platforms. See: crbug.com/346565331.
targets.bundle(
    name = "chromium_multiscreen_gtests",
    targets = [
        "multiscreen_interactive_ui_tests",
    ],
    mixins = [
        "linux-noble",
    ],
    per_test_modifications = {
        "multiscreen_interactive_ui_tests": targets.mixin(
            args = [
                "--gtest_filter=*MultiScreen*:*VirtualDisplayUtil*",
            ],
        ),
    },
)

# Multiscreen tests for desktop platforms. See: crbug.com/346565331.
targets.bundle(
    name = "chromium_multiscreen_gtests_fyi",
    targets = [
        "chromium_multiscreen_gtests",
    ],
)

# On some bots we don't have capacity to run all standard tests (for example
# Android Pie), however there are tracing integration tests we want to
# ensure are still working.
targets.bundle(
    name = "chromium_tracing_gtests",
    targets = [
        "services_unittests",
    ],
)

targets.bundle(
    name = "chromium_web_tests_brfetch_isolated_scripts",
    targets = [
        "brfetch_blink_web_tests",
        "brfetch_blink_wpt_tests",
        "brfetch_headless_shell_wpt_tests",
    ],
    per_test_modifications = {
        # brfetch_blink_web_tests provides coverage for
        # running Layout Tests with BackgroundResourceFetch feature.
        "brfetch_blink_web_tests": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 1,
            ),
            experiment_percentage = 100,
        ),
        # brfetch_blink_wpt_tests provides coverage for
        # running Layout Tests with BackgroundResourceFetch feature.
        "brfetch_blink_wpt_tests": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 1,
            ),
            experiment_percentage = 100,
        ),
        # brfetch_headless_shell_wpt_tests provides coverage for
        # running WPTs with BackgroundResourceFetch feature.
        "brfetch_headless_shell_wpt_tests": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 1,
            ),
            experiment_percentage = 100,
        ),
    },
)

targets.bundle(
    name = "chromium_web_tests_surface_embed_isolated_scripts",
    targets = [
        "surface_embed_chrome_wpt_tests",
    ],
    per_test_modifications = {
        "surface_embed_chrome_wpt_tests": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 7,
            ),
        ),
    },
)

# Pixel tests only enabled on Win 10. So this is
# 'chromium_win_gtests' + 'pixel_browser_tests_gtests' +
# 'desktop_chromium_gtests_skia_gold'. When changing
# something here, also change chromium_win10_gtests_once in the same way.
targets.bundle(
    name = "chromium_win10_gtests",
    targets = [
        "aura_gtests",
        "chromium_gtests",
        "chromium_gtests_for_devices_with_graphical_output",
        "chromium_gtests_for_win_and_linux_only",
        "fieldtrial_browser_tests",
        "pixel_browser_tests_gtests",
        "vr_platform_specific_chromium_gtests",
        "win_specific_chromium_gtests",
        "chromium_gtests_for_win_and_mac",
        "desktop_headless_gtests",
        "desktop_chromium_gtests_no_nacl",
        "desktop_chromium_gtests_skia_gold",
    ],
)

targets.bundle(
    name = "chromium_win_dbg_isolated_scripts",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "components_perftests_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "performance_smoke_test_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_win_gtests",
    targets = [
        "aura_gtests",
        "chromium_gtests",
        "chromium_gtests_for_devices_with_graphical_output",
        "chromium_gtests_for_win_and_linux_only",
        "vr_platform_specific_chromium_gtests",
        "win_specific_chromium_gtests",
        "chromium_gtests_for_win_and_mac",
        "desktop_headless_gtests",
        "desktop_chromium_gtests_no_nacl",
    ],
)

targets.bundle(
    name = "chromium_win_rel_isolated_scripts",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "components_perftests_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "telemetry_desktop_minidump_unittests_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
        "win_specific_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_win_rel_isolated_scripts_code_coverage",
    targets = [
        "gpu_dawn_webgpu_blink_web_tests",
    ],
)

# Like chromium_win_rel_isolated_scripts, but should only include test suites
# that aren't affected by things like extra GN args (e.g. is_debug) or OS
# versions (e.g. Mac-12 vs Mac-13). Note: use chromium_win_rel_isolated_scripts
# if you're setting up a new builder. This group should only be used across
# ~3 builders.
targets.bundle(
    name = "chromium_win_rel_isolated_scripts_once",
    targets = [
        "chromedriver_py_tests_isolated_scripts",
        "components_perftests_isolated_scripts",
        "desktop_chromium_isolated_scripts",
        "desktop_once_isolated_scripts",
        "mojo_python_unittests_isolated_scripts",
        "telemetry_desktop_minidump_unittests_isolated_scripts",
        "telemetry_perf_unittests_isolated_scripts",
        "win_specific_isolated_scripts",
    ],
)

targets.bundle(
    name = "chromium_win_scripts",
    targets = [
        "check_network_annotations",
        "metrics_python_tests",
        "blink_lint",
    ],
)

targets.bundle(
    name = "components_perftests_isolated_scripts",
    targets = [
        "components_perftests",
    ],
    per_test_modifications = {
        "components_perftests": targets.mixin(
            args = [
                "--gtest-benchmark-name=components_perftests",
            ],
        ),
    },
)

# Compilable unit tests of cronet dependencies in:
# //components/cronet/android/dependencies.txt
# TODO(crbug.com/333888734): Add component_unittests or a subset of it.
# TODO(crbug.com/333887705): Make base_unittests compilable and add it.
# TODO(crbug.com/333888747): Make url_unittests compilable and add it.
targets.bundle(
    name = "cronet_clang_coverage_additional_gtests",
    targets = [
        "absl_hardening_tests",
        "crypto_unittests",
        "zlib_unittests",
    ],
)

# Compile targets which are common to most cronet builders in chromium.android
targets.bundle(
    name = "cronet_common_compile_targets",
    additional_compile_targets = [
        "cronet_package",
        "net_unittests",
    ],
)

targets.bundle(
    name = "cronet_dbg_isolated_scripts",
    targets = [
        "cronet_sizes_suite",
    ],
)

targets.bundle(
    name = "cronet_gtests",
    targets = [
        "net_unittests",
    ],
    per_test_modifications = {
        "net_unittests": targets.mixin(
            swarming = targets.swarming(
                shards = 4,
            ),
        ),
    },
)

targets.bundle(
    name = "cronet_python_unittest",
    targets = ["cronet_python_unittests"],
    per_test_modifications = {
        "cronet_python_unittests": targets.per_test_modification(
            remove_mixins = [
            ],
        ),
    },
)

targets.bundle(
    name = "cronet_rel_isolated_scripts",
    targets = [
        "cronet_resource_sizes",
        "cronet_sizes_suite",
    ],
)

targets.bundle(
    name = "cronet_resource_sizes",
    targets = [
    ],
    per_test_modifications = {
    },
)

targets.bundle(
    name = "cronet_sizes_suite",
    targets = [
    ],
    per_test_modifications = {
    },
)

# The same as gpu_dawn_isolated_scripts, but with suites removed that are
# run on the standalone Dawn builders.
targets.bundle(
    name = "dawn_chromium_isolated_scripts",
    targets = [
        "gpu_dawn_common_isolated_scripts",
        "gpu_dawn_webgpu_blink_web_tests",
        "gpu_dawn_webgpu_blink_web_tests_force_swiftshader",
    ],
)

# This compiles tests which are run on Dawn standalone builders, which allows
# us to keep compile coverage of all tests in gpu_dawn_integration_gtests_passthrough
# and gpu_dawn_isolated_scripts even if we use gpu_common_gtests_passthrough
# and gpu_dawn_telemetry_tests for testing.
targets.bundle(
    name = "dawn_standalone_tests_compile_only",
    additional_compile_targets = [
        "dawn_end2end_tests",
        "dawn_perf_tests",
    ],
)

targets.bundle(
    name = "desktop_chromium_gtests",
    targets = [
  "accessibility_unittests",
  "blink_fuzzer_unittests",
  "browser_tests",
  "chrome_app_unittests",
  "chromedriver_unittests",
  "extensions_unittests",
  "filesystem_service_unittests",
  "interactive_ui_tests",
  "message_center_unittests",
  "native_theme_unittests",
  "pdf_unittests",
  "printing_unittests",
  "remoting_unittests",
  "snapshot_unittests",
  "sync_integration_tests",
  "ui_unittests",
  "views_unittests",
],
    per_test_modifications = {
  "browser_tests": targets.mixin(
    swarming = targets.swarming(
      shards = 10,
    ),
  ),
  "interactive_ui_tests": targets.mixin(
    swarming = targets.swarming(
      shards = 3,
    ),
  ),
  "sync_integration_tests": targets.mixin(
    swarming = targets.swarming(
      shards = 3,
    ),
  ),
},
)

targets.bundle(
    name = "desktop_chromium_gtests_no_nacl",
    targets = [
        "accessibility_unittests",
        "blink_fuzzer_unittests",
        "browser_tests",
        "chrome_app_unittests",
        "chromedriver_unittests",
        "extensions_unittests",
        "filesystem_service_unittests",
        "interactive_ui_tests",
        "message_center_unittests",
        "native_theme_unittests",
        "pdf_unittests",
        "printing_unittests",
        "remoting_unittests",
        "snapshot_unittests",
        "sync_integration_tests",
        "ui_unittests",
        "views_unittests",
    ],
    per_test_modifications = {
        "browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 15,
            ),
        ),
        "interactive_ui_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
        "sync_integration_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
    },
)

targets.bundle(
    name = "desktop_chromium_gtests_skia_gold",
    targets = [
        "views_examples_unittests",
    ],
    per_test_modifications = {
        "views_examples_unittests": [
            "skia_gold_test",
        ],
    },
)

targets.bundle(
    name = "desktop_chromium_isolated_scripts",
    targets = [
        "blink_python_tests",
        "blink_web_tests",
        "blink_wpt_tests",
        "chrome_wpt_tests",
        "content_shell_crash_test",
        "flatbuffers_unittests",
        "grit_python_unittests",
        "headless_shell_wpt_tests",
        "telemetry_gpu_unittests",
        "telemetry_unittests",
        "views_perftests",
    ],
    per_test_modifications = {
        "blink_web_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 5,
            ),
        ),
        "blink_wpt_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 7,
            ),
        ),
        "chrome_wpt_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 1,
            ),
        ),
        "headless_shell_wpt_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 18,
            ),
        ),
        "telemetry_gpu_unittests": targets.mixin(
            swarming = targets.swarming(
                idempotent = False,  # https://crbug.com/549140
            ),
        ),
        "telemetry_unittests": targets.mixin(
            args = [
                "--jobs=1",
                # Disable GPU compositing, telemetry_unittests runs on VMs.
                # https://crbug.com/871955
                "--extra-browser-args=--disable-gpu",
            ],
            swarming = targets.swarming(
                shards = 8,
                idempotent = False,  # https://crbug.com/549140
            ),
            resultdb = targets.resultdb(
                enable = True,
            ),
        ),
        "views_perftests": targets.mixin(
            args = [
                "--gtest-benchmark-name=views_perftests",
            ],
        ),
    },
)

targets.bundle(
    name = "desktop_chromium_mac_osxbeta_scripts",
    targets = [
        "content_shell_crash_test",
        "flatbuffers_unittests",
        "grit_python_unittests",
        "telemetry_gpu_unittests",
        "telemetry_unittests",
        "views_perftests",
    ],
    per_test_modifications = {
        "telemetry_gpu_unittests": targets.mixin(
            swarming = targets.swarming(
                idempotent = False,  # https://crbug.com/549140
            ),
        ),
        "telemetry_unittests": targets.mixin(
            args = [
                "--jobs=1",
                # Disable GPU compositing, telemetry_unittests runs on VMs.
                # https://crbug.com/871955
                "--extra-browser-args=--disable-gpu",
            ],
            swarming = targets.swarming(
                shards = 8,
                idempotent = False,  # https://crbug.com/549140
            ),
            resultdb = targets.resultdb(
                enable = True,
            ),
        ),
        "views_perftests": targets.mixin(
            args = [
                "--gtest-benchmark-name=views_perftests",
            ],
        ),
    },
)

# Script tests that only need to run on one builder per desktop platform.
targets.bundle(
    name = "desktop_headless_gtests",
    targets = [
        "headless_browsertests",
        "headless_unittests",
    ],
)

targets.bundle(
    name = "desktop_once_isolated_scripts",
    targets = [
        "test_env_py_unittests",
        "webui_resources_tools_python_unittests",
        "xvfb_py_unittests",
    ],
)

targets.bundle(
    name = "enterprise_companion_gtests_linux",
    targets = [
        "enterprise_companion_integration_tests",
        "enterprise_companion_tests",
    ],
    per_test_modifications = {
        "enterprise_companion_integration_tests": [
            "updater-tests-pool",
        ],
        "enterprise_companion_tests": [
            "updater-default-pool",
        ],
    },
)

targets.bundle(
    name = "enterprise_companion_gtests_mac",
    targets = [
        "enterprise_companion_integration_tests",
        "enterprise_companion_tests",
    ],
    per_test_modifications = {
        "enterprise_companion_integration_tests": [
            "updater-tests-pool",
        ],
        "enterprise_companion_tests": [
            "updater-tests-pool",
        ],
    },
)

targets.bundle(
    name = "enterprise_companion_gtests_win",
    targets = [
        "enterprise_companion_integration_tests",
        "enterprise_companion_tests",
    ],
    per_test_modifications = {
        "enterprise_companion_integration_tests": [
            "integrity_high",
            "updater-default-pool",
        ],
        "enterprise_companion_tests": [
            "integrity_high",
            "updater-default-pool",
        ],
    },
)

targets.bundle(
    name = "fieldtrial_browser_tests",
    targets = [
        "browser_tests_no_field_trial",
        "components_browsertests_no_field_trial",
        "interactive_ui_tests_no_field_trial",
        "sync_integration_tests_no_field_trial",
    ],
    per_test_modifications = {
        "browser_tests_no_field_trial": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 20,
            ),
        ),
        "components_browsertests_no_field_trial": targets.mixin(
            ci_only = True,
        ),
        "interactive_ui_tests_no_field_trial": targets.mixin(
            ci_only = True,
        ),
        "sync_integration_tests_no_field_trial": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
    },
)

targets.bundle(
    name = "fieldtrial_browser_tests_mac",
    targets = [
        "accessibility_unittests_no_field_trial",
        "browser_tests_no_field_trial",
        "components_browsertests_no_field_trial",
        "content_browsertests_no_field_trial",
        "interactive_ui_tests_no_field_trial",
        "sync_integration_tests_no_field_trial",
    ],
    per_test_modifications = {
        "accessibility_unittests_no_field_trial": targets.mixin(
            ci_only = True,
        ),
        "browser_tests_no_field_trial": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 10,
            ),
        ),
        "components_browsertests_no_field_trial": targets.mixin(
            ci_only = True,
        ),
        "content_browsertests_no_field_trial": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 8,
            ),
        ),
        "interactive_ui_tests_no_field_trial": targets.mixin(
            ci_only = True,
        ),
        "sync_integration_tests_no_field_trial": targets.mixin(
            ci_only = True,
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
    },
)

targets.bundle(
    name = "gl_gtests_passthrough",
    targets = [
        "gl_tests_passthrough",
        "gl_unittests",
    ],
    per_test_modifications = {
        "gl_tests_passthrough": targets.mixin(
            linux_args = [
                "--no-xvfb",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

targets.bundle(
    name = "gpu_all_linux_debug_gtests",
    targets = [
        "gpu_linux_debug_gtests",
        "gpu_fyi_linux_debug_gtests",
    ],
)

targets.bundle(
    name = "gpu_all_linux_debug_telemetry_tests",
    targets = [
        "gpu_linux_debug_telemetry_tests",
        "gpu_fyi_linux_debug_telemetry_tests",
    ],
    per_test_modifications = {
        "webgl_conformance_tests": targets.remove(
            reason = "Default behavior only tested on non-FYI",
        ),
    },
)

targets.bundle(
    name = "gpu_all_linux_release_gtests",
    targets = [
        "gpu_linux_release_gtests",
        "gpu_fyi_linux_release_gtests",
    ],
)

targets.bundle(
    name = "gpu_all_linux_release_telemetry_tests",
    targets = [
        "gpu_linux_release_telemetry_tests",
        "gpu_fyi_linux_release_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_all_linux_tsan_gtests",
    targets = [
        "gpu_linux_tsan_gtests",
        "gpu_fyi_linux_tsan_gtests",
    ],
)

targets.bundle(
    name = "gpu_all_linux_wayland_release_telemetry_tests",
    targets = [
        "gpu_linux_wayland_release_telemetry_tests",
        "gpu_fyi_linux_wayland_release_telemetry_tests",
    ],
    per_test_modifications = {
        "webgl_conformance_tests": targets.remove(
            reason = "Only run the default behavior tests on non-FYI testers",
        ),
    },
)

targets.bundle(
    name = "gpu_all_mac_debug_gtests",
    targets = [
        "gpu_mac_debug_gtests",
        "gpu_fyi_mac_debug_gtests",
    ],
)

targets.bundle(
    name = "gpu_all_mac_debug_telemetry_tests",
    targets = [
        "gpu_mac_debug_telemetry_tests",
        "gpu_fyi_mac_debug_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_all_mac_release_gtests",
    targets = [
        "gpu_mac_release_gtests",
        "gpu_fyi_mac_release_gtests",
    ],
)

targets.bundle(
    name = "gpu_all_mac_release_telemetry_tests",
    targets = [
        "gpu_mac_release_telemetry_tests",
        "gpu_fyi_mac_release_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_all_win_debug_gtests",
    targets = [
        "gpu_win_debug_gtests",
        "gpu_fyi_win_debug_gtests",
    ],
)

targets.bundle(
    name = "gpu_all_win_debug_telemetry_tests",
    targets = [
        "gpu_win_debug_telemetry_tests",
        "gpu_fyi_win_debug_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_all_win_release_gtests",
    targets = [
        "gpu_win_release_gtests",
        "gpu_fyi_win_release_gtests",
    ],
)

targets.bundle(
    name = "gpu_all_win_release_isolated_scripts",
    targets = [
        "gpu_win_release_isolated_scripts",
        "gpu_fyi_win_release_isolated_scripts",
    ],
)

targets.bundle(
    name = "gpu_all_win_release_telemetry_tests",
    targets = [
        "gpu_win_release_telemetry_tests",
        "gpu_fyi_win_release_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_angle_linux_telemetry_tests",
    targets = [
        "info_collection_tests",
        "trace_test",
        "webgl_conformance_tests",
        "webgl2_conformance_gl_passthrough_tests",
    ],
)

targets.bundle(
    name = "gpu_angle_mac_telemetry_tests",
    targets = [
        "gpu_webgl2_conformance_metal_passthrough_graphite_telemetry_tests",
        "gpu_webgl_conformance_metal_passthrough_graphite_telemetry_tests",
        "gpu_webgl_conformance_swangle_passthrough_representative_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "info_collection_tests",
    ],
)

targets.bundle(
    name = "gpu_angle_win_intel_nvidia_telemetry_tests",
    targets = [
        "gpu_webgl2_conformance_d3d11_passthrough_telemetry_tests",
        "gpu_webgl_conformance_d3d11_passthrough_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "info_collection_tests",
    ],
)

# The command buffer perf tests are only run on Windows.
# They are mostly driver and platform independent.
targets.bundle(
    name = "gpu_command_buffer_perf_passthrough_isolated_scripts",
    targets = [
        "passthrough_command_buffer_perftests",
    ],
    per_test_modifications = {
        "passthrough_command_buffer_perftests": targets.mixin(
            args = [
                "--gtest-benchmark-name=passthrough_command_buffer_perftests",
                "-v",
                "--use-cmd-decoder=passthrough",
                "--use-angle=gl-null",
                "--fast-run",
            ],
        ),
    },
)

# GPU gtests which run on both the main and FYI waterfalls.
targets.bundle(
    name = "gpu_common_gtests_passthrough",
    targets = [
        "gl_tests_passthrough",
        "gl_unittests",
    ],
    per_test_modifications = {
        "gl_tests_passthrough": targets.mixin(
            args = [
                "--use-gl=angle",
            ],
            desktop_args = [
                "--use-gpu-in-tests",
            ],
            linux_args = [
                "--no-xvfb",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "gl_unittests": [
            targets.mixin(
                desktop_args = [
                    "--use-gpu-in-tests",
                ],
                linux_args = [
                    "--no-xvfb",
                ],
            ),
            "skia_gold_test",
        ],
    },
)

targets.bundle(
    name = "gpu_common_gtests_passthrough_swiftshader",
    targets = [
        "gl_tests_passthrough",
        "gl_unittests",
    ],
    per_test_modifications = {
        "gl_tests_passthrough": targets.mixin(
            args = [
                "--use-gl=angle",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "gl_unittests": [
            "skia_gold_test",
        ],
    },
)

targets.bundle(
    name = "gpu_common_gtests_validating",
    targets = [
        "gl_tests_validating",
        "gl_unittests",
    ],
    per_test_modifications = {
        "gl_tests_validating": targets.mixin(
            desktop_args = [
                "--use-gpu-in-tests",
            ],
            linux_args = [
                "--no-xvfb",
            ],
        ),
        "gl_unittests": [
            targets.mixin(
                desktop_args = [
                    "--use-gpu-in-tests",
                ],
                linux_args = [
                    "--no-xvfb",
                ],
            ),
            "skia_gold_test",
        ],
    },
)

targets.bundle(
    name = "gpu_common_metal_passthrough_graphite_telemetry_tests",
    targets = [
        "gpu_metal_passthrough_graphite_telemetry_tests",
        "gpu_webgl_conformance_metal_passthrough_graphite_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "info_collection_tests",
        "trace_test",
    ],
)

# Same as gpu_dawn_telemetry_tests, but without SwiftShader tests since
# SwiftShader is not used on Android.
targets.bundle(
    name = "gpu_dawn_asan_isolated_scripts",
    targets = [
        "gpu_dawn_common_isolated_scripts",
        "gpu_dawn_perf_smoke_isolated_scripts",
        "gpu_dawn_webgpu_blink_web_tests",
        "gpu_dawn_webgpu_blink_web_tests_force_swiftshader",
    ],
)

targets.bundle(
    name = "gpu_dawn_common_isolated_scripts",
    targets = [
        "telemetry_gpu_unittests",
    ],
    per_test_modifications = {
        # Test that expectations files are well-formed.
        "telemetry_gpu_unittests": targets.mixin(
            swarming = targets.swarming(
                # https://crbug.com/549140
                idempotent = False,
            ),
        ),
    },
)

targets.bundle(
    name = "gpu_dawn_compat_telemetry_tests",
    targets = [
        "gpu_dawn_web_platform_webgpu_cts_force_swiftshader",
        "gpu_dawn_webgpu_compat_cts",
        "gpu_dawn_webgpu_cts",
    ],
)

targets.bundle(
    name = "gpu_dawn_galaxy_s24_gtests",
    targets = [
        "dawn_end2end_tests",
    ],
    per_test_modifications = {
        "dawn_end2end_tests": targets.mixin(
        ),
    },
)

targets.bundle(
    name = "gpu_dawn_galaxy_s24_telemetry_tests",
    targets = [
        "webgpu_cts_tests",
    ],
    per_test_modifications = {
        "webgpu_cts_tests": [
            targets.mixin(
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
    },
)

# GPU gtests that test only Dawn
targets.bundle(
    name = "gpu_dawn_gtests",
    targets = [
        "dawn_end2end_implicit_device_sync_tests",
        "dawn_end2end_skip_validation_tests",
        "dawn_end2end_tests",
        "dawn_end2end_wire_tests",
    ],
    per_test_modifications = {
        "dawn_end2end_implicit_device_sync_tests": targets.mixin(
            linux_args = [
                "--no-xvfb",
            ],
            ci_only = True,  # https://crbug.com/dawn/1749
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "dawn_end2end_skip_validation_tests": targets.mixin(
            linux_args = [
                "--no-xvfb",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "dawn_end2end_tests": targets.mixin(
            linux_args = [
                "--no-xvfb",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
        "dawn_end2end_wire_tests": targets.mixin(
            linux_args = [
                "--no-xvfb",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

targets.bundle(
    name = "gpu_dawn_gtests_no_dxc",
    targets = [
        "dawn_end2end_no_dxc_tests",
    ],
    per_test_modifications = {
        "dawn_end2end_no_dxc_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

targets.bundle(
    name = "gpu_dawn_gtests_no_dxc_with_validation",
    targets = [
        "dawn_end2end_no_dxc_validation_layers_tests",
    ],
    per_test_modifications = {
        "dawn_end2end_no_dxc_validation_layers_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

# GPU gtests that test only Dawn with backend validation layers
targets.bundle(
    name = "gpu_dawn_gtests_with_validation",
    targets = [
        "dawn_end2end_validation_layers_tests",
    ],
    per_test_modifications = {
        "dawn_end2end_validation_layers_tests": targets.mixin(
            linux_args = [
                "--no-xvfb",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

targets.bundle(
    name = "gpu_dawn_integration_asan_gtests_passthrough",
    targets = [
        "gpu_common_gtests_passthrough",
        "gpu_dawn_gtests",
        "gpu_dawn_gtests_no_dxc",
    ],
)

# GPU gtests that test Dawn and integration with Chromium
# These tests are run both on the CI and trybots which test DEPS Dawn.
targets.bundle(
    name = "gpu_dawn_integration_gtests_passthrough",
    targets = [
        "gpu_common_gtests_passthrough",
        "gpu_dawn_gtests",
        "gpu_dawn_gtests_with_validation",
    ],
)

targets.bundle(
    name = "gpu_dawn_integration_gtests_passthrough_win_x64",
    targets = [
        "gpu_common_gtests_passthrough",
        "gpu_dawn_gtests",
        "gpu_dawn_gtests_no_dxc",
        "gpu_dawn_gtests_no_dxc_with_validation",
        "gpu_dawn_gtests_with_validation",
    ],
)

targets.bundle(
    name = "gpu_dawn_isolated_scripts",
    targets = [
        "gpu_dawn_common_isolated_scripts",
        "gpu_dawn_perf_smoke_isolated_scripts",
        "gpu_dawn_webgpu_blink_web_tests",
        "gpu_dawn_webgpu_blink_web_tests_force_swiftshader",
    ],
)

targets.bundle(
    name = "gpu_dawn_perf_smoke_isolated_scripts",
    targets = [
        "dawn_perf_tests",
    ],
)

targets.bundle(
    name = "gpu_dawn_telemetry_tests",
    targets = [
        "gpu_dawn_web_platform_webgpu_cts_force_swiftshader",
        "gpu_dawn_webgpu_cts",
    ],
)

targets.bundle(
    name = "gpu_dawn_telemetry_tests_fxc",
    targets = [
        "gpu_dawn_web_platform_webgpu_cts_force_swiftshader",
        "gpu_dawn_webgpu_cts_fxc",
    ],
)

targets.bundle(
    name = "gpu_dawn_telemetry_win_x64_tests",
    targets = [
        "gpu_dawn_web_platform_webgpu_cts_force_swiftshader",
        "gpu_dawn_webgpu_cts",
        "gpu_dawn_webgpu_cts_fxc",
    ],
)

targets.bundle(
    name = "gpu_dawn_tsan_gtests",
    targets = [
        "gpu_dawn_gtests",
    ],
)

targets.bundle(
    name = "gpu_dawn_web_platform_webgpu_cts_force_swiftshader",
    targets = [
        "webgpu_swiftshader_web_platform_cts_tests",
        "webgpu_swiftshader_web_platform_cts_with_validation_tests",
    ],
    per_test_modifications = {
        # We intentionally do not have worker versions of these tests since
        # non-SwiftShader coverage should be sufficient.
        "webgpu_swiftshader_web_platform_cts_tests": [
            targets.mixin(
                args = [
                    "--use-webgpu-adapter=swiftshader",
                    "--test-filter=*web_platform*",
                ],
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_swiftshader_web_platform_cts_with_validation_tests": [
            targets.mixin(
                args = [
                    "--use-webgpu-adapter=swiftshader",
                    "--test-filter=*web_platform*",
                    "--enable-dawn-backend-validation",
                ],
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
    },
)

targets.bundle(
    name = "gpu_dawn_webgpu_blink_web_tests",
    targets = [
        "webgpu_blink_web_tests",
        "webgpu_blink_web_tests_with_backend_validation",
    ],
    per_test_modifications = {
        "webgpu_blink_web_tests": [
            targets.mixin(
                args = [
                    "--flag-specific=webgpu",
                ],
            ),
            "webgpu_cts",
        ],
        "webgpu_blink_web_tests_with_backend_validation": [
            targets.mixin(
                args = [
                    "--flag-specific=webgpu-with-backend-validation",
                    # Increase the timeout when using backend validation layers (crbug.com/1208253)
                    "--timeout-ms=30000",
                ],
            ),
            "webgpu_cts",
        ],
    },
)

targets.bundle(
    name = "gpu_dawn_webgpu_blink_web_tests_force_swiftshader",
    targets = [
        "webgpu_swiftshader_blink_web_tests",
        "webgpu_swiftshader_blink_web_tests_with_backend_validation",
    ],
    per_test_modifications = {
        "webgpu_swiftshader_blink_web_tests": [
            targets.mixin(
                args = [
                    "--flag-specific=webgpu-swiftshader",
                ],
            ),
            "webgpu_cts",
        ],
        "webgpu_swiftshader_blink_web_tests_with_backend_validation": [
            targets.mixin(
                args = [
                    "--flag-specific=webgpu-swiftshader-with-backend-validation",
                    # Increase the timeout when using backend validation layers (crbug.com/1208253)
                    "--timeout-ms=30000",
                ],
            ),
            "webgpu_cts",
        ],
    },
)

targets.bundle(
    name = "gpu_dawn_webgpu_compat_cts",
    targets = [
        "webgpu_cts_compat_tests",
        "webgpu_cts_compat_min_es31_tests",
    ],
    per_test_modifications = {
        # Worker versions of compat tests intentionally omitted since it is
        # unlikely that the compat path will interact with workers.
        "webgpu_cts_compat_tests": [
            targets.mixin(
                args = [
                    "--extra-browser-args=--enable-features=WebGPUExperimentalFeatures --use-webgpu-adapter=opengles",
                ],
                linux_args = [
                    "--extra-browser-args=--use-angle=gl",
                ],
                swarming = targets.swarming(
                    shards = 14,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
        ],
        "webgpu_cts_compat_min_es31_tests": [
            targets.mixin(
                args = [
                    "--extra-browser-args=--enable-features=WebGPUExperimentalFeatures --use-webgpu-adapter=opengles --enable-dawn-features=gl_force_es_31_and_no_extensions",
                ],
                linux_args = [
                    "--extra-browser-args=--use-angle=gl",
                ],
                swarming = targets.swarming(
                    shards = 14,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
        ],
    },
)

targets.bundle(
    name = "gpu_dawn_webgpu_cts",
    targets = [
        "webgpu_cts_dedicated_worker_tests",
        "webgpu_cts_service_worker_tests",
        "webgpu_cts_shared_worker_tests",
        "webgpu_cts_tests",
        "webgpu_cts_with_validation_tests",
        # We intentionally do not have validation + worker tests since
        # no validation + worker should provide sufficient coverage.
    ],
    per_test_modifications = {
        "webgpu_cts_dedicated_worker_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 1,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_service_worker_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 1,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_shared_worker_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 1,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 14,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_with_validation_tests": [
            targets.mixin(
                args = [
                    "--enable-dawn-backend-validation",
                ],
                swarming = targets.swarming(
                    shards = 14,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
    },
)

targets.bundle(
    name = "gpu_dawn_webgpu_cts_asan",
    # We intentionally do not have fxc + worker tests since dxc + worker
    # should provide sufficient coverage.
    targets = [
        "webgpu_cts_dedicated_worker_tests",
        "webgpu_cts_fxc_tests",
        "webgpu_cts_service_worker_tests",
        "webgpu_cts_shared_worker_tests",
        "webgpu_cts_tests",
    ],
    per_test_modifications = {
        "webgpu_cts_dedicated_worker_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 1,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_fxc_tests": [
            targets.mixin(
                args = [
                    "--use-fxc",
                ],
                swarming = targets.swarming(
                    shards = 16,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_service_worker_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 1,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_shared_worker_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 1,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 16,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
    },
)

targets.bundle(
    name = "gpu_dawn_webgpu_cts_default_features",
    targets = [
        "webgpu_cts_default_features_tests",
    ],
    per_test_modifications = {
        "webgpu_cts_default_features_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 14,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
    },
)

targets.bundle(
    name = "gpu_dawn_webgpu_cts_fxc",
    # We intentionally do not have fxc + worker tests since dxc + worker
    # should provide sufficient coverage.
    targets = [
        "webgpu_cts_fxc_tests",
        "webgpu_cts_fxc_with_validation_tests",
    ],
    per_test_modifications = {
        "webgpu_cts_fxc_tests": [
            targets.mixin(
                args = [
                    "--use-fxc",
                ],
                ci_only = True,
                swarming = targets.swarming(
                    shards = 14,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
        "webgpu_cts_fxc_with_validation_tests": [
            targets.mixin(
                args = [
                    "--enable-dawn-backend-validation",
                    "--use-fxc",
                ],
                ci_only = True,
                swarming = targets.swarming(
                    shards = 14,
                ),
            ),
            "gpu_integration_test_common_args",
            "webgpu_telemetry_cts",
            "linux_vulkan",
        ],
    },
)

targets.bundle(
    name = "gpu_default_and_optional_win_media_foundation_specific_gtests",
    targets = [
        # MediaFoundation browser tests, which currently only run on Windows OS,
        # and require physical hardware.
        "media_foundation_browser_tests",
    ],
    per_test_modifications = {
        "media_foundation_browser_tests": targets.mixin(
            args = [
                "--use-gpu-in-tests",
            ],
        ),
    },
)

targets.bundle(
    name = "gpu_default_and_optional_win_specific_gtests",
    targets = [
        "xr_browser_tests",
    ],
    per_test_modifications = {
        "xr_browser_tests": targets.mixin(
            args = [
                # The Windows machines this is run on should always meet all the
                # requirements, so skip the runtime checks to help catch issues, e.g.
                # if we're incorrectly being told a DirectX 11.1 device isn't
                # available
                "--ignore-runtime-requirements=*",
            ],
        ),
    },
)

targets.bundle(
    name = "gpu_desktop_passthrough_gtests",
    targets = [
        "gpu_common_gtests_passthrough",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "angle_unittests",
    ],
)

targets.bundle(
    name = "gpu_fyi_and_optional_non_linux_gtests",
    targets = [
        # gpu_unittests is killing the Swarmed Linux GPU bots similarly to
        # how content_unittests was: http://crbug.com/763498 .
        "gpu_unittests",
    ],
)

targets.bundle(
    name = "gpu_fyi_and_optional_win_specific_gtests",
    targets = [
        # WebNN DirectML backend unit tests, which currently only run on
        # Windows OS, and require physical hardware.
        "services_webnn_unittests",
    ],
    per_test_modifications = {
        "services_webnn_unittests": targets.mixin(
            args = [
                "--use-gpu-in-tests",
            ],
        ),
    },
)

targets.bundle(
    name = "gpu_fyi_linux_debug_gtests",
    targets = [
        # No Linux/Debug FYI-only gtests at the moment.
    ],
)

targets.bundle(
    name = "gpu_fyi_linux_debug_telemetry_tests",
    targets = [
        # No Linux/Debug FYI-only tests at the moment.
    ],
)

targets.bundle(
    name = "gpu_fyi_linux_release_gtests",
    targets = [
        "mappable_buffer_tests_suite",
        "gpu_vulkan_gtests",
    ],
)

targets.bundle(
    name = "gpu_fyi_linux_release_telemetry_tests",
    targets = [
        # TODO(jonross): remove this once Vulkan Swiftshader and Vulkan GL interop
        # paths are merged.
        "vulkan_pixel_skia_gold_test",
        "webcodecs_tests",
        "webgl2_conformance_gl_passthrough_tests",
        "webgl2_conformance_webgl_on_webgpu_tests",
        "webgl_conformance_webgl_on_webgpu_tests",
        "webrtc_tests",
    ],
)

targets.bundle(
    name = "gpu_fyi_linux_tsan_gtests",
    targets = [
        "gpu_common_gtests_passthrough_swiftshader",
    ],
)

# Wayland tests use GLES instead of GL due to how ANGLE interfaces with
# Wayland vs. X11.
targets.bundle(
    name = "gpu_fyi_linux_wayland_release_telemetry_tests",
    targets = [
        "gpu_fyi_linux_release_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "webgl2_conformance_gles_passthrough_tests",
        "webgl_conformance_gles_passthrough_tests",
    ],
    per_test_modifications = {
        "vulkan_pixel_skia_gold_test": targets.per_test_modification(
            replacements = targets.replacements(
                args = {
                    # Replaces --use-angle=gl with --use-angle=gles since
                    # Wayland requires running tests with GLES, not GL.
                    "--extra-browser-args": "--use-vulkan=native --disable-vulkan-fallback-to-gl-for-testing --enable-features=Vulkan --use-gl=angle --use-angle=gles --use-cmd-decoder=passthrough",
                },
            ),
        ),
        "webgl2_conformance_gl_passthrough_tests": targets.remove(
            reason = "Wayland requires running tests with GLES, not GL",
        ),
        "webgl2_conformance_webgl_on_webgpu_tests": targets.remove(
            reason = "WebGLOnWebGPU is only tested on X11 for now",
        ),
        "webgl_conformance_webgl_on_webgpu_tests": targets.remove(
            reason = "WebGLOnWebGPU is only tested on X11 for now",
        ),
    },
)

targets.bundle(
    name = "gpu_fyi_mac_debug_gtests",
    targets = [
        # Identical to the release gtests for now
        "gpu_fyi_mac_release_gtests",
    ],
)

targets.bundle(
    name = "gpu_fyi_mac_debug_telemetry_tests",
    targets = [
        # No FYI-only Mac/Debug Telemetry tests at this time.
    ],
)

targets.bundle(
    name = "gpu_fyi_mac_release_gtests",
    targets = [
        "gpu_fyi_and_optional_non_linux_gtests",
        "gpu_fyi_mac_specific_gtests",
    ],
)

targets.bundle(
    name = "gpu_fyi_mac_release_telemetry_tests",
    targets = [
        "gpu_webrtc_metal_passthrough_graphite_telemetry_test",
        "gpu_webcodecs_metal_passthrough_graphite_telemetry_test",
        "gpu_webgl2_conformance_metal_passthrough_graphite_telemetry_tests",
        "gpu_webgl_conformance_swangle_passthrough_representative_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_fyi_mac_specific_gtests",
    targets = [
        "screen_capture_kit_mac_tests",
        # Face and barcode detection unit tests, which currently only run on
        # Mac OS, and require physical hardware.
        "services_unittests",
    ],
    per_test_modifications = {
        "services_unittests": targets.mixin(
            args = [
                "--gtest_filter=*Detection*",
                "--use-gpu-in-tests",
            ],
        ),
    },
)

targets.bundle(
    name = "gpu_fyi_vulkan_swiftshader_gtests",
    targets = [
        "vulkan_swiftshader_content_browsertests",
    ],
    per_test_modifications = {
        "vulkan_swiftshader_content_browsertests": targets.mixin(
            args = [
                "--enable-gpu",
                "--test-launcher-bot-mode",
                "--test-launcher-jobs=1",
                "--test-launcher-filter-file=../../testing/buildbot/filters/vulkan.content_browsertests.filter",
                "--enable-features=UiGpuRasterization,Vulkan",
                "--use-vulkan=swiftshader",
                "--enable-gpu-rasterization",
                "--disable-software-compositing-fallback",
                "--disable-vulkan-fallback-to-gl-for-testing",
                "--disable-headless-mode",
            ],
            linux_args = [
                "--no-xvfb",
            ],
        ),
    },
)

targets.bundle(
    name = "gpu_fyi_win_debug_gtests",
    targets = [
        # Identical to the release gtests for now.
        "gpu_fyi_win_release_gtests",
    ],
)

targets.bundle(
    name = "gpu_fyi_win_debug_telemetry_tests",
    targets = [
        # No FYI-only Windows/Debug Telemetry tests at this time.
    ],
)

targets.bundle(
    name = "gpu_fyi_win_release_gtests",
    targets = [
        "gpu_default_and_optional_win_media_foundation_specific_gtests",
        "gpu_fyi_and_optional_non_linux_gtests",
        "gpu_fyi_and_optional_win_specific_gtests",
    ],
)

targets.bundle(
    name = "gpu_fyi_win_release_isolated_scripts",
    targets = [
        "gpu_command_buffer_perf_passthrough_isolated_scripts",
    ],
)

targets.bundle(
    name = "gpu_fyi_win_release_telemetry_tests",
    targets = [
        "gpu_webgl2_conformance_d3d11_passthrough_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "context_lost_passthrough_graphite_tests",
        "expected_color_pixel_passthrough_graphite_test",
        "pixel_skia_gold_passthrough_graphite_test",
        "screenshot_sync_passthrough_graphite_tests",
        "webcodecs_tests",
        "webrtc_tests",
    ],
)

targets.bundle(
    name = "gpu_linux_debug_gtests",
    targets = [
        "gpu_desktop_passthrough_gtests",
    ],
)

targets.bundle(
    name = "gpu_linux_debug_telemetry_tests",
    targets = [
        "gpu_passthrough_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "info_collection_tests",
        "trace_test",
        "webgl_conformance_tests",
    ],
)

targets.bundle(
    name = "gpu_linux_release_gtests",
    targets = [
        "gpu_desktop_passthrough_gtests",
    ],
)

targets.bundle(
    name = "gpu_linux_release_telemetry_tests",
    targets = [
        "context_lost_passthrough_tests",
        "expected_color_pixel_passthrough_test",
        "gpu_process_launch_tests",
        "hardware_accelerated_feature_tests",
        "info_collection_tests",
        "pixel_skia_gold_passthrough_test",
        "screenshot_sync_passthrough_tests",
        "trace_test",
        "webgl_conformance_tests",
    ],
)

targets.bundle(
    name = "gpu_linux_tsan_gtests",
    targets = [
        # No non-FYI Linux/TSan gtests at the moment.
    ],
)

targets.bundle(
    name = "gpu_linux_wayland_release_telemetry_tests",
    targets = [
        "gpu_linux_release_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_mac_debug_gtests",
    targets = [
        "gpu_mac_release_gtests",
    ],
)

targets.bundle(
    name = "gpu_mac_debug_telemetry_tests",
    targets = [
        "gpu_mac_release_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_mac_release_gtests",
    targets = [
        "gpu_desktop_passthrough_gtests",
    ],
)

targets.bundle(
    name = "gpu_mac_release_telemetry_tests",
    targets = [
        "gpu_metal_passthrough_graphite_telemetry_tests",
        "gpu_webgl_conformance_metal_passthrough_graphite_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "info_collection_tests",
        "trace_test",
    ],
)

targets.bundle(
    name = "gpu_noop_sleep_telemetry_test",
    targets = [
  "noop_sleep_tests",
],
    per_test_modifications = {
  "noop_sleep_tests": [
    "gpu_integration_test_common_args",
  ],
},
)

targets.bundle(
    name = "gpu_swangle_telemetry_tests",
    targets = [
        "gpu_webgl_conformance_swangle_passthrough_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_telemetry_tests_v8",
    targets = [
  "legacy_context_lost_validating_tests",
  "legacy_expected_color_pixel_validating_test",
  "legacy_gpu_process_launch_tests",
  "legacy_hardware_accelerated_feature_tests",
  "legacy_info_collection_tests",
  "legacy_pixel_skia_gold_validating_test",
  "legacy_screenshot_sync_validating_tests",
  "legacy_trace_test",
  "legacy_webgl_conformance_gles_passthrough_tests",
  "legacy_webgl_conformance_validating_tests",
],
)

targets.bundle(
    name = "gpu_vulkan_gtests",
    targets = [
        "vulkan_tests",
    ],
    per_test_modifications = {
        "vulkan_tests": targets.mixin(
            desktop_args = [
                "--use-gpu-in-tests",
            ],
            linux_args = [
                "--no-xvfb",
            ],
        ),
    },
)

targets.bundle(
    name = "gpu_webgl_conformance_swangle_passthrough_telemetry_tests",
    targets = [
        "webgl_conformance_swangle_passthrough_tests",
    ],
    per_test_modifications = {
        "webgl_conformance_swangle_passthrough_tests": [
            targets.mixin(
                args = [
                    "--xvfb",
                ],
                swarming = targets.swarming(
                    shards = 1,
                ),
            ),
        ],
    },
)

targets.bundle(
    name = "gpu_win_debug_gtests",
    targets = [
        "gpu_win_release_gtests",
    ],
)

targets.bundle(
    name = "gpu_win_debug_telemetry_tests",
    targets = [
        "gpu_win_release_telemetry_tests",
    ],
)

targets.bundle(
    name = "gpu_win_release_gtests",
    targets = [
        "gpu_common_gtests_passthrough",
        "gpu_default_and_optional_win_specific_gtests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "angle_unittests",
    ],
)

targets.bundle(
    name = "gpu_win_release_isolated_scripts",
    targets = [
        # No non-FYI isolated script tests at the moment.
    ],
)

targets.bundle(
    name = "gpu_win_release_telemetry_tests",
    targets = [
        "gpu_webgl_conformance_d3d11_passthrough_telemetry_tests",

        # Migrated individual tests below.
        # TODO(crbug.com/541312843): Remove this comment once all tests are
        # directly included.
        "context_lost_passthrough_tests",
        "expected_color_pixel_passthrough_test",
        "gpu_process_launch_tests",
        "hardware_accelerated_feature_tests",
        "info_collection_tests",
        "pixel_skia_gold_passthrough_test",
        "screenshot_sync_passthrough_tests",
        "trace_test",
    ],
)

# Use this for targets for which we only need the bare minimum coverage,
# roughly one build config per platform.
targets.bundle(
    name = "gtests_once",
    targets = [
        "layer_list_mode_cc_unittests",
        "trees_in_viz_blink_platform_unittests",
        "trees_in_viz_cc_unittests",
    ],
    per_test_modifications = {
        "trees_in_viz_blink_platform_unittests": [
            "skia_gold_test",
        ],
    },
)

targets.bundle(
    name = "headless_browser_gtests",
    targets = [
        "headless_browsertests",
        "headless_unittests",
    ],
)

targets.bundle(
    name = "headless_shell_wpt_tests_isolated_scripts",
    targets = [
        "headless_shell_wpt_tests_inverted",
    ],
    per_test_modifications = {
        "headless_shell_wpt_tests_inverted": targets.mixin(
            swarming = targets.swarming(
                shards = 5,
            ),
        ),
    },
)

targets.bundle(
    name = "js_code_coverage_browser_tests_suite",
    targets = [
        "js_code_coverage_browser_tests",
    ],
    per_test_modifications = {
        "js_code_coverage_browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 30,
            ),
        ),
    },
)

targets.bundle(
    name = "leak_detection_isolated_scripts",
    targets = [
        "memory.leak_detection",
    ],
    per_test_modifications = {
        "memory.leak_detection": targets.mixin(
            swarming = targets.swarming(
                shards = 10,
                expiration_sec = 36000,
                hard_timeout_sec = 10800,
                io_timeout_sec = 3600,
            ),
        ),
    },
)

targets.bundle(
    name = "legacy_context_lost_validating_tests",
    targets = [
  "context_lost_validating_tests",
],
)

targets.bundle(
    name = "legacy_expected_color_pixel_validating_test",
    targets = [
  "expected_color_pixel_validating_test",
],
)

targets.bundle(
    name = "legacy_gpu_process_launch_tests",
    targets = [
  "gpu_process_launch_tests",
],
)

targets.bundle(
    name = "legacy_hardware_accelerated_feature_tests",
    targets = [
  "hardware_accelerated_feature_tests",
],
)

targets.bundle(
    name = "legacy_pixel_skia_gold_validating_test",
    targets = [
  "pixel_skia_gold_validating_test",
],
)

targets.bundle(
    name = "legacy_screenshot_sync_validating_tests",
    targets = [
  "screenshot_sync_validating_tests",
],
)

targets.bundle(
    name = "legacy_webgl_conformance_gles_passthrough_tests",
    targets = [
  "webgl_conformance_gles_passthrough_tests",
],
)

targets.bundle(
    name = "legacy_webgl_conformance_validating_tests",
    targets = [
  "webgl_conformance_validating_tests",
],
)

targets.bundle(
    name = "linux_flavor_specific_chromium_gtests",
    targets = [
  "sandbox_linux_unittests",
],
)

targets.bundle(
    name = "linux_force_accessibility_gtests",
    targets = [
        "browser_tests",
        "content_browsertests",
        "interactive_ui_tests",
    ],
    per_test_modifications = {
        "browser_tests": targets.mixin(
            args = [
                "--force-renderer-accessibility",
                "--test-launcher-filter-file=../../testing/buildbot/filters/accessibility-linux.browser_tests.filter",
            ],
            swarming = targets.swarming(
                shards = 20,
            ),
        ),
        "content_browsertests": targets.mixin(
            args = [
                "--force-renderer-accessibility",
                "--test-launcher-filter-file=../../testing/buildbot/filters/accessibility-linux.content_browsertests.filter",
            ],
            swarming = targets.swarming(
                shards = 8,
            ),
        ),
        "interactive_ui_tests": targets.mixin(
            args = [
                "--force-renderer-accessibility",
                "--test-launcher-filter-file=../../testing/buildbot/filters/accessibility-linux.interactive_ui_tests.filter",
            ],
            swarming = targets.swarming(
                shards = 6,
            ),
        ),
    },
)

targets.bundle(
    name = "linux_specific_xr_gtests",
    targets = [
        "xr_browser_tests",
    ],
)

targets.bundle(
    name = "linux_viz_gtests",
    targets = [
        "gpu_fyi_vulkan_swiftshader_gtests",
    ],
)

# Last MacOS supported on intel
# Run select tests to ensure minimum coverage
targets.bundle(
    name = "mac26_x86_tests",
    targets = [
        "telemetry_gpu_unittests",
        "gpu_unittests",
    ],
)

# TODO(crbug.com/40223516): Remove this set of test suites when LSan can be
# enabled Mac ASan bots. This list will be gradually filled with more tests
# until the bot has parity with ASan bots, and the ASan bot can then enable
# LSan and the mac-lsan-fyi-rel bot go away.
targets.bundle(
    name = "mac_lsan_fyi_gtests",
    targets = [
        "absl_hardening_tests",
        "accessibility_unittests",
        "base_unittests",
        "blink_heap_unittests",
        "blink_platform_unittests",
        "blink_unittests",
        "cc_unittests",
        "components_unittests",
        "content_unittests",
        "crashpad_tests",
        "device_unittests",
        "net_unittests",
    ],
)

targets.bundle(
    name = "mac_specific_chromium_gtests",
    targets = [
        "power_sampler_unittests",
        "sandbox_unittests",
        "updater_tests",
        "xr_browser_tests",
    ],
)

targets.bundle(
    name = "mac_specific_isolated_scripts",
    targets = [
        "mac_signing_tests",
        "views_examples_mac_signing_tests",
    ],
)

targets.bundle(
    name = "mac_vm_tests",
    targets = [
        "base_unittests",
        "browser_tests",
    ],
    per_test_modifications = {
        "browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 25,
            ),
        ),
    },
)

targets.bundle(
    name = "mappable_buffer_tests_suite",
    targets = [
        "mappable_buffer_tests",
    ],
    per_test_modifications = {
        "mappable_buffer_tests": targets.mixin(
            args = [
                "--enable-gpu",
                "--use-gpu-in-tests",
                "--gtest_filter=*MappableBufferTest*",
            ],
            linux_args = [
                "--no-xvfb",
            ],
        ),
    },
)

targets.bundle(
    name = "mojo_python_unittests_isolated_scripts",
    targets = [
        "mojo_python_unittests",
    ],
)

targets.bundle(
    name = "network_service_extra_gtests",
    targets = [
        "network_service_fyi_gtests",
    ],
)

targets.bundle(
    name = "network_service_fyi_gtests",
    targets = [
        "network_service_web_request_proxy_browser_tests",
    ],
    per_test_modifications = {
        "network_service_web_request_proxy_browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 15,
            ),
        ),
    },
)

targets.bundle(
    name = "perfetto_diff_tests_bundle",
    targets = [
        "perfetto_diff_tests",
    ],
)

targets.bundle(
    name = "perfetto_gtests",
    targets = [
        "base_unittests",
        "browser_tests",
        "content_browsertests",
        "perfetto_unittests",
        "services_unittests",
    ],
    per_test_modifications = {
        "browser_tests": targets.mixin(
            args = [
                "--gtest_filter=ChromeTracingDelegateBrowserTest.*",
            ],
        ),
        "content_browsertests": targets.mixin(
            swarming = targets.swarming(
                shards = 8,
            ),
        ),
    },
)

targets.bundle(
    name = "performance_smoke_test_isolated_scripts",
    targets = [
        "performance_test_suite",
    ],
    per_test_modifications = {
        "performance_test_suite": targets.mixin(
            args = [
                "--pageset-repeat=1",
                "--test-shard-map-filename=smoke_test_benchmark_shard_map.json",
            ],
            swarming = targets.swarming(
                shards = 2,
                hard_timeout_sec = 960,
            ),
            resultdb = targets.resultdb(
                enable = True,
            ),
        ),
    },
)

targets.bundle(
    name = "pixel_browser_tests_gtests",
    targets = [
        "pixel_browser_tests",
        "pixel_interactive_ui_tests",
    ],
    per_test_modifications = {
        "pixel_browser_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
        "pixel_interactive_ui_tests": targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    },
)

# TODO(dpranke): These are run on the p/chromium waterfall; they should
# probably be run on other builders, and we should get rid of the p/chromium
# waterfall.
targets.bundle(
    name = "public_build_scripts",
    targets = [
        "checkbins",
    ],
)

targets.bundle(
    name = "pytype_tests",
    targets = [
        "blink_pytype",
        "gold_common_pytype",
        "gpu_pytype",
        "testing_pytype",
    ],
)

targets.bundle(
    name = "swangle_gtests",
    targets = [
        "angle_deqp_egl_tests",
        "angle_deqp_gles2_tests",
        "angle_deqp_gles31_rotate180_tests",
        "angle_deqp_gles31_rotate270_tests",
        "angle_deqp_gles31_rotate90_tests",
        "angle_deqp_gles31_tests",
        "angle_deqp_gles3_rotate180_tests",
        "angle_deqp_gles3_rotate270_tests",
        "angle_deqp_gles3_rotate90_tests",
        "angle_deqp_gles3_tests",
        "angle_deqp_khr_gles2_tests",
        "angle_deqp_khr_gles31_tests",
        "angle_deqp_khr_gles3_tests",
        "angle_end2end_tests",
    ],
    per_test_modifications = {
        "angle_deqp_egl_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles2_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles31_rotate180_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles31_rotate270_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles31_rotate90_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles31_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            swarming = targets.swarming(
                shards = 10,
            ),
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles3_rotate180_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles3_rotate270_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles3_rotate90_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_gles3_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            swarming = targets.swarming(
                shards = 4,
            ),
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_khr_gles2_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_khr_gles31_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_deqp_khr_gles3_tests": targets.mixin(
            args = [
                "--use-angle=swiftshader",
            ],
            use_isolated_scripts_api = True,
        ),
        "angle_end2end_tests": targets.mixin(
            args = [
                "--gtest_filter=*Vulkan_SwiftShader*",
            ],
            swarming = targets.swarming(
                shards = 2,
            ),
            use_isolated_scripts_api = True,
        ),
    },
)

targets.bundle(
    name = "telemetry_desktop_minidump_unittests_isolated_scripts",
    targets = [
        # Takes ~2.5 minutes of bot time to run.
        "telemetry_desktop_minidump_unittests",
    ],
    per_test_modifications = {
        "telemetry_desktop_minidump_unittests": targets.mixin(
            resultdb = targets.resultdb(
                enable = True,
            ),
        ),
    },
)

targets.bundle(
    name = "telemetry_perf_unittests_isolated_scripts",
    targets = [
        "telemetry_perf_unittests",
    ],
    per_test_modifications = {
        "telemetry_perf_unittests": targets.mixin(
            args = [
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
    },
)

targets.bundle(
    name = "telemetry_perf_unittests_isolated_scripts_xvfb",
    targets = [
        "telemetry_perf_unittests",
    ],
    per_test_modifications = {
        "telemetry_perf_unittests": targets.mixin(
            args = [
                # TODO(crbug.com/40129085): Remove this once Crashpad is the default.
                "--extra-browser-args=--enable-crashpad",
                "--xvfb",
            ],
            swarming = targets.swarming(
                shards = 12,
                # https://crbug.com/549140
                idempotent = False,
            ),
            resultdb = targets.resultdb(
                enable = True,
            ),
        ),
    },
)

targets.bundle(
    name = "test_traffic_annotation_auditor_script",
    targets = [
        "test_traffic_annotation_auditor",
    ],
)

targets.bundle(
    name = "trees_in_viz_disabled_tests",
    targets = [
        "cc_unittests",
        "viz_unittests",
        "blink_web_tests",
        "content_browsertests",
        "browser_tests",
    ],
    per_test_modifications = {
        "cc_unittests": targets.mixin(
            args = ["--disable-features=TreesInViz"],
        ),
        "viz_unittests": targets.mixin(
            args = ["--disable-features=TreesInViz"],
        ),
        "blink_web_tests": targets.mixin(
            args = ["--additional-driver-flag=--disable-features=TreesInViz"],
        ),
        "content_browsertests": targets.mixin(
            args = ["--disable-features=TreesInViz"],
        ),
        "browser_tests": targets.mixin(
            args = ["--disable-features=TreesInViz"],
        ),
    },
)

targets.bundle(
    name = "trees_in_viz_enabled_tests",
    targets = [
        "cc_unittests",
        "viz_unittests",
        "blink_web_tests",
        "content_browsertests",
        "browser_tests",
    ],
    per_test_modifications = {
        "cc_unittests": targets.mixin(
            args = ["--enable-features=TreesInViz"],
        ),
        "viz_unittests": targets.mixin(
            args = ["--enable-features=TreesInViz"],
        ),
        "blink_web_tests": targets.mixin(
            args = ["--additional-driver-flag=--enable-features=TreesInViz"],
        ),
        "content_browsertests": targets.mixin(
            args = ["--enable-features=TreesInViz"],
        ),
        "browser_tests": targets.mixin(
            args = ["--enable-features=TreesInViz"],
        ),
    },
)

targets.bundle(
    name = "updater_gtests_linux",
    targets = [
        "updater_tests",
        "updater_fuzztests",
        # 'updater_tests_system' is not yet supported on Linux.
    ],
    per_test_modifications = {
        "updater_tests": [
            "updater-default-pool",
        ],
    },
)

targets.bundle(
    name = "updater_gtests_mac",
    targets = [
        "updater_tests",
        "updater_tests_system",
        "updater_fuzztests",
    ],
    per_test_modifications = {
        "updater_tests": [
            "updater-default-pool",
        ],
        "updater_tests_system": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "updater-tests-pool",
        ],
    },
)

targets.bundle(
    name = "updater_gtests_win",
    targets = [
        "updater_tests",
        "updater_tests_system",
        "updater_fuzztests",
    ],
    per_test_modifications = {
        "updater_tests": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                    hard_timeout_sec = 7200,
                ),
            ),
            "integrity_high",
            "updater-default-pool",
        ],
        "updater_tests_system": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                    hard_timeout_sec = 7200,
                ),
            ),
            "integrity_high",
            "updater-default-pool",
        ],
    },
)

targets.bundle(
    name = "updater_gtests_win_uac",
    targets = [
        "updater_tests_system",
        "updater_tests_win_uac",
    ],
    per_test_modifications = {
        "updater_tests_system": [
            targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                    hard_timeout_sec = 7200,
                ),
            ),
            "integrity_high",
            "updater-win-uac-pool",
        ],
        "updater_tests_win_uac": [
            "updater-win-uac-pool",
        ],
    },
)

targets.bundle(
    name = "updater_user_gtests_mac",
    targets = [
        "updater_tests",
    ],
    per_test_modifications = {
        "updater_tests": [
            "updater-default-pool",
        ],
    },
)

targets.bundle(
    name = "upload_perfetto",
    targets = [
        "upload_trace_processor",
    ],
)

targets.bundle(
    name = "vr_platform_specific_chromium_gtests",
    targets = [
        # Only run on platforms that intend to support WebVR in the near
        # future.
        "vr_common_unittests",
    ],
)

targets.bundle(
    name = "webrtc_chromium_gtests",
    targets = [
        "browser_tests",
        # TODO(crbug.com/246519185) - Py3 incompatible, decide if to keep test.:
        # Uncomment per-test modification if enabling
        # "browser_tests_apprtc",
        "browser_tests_functional",
        "content_browsertests",
        "content_browsertests_sequential",
        "content_browsertests_stress",
        "content_unittests",
        "remoting_unittests",
    ],
    per_test_modifications = {
        "browser_tests": targets.mixin(
            args = [
                "--gtest_filter=WebRtcStatsPerfBrowserTest.*:WebRtcVideoDisplayPerfBrowserTests*:WebRtcVideoQualityBrowserTests*:WebRtcVideoHighBitrateBrowserTest*:WebRtcWebcamBrowserTests*",
                "--ui-test-action-max-timeout=300000",
                "--test-launcher-timeout=350000",
                "--test-launcher-jobs=1",
                "--test-launcher-bot-mode",
                "--test-launcher-print-test-stdio=always",
            ],
        ),
        # "browser_tests_apprtc": targets.mixin(
        #     args = [
        #         "--gtest_filter=WebRtcApprtcBrowserTest.*",
        #         "--test-launcher-jobs=1",
        #     ],
        # ),
        # Run all normal WebRTC content_browsertests. This is mostly so
        # the FYI bots can detect breakages.
        "content_browsertests": targets.mixin(
            args = [
                "--gtest_filter=WebRtc*",
            ],
        ),
        "content_unittests": targets.mixin(
            args = [
                "--test-launcher-filter-file=../../testing/buildbot/filters/webrtc.content_unittests.filter",
            ],
        ),
        "remoting_unittests": targets.mixin(
            args = [
                "--gtest_filter=Webrtc*",
            ],
        ),
    },
)

targets.bundle(
    name = "webrtc_chromium_simple_gtests",
    targets = [
        "content_browsertests",
        "content_browsertests_sequential",
    ],
    per_test_modifications = {
        "content_browsertests": targets.mixin(
            args = [
                "--gtest_filter=WebRtc*",
            ],
        ),
    },
)

targets.bundle(
    name = "win_specific_chromium_gtests",
    targets = [
        "chrome_elf_unittests",
        "delayloads_unittests",
        "elevated_tracing_service_unittests",
        "elevation_service_unittests",
        "gcp_unittests",
        "install_static_unittests",
        "installer_util_unittests",
        "notification_helper_unittests",
        "sbox_integration_tests",
        "sbox_unittests",
        "sbox_validation_tests",
        "setup_unittests",
        "updater_tests",
        "updater_tests_system",
        "zucchini_unittests",
    ],
    per_test_modifications = {
        "installer_util_unittests": targets.mixin(
            swarming = targets.swarming(
                dimensions = {
                    "integrity": "high",
                },
            ),
        ),
        "sbox_integration_tests": targets.mixin(
            swarming = targets.swarming(
                dimensions = {
                    "integrity": "high",
                },
            ),
        ),
        "setup_unittests": targets.mixin(
            swarming = targets.swarming(
                dimensions = {
                    "integrity": "high",
                },
            ),
        ),
        "updater_tests_system": targets.mixin(
            swarming = targets.swarming(
                shards = 2,
                hard_timeout_sec = 7200,
            ),
        ),
    },
)

targets.bundle(
    name = "win_specific_xr_perf_tests",
    targets = [
        "xr.webxr.static",
    ],
    per_test_modifications = {
        "xr.webxr.static": targets.mixin(
            experiment_percentage = 100,
        ),
    },
)

targets.bundle(
    name = "win_x86_specific_smoke_tests",
    targets = [
        "base_unittests",
        "sbox_integration_tests",
        "sbox_unittests",
        "sbox_validation_tests",
    ],
    per_test_modifications = {
        "sbox_integration_tests": targets.mixin(swarming = targets.swarming(dimensions = {
            "integrity": "high",
        })),
    },
)

# go/keep-sorted end
# No bundle definitions should be below
