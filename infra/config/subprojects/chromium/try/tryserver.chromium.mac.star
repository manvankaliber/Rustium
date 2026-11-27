# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Definitions of builders in the tryserver.chromium.mac builder group."""

load("@chromium-luci//branches.star", "branches")
load("@chromium-luci//builder_config.star", "builder_config")
load("@chromium-luci//builders.star", "cpu", "os")
load("@chromium-luci//consoles.star", "consoles")
load("@chromium-luci//gn_args.star", "gn_args")
load("@chromium-luci//gpu.star", shared_gpu = "gpu")
load("@chromium-luci//html.star", "linkify_builder")
load("@chromium-luci//targets.star", "targets")
load("@chromium-luci//try.star", "try_")
load("//lib/gpu.star", "gpu")
load("//lib/siso.star", "siso")
load("//lib/try_constants.star", "try_constants")
load("//lib/xcode.star", "xcode")
load("//project.star", "settings")

try_.defaults.set(
    executable = try_constants.DEFAULT_EXECUTABLE,
    builder_group = "tryserver.chromium.mac",
    pool = try_constants.DEFAULT_POOL,
    builderless = True,
    os = os.MAC_DEFAULT,
    ssd = True,
    execution_timeout = try_constants.DEFAULT_EXECUTION_TIMEOUT,
    experiments = {
        "chromium_tests.resultdb_module": 100,
    },
    orchestrator_cores = "2|4",
    orchestrator_siso_remote_jobs = siso.remote_jobs.HIGH_JOBS_FOR_CQ,
    service_account = try_constants.DEFAULT_SERVICE_ACCOUNT,
    siso_keep_going = siso.KEEP_GOING,
    siso_project = siso.project.DEFAULT_UNTRUSTED,
)

targets.builder_defaults.set(
    mixins = [
        "chromium-tester-service-account",
    ],
)

consoles.list_view(
    name = "tryserver.chromium.mac",
    branch_selector = [
        branches.selector.MAC_BRANCHES,
    ],
)

try_.builder(
    name = "mac-arm64-clobber-rel",
    mirrors = [
        "ci/mac-arm64-archive-rel",
    ],
    gn_args = "ci/mac-arm64-archive-rel",
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    properties = {
        # The format of these properties is defined at archive/properties.proto
        "$build/archive": {
            "source_side_spec_path": [
                "src",
                "infra",
                "archive_config",
                "mac-arm64-archive-rel.json",
            ],
            "verify_paths_only": True,
        },
    },
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac-arm64-on-arm64-rel",
    mirrors = [
        "ci/mac-arm64-on-arm64-rel",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/mac-arm64-on-arm64-rel",
            "release_try_builder",
        ],
    ),
    builderless = False,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac-clobber-rel",
    mirrors = [
        "ci/mac-archive-rel",
    ],
    gn_args = "ci/mac-archive-rel",
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    properties = {
        # The format of these properties is defined at archive/properties.proto
        "$build/archive": {
            "source_side_spec_path": [
                "src",
                "infra",
                "archive_config",
                "mac-archive-rel.json",
            ],
            "verify_paths_only": True,
        },
    },
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac-intel-on-arm64-rel",
    mirrors = [
        "ci/mac-intel-on-arm64-rel",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/mac-intel-on-arm64-rel",
            "release_try_builder",
        ],
    ),
    builderless = False,
    cpu = cpu.ARM64,
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac-fieldtrial-tester",
    mirrors = [
        "ci/mac-arm64-rel",
        "ci/mac-fieldtrial-tester",
    ],
    gn_args = "ci/mac-arm64-rel",
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    execution_timeout = 6 * time.hour,
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac-builder-next",
    mirrors = ["ci/Mac Builder Next"],
    gn_args = "ci/Mac Builder Next",
    builderless = False,
    os = os.MAC_BETA,
    cpu = cpu.ARM64,
    siso_remote_jobs = siso.remote_jobs.HIGH_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac-perfetto-rel",
    mirrors = [
        "ci/mac-perfetto-rel",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/mac-perfetto-rel",
            "release_try_builder",
        ],
    ),
    cpu = cpu.ARM64,
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.orchestrator_builder(
    name = "mac-rel",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/mac-arm64-rel",
        "ci/mac26-arm64-rel-tests",
        "ci/GPU Mac arm64 Builder",
        "ci/Mac Retina Release (Apple M2)",
    ],
    gn_args = gn_args.config(
        configs = [
            "gpu_tests",
            "release_try_builder",
            "remoteexec",
            "no_symbols",
            "use_clang_coverage",
            "partial_code_coverage_instrumentation",
            "enable_dangling_raw_ptr_feature_flag",
            "enable_backup_ref_ptr_feature_flag",
            "enable_rust_clippy",
            "mac",
            "arm64",
        ],
    ),
    compilator = "mac-rel-compilator",
    contact_team_email = "bling-engprod@google.com",
    coverage_test_types = ["overall", "unit"],
    cq_settings = try_.cq_settings(
        on_default_cq = True,
        reuse_max_commit_distance = 800 if settings.is_main else None,
    ),
    experiments = {
        # go/nplus1shardsproposal
        "chromium.add_one_test_shard": 10,
        # crbug/940930
        "chromium.enable_cleandead": 100,
        # go/rts-project-proposal
        "chromium_rts.filter_file_analysis": 100,
        # crbug.com/40280175
        "chromium_checkout.expand_submodules": 100,
        "luci.buildbucket.run_in_turboci": 100,
    },
    main_list_view = "try",
    use_clang_coverage = True,
    # TODO (crbug.com/1372179): Use orchestrator pool once overloaded test pools
    # are addressed
    #use_orchestrator_pool = True,
)

