"""
Partial workspace defintion for the TFLite Support Library. See WORKSPACE for usage.
"""

load("@org_tensorflow//tensorflow:workspace3.bzl", "workspace")
load("@rules_jvm_external//:defs.bzl", "maven_install")  # Maven dependencies.

def tflite_support_workspace6():
    """Partial workspace definition for the TFLite Support Library. See WORKSPACE for usage."""
    maven_install(
        artifacts = [
            "com.google.truth:truth:jar:1.1",
            "commons-io:commons-io:jar:2.8.0",
            "org.mockito:mockito-core:jar:3.0.0",
            "org.mockito:mockito-inline:jar:3.0.0",
            "junit:junit:jar:4.13",
        ],
        repositories = [
            "https://repo1.maven.org/maven2",
        ],
        fetch_sources = True,
        version_conflict_policy = "pinned",
    )

    workspace()
