load("//third_party/bazel_rules/rules_cc/cc:cc_library.bzl", "cc_library")
load("//third_party/bazel_rules/rules_cc/cc:cc_test.bzl", "cc_test")

package(
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "gpu_plugin",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@org_tensorflow//tensorflow/lite/acceleration/configuration:gpu_plugin",
    ],
    alwayslink = 1,  # For registration to always run.
)


cc_library(
    name = "xnnpack_plugin",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@org_tensorflow//tensorflow/lite/acceleration/configuration:xnnpack_plugin",
    ],
    alwayslink = 1,  # For registration to always run.
)

# To use the edgetpu_coral_plugin externally, add the following flags to the bazel command:
# --define darwinn_portable=1
cc_library(
    name = "edgetpu_coral_plugin",
    srcs = ["edgetpu_coral_plugin.cc"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@libedgetpu//tflite/public:edgetpu_c",
        "@libedgetpu//tflite/public:oss_edgetpu_direct_all",  # buildcleaner: keep
        "@org_tensorflow//tensorflow/lite/acceleration/configuration:configuration_fbs",
        "@org_tensorflow//tensorflow/lite/acceleration/configuration:delegate_registry",
    ],
    alwayslink = 1,  # For registration to always run.
)

# To test it externally, plugin a Coral device, and run the following command:
# bazel test tensorflow_lite_support/acceleration/configuration:edgetpu_coral_plugin_test \
# --define darwinn_portable=1
cc_test(
    name = "edgetpu_coral_plugin_test",
    srcs = ["edgetpu_coral_plugin_test.cc"],
    data = [
        "//tensorflow_lite_support/acceleration/configuration/testdata:test_files",
    ],
    tags = [
        "manual",
        "notap",  # Requires edge TPU device.
    ],
    deps = [
        ":edgetpu_coral_plugin",
        "//tensorflow_lite_support/cc/task/vision/utils:image_utils",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/lite:framework",
        "@org_tensorflow//tensorflow/lite/acceleration/configuration:configuration_fbs",
        "@org_tensorflow//tensorflow/lite/acceleration/configuration:delegate_registry",
        "@org_tensorflow//tensorflow/lite/c:common",
        "@org_tensorflow//tensorflow/lite/kernels:builtin_ops",
    ],
)