try_.compilator_builder(
    name = "mac-rel-compilator",
    branch_selector = branches.selector.MAC_BRANCHES,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    main_list_view = "try",
)

try_.builder(
    name = "mac-devtools-frontend-rel",
    description_html = "Runs DevTools Frontend tests exactly like mac-rel",
    mirrors = [
        "ci/mac-devtools-frontend-rel",
    ],
    gn_args = "ci/mac-devtools-frontend-rel",
    cpu = cpu.ARM64,
    contact_team_email = "chrome-devtools@google.com",
    cq_settings = try_.cq_settings(
        includable_only = True,
    ),
    main_list_view = "try",
)

try_.builder(
    name = "mac-webdriver-bidi-rel",
    description_html = "Runs webdriver bidi tests exactly like mac-rel",
    mirrors = [
        "ci/mac-webdriver-bidi-rel",
    ],
    gn_args = "ci/mac-webdriver-bidi-rel",
    cpu = cpu.ARM64,
    contact_team_email = "chrome-devtools@google.com",
    cq_settings = try_.cq_settings(
        location_filters = [
            "third_party/chromium-bidi/.+",
        ],
    ),
    main_list_view = "try",
)

try_.orchestrator_builder(
    name = "mac-gpu-rel",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/GPU Mac Builder",
        "ci/Mac Release (Intel)",
        "ci/Mac Retina Release (AMD)",
    ],
    gn_args = gn_args.config(
        configs = [
            "gpu_tests",
            "release_try_builder",
            "remoteexec",
            "no_symbols",
            "use_clang_coverage",
            "partial_code_coverage_instrumentation",
            "enable_dangling_raw_ptr_feature_flag",
            "enable_backup_ref_ptr_feature_flag",
            "mac",
            "x64",
        ],
    ),
    compilator = "mac-gpu-rel-compilator",
    contact_team_email = "chrome-gpu-infra@google.com",
    coverage_test_types = ["overall", "unit"],
    cq_settings = try_.cq_settings(
        on_default_cq = True,
        reuse_max_commit_distance = 800 if settings.is_main else None,
    ),
    experiments = {
        # go/nplus1shardsproposal
        "chromium.add_one_test_shard": 10,
        # crbug.com/940930
        "chromium.enable_cleandead": 100,
        # go/rts-project-proposal
        "chromium_rts.filter_file_analysis": 100,
        # crbug.com/40280175
        "chromium_checkout.expand_submodules": 100,
        "luci.buildbucket.run_in_turboci": 2,
    },
    main_list_view = "try",
    use_clang_coverage = True,
)

