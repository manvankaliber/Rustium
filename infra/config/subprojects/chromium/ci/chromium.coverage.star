# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("@chromium-luci//args.star", "args")
load("@chromium-luci//branches.star", "branches")
load("@chromium-luci//builder_config.star", "builder_config")
load("@chromium-luci//builder_health_indicators.star", "health_spec")
load("@chromium-luci//builders.star", "builders", "cpu", "os")
load("@chromium-luci//ci.star", "ci")
load("@chromium-luci//consoles.star", "consoles")
load("@chromium-luci//gn_args.star", "gn_args")
load("@chromium-luci//targets.star", "targets")
load("//lib/ci_constants.star", "ci_constants")
load("//lib/gardener_rotations.star", "gardener_rotations")
load("//lib/siso.star", "siso")
load("//lib/xcode.star", "xcode")
load("//project.star", "settings")

# crbug/1408581 - The code coverage CI builders are expected to be triggered
# off the same ref every 24 hours. This poller is configured with a schedule
# to ensure this - setting schedules on the builder configuration does not
# guarantee that they are triggered off the same ref.
luci.gitiles_poller(
    name = "code-coverage-gitiles-trigger",
    bucket = "ci",
    repo = "https://chromium.googlesource.com/chromium/src",
    refs = [settings.ref],
    # Trigger coverage jobs once a day at 4 am UTC(8 pm PST)
    schedule = "0 4 * * *",
)

ci.defaults.set(
    executable = ci_constants.DEFAULT_EXECUTABLE,
    builder_group = "chromium.coverage",
    pool = ci_constants.DEFAULT_POOL,
    cores = 32,
    ssd = True,
    execution_timeout = 20 * time.hour,
    experiments = {
        "chromium_tests.resultdb_module": 100,
    },
    health_spec = health_spec.default(),
    priority = ci_constants.DEFAULT_FYI_PRIORITY,
    service_account = ci_constants.DEFAULT_SERVICE_ACCOUNT,
    shadow_service_account = ci_constants.DEFAULT_SHADOW_SERVICE_ACCOUNT,
    siso_project = siso.project.DEFAULT_TRUSTED,
    siso_remote_jobs = siso.remote_jobs.DEFAULT,
)

targets.builder_defaults.set(
    mixins = [
        "chromium-tester-service-account",
    ],
)

consoles.console_view(
    name = "chromium.coverage",
    title = "Code Coverage CI Builders",
)

def nightly_coverage_builder(**kwargs):
    kwargs.setdefault("triggered_by", ["code-coverage-gitiles-trigger"])
    kwargs.setdefault("triggering_policy", scheduler.greedy_batching(
        max_concurrent_invocations = 2,
    ))
    return ci.builder(
        schedule = "triggered",
        **kwargs
    )

def continuous_fuzz_coverage_builder(
        *,
        name,
        target_platform,
        os,
        category,
        short_name,
        fuzz_engine,
        target_arch = None,
        gn_extra_configs = [],
        **kwargs):
    gn_configs = [
        "use_clang_coverage",
        "no_symbols",
        "static",
        "mojo_fuzzer",
        fuzz_engine,
        "dcheck_off",
        "remoteexec",
        "pdf_xfa",
        "release",
        "x64",
    ] + gn_extra_configs

    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = ["use_clang_coverage"],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_clang",
            apply_configs = [
                "clobber",
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_arch = target_arch,
            target_bits = 64,
            target_platform = target_platform,
        ),
    )

    properties = {
        "collect_fuzz_coverage": True,
        "fuzz_engine": fuzz_engine,
    }
    if "properties" in kwargs:
        properties.update(kwargs.pop("properties"))

    kwargs.setdefault("builderless", True)
    kwargs.setdefault("contact_team_email", "chrome-fuzzing-core@google.com")
    kwargs.setdefault("executable", "recipe:chromium/fuzz")

    # TODO(crbug.com/449026537): Remove elevated timeout once performance improves.
    kwargs.setdefault("execution_timeout", 48 * time.hour)
    kwargs.setdefault("notifies", ["chrome-fuzzing-core"])
    kwargs.setdefault("triggered_by", ["chromium-gitiles-trigger"])
    kwargs.setdefault("triggering_policy", scheduler.greedy_batching(
        max_concurrent_invocations = 1,
    ))

    return ci.builder(
        name = name,
        builder_spec = builder_spec,
        gn_args = gn_args.config(
            configs = gn_configs,
        ),
        os = os,
        console_view_entry = [
            consoles.console_view_entry(
                category = category,
                short_name = short_name,
            ),
        ],
        properties = properties,
        schedule = "triggered",
        **kwargs
    )

