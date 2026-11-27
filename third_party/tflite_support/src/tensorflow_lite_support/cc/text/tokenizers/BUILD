# This package contains C++ support libraries that Java libraries can invoke.
load("//third_party/bazel_rules/rules_cc/cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//tensorflow_lite_support:users"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "tokenizer",
    hdrs = [
        "tokenizer.h",
    ],
    deps = [
        "@com_google_absl//absl/strings",
    ],
)


cc_library(
    name = "bert_tokenizer",
    srcs = [
        "bert_tokenizer.cc",
    ],
    hdrs = [
        "bert_tokenizer.h",
    ],
    deps = [
        ":tokenizer",
        "//tensorflow_lite_support/cc/utils:common_utils",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow_text//tensorflow_text/core/kernels:regex_split",
        "@org_tensorflow_text//tensorflow_text/core/kernels:wordpiece_tokenizer",
    ],
)


cc_library(
    name = "sentencepiece_tokenizer",
    hdrs = [
        "sentencepiece_tokenizer.h",
    ],
    deps = [
        ":tokenizer",
        "@com_google_absl//absl/log:check",
        "@com_google_sentencepiece//src:sentencepiece_processor",
    ],
)


cc_library(
    name = "tokenizer_utils",
    srcs = ["tokenizer_utils.cc"],
    hdrs = [
        "tokenizer_utils.h",
    ],
    deps = [
        ":bert_tokenizer",
        ":regex_tokenizer",
        ":sentencepiece_tokenizer",
        ":tokenizer",
        "//tensorflow_lite_support/cc:common",
        "//tensorflow_lite_support/cc/port:status_macros",
        "//tensorflow_lite_support/cc/port:statusor",
        "//tensorflow_lite_support/metadata:metadata_schema_cc",
        "//tensorflow_lite_support/metadata/cc:metadata_extractor",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:string_view",
    ],
)

cc_library(
    name = "regex_tokenizer",
    srcs = [
        "regex_tokenizer.cc",
    ],
    hdrs = [
        "regex_tokenizer.h",
    ],
    deps = [
        ":tokenizer",
        "//tensorflow_lite_support/cc/utils:common_utils",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
        "@com_googlesource_code_re2//:re2",
    ],
)