try_.compilator_builder(
    name = "mac-gpu-rel-compilator",
    branch_selector = branches.selector.MAC_BRANCHES,
    cpu = cpu.ARM64,
    contact_team_email = "chrome-gpu-infra@google.com",
    main_list_view = "try",
)

try_.builder(
    name = "mac-libfuzzer-asan-rel",
    executable = "recipe:chromium/fuzz",
    mirrors = ["ci/Libfuzzer Upload Mac ASan"],
    gn_args = gn_args.config(
        configs = [
            "ci/Libfuzzer Upload Mac ASan",
            "dcheck_always_on",
            "no_symbols",
            "skip_generate_fuzzer_owners",
        ],
    ),
    # TODO(crbug.com/543006750): Revert to MAC_DEFAULT after arm migration.
    os = os.MAC_15,
    contact_team_email = "chrome-fuzzing-core@google.com",
)

try_.builder(
    name = "mac-lsan-fyi-rel",
    mirrors = [
        "ci/mac-lsan-fyi-rel",
    ],
    gn_args = "ci/mac-lsan-fyi-rel",
    cpu = cpu.ARM64,
)

try_.builder(
    name = "mac-ubsan-fyi-rel",
    mirrors = [
        "ci/mac-ubsan-fyi-rel",
    ],
    gn_args = "ci/mac-ubsan-fyi-rel",
    os = os.MAC_DEFAULT,
    cpu = cpu.ARM64,
)

try_.builder(
    name = "mac-vm",
    mirrors = ["ci/mac-vm"],
    gn_args = "ci/mac-vm",
    builderless = True,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
)

try_.builder(
    name = "mac13-arm64-rel",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/mac-arm64-rel",
        "ci/mac13-arm64-rel-tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "arm64",
            "gpu_tests",
            "release_try_builder",
            "remoteexec",
            "no_symbols",
            "mac",
        ],
    ),
    builderless = True,
    cores = None,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    main_list_view = "try",
)

try_.builder(
    name = "mac14-arm64-rel",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/mac-arm64-rel",
        "ci/mac14-arm64-rel-tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "arm64",
            "gpu_tests",
            "release_try_builder",
            "remoteexec",
            "no_symbols",
            "mac",
        ],
    ),
    builderless = True,
    cores = None,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    main_list_view = "try",
)

try_.builder(
    name = "mac15-arm64-rel",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/mac-arm64-rel",
        "ci/mac15-arm64-rel-tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "arm64",
            "gpu_tests",
            "release_try_builder",
            "remoteexec",
            "no_symbols",
            "mac",
        ],
    ),
    builderless = True,
    cores = None,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    main_list_view = "try",
)

try_.builder(
    name = "mac26-arm64-rel",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/mac-arm64-rel",
        "ci/mac26-arm64-rel-tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "arm64",
            "gpu_tests",
            "release_try_builder",
            "remoteexec",
            "no_symbols",
            "mac",
        ],
    ),
    builderless = True,
    cores = None,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    main_list_view = "try",
)

try_.builder(
    name = "mac13-tests",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/Mac Builder",
        "ci/Mac13 Tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/Mac Builder",
            "release_try_builder",
            "remoteexec",
            "x64",
        ],
    ),
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    properties = {
        "$build/test_utils": {
            "min_failed_suites_to_skip_retry": 10,
        },
    },
)

try_.builder(
    name = "mac14-tests",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/Mac Builder",
        "ci/mac14-tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/Mac Builder",
            "release_try_builder",
            "remoteexec",
        ],
    ),
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
)

try_.builder(
    name = "mac15-x64-rel-tests",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/Mac Builder",
        "ci/mac15-x64-rel-tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/Mac Builder",
            "release_try_builder",
            "remoteexec",
        ],
    ),
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
)

