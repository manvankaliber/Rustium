"""
Partial workspace defintion for the TFLite Support Library. See WORKSPACE for usage.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@build_bazel_apple_support//lib:repositories.bzl", "apple_support_dependencies")
load("@build_bazel_rules_apple//apple:repositories.bzl", "apple_rules_dependencies")
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
load("@upb//bazel:repository_defs.bzl", "bazel_version_repository")
load("//third_party/py:python_configure.bzl", "python_configure")
load("//third_party/tensorflow:tf_configure.bzl", "tf_configure")

def tflite_support_workspace2():
    """Partial workspace definition for the TFLite Support Library. See WORKSPACE for usage."""
    tf_configure(name = "local_config_tf")
    grpc_deps()
    apple_rules_dependencies()
    apple_support_dependencies()

    bazel_version_repository(name = "bazel_version")

    python_configure(name = "local_config_python")
