# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Definitions of builders in the chromium.memory builder group."""

load("@chromium-luci//args.star", "args")
load("@chromium-luci//branches.star", "branches")
load("@chromium-luci//builder_config.star", "builder_config")
load("@chromium-luci//builder_health_indicators.star", "health_spec")
load("@chromium-luci//builders.star", "builders", "cpu", "os")
load("@chromium-luci//ci.star", "ci")
load("@chromium-luci//consoles.star", "consoles")
load("@chromium-luci//gn_args.star", "gn_args")
load("@chromium-luci//html.star", "linkify")
load("@chromium-luci//targets.star", "targets")
load("//lib/ci_constants.star", "ci_constants")
load("//lib/gardener_rotations.star", "gardener_rotations")
load("//lib/siso.star", "siso")
load("//lib/xcode.star", "xcode")

ci.defaults.set(
    executable = ci_constants.DEFAULT_EXECUTABLE,
    builder_group = "chromium.memory",
    builder_config_settings = builder_config.ci_settings(
        retry_failed_shards = True,
        # Shards of browser_tests and interactive_ui_tests are fundamentally
        # flaky on various sanitizer builds, and end up timing out without any
        # results. Such shards are considered "invalid". crbug.com/429435587 is
        # on file to address the fundamental flakiness, but a proper fix is
        # not likely. So just retry all such invalid shards on all memory
        # builders.
        retry_invalid_shards = True,
    ),
    pool = ci_constants.DEFAULT_POOL,
    cores = 8,
    os = os.LINUX_DEFAULT,
    gardener_rotations = gardener_rotations.CHROMIUM,
    tree_closing = True,
    tree_closing_notifiers = ci_constants.DEFAULT_TREE_CLOSING_NOTIFIERS,
    main_console_view = "main",
    contact_team_email = "chrome-sanitizer-builder-owners@google.com",
    execution_timeout = ci_constants.DEFAULT_EXECUTION_TIMEOUT,
    experiments = {
        "chromium_tests.resultdb_module": 100,
    },
    health_spec = health_spec.default(),
    service_account = ci_constants.DEFAULT_SERVICE_ACCOUNT,
    shadow_service_account = ci_constants.DEFAULT_SHADOW_SERVICE_ACCOUNT,
    siso_project = siso.project.DEFAULT_TRUSTED,
    siso_remote_jobs = siso.remote_jobs.HIGH_JOBS_FOR_CI,
)

targets.builder_defaults.set(
    mixins = [
        "chromium-tester-service-account",
    ],
)

consoles.console_view(
    name = "chromium.memory",
    branch_selector = branches.selector.LINUX_BRANCHES,
    ordering = {
        None: ["win", "mac", "linux"],
        "*build-or-test*": consoles.ordering(short_names = ["bld", "tst"]),
        "linux|TSan v2": "*build-or-test*",
        "linux|asan lsan": "*build-or-test*",
        "linux|webkit": consoles.ordering(short_names = ["asn", "msn"]),
    },
)

# TODO(gbeaty) Find a way to switch testers to use ci.thin_tester while ensuring
# that the builders and testers targeting linux set the necessary notifies

def linux_memory_builder(*, name, **kwargs):
    kwargs["notifies"] = kwargs.get("notifies", []) + ["linux-memory"]
    return ci.builder(name = name, **kwargs)

