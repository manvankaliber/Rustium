# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compile target declarations

Compile targets can be referenced in additional_compile_targets for a builder in
waterfalls.pyl or as additional_compile_targets in a bundle declaration.
"""

load("@chromium-luci//targets.star", "targets")

targets.compile_target(
    name = "all",
)

targets.compile_target(
    name = "base_nocompile_tests",
    label = "//base:base_nocompile_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "blackbox_fuzzing_targets",
    label = "//:blackbox_fuzzing_targets",
)

targets.compile_target(
    name = "blink_platform_nocompile_tests",
    label = "//third_party/blink/renderer/platform:blink_platform_nocompile_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "blink_probes_nocompile_tests",
    label = "//third_party/blink/renderer/core/probe:blink_probes_nocompile_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "blink_tests",
    label = "//:blink_tests",
)

targets.compile_target(
    name = "chrome",
    label = "//chrome:chrome",
)

targets.compile_target(
    name = "chrome_official_builder",
    label = "//:chrome_official_builder",
)

targets.compile_target(
    name = "chrome_official_builder_no_unittests",
    label = "//:chrome_official_builder_no_unittests",
)

targets.compile_target(
    name = "chrome_sandbox",
    label = "//sandbox/linux:chrome_sandbox",
)

targets.compile_target(
    name = "chromedriver",
    label = "//chrome/test/chromedriver:chromedriver_server",
)

targets.compile_target(
    name = "chromedriver_group",
    label = "//:chromedriver_group",
)

targets.compile_target(
    name = "chrome/browser/media/router",
    label = "//chrome/browser/media/router:router",
)

targets.compile_target(
    name = "chrome/browser/media/router:openscreen_unittests",
    label = "//chrome/browser/media/router:openscreen_unittests",
)

targets.compile_target(
    name = "chrome/browser/media/router:unittests",
    label = "//chrome/browser/media/router:unittests",
)

targets.compile_target(
    name = "chrome/installer/linux",
    label = "//chrome/installer/linux:linux",
)

targets.compile_target(
    name = "chrome/installer/mac",
    label = "//chrome/installer/mac:mac",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "chromium_builder_perf",
    label = "//:chromium_builder_perf",
)

targets.compile_target(
    name = "components/media_router/common/providers/cast/channel",
    label = "//components/media_router/common/providers/cast/channel:cast_channel",
)

targets.compile_target(
    name = "components/media_router/common/providers/cast/channel:unit_tests",
    label = "//components/media_router/common/providers/cast/channel:unit_tests",
)

targets.compile_target(
    name = "components/media_router/common/providers/cast/certificate",
    label = "//components/media_router/common/providers/cast/certificate",
)

targets.compile_target(
    name = "components/media_router/common/providers/cast/certificate:unit_tests",
    label = "//components/media_router/common/providers/cast/certificate:unit_tests",
)

targets.compile_target(
    name = "components/mirroring/browser",
    label = "//components/mirroring/browser:browser",
)

targets.compile_target(
    name = "components/mirroring/service:mirroring_service",
    label = "//components/mirroring/service:mirrroring_service",
)

targets.compile_target(
    name = "components/mirroring:mirroring_tests",
    label = "//components/mirroring:mirroring_tests",
)

targets.compile_target(
    name = "components/mirroring:mirroring_unittests",
    label = "//components/mirroring:mirroring_unittests",
)

targets.compile_target(
    name = "components/openscreen_platform",
    label = "//components/openscreen_platform",
)

targets.compile_target(
    name = "content_shell",
    label = "//content/shell:content_shell",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "content_nocompile_tests",
    label = "//content/test:content_nocompile_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "cronet_package",
    label = "//components/cronet:cronet_package",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "empty_main",
    label = "//testing:empty_main",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "gn_all",
    label = "//:gn_all",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "linux_symbols",
    label = "//chrome:linux_symbols",
)

targets.compile_target(
    name = "mini_installer",
    label = "//chrome/installer/mini_installer:mini_installer",
)

targets.compile_target(
    name = "nacl_helper",
    label = "//components/nacl/loader:nacl_helper",
    skip_usage_check = True,
)

targets.compile_target(
    name = "nacl_helper_bootstrap",
    label = "//native_client/src/trusted/service_runtime/linux:bootstrap",
    skip_usage_check = True,
)

targets.compile_target(
    name = "ondevice_model_benchmark",
    label = "//components/optimization_guide/internal:ondevice_model_benchmark",
)

targets.compile_target(
    name = "pdf_fuzzers",
    label = "//pdf/pdfium/fuzzers:pdf_fuzzers",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "pdfium_test",
    label = "//third_party/pdfium/testing/pdfium_test",
)

targets.compile_target(
    name = "postmortem-metadata",
    label = "//v8:postmortem-metadata",
)

targets.compile_target(
    name = "previous_version_mini_installer",
    label = "//chrome/installer/mini_installer:previous_version_mini_installer",
)

targets.compile_target(
    name = "remoting/host:host",
    label = "//remoting/host:host",
)

targets.compile_target(
    name = "symupload",
    label = "//third_party/breakpad:symupload",
)

targets.compile_target(
    name = "telemetry_gpu_integration_test_scripts_only",
    label = "//chrome/test:telemetry_gpu_integration_test_scripts_only",
)

targets.compile_target(
    name = "trace_processor_shell",
    label = "//third_party/perfetto/src/trace_processor:trace_processor_shell",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "traffic_annotation_auditor_dependencies",
    label = "//:traffic_annotation_auditor_dependencies",
)

targets.compile_target(
    name = "traffic_annotation_proto",
    label = "//chrome/browser/privacy:traffic_annotation_proto",
)

targets.compile_target(
    name = "chrome/updater:all",
    label = "//chrome/updater:all",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "v4l2_stateless_decoder",
    label = "//media/gpu/v4l2:v4l2_stateless_decoder",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "v4l2_unittest",
    label = "//media/gpu/v4l2:v4l2_unittest",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "video_decode_accelerator_perf_tests",
    label = "//media/gpu/test:video_decode_accelerator_perf_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "video_encode_accelerator_perf_tests",
    label = "//media/gpu/test:video_encode_accelerator_perf_tests",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "chrome/enterprise_companion:all",
    label = "//chrome/enterprise_companion:all",
    # All references have been moved to starlark
    skip_usage_check = True,
)

targets.compile_target(
    name = "image_processor_perf_test",
    label = "//media/gpu/linux:image_processor_perf_test",
    # All references have been moved to starlark
    skip_usage_check = True,
)
