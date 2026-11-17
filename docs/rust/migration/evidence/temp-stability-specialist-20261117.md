Repair [finding 2](/D:/Rustium/docs/rust/migration/evidence/net-runtime-harness-review-20261117.md) by replacing [the path-only helper](/D:/RustiumReview/docs/rust/migration/drafts/net-runtime-harness/integration-proposal/.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/native_delivery.py:515) with a **context-managed allocation under one preprovisioned parent**, such as `D:\RustiumTmp`.

A second `resolve()` is insufficient. Also, **denying write/delete sharing alone does not establish junction stability**: Windows excludes attribute access from sharing restrictions, and the filesystem specification permits `FSCTL_SET_REPARSE_POINT` when either `FILE_WRITE_DATA` or `FILE_WRITE_ATTRIBUTES` is granted. [CreateFile documentation](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea), [reparse-setting access requirements](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/4aeefef8-92c3-4abc-af7a-a610caf8a165).

### Controlled-parent contract

Keep the implementation deliberately specific:

- Accept only the configured, absolute, top-level parent `D:\RustiumTmp`. Reject alternate/nested parents rather than resolving them into acceptance.
- Require an existing ordinary directory on a local fixed NTFS volume. Do not create missing ancestors during execution.
- Provision its protected DACL once, outside execution, with a **non-inheriting denial of `FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES` (`0x102`)** for ordinary callers, including the runner. On a directory, denying `FILE_WRITE_DATA` denies adding files; allow `FILE_ADD_SUBDIRECTORY` so run directories can still be allocated.
- Give the runner suitable read/list/traverse rights and inherited access for the newly created run directories. The parent-specific denial must not propagate into their contents.
- Record the provisioned parent’s volume/file identity and expected owner/DACL; verify these through its opened handle.

