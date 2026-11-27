# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Variant declarations

Variants are used to expand tests within a matrix compound suite into multiple
tests and applying variant-specific modifications to the tests.
"""

load("@chromium-luci//targets.star", "targets")

targets.variant(
    name = "AMD_RADEON_RX_5500_XT",
    identifier = "AMD Radeon RX 5500 XT",
    mixins = [
        "amd_radeon_rx_5500_xt",
    ],
)

targets.variant(
    name = "CHANNEL_BETA",
    identifier = "Beta Channel",
    args = [
        "--channel=beta",
    ],
)

targets.variant(
    name = "CHANNEL_DEV",
    identifier = "Dev Channel",
    args = [
        "--channel=dev",
    ],
)

targets.variant(
    name = "CHANNEL_STABLE",
    identifier = "Stable Channel",
    args = [
        "--channel=stable",
    ],
)

targets.variant(
    name = "DISABLE_FIELD_TRIAL_CONFIG",
    identifier = "Disable Field Trial Config",
    args = [
        "--disable-field-trial-config",
    ],
)

targets.variant(
    name = "SINGLE_GROUP_PER_STUDY_PREFER_EXISTING_BEHAVIOR",
    identifier = "Single Group Per Study Prefer Existing Behavior Field Trial Config",
    args = [
        "--variations-test-seed-path=../../components/variations/test_data/cipd/single_group_per_study_prefer_existing_behavior/seed.json",
        "--accept-empty-variations-seed-signature",
        "--disable-field-trial-config",
        "--fake-variations-channel=stable",
    ],
)

targets.variant(
    name = "SINGLE_GROUP_PER_STUDY_PREFER_NEW_BEHAVIOR",
    identifier = "Single Group Per Study Prefer New Behavior Field Trial Config",
    args = [
        "--variations-test-seed-path=../../components/variations/test_data/cipd/single_group_per_study_prefer_new_behavior/seed.json",
        "--accept-empty-variations-seed-signature",
        "--disable-field-trial-config",
        "--fake-variations-channel=stable",
    ],
)

targets.variant(
    name = "INTEL_UHD_630_OR_770",
    identifier = "Intel UHD 630 or 770",
    mixins = [
        "intel_uhd_630_or_770",
    ],
)

targets.variant(
    name = "LINUX_INTEL_UHD_630_STABLE",
    identifier = "UHD 630",
    mixins = [
        "linux_intel_uhd_630_stable",
    ],
)

targets.variant(
    name = "LINUX_NVIDIA_GTX_1660_STABLE",
    identifier = "GTX 1660",
    mixins = [
        "linux_nvidia_gtx_1660_stable",
    ],
)

targets.variant(
    name = "MAC_MINI_INTEL_GPU_STABLE",
    identifier = "8086:3e9b",
    mixins = [
        "mac_mini_intel_gpu_stable",
    ],
)

targets.variant(
    name = "MAC_RETINA_AMD_GPU_STABLE",
    identifier = "1002:7340",
    mixins = [
        "mac_retina_amd_gpu_stable",
    ],
)

targets.variant(
    name = "NVIDIA_GEFORCE_GTX_1660",
    identifier = "NVIDIA GeForce GTX 1660",
    mixins = [
        "nvidia_geforce_gtx_1660",
    ],
)

targets.variant(
    name = "WIN10_INTEL_UHD_630_STABLE",
    identifier = "8086:9bc5",
    mixins = [
        "swarming_containment_auto",
        "win10_intel_uhd_630_stable",
    ],
)

targets.variant(
    name = "WIN10_NVIDIA_GTX_1660_STABLE",
    identifier = "10de:2184",
    mixins = [
        "win10_nvidia_gtx_1660_stable",
    ],
)