def coverage_webview_builder(**kwargs):
    kwargs.setdefault("triggered_by", [])
    kwargs.setdefault("triggering_policy", scheduler.greedy_batching(
        max_concurrent_invocations = 2,
    ))
    return ci.builder(
        schedule = "triggered",
        **kwargs
    )

nightly_coverage_builder(
    name = "linux-js-code-coverage",
    description_html = "Builder for creating Linux x64 JavaScript code coverage builds.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "release_builder",
            "remoteexec",
            "clang",
            "no_symbols",
            "use_javascript_coverage",
            "optimize_webui_off",
            "linux",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "js_code_coverage_browser_tests_suite",
        ],
        mixins = [
            "isolate_profile_data",
            "linux-jammy",
        ],
    ),
    os = os.LINUX_DEFAULT,
    console_view_entry = [
        consoles.console_view_entry(
            category = "linux",
            short_name = "js",
        ),
    ],
    export_coverage_to_zoss = True,
    siso_remote_jobs = siso.remote_jobs.HIGH_JOBS_FOR_CI,
    use_javascript_coverage = True,
)

# Experimental builder. Does not export_coverage_to_zoss.
continuous_fuzz_coverage_builder(
    name = "linux-fuzz-coverage",
    description_html = "Experimental builder for creating Linux x64 libFuzzer Clang code coverage builds.",
    os = os.LINUX_DEFAULT,
    free_space = builders.free_space.high,
    target_platform = builder_config.target_platform.LINUX,
    category = "linux-fuzz",
    fuzz_engine = "libfuzzer",
    gn_extra_configs = [
        "linux",
    ],
    short_name = "lnx-fuzz",
)

# Experimental builder. Does not export_coverage_to_zoss.
continuous_fuzz_coverage_builder(
    name = "linux-centipede-fuzz-coverage",
    description_html = "This builder collects code coverage for centipede.",
    os = os.LINUX_DEFAULT,
    free_space = builders.free_space.high,
    target_platform = builder_config.target_platform.LINUX,
    category = "linux-fuzz",
    fuzz_engine = "centipede",
    gn_extra_configs = [
        "linux",
    ],
    short_name = "centipede",
)