try_.builder(
    name = "mac26-x64-rel-tests",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/Mac Builder",
        "ci/mac26-x64-rel-tests",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/Mac Builder",
            "release_try_builder",
            "remoteexec",
        ],
    ),
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
)

try_.builder(
    name = "mac-no-initial-webui-rel",
    mirrors = [
        "ci/mac-arm64-rel",
        "ci/mac-no-initial-webui-rel",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/mac-arm64-rel",
            "release_try_builder",
            "remoteexec",
        ],
    ),
    cpu = cpu.ARM64,
    contact_team_email = "chrome-webium-product-eng@google.com",
)

try_.builder(
    name = "mac_chromium_asan_rel_ng",
    mirrors = [
        "ci/Mac ASan 64 Builder",
        "ci/Mac ASan 64 Tests (1)",
    ],
    gn_args = gn_args.config(
        configs = [
            "asan",
            "dcheck_always_on",
            "release_builder",
            "remoteexec",
            "mac",
            "x64",
        ],
    ),
    # TODO(crbug.com/543006750): Revert to MAC_DEFAULT after arm migration.
    os = os.MAC_15,
    contact_team_email = "chrome-sanitizer-builder-owners@google.com",
    siso_remote_jobs = siso.remote_jobs.HIGH_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac_chromium_compile_dbg_ng",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/mac-arm64-dbg",
    ],
    builder_config_settings = builder_config.try_settings(
        include_all_triggered_testers = True,
        is_compile_only = True,
    ),
    gn_args = gn_args.config(
        configs = [
            "ci/mac-arm64-dbg",
        ],
    ),
    cores = None,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    cq_settings = try_.cq_settings(
        on_default_cq = True,
        reuse_max_commit_distance = 800 if settings.is_main else None,
    ),
    experiments = {
        # crbug.com/940930
        "chromium.enable_cleandead": 100,
    },
    main_list_view = "try",
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac-x64-compile-dbg",
    branch_selector = branches.selector.MAC_BRANCHES,
    mirrors = [
        "ci/Mac Builder (dbg)",
    ],
    builder_config_settings = builder_config.try_settings(
        include_all_triggered_testers = True,
        is_compile_only = True,
    ),
    gn_args = gn_args.config(
        configs = [
            "ci/Mac Builder (dbg)",
        ],
    ),
    cores = None,
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    experiments = {
        # crbug.com/940930
        "chromium.enable_cleandead": 100,
    },
    main_list_view = "try",
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac_chromium_compile_rel_ng",
    mirrors = [
        "ci/Mac Builder",
    ],
    builder_config_settings = builder_config.try_settings(
        include_all_triggered_testers = True,
        is_compile_only = True,
    ),
    gn_args = gn_args.config(
        configs = [
            "gpu_tests",
            "release_try_builder",
            "remoteexec",
            "mac",
            "x64",
        ],
    ),
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac_chromium_dbg_ng",
    mirrors = [
        "ci/mac-arm64-dbg",
        "ci/mac26-tests-dbg",
    ],
    gn_args = "ci/mac-arm64-dbg",
    cpu = cpu.ARM64,
    contact_team_email = "bling-engprod@google.com",
    siso_remote_jobs = siso.remote_jobs.LOW_JOBS_FOR_CQ,
)

try_.builder(
    name = "mac_upload_clang",
    description_html = "Builds and uploads Clang toolchain packages for Mac x64.",
    executable = "recipe:chromium_toolchain/package_clang",
    builderless = False,
    # Mac 15 is the last mac version widely supported on x64.
    os = os.MAC_15,
    execution_timeout = 8 * time.hour,
)

try_.builder(
    name = "mac_upload_clang_arm",
    description_html = "Builds and uploads Clang toolchain packages for Mac ARM64.",
    executable = "recipe:chromium_toolchain/package_clang",
    builderless = False,
    cpu = cpu.ARM64,
    execution_timeout = 8 * time.hour,
)

