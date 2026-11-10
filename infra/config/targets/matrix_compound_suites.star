# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Matrix Compound suite declarations

Matrix compound suites are a collection of basic suites that can be referenced
by a builder in //testing/buildbot/waterfalls.pyl and additionally support
expanding tests into multiple instances using variants (defined in
./variants.star). Suites also define a bundle containing the same tests as the
suite, so they can be used wherever a bundle is expected.

The legacy_ prefix denotes the ability for basic suites to be referenced in
//testing/buildbot. Once a suite is no longer referenced via //testing/buildbot,
targets.bundle can be used for grouping tests in a more flexible manner (mixing
test types and/or compile targets and arbitrary nesting). Named bundles are
defined in ./bundles.star.
"""

load("@chromium-luci//targets.star", "targets")

targets.legacy_matrix_compound_suite(
    name = "model_validation_gce_matrix_tests",
    basic_suites = {
        "model_validation_tests_light_suite": targets.legacy_matrix_config(
            # Run model validation tests on GCE when possible to save bare
            # metal resources. This cannot be done for ondevice_stability_tests
            # because it requires a GPU.
            mixins = [
                "gce",
            ],
            variants = [
                "CHANNEL_BETA",
                "CHANNEL_DEV",
                "CHANNEL_STABLE",
            ],
        ),
        "ondevice_stability_tests_light_suite": targets.legacy_matrix_config(
            # For stability tests we need a GPU, so we run them on NVIDIA
            # GeForce GTX 1660s, which are most available in the intelligence
            # pool.
            mixins = [
                "nvidia_geforce_gtx_1660",
            ],
            variants = [
                "CHANNEL_BETA",
                "CHANNEL_DEV",
                "CHANNEL_STABLE",
            ],
        ),
    },
)

targets.legacy_matrix_compound_suite(
    name = "model_validation_matrix_tests",
    basic_suites = {
        "model_validation_tests_light_suite": targets.legacy_matrix_config(
            variants = [
                "CHANNEL_BETA",
                "CHANNEL_DEV",
                "CHANNEL_STABLE",
            ],
        ),
        "ondevice_stability_tests_light_suite": targets.legacy_matrix_config(
            variants = [
                "CHANNEL_BETA",
                "CHANNEL_DEV",
                "CHANNEL_STABLE",
            ],
        ),
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_desktop_gtests",
    basic_suites = {
        "optimization_guide_nogpu_gtests": None,
        "optimization_guide_gpu_gtests": None,
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_mac_script_tests",
    basic_suites = {
        "model_validation_tests_suite": None,
        "model_validation_tests_light_suite": None,
        "ondevice_api_scrape_tests_suite": None,
        "ondevice_stability_tests_suite": None,
        "litert_e2e_tests_gpu_suite": None,
        "litert_e2e_tests_cpu_suite": None,
        "litert_lm_advanced_main_legacy_tests_cpu_suite": None,
        "litert_lm_advanced_main_legacy_tests_gpu_suite": None,
        "chrome_ai_wpt_tests_manifest_suite": None,
        "chrome_ai_wpt_tests_manifest_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gemma4_suite": None,
        "chrome_ai_wpt_tests_manifest_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "ci_only",
                "experiments",
            ],
        ),
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_win_arm64_script_tests",
    basic_suites = {
        "chrome_ai_wpt_tests_manifest_suite": None,
        "chrome_ai_wpt_tests_manifest_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gemma4_suite": None,
        "chrome_ai_wpt_tests_manifest_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "ci_only",
                "experiments",
            ],
        ),
        "model_validation_tests_light_suite": None,
        "model_validation_tests_suite": None,
        "ondevice_stability_tests_suite": None,
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_linux_gtests",
    basic_suites = {
        "optimization_guide_nogpu_gtests": targets.legacy_matrix_config(
            mixins = [
                "gce",
            ],
        ),
        "optimization_guide_gpu_gtests": targets.legacy_matrix_config(
            # TODO(b:322815244): Add AMD variant once driver issues are fixed.
            variants = [
                "NVIDIA_GEFORCE_GTX_1660",
            ],
        ),
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_linux_script_tests",
    basic_suites = {
        "model_validation_tests_suite": targets.legacy_matrix_config(
            mixins = [
                "gce",
            ],
        ),
        "model_validation_tests_light_suite": targets.legacy_matrix_config(
            mixins = [
                "gce",
            ],
        ),
        "ondevice_api_scrape_tests_suite": targets.legacy_matrix_config(
            variants = [
                "NVIDIA_GEFORCE_GTX_1660",
            ],
        ),
        "ondevice_stability_tests_suite": targets.legacy_matrix_config(
            variants = [
                "NVIDIA_GEFORCE_GTX_1660",
            ],
        ),
        "litert_e2e_tests_gpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
            ],
        ),
        "litert_e2e_tests_cpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
            ],
        ),
        "litert_lm_advanced_main_legacy_tests_cpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "x64_ai_wpt_shards",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "x64_ai_wpt_shards",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_gemma4_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "x64_ai_wpt_shards",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "x64_ai_wpt_shards",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "linux_gpu_high_tier_ai_wpt_dimensions",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "linux_gpu_high_tier_ai_wpt_dimensions",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "linux_gpu_high_tier_gemma4_ai_wpt_dimensions",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "linux_gpu_high_tier_gemma4_ai_wpt_dimensions",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "linux_gpu_high_tier_gemma4_ai_wpt_dimensions",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "linux_gpu_high_tier_gemma4_ai_wpt_dimensions",
                "ci_only",
                "experiments",
            ],
        ),
        "litert_lm_advanced_main_legacy_tests_gpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
            ],
        ),
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_win_gtests",
    basic_suites = {
        "optimization_guide_nogpu_gtests": targets.legacy_matrix_config(
            mixins = [
                "gce",
                "win10",
            ],
        ),
        "optimization_guide_gpu_gtests": targets.legacy_matrix_config(
            mixins = [
                "win10",
            ],
            variants = [
                "AMD_RADEON_RX_5500_XT",
                "INTEL_UHD_630_OR_770",
                "NVIDIA_GEFORCE_GTX_1660",
            ],
        ),
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_win32_script_tests",
    basic_suites = {
        "model_validation_tests_suite": targets.legacy_matrix_config(
            mixins = [
                "gce",
            ],
        ),
        "model_validation_tests_light_suite": targets.legacy_matrix_config(
            mixins = [
                "gce",
            ],
        ),
        "ondevice_stability_tests_suite": targets.legacy_matrix_config(
            variants = [
                "AMD_RADEON_RX_5500_XT",
                "NVIDIA_GEFORCE_GTX_1660",
            ],
        ),
    },
)

targets.legacy_matrix_compound_suite(
    name = "optimization_guide_win64_script_tests",
    basic_suites = {
        "model_validation_tests_suite": targets.legacy_matrix_config(
            mixins = [
                "gce",
                "win10",
            ],
        ),
        "model_validation_tests_light_suite": targets.legacy_matrix_config(
            mixins = [
                "gce",
                "win10",
            ],
        ),
        "ondevice_api_scrape_tests_suite": targets.legacy_matrix_config(
            mixins = [
                "win10",
            ],
            variants = [
                "AMD_RADEON_RX_5500_XT",
                "INTEL_UHD_630_OR_770",
                "NVIDIA_GEFORCE_GTX_1660",
            ],
        ),
        "ondevice_stability_tests_suite": targets.legacy_matrix_config(
            mixins = [
                "win10",
            ],
            variants = [
                "AMD_RADEON_RX_5500_XT",
                "INTEL_UHD_630_OR_770",
                "NVIDIA_GEFORCE_GTX_1660",
            ],
        ),
        "litert_e2e_tests_gpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "win10",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "win10",
                "x64_ai_wpt_shards",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "win10",
                "x64_ai_wpt_shards",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_gemma4_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "win10",
                "x64_ai_wpt_shards",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_cpu_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "gce",
                "win10",
                "x64_ai_wpt_shards",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_ai_wpt_dimensions",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_ai_wpt_dimensions",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_gemma4_ai_wpt_dimensions",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_high_tier_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_gemma4_ai_wpt_dimensions",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_ai_wpt_dimensions",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_ai_wpt_dimensions",
                "ci_only",
                "experiments",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_gemma4_ai_wpt_dimensions",
            ],
        ),
        "chrome_ai_wpt_tests_manifest_gpu_low_tier_gemma4_conversation_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "x64_ai_wpt_shards",
                "win_gpu_gemma4_ai_wpt_dimensions",
                "ci_only",
                "experiments",
            ],
        ),
        "litert_e2e_tests_cpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "win10",
            ],
        ),
        "litert_lm_advanced_main_legacy_tests_cpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "win10",
            ],
        ),
        "litert_lm_advanced_main_legacy_tests_gpu_suite": targets.legacy_matrix_config(
            mixins = [
                "chrome-intelligence-swarming-pool",
                "non-gce",
                "win10",
            ],
        ),
    },
)