# Experimental builder. Does not export_coverage_to_zoss.
nightly_coverage_builder(
    name = "linux-x64-fuzzilli-coverage",
    description_html = "This builder collects code coverage for V8 Fuzzilli tests.",
    executable = "recipe:chromium/fuzz",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = ["use_clang_coverage"],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_clang",
            apply_configs = [
                "clobber",
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "dcheck_always_on",
            "v8_backtrace",
            "v8_debug",
            "v8_heap",
            "v8_static",
            "use_clang_coverage",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    builderless = True,
    os = os.LINUX_DEFAULT,
    console_view_entry = [
        consoles.console_view_entry(
            category = "linux-fuzz",
            short_name = "fuzzlli-x64",
        ),
    ],
    contact_team_email = "v8-security@google.com",
    notifies = ["chrome-fuzzing-core"],
    properties = {
        "collect_fuzz_coverage": True,
        "fuzz_engine": "fuzzilli",
    },
)

nightly_coverage_builder(
    name = "linux-code-coverage",
    description_html = "Builder for creating Linux x64 Clang code coverage builds.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
                # This is necessary due to this builder running the
                # telemetry_perf_unittests suite.
                "chromium_with_telemetry_dependencies",
                "use_clang_coverage",
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium",
            apply_configs = ["mb"],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        args = {
            "enable_single_byte_coverage": True,
        },
        configs = [
            "release_builder",
            "remoteexec",
            "clang",
            "use_clang_coverage",
            "no_symbols",
            "chrome_with_codecs",
            "linux",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_linux_gtests",
            "chromium_linux_rel_isolated_scripts_code_coverage",
            "gpu_dawn_webgpu_cts",
            "gtests_once",
            "chromium_linux_scripts",
        ],
        mixins = [
            "isolate_profile_data",
            "linux-jammy",
        ],
        per_test_modifications = {
            "blink_pytype": targets.remove(
                reason = "pytype isn't impacted by building with coverage",
            ),
            "blink_web_tests": targets.mixin(
                args = [
                    "--additional-env-var=LLVM_PROFILE_FILE=${ISOLATED_OUTDIR}/profraw/default-%2m%c.profraw",
                ],
                swarming = targets.swarming(
                    shards = 8,
                ),
            ),
            "blink_wpt_tests": targets.mixin(
                args = [
                    "--additional-env-var=LLVM_PROFILE_FILE=${ISOLATED_OUTDIR}/profraw/default-%2m%c.profraw",
                ],
            ),
            "browser_tests": targets.mixin(
                args = [
                    "--no-sandbox",
                ],
                swarming = targets.swarming(
                    shards = 50,
                ),
            ),
            "check_static_initializers": targets.remove(
                reason = "Coverage instrumentation adds static initializers, so this test will always fail.",
            ),
            "content_browsertests": targets.mixin(
                args = [
                    "--no-sandbox",
                ],
                swarming = targets.swarming(
                    shards = 12,
                ),
            ),
            "gold_common_pytype": targets.remove(
                reason = "pytype isn't impacted by building with coverage",
            ),
            "gpu_pytype": targets.remove(
                reason = "pytype isn't impacted by building with coverage",
            ),
            "interactive_ui_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 10,
                ),
            ),
            "not_site_per_process_blink_web_tests": targets.mixin(
                args = [
                    "--additional-env-var=LLVM_PROFILE_FILE=${ISOLATED_OUTDIR}/profraw/default-%2m%c.profraw",
                    "--timeout-multiplier=5",
                ],
                swarming = targets.swarming(
                    shards = 20,
                ),
            ),
            "sync_integration_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
            "testing_pytype": targets.remove(
                reason = "pytype isn't impacted by building with coverage",
            ),
            # These tests must run with a GPU.
            "webgpu_blink_web_tests": [
                "linux_nvidia_gtx_1660_stable",
            ],
            "webgpu_blink_web_tests_with_backend_validation": targets.remove(
                reason = "Remove from bots where capacity is constrained.",
            ),
            # These tests must run with a GPU.
            "webgpu_cts_tests": [
                "linux_nvidia_gtx_1660_stable",
            ],
            "webgpu_cts_with_validation_tests": targets.remove(
                reason = "Don't need validation layers on code coverage bots.",
            ),
            "webgpu_cts_dedicated_worker_tests": [
                "linux_nvidia_gtx_1660_stable",
            ],
            "webgpu_cts_service_worker_tests": targets.remove(
                reason = "Dedicated worker tests are probably sufficient.",
            ),
            "webgpu_cts_shared_worker_tests": targets.remove(
                reason = "Dedicated worker tests are probably sufficient.",
            ),
        },
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.LINUX,
    ),
    os = os.LINUX_DEFAULT,
    console_view_entry = [
        consoles.console_view_entry(
            category = "linux",
            short_name = "lnx",
        ),
    ],
    coverage_test_types = ["overall", "unit"],
    export_coverage_to_zoss = True,
    use_clang_coverage = True,
)

nightly_coverage_builder(
    name = "mac-code-coverage",
    description_html = "Builder for creating Mac ARM64 Clang code coverage builds.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = ["use_clang_coverage"],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium",
            apply_configs = ["mb"],
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "release_builder",
            "remoteexec",
            "clang",
            "use_clang_coverage",
            "no_symbols",
            "chrome_with_codecs",
            "mac",
            "arm64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_mac_gtests",
            "chromium_mac_rel_isolated_scripts_code_coverage",
            # TODO(crbug.com/40249801): Enable gpu_dawn_webgpu_cts
            "gtests_once",
        ],
        mixins = [
            "isolate_profile_data",
            "mac_default_arm64",
        ],
        per_test_modifications = {
            "browser_tests": targets.remove(
                reason = "https://crbug.com/1201386",
            ),
            "content_browsertests": targets.mixin(
                args = [
                    "--coverage-continuous-mode=1",
                ],
            ),
            "interactive_ui_tests": targets.mixin(
                args = [
                    "--coverage-continuous-mode=1",
                ],
            ),
            "sync_integration_tests": targets.mixin(
                args = [
                    "--coverage-continuous-mode=1",
                ],
            ),
        },
    ),
    builderless = True,
    cores = None,
    os = os.MAC_ANY,
    cpu = cpu.ARM64,
    console_view_entry = [
        consoles.console_view_entry(
            category = "mac",
            short_name = "mac",
        ),
    ],
    coverage_test_types = ["overall", "unit"],
    export_coverage_to_zoss = True,
    siso_remote_jobs = siso.remote_jobs.HIGH_JOBS_FOR_CI,
    use_clang_coverage = True,
)