try_.builder(
    name = "mac_upload_rust",
    description_html = "Builds and uploads Rust toolchain packages for Mac x64.",
    executable = "recipe:chromium_toolchain/package_rust",
    builderless = False,
    # Mac 15 is the last mac version widely supported on x64.
    os = os.MAC_15,
    execution_timeout = 8 * time.hour,
)

try_.builder(
    name = "mac_upload_rust_arm",
    description_html = "Builds and uploads Rust toolchain packages for Mac ARM64.",
    executable = "recipe:chromium_toolchain/package_rust",
    builderless = False,
    cpu = cpu.ARM64,
    execution_timeout = 6 * time.hour,
)

try_.builder(
    name = "mac-code-coverage",
    mirrors = ["ci/mac-code-coverage"],
    gn_args = "ci/mac-code-coverage",
    cpu = cpu.ARM64,
    execution_timeout = 10 * time.hour,
)

try_.builder(
    name = "mac-treesinviz-disabled-rel",
    mirrors = [
        "ci/mac-treesinviz-disabled-rel",
    ],
    gn_args = "ci/mac-treesinviz-disabled-rel",
    os = os.MAC_DEFAULT,
    cpu = cpu.ARM64,
    contact_team_email = "chrome-gpu-team@google.com",
)

shared_gpu.try_.mac_optional_builder(
    name = "mac_optional_gpu_tests_rel",
    branch_selector = branches.selector.MAC_BRANCHES,
    description_html = ("Runs GPU tests on Mac Minis with Intel UHD 630 GPUs and Macbook Pros with AMD GPUs. " +
                        "Only automatically added to CLs that touch GPU-related files."),
    mirrors = [
        "ci/GPU FYI Mac Builder",
        "ci/Mac FYI Release (Intel)",
        "ci/Mac FYI Retina Release (AMD)",
    ],
    gn_args = "ci/GPU FYI Mac Builder",
    alerts_enabled = False,
    contact_team_email = "chrome-gpu-infra@google.com",
    cq_settings = try_.cq_settings(
        location_filters = gpu.try_.optional_trybot_location_filters.MAC,
    ),
    experiments = {
        "luci.buildbucket.run_in_turboci": 100,
    },
    main_list_view = "try",
    max_concurrent_builds = 7,
    service_account = gpu.try_.SERVICE_ACCOUNT,
)

shared_gpu.try_.mac_optional_builder(
    name = "gpu-fyi-cq-mac-arm64",
    branch_selector = branches.selector.MAC_BRANCHES,
    description_html = ("Runs GPU tests on M2 Macbook Pros. Only automatically added to CLs that " +
                        "touch GPU-related files"),
    mirrors = [
        "ci/GPU FYI Mac arm64 Builder",
        "ci/Mac FYI Retina Release (Apple M2)",
    ],
    builder_config_settings = builder_config.try_settings(
        retry_failed_shards = False,
    ),
    gn_args = "ci/GPU FYI Mac arm64 Builder",
    alerts_enabled = False,
    contact_team_email = "chrome-gpu-infra@google.com",
    cq_settings = try_.cq_settings(
        location_filters = gpu.try_.optional_trybot_location_filters.MAC,
    ),
    experiments = {
        "luci.buildbucket.run_in_turboci": 100,
    },
    main_list_view = "try",
    max_concurrent_builds = 7,
    service_account = gpu.try_.SERVICE_ACCOUNT,
)

try_.builder(
    name = "mac-separate-renderer-rel",
    mirrors = [
        "ci/mac-separate-renderer-fyi-rel",
    ],
    gn_args = gn_args.config(
        configs = [
            "ci/mac-separate-renderer-fyi-rel",
            "release_try_builder",
            "dcheck_always_on",
        ],
    ),
    cores = None,
    os = os.MAC_DEFAULT,
    cpu = cpu.ARM64,
    contact_team_email = "toyoshim@chromium.org",
    cq_settings = try_.cq_settings(
        includable_only = True,
    ),
)