linux_memory_builder(
    name = "Linux ASan LSan Builder",
    branch_selector = branches.selector.LINUX_BRANCHES,
    description_html = "Compiles a Linux x64 release build with AddressSanitizer and LeakSanitizer.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_asan",
            apply_configs = [
                "lsan",
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        args = {
            # ASan test binary links are too large to run many concurrently.
            "concurrent_links": 2,
        },
        configs = [
            "asan",
            "lsan",
            "fail_on_san_warnings",
            # Sanitizer builds set enable_fuzztest_fuzz by default, which adds
            # coverage instrumentation to all code. CI never uses --fuzz, so
            # disable it to reduce test runtime.
            "no_fuzztest_fuzz",
            "release_builder",
            "dcheck_always_on",
            "minimal_symbols",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    # crbug.com/372503456: This builder gets died on a 8 cores bot when it fails
    # to get builder cache.
    cores = 16,
    ssd = True,
    # TODO(crbug.com/388307198): Re-enable tree closing when the bot is stable.
    tree_closing = False,
    console_view_entry = consoles.console_view_entry(
        category = "linux|asan lsan",
        short_name = "bld",
    ),
    cq_mirrors_console_view = "mirrors",
)

linux_memory_builder(
    name = "Linux ASan LSan Tests (1)",
    branch_selector = branches.selector.LINUX_BRANCHES,
    description_html = "Runs tests against a Linux x64 AddressSanitizer and LeakSanitizer build.",
    parent = "ci/Linux ASan LSan Builder",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "chromium",
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_asan",
            apply_configs = [
                "lsan",
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    targets = targets.bundle(
        targets = [
            "chromium_linux_and_gl_gtests",
        ],
        mixins = [
            targets.mixin(
                args = [
                    "--test-launcher-print-test-stdio=always",
                ],
            ),
            "linux-jammy",
        ],
        per_test_modifications = {
            "accessibility_unittests": targets.mixin(
                args = [
                    # TODO(crbug.com/40266898): Re-enable.
                    "--gtest_filter=-AXPlatformNodeAuraLinuxTest.AtkComponentScrollTo:AtkUtilAuraLinuxTest.*",
                ],
            ),
            "blink_unittests": [
                targets.mixin(
                    # These are very slow on the ASAN trybot for some reason.
                    # crbug.com/1257927
                    swarming = targets.swarming(
                        shards = 5,
                    ),
                ),
                # TODO(crbug.com/579036960): Remove once e4 failures are fixed.
                "e2-standard-8",
            ],
            "browser_tests": targets.mixin(
                args = [
                    "--test-launcher-filter-file=../../testing/buildbot/filters/linux.asan.browser_tests.filter",
                ],
                ci_only = True,
                # These are very slow on the ASAN trybot for some reason.
                # crbug.com/1257927
                swarming = targets.swarming(
                    hard_timeout_sec = 5400,  # 90 minutes,
                    shards = 70,
                ),
            ),
            "components_browsertests": targets.mixin(
                ci_only = True,
            ),
            "components_unittests": targets.mixin(
                enable_rts_filtering = True,
                # These are very slow on the ASAN trybot for some reason.
                # crbug.com/1257927
                swarming = targets.swarming(
                    shards = 5,
                ),
            ),
            "content_browsertests": targets.mixin(
                enable_rts_filtering = True,
                swarming = targets.swarming(
                    shards = 24,
                ),
            ),
            "content_unittests": targets.mixin(
                # These are very slow on the ASAN trybot for some reason.
                # crbug.com/1257927
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "gin_unittests": targets.remove(
                reason = "https://crbug.com/831667",
            ),
            "gl_tests_passthrough": [
                targets.mixin(
                    # TODO(kbr): figure out a better way to specify blocks of
                    # arguments like this for tests on multiple machines.
                    args = [
                        "--use-gpu-in-tests",
                        "--no-xvfb",
                    ],
                ),
                "linux_nvidia_gtx_1660_stable",
            ],
            "interactive_ui_tests": targets.mixin(
                # These are slow on the ASan trybot for some reason, crbug.com/1257927
                swarming = targets.swarming(
                    # Move to faster machine types to reduce capacity impact.
                    # TODO(crbug.com/541675870): Can remove this if/when
                    # everything's been migrated.
                    optional_dimensions = {
                        30: {
                            "cpu": "x86-64-e4",
                        },
                    },
                    shards = 12,
                ),
            ),
            "net_unittests": targets.mixin(
                # These are very slow on the ASAN trybot for some reason.
                # crbug.com/1257927
                swarming = targets.swarming(
                    shards = 16,
                ),
            ),
            "sync_integration_tests": targets.mixin(
                enable_rts_filtering = True,
                swarming = targets.swarming(
                    shards = 8,
                ),
            ),
            "unit_tests": targets.mixin(
                # These are slow on the ASAN trybot for some reason.
                # crbug.com/1257927
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
        },
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|asan lsan",
        short_name = "tst",
    ),
    cq_mirrors_console_view = "mirrors",
    siso_project = None,
)

linux_memory_builder(
    name = "Linux TSan Builder",
    branch_selector = branches.selector.LINUX_BRANCHES,
    description_html = "Compiles a Linux x64 release build with ThreadSanitizer.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_tsan2",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "tsan",
            "fail_on_san_warnings",
            "release_builder",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|TSan v2",
        short_name = "bld",
    ),
    cq_mirrors_console_view = "mirrors",
)

linux_memory_builder(
    name = "Linux CFI",
    description_html = "Builds and runs tests on Linux x64 release builds with Control Flow Integrity (CFI) enabled.",
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
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "cfi_full",
            "cfi_icall",
            "cfi_diag",
            "thin_lto",
            "release",
            "static",
            "dcheck_always_on",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_linux_and_gl_gtests",
        ],
        mixins = [
            "linux-jammy",
        ],
        per_test_modifications = {
            "browser_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 94,
                ),
            ),
            "crashpad_tests": targets.remove(
                reason = "https://crbug.com/crashpad/306",
            ),
            "gl_tests_passthrough": [
                targets.mixin(
                    args = [
                        "--use-gpu-in-tests",
                        "--no-xvfb",
                    ],
                ),
                "linux_nvidia_gtx_1660_stable",
            ],
            "interactive_ui_tests": targets.mixin(
                # Slow on certain debug builders, see crbug.com/1513713.
                swarming = targets.swarming(
                    shards = 26,
                ),
            ),
            "sync_integration_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 6,
                ),
            ),
        },
    ),
    cores = 32,
    ssd = True,
    console_view_entry = consoles.console_view_entry(
        category = "cfi",
        short_name = "lnx",
    ),
    # TODO(thakis): Remove once https://crbug.com/927738 is resolved.
    execution_timeout = 5 * time.hour,
)

