# Minimal auth component-link proposal

The failed full-cohort link names eight auth symbols: Latin1 charset constant,
ConvertToUtf8AndNormalize, three EntryMapKey methods, Identity constructor,
NTLM Factory constructor and SSPILibraryDefault constructor. These five header
proposals annotate only those existing definitions with NET_EXPORT_PRIVATE.
No algorithm, data member, base, virtual declaration or signature is changed.
The separate named auth provider candidate removes its native NTLM factory
construction, but the current retained baseline still needs that old symbol.

EntryMapKey's frozen comparison is native tuple comparison; its constructor
stores the existing key values. Identity is default construction. SSPI remains
an explicitly native OS/provider boundary, not a completed Rust rewrite. The
ICU conversion implementation and its charset/data/provider dependencies remain
native and need supported-platform identity/behavior review. No policy was
substituted for an independent reference algorithm by this proposal.

Review declaration/definition ownership and additive ABI, then apply with all
before guards under the director gate. Rebuild producer and affected consumer
headers, capture actual Net DLL exports/import library and link command, verify
all eight symbols resolve and no duplicate implementation is introduced. Keep
the full ordinary workload/test membership and all reference/provider debt.
This is not a compiler, binary ABI, runtime, performance or cutover pass.
