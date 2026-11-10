# Cronet Project Overview

This checkout retains Cronet's portable C++ networking implementation and
portable code-generation and dependency-analysis tools. The Android Java API,
JNI adapters, sample APK, and device testing tools have been removed.

The shared implementation wraps Chromium's `//net` stack. `cronet_context.cc`
manages contexts and network/file task runners; `cronet_url_request.cc` handles
request lifecycles; `url_request_context_config.cc` parses context settings.
The `cronet_common` source set and its common test sources are registered in
[BUILD.gn](BUILD.gn). They are not standalone executables or an Android SDK.

The `tools/` directory retains portable IDL/C binding generation, symbol-map
utilities, and dependency analysis. Consult each tool's current callers before
changing shared behavior.

## Building and Testing

Build and test instructions are maintained in dedicated documentation files.
Agents must use these as the primary source of truth while remaining aware of
the local execution environment.

- **Build Instructions:** See [build_instructions.md](build_instructions.md).
- **Test Instructions:** See [test_instructions.md](test_instructions.md).

### AI Guidelines for Building and Testing

1.  **Environment Awareness:** Before executing any shell commands, identify if
    you are in a managed environment (e.g., Cider-V). If so, **do not** run raw
    `autoninja` or `cr_cronet.py` commands; instead, use the environment's
    built-in build/test tools or APIs.
2.  **Output Directory (`<out_dir>`)**: Some documentation uses the `<out_dir>`
    placeholder. You should determine the correct directory by inspecting the
    `out/` directory (e.g., `out/Debug-arm64`).
3.  **Verification**: Always verify the build or test target existence within
    the `BUILD.gn` files before attempting to run it.

## Coding Style

As a component of the larger Chromium project, Cronet adheres strictly to
Chromium's established coding standards. Before submitting any code, ensure your
changes comply with the following guidelines:

- **C++**: Follows the [Google C++ Style
  Guide](https://google.github.io/styleguide/cppguide.html), with a few
  [Chromium-specific
  rules](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md)
  (e.g., using `base::` utilities like `base::Bind` and `base::OnceCallback`
  instead of standard library equivalents where specified).
- **Java**: Follows the [Chromium Java Style
  Guide](https://chromium.googlesource.com/chromium/src/+/main/styleguide/java/java.md),
  which is heavily based on the Google Java Style Guide.
- **Python**: Used primarily for tooling (`components/cronet/tools/`). Follows
  [PEP 8](https://peps.python.org/pep-0008/) and the [Chromium Python Style
  Guide](https://chromium.googlesource.com/chromium/src/+/main/styleguide/python/python.md).
- **Build Files (GN)**: `BUILD.gn` and `.gni` files must be formatted using the
  built-in formatter. Run `gn format <filename>` to automatically format them.

**Pre-upload Checks:** Chromium uses `git cl` for code review. When you run `git
cl upload`, a series of presubmit scripts (configured in `PRESUBMIT.py`) will
automatically run linting and formatting checks (e.g., `clang-format` for
C++/Java, `yapf` for Python) to ensure your code matches these conventions. You
can manually format your code before uploading using `git cl format`.