linux_memory_builder(
    name = "Linux MSan Builder",
    description_html = "Compiles a Linux x64 release build with MemorySanitizer.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
                "checkout_instrumented_libraries",
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_msan",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "msan",
            "fail_on_san_warnings",
            "release_builder",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    # Requires dedicated extra memory builder (crbug.com/352281723).
    builderless = False,
    ssd = True,
    console_view_entry = consoles.console_view_entry(
        category = "linux|msan",
        short_name = "bld",
    ),
)

linux_memory_builder(
    name = "Linux MSan Tests",
    description_html = "Runs tests against a Linux x64 MemorySanitizer build.",
    parent = "Linux MSan Builder",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_msan",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    targets = targets.bundle(
        targets = [
            "chromium_linux_and_gl_gtests",
        ],
        mixins = [
            targets.mixin(
                args = [
                    "--test-launcher-print-test-stdio=always",
                ],
            ),
            "linux-jammy",
            "x86-64",
        ],
        per_test_modifications = {
            "browser_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 119,
                ),
            ),
            "content_browsertests": targets.mixin(
                swarming = targets.swarming(
                    # https://crbug.com/1497706
                    shards = 10,
                ),
            ),
            "gl_tests_passthrough": targets.remove(
                reason = "Can't run on MSAN because gl_tests uses the hardware driver, which isn't instrumented.",
            ),
            "gl_unittests": targets.remove(
                reason = "Can't run on MSAN because gl_unittests uses the hardware driver, which isn't instrumented.",
            ),
            "interactive_ui_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 51,
                ),
            ),
            "services_unittests": targets.remove(
                reason = "https://crbug.com/831676",
            ),
            "sync_integration_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 6,
                ),
            ),
        },
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|msan",
        short_name = "tst",
    ),
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CI,
)

ci.builder(
    name = "Mac ASan 64 Builder",
    description_html = "Compiles a Mac x64 release build with AddressSanitizer.",
    triggering_policy = scheduler.greedy_batching(
        max_concurrent_invocations = 2,
    ),
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_asan",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "asan",
            "minimal_symbols",
            "release_builder",
            "remoteexec",
            "dcheck_always_on",
            "mac",
            "x64",
        ],
    ),
    builderless = False,
    cores = None,  # Swapping between 8 and 24
    # TODO(crbug.com/543006750): Revert to MAC_DEFAULT after arm migration.
    os = os.MAC_15,
    console_view_entry = consoles.console_view_entry(
        category = "mac",
        short_name = "bld",
    ),
)

