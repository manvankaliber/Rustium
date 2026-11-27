# Description:
# Test targets for TFLite MetadataExtractor.
load("@org_tensorflow//tensorflow/java:build_defs.bzl", "JAVACOPTS")
load("//third_party/bazel_rules/rules_java/java:defs.bzl", "java_test")

package(
    default_visibility = ["//visibility:private"],
    licenses = ["notice"],  # Apache 2.0
)


java_test(
    name = "MetadataParserTest",
    size = "small",
    srcs = ["org/tensorflow/lite/support/metadata/MetadataParserTest.java"],
    javacopts = JAVACOPTS,
    tags = ["no_oss"],
    test_class = "org.tensorflow.lite.support.metadata.MetadataParserTest",
    deps = [
        "//tensorflow_lite_support/metadata/java:tensorflowlite_support_metadata_lib",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
