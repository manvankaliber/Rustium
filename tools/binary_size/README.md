# Tools for Analyzing Chrome's Binary Size

SuperSize collects and compares symbol-level size information from desktop ELF
binaries, linker maps and `.pak` resources. Its archive collector uses Chromium's
build outputs and LLVM tools. Windows-specific size analysis is documented at
https://www.chromium.org/developers/windows-binary-sizes.

## SuperSize

The command-line tool creates and inspects `.size` and `.sizediff` files. The web
viewer visualizes those files. See
[the SuperSize guide](libsupersize/README.md) for archive, query and diff commands.
Existing reports remain readable, including historical Java/Dex symbols.

## diagnose_bloat.py

This tool diagnoses binary size changes between revisions on Linux. It checks out
and synchronizes revisions in the local checkout, builds them using release GN
arguments, archives their native size information and displays the differences.
Use a separate checkout if you need to preserve local work.

```bash
# Build and compare HEAD^ and HEAD.
tools/binary_size/diagnose_bloat.py HEAD -v

# Build and compare using jj.
tools/binary_size/diagnose_bloat.py @ --jj -v

# Override build arguments.
tools/binary_size/diagnose_bloat.py HEAD --gn-args="is_official_build=false" -v

# Compare contiguous revisions in a subrepository.
tools/binary_size/diagnose_bloat.py AFTER_REV --reference-rev BEFORE_REV --subrepo v8 --all -v

# Show all options.
tools/binary_size/diagnose_bloat.py -h
```

## Other Size Tools

[Bloaty McBloatface](https://github.com/google/bloaty) analyzes executable sizes.
Chromium usage is tracked in [crbug/698733](https://crbug.com/698733).