linux_memory_builder(
    name = "Linux TSan Tests",
    branch_selector = branches.selector.LINUX_BRANCHES,
    description_html = "Runs tests against a Linux x64 ThreadSanitizer build.",
    parent = "ci/Linux TSan Builder",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_tsan2",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    targets = targets.bundle(
        targets = [
            "chromium_linux_and_gl_and_vulkan_gtests",
        ],
        mixins = [
            targets.mixin(
                args = [
                    "--test-launcher-print-test-stdio=always",
                ],
            ),
            "linux-jammy",
        ],
        per_test_modifications = {
            "blink_unittests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "browser_tests": targets.remove(
                reason = "https://crbug.com/368525",
            ),
            "cc_unittests": targets.mixin(
                swarming = targets.swarming(
                    shards = 3,
                ),
            ),
            "components_browsertests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "components_unittests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "content_browsertests": targets.mixin(
                # https://crbug.com/1498240
                ci_only = True,
                swarming = targets.swarming(
                    shards = 30,
                ),
            ),
            "crashpad_tests": targets.remove(
                reason = "https://crbug.com/crashpad/304",
            ),
            "gl_tests_passthrough": [
                "gpu_linux_gce_stable",
                targets.mixin(
                    args = [
                        "--test-launcher-filter-file=../../testing/buildbot/filters/linux.swiftshader.tsan.gl_tests_passthrough.filter",
                    ],
                ),
            ],
            "interactive_ui_tests": targets.mixin(
                # https://crbug.com/1498240
                ci_only = True,
                # These are slow on the TSan bots for some reason, crbug.com/1257927
                swarming = targets.swarming(
                    # Adjusted for testing, see https://crbug.com/1179567
                    shards = 32,
                ),
            ),
            "net_unittests": targets.mixin(
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
            "sync_integration_tests": targets.mixin(
                # https://crbug.com/1498240
                ci_only = True,
                swarming = targets.swarming(
                    shards = 6,
                ),
            ),
            "unit_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
        },
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|TSan v2",
        short_name = "tst",
    ),
    cq_mirrors_console_view = "mirrors",
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CI,
)

linux_memory_builder(
    name = "Linux UBSan Builder",
    description_html = "Compiles a linux build with ubsan.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_msan",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        args = {
            # UBSan test binary links are too large to run many concurrently.
            "concurrent_links": 4,
        },
        configs = [
            "ubsan_no_recover",
            "fail_on_san_warnings",
            # Sanitizer builds set enable_fuzztest_fuzz by default, which adds
            # coverage instrumentation to all code. CI never uses --fuzz, so
            # disable it to reduce test runtime.
            "no_fuzztest_fuzz",
            "release_builder",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    builderless = True,
    cores = 32,
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|ubsan",
        short_name = "bld",
    ),
)

linux_memory_builder(
    name = "Linux UBSan Tests",
    description_html = "Runs tests against a linux ubsan build.",
    parent = "ci/Linux UBSan Builder",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    targets = targets.bundle(
        targets = [
            "chromium_linux_gtests",
        ],
        mixins = [
            "isolate_profile_data",
            "linux-jammy",
        ],
        per_test_modifications = {
            "blink_unittests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "browser_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 34,
                ),
            ),
            "content_browsertests": targets.mixin(
                swarming = targets.swarming(
                    shards = 16,
                ),
            ),
            "interactive_ui_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 12,
                ),
            ),
            "unit_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
        },
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|ubsan",
        short_name = "tst",
    ),
    cq_mirrors_console_view = "mirrors",
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CI,
)

ci.builder(
    name = "Mac ASan 64 Tests (1)",
    description_html = "Runs tests against a Mac x64 AddressSanitizer build.",
    parent = "Mac ASan 64 Builder",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "chromium",
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_asan",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
    ),
    targets = targets.bundle(
        targets = [
            "chromium_mac_gtests",
        ],
        mixins = [
            targets.mixin(
                args = [
                    "--test-launcher-print-test-stdio=always",
                ],
            ),
            "mac_15_x64",
        ],
        per_test_modifications = {
            "browser_tests": targets.mixin(
                # crbug.com/1196416
                args = [
                    "--test-launcher-filter-file=../../testing/buildbot/filters/mac.mac-rel.browser_tests.filter",
                ],
                # https://crbug.com/1251657
                experiment_percentage = 100,
                swarming = targets.swarming(
                    shards = 30,
                ),
            ),
            "content_browsertests": targets.mixin(
                # https://crbug.com/1200640
                experiment_percentage = 100,
            ),
            "interactive_ui_tests": targets.mixin(
                # https://crbug.com/1251656
                experiment_percentage = 100,
            ),
            "sync_integration_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 8,
                ),
            ),
            "unit_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
        },
    ),
    builderless = False,
    cores = 12,
    # TODO(crbug.com/543006750): Revert to MAC_DEFAULT after arm migration.
    os = os.MAC_15,
    console_view_entry = consoles.console_view_entry(
        category = "mac",
        short_name = "tst",
    ),
    siso_project = None,
)

_WEB_TESTS_LINK = "https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_tests.md"

