"""Build definitions supporting platform-independent native build."""

load("@org_tensorflow//tensorflow:tensorflow.bzl", "tf_copts")

def micore_if(ios = [], default = []):
    """Helper to create a select.

    Args:
      ios: what to return if compiling for iOS.
      default: what to return otherwise.
    Returns:
      the `ios` list for Apple compilation and the `default` list otherwise.
    """
    return select({
        ":apple": ios,
        "//conditions:default": default,
    })

def micore_tf_copts():
    """C options for Tensorflow builds.

    Returns:
      a list of copts which must be used by each cc_library which
      refers to Tensorflow. Enables the library to compile for
      Apple platforms and Linux.
    """
    return tf_copts() + [
        "-Wno-narrowing",
        "-Wno-sign-compare",
        "-Wno-overloaded-virtual",
    ]

def micore_tf_deps():
    """Dependencies for Tensorflow builds.

    Returns:
      list of dependencies which must be used by each cc_library
      which refers to Tensorflow. Enables the library to compile for
      Apple platforms and Linux. Use this macro instead of directly
      declaring dependencies on Tensorflow.
    """
    return micore_if(
        ios = [
            "@org_tensorflow//tensorflow/core:portable_tensorflow_lib",
            "@gemmlowp//:eight_bit_int_gemm",
            "@fft2d//:fft2d",
        ],
        default = [
            # Standard references for Tensorflow when building for Linux. We use
            # an indirection via the alias targets below, to facilitate whitelisting
            # these deps in the mobile license presubmit checks.
            "@local_config_tf//:libtensorflow_framework",
            "@local_config_tf//:tf_header_lib",
        ],
    )
