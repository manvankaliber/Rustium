# Testing Cronet

Follow the [desktop build instructions](build_instructions.md) first.
The common networking implementation has C++ test sources in the
`//components/cronet:cronet_common_unittests` source set. It must be linked into
a test executable; it cannot be run directly. Check the current GN dependency
graph for the executable that includes it before building or running tests.

The shared Chromium networking tests are available through `net_unittests`.
Use the test executable and filters appropriate to the behavior being changed.
The Android instrumentation, Java API, device logging, and device stack-trace
workflows have been removed.

Portable Python tooling tests include:

```shell
python3 -m unittest components.cronet.tools.symbols_map_generator.generate_version_script_from_symbols_intersection_unittest
```

In a managed environment, use its build and test tools. These commands describe
the local CLI workflow; they are not a record of completed verification.