ci.builder(
    name = "linux-blink-asan-rel",
    description_html = "Runs {} with address-sanitized binaries.".format(
        linkify(
            _WEB_TESTS_LINK,
            "web (platform) tests",
        ),
    ),
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_clang",
            apply_configs = [
                "asan",
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "asan",
            "lsan",
            "release_builder_blink",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_blink_isolated_scripts",
        ],
        mixins = [
            "linux-jammy",
        ],
        per_test_modifications = {
            "chrome_wpt_tests": targets.mixin(
                args = [
                    "-j6",
                ],
            ),
            "blink_web_tests": targets.mixin(
                args = [
                    "--timeout-ms",
                    "48000",
                ],
                swarming = targets.swarming(
                    shards = 8,
                ),
            ),
            "blink_wpt_tests": targets.mixin(
                args = [
                    "--timeout-ms",
                    "48000",
                ],
                swarming = targets.swarming(
                    shards = 12,
                ),
            ),
            "headless_shell_wpt_tests": targets.mixin(
                args = [
                    "-j6",
                ],
            ),
        },
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|blink",
        short_name = "asn",
    ),
)

ci.builder(
    name = "linux-blink-leak-rel",
    description_html = "Runs {} with {} enabled.".format(
        linkify(
            _WEB_TESTS_LINK,
            "web (platform) tests",
        ),
        linkify(
            "https://chromium.googlesource.com/chromium/src/+/main/third_party/blink/renderer/controller/blink_leak_detector.h",
            "DOM leak detection",
        ),
    ),
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium",
            apply_configs = [
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "release_builder_blink",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_blink_isolated_scripts",
        ],
        additional_compile_targets = [
            "blink_tests",
        ],
        mixins = [
            "linux-jammy",
            "web-test-leak",
        ],
        per_test_modifications = {
            "blink_web_tests": targets.mixin(
                args = [
                    "--timeout-ms",
                    "48000",
                ],
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
            "blink_wpt_tests": targets.mixin(
                args = [
                    "--timeout-ms",
                    "48000",
                ],
                swarming = targets.swarming(
                    shards = 6,
                ),
            ),
            # TODO(crbug.com/422245278): Reenable print-reftests once the bug is
            # fixed.
            "headless_shell_wpt_tests": targets.per_test_modification(
                replacements = targets.replacements(
                    args = {
                        "print-reftest": None,
                    },
                ),
            ),
        },
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|blink",
        short_name = "lk",
    ),
)

ci.builder(
    name = "linux-blink-msan-rel",
    description_html = "Runs {} with memory-sanitized binaries.".format(
        linkify(
            _WEB_TESTS_LINK,
            "web (platform) tests",
        ),
    ),
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
            apply_configs = [
                "checkout_instrumented_libraries",
            ],
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_clang",
            apply_configs = [
                "asan",
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "msan",
            "release_builder_blink",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_blink_isolated_scripts",
        ],
        mixins = [
            "linux-jammy",
        ],
        per_test_modifications = {
            "blink_web_tests": targets.mixin(
                args = [
                    "--timeout-ms",
                    "66000",
                ],
                swarming = targets.swarming(
                    expiration_sec = 36000,
                    hard_timeout_sec = 10800,
                    io_timeout_sec = 3600,
                    shards = 8,
                ),
            ),
            "blink_wpt_tests": targets.mixin(
                args = [
                    "--timeout-ms",
                    "66000",
                ],
                swarming = targets.swarming(
                    expiration_sec = 36000,
                    hard_timeout_sec = 10800,
                    io_timeout_sec = 3600,
                    shards = 12,
                ),
            ),
            "headless_shell_wpt_tests": targets.mixin(
                args = [
                    "-j6",
                ],
                swarming = targets.swarming(
                    expiration_sec = 36000,
                    hard_timeout_sec = 10800,
                    io_timeout_sec = 3600,
                ),
            ),
        },
    ),
    ssd = None,
    console_view_entry = consoles.console_view_entry(
        category = "linux|blink",
        short_name = "msn",
    ),
)

