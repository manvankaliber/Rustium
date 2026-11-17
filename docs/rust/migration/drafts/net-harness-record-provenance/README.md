# Retained ordinary harness capture gate

This addresses the provenance limitation in independent review984d2f06. The
strict schema comparator remains frozen as schema.py; its synthetic success
alone does not qualify source/binary provenance. qualify_records.py requires a
director-trusted, SHA-256-pinned provenance manifest binding both captures to
the existing ordinary conductor's copied snapshots, successful current build/ABI
freshness receipt, external Python qualification, execution result and raw output.
It verifies copied file bytes, data/directory membership, source mappings and
executable identity before comparing the complete expected records.

The reviewed_mapping_manifest is the guarded parent source proposal manifest,
not a claim that current after-images equal original before-images. Original
contract sources map through its before/after hashes. A repeatability comparison
requires both captures to use reviewed after-images; reviewed-source-change
requires the first capture to use original images. These relations are explicit.
Store fixture timestamps must still match; different system-time fixtures remain
noncomparable. Timing mode is rejected: benchmark execution/provenance needs its
separate gate, and no timing workload was run here.

Pins must originate in trusted retained director/native-gate evidence. This tool
does not authenticate arbitrary caller-authored receipts, replace independent
review or establish that synthetic files were executed. Actual current captures
are not yet available: the full Network consumer build is still running. No real
native capture, source-change parity, performance or cutover claim is made.

The source-only synthetic suite covers complete repeatability and reviewed source
transitions; missing/wrong mappings; changed source bytes and snapshot membership;
hardlink rejection; incorrect/failed execution receipts; output disagreement;
nonfinite JSON; and optimized-Python CLI rejection. Tests use unmistakably fake
binary/source bytes and do not run a compiler or native workload.
