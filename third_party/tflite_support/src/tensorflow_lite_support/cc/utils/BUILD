load("@org_tensorflow//tensorflow/lite/core/shims:cc_library_with_tflite.bzl", "cc_library_with_tflite")
load("//third_party/bazel_rules/rules_cc/cc:cc_library.bzl", "cc_library")

package(
    licenses = ["notice"],  # Apache 2.0
)


cc_library(
    name = "common_utils",
    srcs = [
        "common_utils.cc",
    ],
    hdrs = [
        "common_utils.h",
    ],
    visibility = [
        "//tensorflow_lite_support:internal",
    ],
    deps = [
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
    ],
)
