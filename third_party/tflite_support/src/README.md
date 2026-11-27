# TensorFlow Lite Support

TFLite Support is a toolkit that helps users to develop ML and deploy TFLite
models onto mobile devices. It works cross-Platform and is supported on Java,
C++ (WIP), and Swift (WIP). The TFLite Support project consists of the following
major components:

*   **TFLite Support Library**: a cross-platform library that helps to deploy
    TFLite models onto mobile devices.
*   **TFLite Model Metadata**: (metadata populator and metadata extractor
    library): includes both human and machine readable information about what a
    model does and how to use the model.
*   **TFLite Support Task Library**: a flexible and ready-to-use library for
    common machine learning model types, such as classification and detection,
    client can also build their own native/iOS inference API on Task
    Library infra.

TFLite Support library serves different tiers of deployment requirements from
easy onboarding to fully customizable. There are two major use cases that
TFLite Support targets at:

*   **Provide optimized model interface for popular ML tasks**. \
    The model interfaces provided by the TFLite Support Task Library are
    optimized for their task-specific interfaces. Users can also swap their own custom models with
    the default models in each task.

*   **Provide the flexibility to customize model interface and build inference
    pipelines**. \
    The TFLite Support Util Library contains varieties of util methods and data
    structures to perform pre/post processing and data conversion. It is also
    designed to match the behavior of TensorFlow modules, such as TF.Image and
    TF.text, ensuring consistency from training to inferencing.

See the
[documentation on tensorflow.org](https://www.tensorflow.org/lite/inference_with_metadata/overview)
for more instruction and examples.

## Build Instructions

Use Chromium GN targets for the integrated desktop build. The remaining
standalone host libraries also retain their Bazel definitions.

## How to contribute

Please issue a pull request and assign @lu-wang-g for a code review.

## Contact us

Let us know what you think about TFLite Support by creating a
[new Github issue](https://github.com/tensorflow/tflite-support/issues/new), or
email us at tflite-support-team@google.com.