ci.builder(
    name = "win-asan",
    description_html = "Builds and runs tests on Windows x64 release builds with AddressSanitizer.",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "chromium",
        ),
        chromium_config = builder_config.chromium_config(
            config = "chromium_win_clang_asan",
            apply_configs = [
                # TODO(https://crbug.com/440203328): cache is causing build
                # failures.
                "clobber",
                "mb",
            ],
            build_config = builder_config.build_config.RELEASE,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "asan",
            "fuzzer",
            "static",
            "v8_heap",
            "minimal_symbols",
            "release_builder",
            "remoteexec",
            "win",
            "x64",
        ],
    ),
    targets = targets.bundle(
        targets = [
            "chromium_win_gtests",
        ],
        mixins = [
            "win10",
        ],
        per_test_modifications = {
            "blink_unittests": targets.mixin(
                swarming = targets.swarming(
                    shards = 6,
                ),
            ),
            "browser_tests": targets.mixin(
                # Tests shows tests run faster with fewer retries by using fewer jobs crbug.com/1411912
                args = [
                    "--test-launcher-jobs=3",
                ],
                # These are very slow on the ASAN trybot for some reason.
                # crbug.com/1257927
                swarming = targets.swarming(
                    shards = 190,
                ),
            ),
            "components_unittests": targets.mixin(
                # TODO(crbug.com/41491387): With a single shard seems to hit time limit.
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
            "content_browsertests": targets.mixin(
                # Tests shows tests run faster with fewer retries by using fewer jobs crbug.com/1411912
                args = [
                    "--test-launcher-jobs=3",
                ],
                swarming = targets.swarming(
                    # ASAN bot is slow: https://crbug.com/1484550#c3
                    shards = 32,
                ),
            ),
            "gcp_unittests": targets.mixin(
                # Flakily times out with only a single shard due to slow runtime.
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "interactive_ui_tests": targets.mixin(
                # Tests shows tests run faster with fewer retries by using fewer jobs crbug.com/1411912
                args = [
                    "--test-launcher-jobs=3",
                ],
                swarming = targets.swarming(
                    shards = 35,
                ),
            ),
            "net_unittests": targets.mixin(
                # TODO(crbug.com/40200867): net_unittests is slow under ASan.
                swarming = targets.swarming(
                    shards = 16,
                ),
            ),
            "sync_integration_tests": targets.mixin(
                # Tests shows tests run faster with fewer retries by using fewer jobs crbug.com/1411912
                args = [
                    "--test-launcher-jobs=3",
                ],
                swarming = targets.swarming(
                    shards = 6,
                ),
            ),
            "unit_tests": targets.mixin(
                swarming = targets.swarming(
                    # ASAN bot is slow: https://crbug.com/1484550#c4
                    shards = 4,
                ),
            ),
            "updater_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
            "updater_tests_system": targets.mixin(
                # crbug.com/369478225: These are slow and could timeout on the ASAN
                swarming = targets.swarming(
                    shards = 2,
                ),
            ),
        },
    ),
    builderless = True,
    cores = 16,
    os = os.WINDOWS_DEFAULT,
    ssd = True,
    console_view_entry = consoles.console_view_entry(
        category = "win",
        short_name = "asn",
    ),
    # This builder is normally using 2.5 hours to run with a cached builder. And
    # 1.5 hours additional setup time without cache, https://crbug.com/1311134.
    execution_timeout = 5 * time.hour,
    siso_remote_jobs = siso.remote_jobs.DEFAULT,
)

ci.builder(
    name = "linux-codeql-generator",
    description_html = "Compiles a CodeQL database on a Linux host and uploads the result.",
    executable = "recipe:chrome_codeql_database_builder",
    # Run once daily at 5am Pacific/1 PM UTC
    schedule = "0 13 * * *",
    triggered_by = [],
    cores = 32,
    ssd = True,
    gardener_rotations = args.ignore_default(None),
    console_view_entry = [
        consoles.console_view_entry(
            category = "codeql-linux",
            short_name = "cdql-lnx",
        ),
    ],
    contact_team_email = "chrome-memory-safety-team@google.com",
    execution_timeout = 18 * time.hour,
    notifies = ["codeql-infra"],
    properties = {
        "codeql_version": "version:3@2.18.1",
    },
)

ci.builder(
    name = "linux-codeql-query-runner",
    description_html = "Runs a set of CodeQL queries against a CodeQL database on a Linux host and uploads the result.",
    executable = "recipe:chrome_codeql_query_runner",
    # Run once daily at 5am Pacific/1 PM UTC
    schedule = "0 13 * * *",
    triggered_by = [],
    cores = 32,
    ssd = True,
    gardener_rotations = args.ignore_default(None),
    console_view_entry = [
        consoles.console_view_entry(
            category = "codeql-linux-queries",
            short_name = "cdql-lnx-qrs",
        ),
    ],
    contact_team_email = "chrome-memory-safety-team@google.com",
    execution_timeout = 18 * time.hour,
    notifies = ["codeql-infra"],
    properties = {
        "codeql_version": "version:3@2.18.1",
    },
)
