# ![Logo](chrome/app/theme/chromium/product_logo_64.png) Chromium

Chromium is an open-source browser project that aims to build a safer, faster,
and more stable way for all users to experience the web.

The project's web site is https://www.chromium.org.

To check out the source code locally, don't use `git clone`! Instead,
follow [the instructions on how to get the code](docs/get_the_code.md).

Documentation in the source is rooted in [docs/README.md](docs/README.md).

Learn how to [Get Around the Chromium Source Code Directory
Structure](https://www.chromium.org/developers/how-tos/getting-around-the-chrome-source-code).

For historical reasons, there are some small top level directories. Now the
guidance is that new top level directories are for product (e.g. Chrome,
Android WebView, Ash). Even if these products have multiple executables, the
code should be in subdirectories of the product.

If you found a bug, please file it at https://crbug.com/new.

## Licenses and upstream build compatibility

The root [LICENSE](LICENSE) contains the full consolidated license texts and
attributions, indexed by their original repository paths. Copyright notices
embedded in source files remain in place. This consolidation does not change
the licenses or other obligations applying to inherited material.

Repeated terms are stored once as shared text blocks inside that same file.
Each component document retains its notices and references the shared terms
that form part of it. Different wording and exceptions remain distinct;
formatting differences are recorded so the original documents can be restored
exactly. No legal text is fetched externally or stored in a separate archive.

After updating document records, `python tools/licenses/license_bundle.py
--compact` rebuilds the shared text blocks and verifies lossless restoration
before replacing the root file. Compaction preserves original wording; it
does not infer licensing equivalence from license names.

Verify the bundle with `python tools/licenses/license_bundle.py --verify`.
Before running legacy Chromium builds, package tools, or tests that require
the original legal-document paths, run:

```sh
python tools/licenses/license_bundle.py --materialize
```

These are generated local copies, not additional authoritative license files.
Remove them afterward with
`python tools/licenses/license_bundle.py --clean-materialized`. Both commands
refuse to overwrite or delete differing local files. The root bundle is never
overwritten. Chromium's license generator reads the bundled records directly.

Include the complete root bundle with standalone source and binary releases.
When adding dependencies or restructuring components, update the bundle's
texts and provenance mappings to keep their applicable terms identifiable.
