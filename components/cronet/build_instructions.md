# Cronet build instructions

Cronet's portable C++ networking code remains available for desktop hosts.
The Android Java API, APK packaging, and device build helper have been removed.

Use the repository's desktop checkout and toolchain setup. In a standard local
CLI environment, generate a desktop build directory and build the common code:

```shell
gn gen out/Default
autoninja -C out/Default components/cronet:cronet_common
```

`cronet_common` is a source set, not a standalone packaged SDK. Inspect
[BUILD.gn](BUILD.gn) for the current targets and their dependencies. In a managed
environment, use that environment's build tools.

See [test_instructions.md](test_instructions.md) for verification.