# Experimental builder. Does not export_coverage_to_zoss.
continuous_fuzz_coverage_builder(
    name = "mac-libfuzzer-coverage",
    description_html = "This builder collects code coverage for fuzz targets on Mac.",
    cores = None,
    # TODO(crbug.com/543006750): Revert to MAC_DEFAULT after arm migration.
    os = os.MAC_15,
    target_arch = builder_config.target_arch.INTEL,
    target_platform = builder_config.target_platform.MAC,
    category = "mac-fuzz",
    execution_timeout = 72 * time.hour,
    fuzz_engine = "libfuzzer",
    gn_extra_configs = [
        "chrome_with_codecs",
        "mac",
    ],
    short_name = "mac-libfuzz",
)

nightly_coverage_builder(
    name = "win10-code-coverage",
    description_html = "Builder for creating Windows 10 x64 Clang code coverage builds.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = ["use_clang_coverage"],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium",
            apply_configs = ["mb"],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "release_builder",
            "remoteexec",
            "clang",
            "use_clang_coverage",
            "no_symbols",
            "chrome_with_codecs",
            "win",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_win_gtests",
            "chromium_win_rel_isolated_scripts_code_coverage",
            "gpu_dawn_webgpu_cts",
            "gtests_once",
        ],
        mixins = [
            "isolate_profile_data",
            "win10",
        ],
        per_test_modifications = {
            "blink_unittests": targets.mixin(
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
            "browser_tests": targets.mixin(
                swarming = targets.swarming(
                    dimensions = {
                        "pool": "chromium.tests.coverage",
                        "ssd": "1",
                    },
                    shards = 40,
                ),
            ),
            "components_unittests": targets.mixin(
                swarming = targets.swarming(
                    hard_timeout_sec = 5400,
                    shards = 6,
                ),
            ),
            "content_browsertests": targets.mixin(
                swarming = targets.swarming(
                    dimensions = {
                        "pool": "chromium.tests.coverage",
                        "ssd": "1",
                    },
                    shards = 40,
                ),
            ),
            "content_unittests": targets.mixin(
                swarming = targets.swarming(
                    hard_timeout_sec = 5400,
                    shards = 2,
                ),
            ),
            "interactive_ui_tests": targets.mixin(
                swarming = targets.swarming(
                    dimensions = {
                        "pool": "chromium.tests.coverage",
                        "ssd": "1",
                    },
                    shards = 32,
                ),
            ),
            "sync_integration_tests": targets.mixin(
                swarming = targets.swarming(
                    dimensions = {
                        "pool": "chromium.tests.coverage",
                        "ssd": "1",
                    },
                    shards = 4,
                ),
            ),
            "unit_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 6,
                ),
            ),
            # These tests must run with a GPU.
            "webgpu_blink_web_tests": [
                "win10_nvidia_gtx_1660_stable",
            ],
            "webgpu_blink_web_tests_with_backend_validation": targets.remove(
                reason = "Don't need validation layers on code coverage bots.",
            ),
            "webgpu_cts_tests": [
                "win10_nvidia_gtx_1660_stable",
            ],
            "webgpu_cts_with_validation_tests": targets.remove(
                reason = "Don't need validation layers on code coverage bots.",
            ),
            "webgpu_cts_dedicated_worker_tests": [
                "win10_nvidia_gtx_1660_stable",
            ],
            "webgpu_cts_service_worker_tests": targets.remove(
                reason = "Dedicated worker tests are probably sufficient.",
            ),
            "webgpu_cts_shared_worker_tests": targets.remove(
                reason = "Dedicated worker tests are probably sufficient.",
            ),
        },
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE_X64,
        os_type = targets.os_type.WINDOWS,
    ),
    builderless = True,
    os = os.WINDOWS_DEFAULT,
    console_view_entry = [
        consoles.console_view_entry(
            category = "win",
            short_name = "win10",
        ),
    ],
    coverage_test_types = ["overall", "unit"],
    export_coverage_to_zoss = True,
    use_clang_coverage = True,
)

# Experimental builder. Does not export_coverage_to_zoss.
continuous_fuzz_coverage_builder(
    name = "win-libfuzzer-coverage",
    description_html = "This builder collects code coverage for fuzz targets on Windows.",
    cores = 64,
    os = os.WINDOWS_10,
    ssd = None,
    free_space = builders.free_space.high,
    target_platform = builder_config.target_platform.WIN,
    category = "win-fuzz",
    fuzz_engine = "libfuzzer",
    gn_extra_configs = [
        "asan",
        "chrome_with_codecs",
        "win",
    ],
    short_name = "win-libfuzz",
)