Windows permits explicit security descriptors at directory creation and handle-based security-descriptor inspection. Parent permissions do not automatically govern existing children; inheritance must be specified deliberately. [File security and access rights](https://learn.microsoft.com/en-us/windows/win32/fileio/file-security-and-access-rights).

**Trust boundary:** the volume/drive-letter mapping, provisioning owner, administrator/kernel operations, and sealed-snapshot namespace remain trusted and stable during execution. Provisioning must start without outstanding handles carrying the denied rights; changing a DACL does not revoke already granted handles. Other automation must not rewrite these ACLs. This is a correctness mechanism for the controlled runner workspace, not containment against arbitrary same-account or privileged interference.

### Allocation and verification

Implement these small ctypes helpers with explicit `argtypes`/`restype`:

```python
# pin_directory():
# CreateFileW(
#   path,
#   FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | READ_CONTROL,
#   FILE_SHARE_READ,                    # omit WRITE and DELETE
#   None, OPEN_EXISTING,
#   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
#   None)
#
# Reject INVALID_HANDLE_VALUE.
# Query attributes and identity through that same handle.
# Require DIRECTORY and reject every REPARSE_POINT tag.
```

Request `FILE_LIST_DIRECTORY`, not merely metadata access, for the retained sharing guard. Reject sharing violations; do not retry with weaker sharing.

The context manager’s core should have this shape; the helper names below describe required operations rather than complete runnable code:

```python
@contextmanager
def execution_temporary_root(snapshot, requested_parent):
    require_exact_configured_parent(requested_parent)
    with ExitStack() as handles:
        root = handles.enter_context(pin_directory("D:\\"))
        require_local_fixed_ntfs(root)

        parent = handles.enter_context(pin_directory(r"D:\RustiumTmp"))
        verify_provisioned_identity_and_acl(parent)

        snapshot_identity = opened_snapshot_identity(snapshot)
        require_disjoint(final_guid_path(parent), snapshot_identity.path)

        # Only now may any allocation occur.
        for _ in range(32):
            name = "net-" + secrets.token_hex(8)   # one ASCII component
            path = r"D:\RustiumTmp" + "\\" + name
            require_short_path(path)

            try:
                create_directory_with_run_acl(path)
            except FileExistsError:
                continue
            break
        else:
            raise ValueError("Cannot allocate unique temporary directory")

        run = handles.enter_context(pin_directory(path))
        require_ordinary_directory(run)
        verify_run_acl(run)

        require_equal(final_guid_path(run),
                      append_component(final_guid_path(parent), name))
        require_disjoint(final_guid_path(run), snapshot_identity.path)

        # Convert only a verified DOS "\\?\\D:\\..." result to "D:\\...".
        # Reject UNC/device results and unexpected aliases.
        env_path = checked_dos_path(run, expected=path)
        require_short_path(env_path)

        # Reopen the actual environment spelling and compare volume/file ID.
        with pin_directory(env_path) as probe:
            require_equal(file_identity(probe), file_identity(run))

        yield env_path, allocation_evidence(parent, run, env_path)
        # Retain directory contents as existing evidence policy requires.
```

Important implementation details:

- Create the run directory with a protected DACL that **denies `FILE_WRITE_ATTRIBUTES` on that directory only**, while allowing normal fixture creation inside it. Supply the descriptor to `CreateDirectoryW` at creation, not afterward.
- Its retained handle denies write/delete sharing. Combined with the attribute denial, this closes the two documented access routes for changing that directory into a reparse point while guarded.
- A reparse replacement during the create/open interval is opened without following it and rejected. No fixture or probe file is created before verification. Arbitrary concurrent replacement with another ordinary directory is outside the stated controlled-owner boundary.
- Retry only name collisions. All other allocation, inspection, ACL, or identity errors fail closed.
- Use handle-derived normalized volume-GUID paths for component-aware containment checks and a volume/file-ID tuple for identity. Do not use string-prefix containment.
- `GetFinalPathNameByHandleW` needs a resize loop; its insufficient-buffer return includes the terminator. Strip only the recognized DOS extended prefix when producing the short child environment value. [API contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfinalpathnamebyhandlew).
- Keep the existing parent limit if useful, but also bound the **actual environment value**. With the fixed parent and proposed name it is 34 ASCII characters. That remains a fixture-path reserve, not proof that both failing fixtures fit.

### Integration and resource lifetimes

Change the call site to encompass the entire program loop:

```python
with execution_temporary_root(snapshot, args.temporary_root) as (temporary, proof):
    env["TMP"] = env["TEMP"] = temporary
    save_new(evidence / "execution-environment.json", proof)
    # Existing complete case loop, including waits and timeout handling.
```

Retain the root, parent, and run handles until every relevant child has finished. Make them non-inheritable and use `Popen(..., close_fds=True)`. Free allocated security descriptors with `LocalFree`; close all handles through `finally`/`ExitStack`, including partial setup failures.

The current timeout logic kills only the immediate process. If launcher descendants can survive it, require their termination before releasing these guards—potentially through the runner’s separate process-lifetime handling. Do not claim protection after guard release. Leave the temporary contents retained; avoid recursive cleanup on rejection.

### Focused ordinary validation

The director should exercise:

1. Successful allocation, exact child `TMP`/`TEMP`, writable nested fixtures, recorded volume/file identity, and retained contents.
2. Existing parent junction, symlink, wrong identity/DACL, nested parent, UNC path, missing parent, and unsupported volume: refusal before allocation.
3. Controlled rename/replacement attempts against the parent while paused before allocation: refusal, with no snapshot membership change.
4. Attribute-only reparse mutation attempts: access denied by the configured ACL, rather than assumed blocked by sharing.
5. Injected run-path junction replacement between creation and opening: rejection before child launch.
6. Name collision, overlength/unexpected final path, inspection failure, launch failure, timeout, and exception: no environment use before verification; handles released after child completion.
7. Native fixture compatibility with the retained run-directory handle and directory-only ACL restriction.

No files were changed and no tests, builds, or campaigns were run. This proposal uses documented Win32 operations and avoids depending on an unestablished concurrent-reparse guarantee for handle-relative `NtCreateFile`.
