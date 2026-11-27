// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#[path = "net_diagnostics_debug_formatting_core.rs"]
mod core;
use std::ffi::c_char;

/// Required byte count, or usize::MAX for invalid flags. No allocation.
#[unsafe(no_mangle)]
pub extern "C" fn net_diagnostics_load_flags_length(flags: i32) -> usize {
    core::load_flags_length(flags).unwrap_or(usize::MAX)
}

/// # Safety
/// A non-null output with 0 < len <= isize::MAX must identify len initialized,
/// uniquely writable bytes. Null or oversized ranges are rejected before any
/// borrow. No borrow survives this synchronous call.
/// Invalid flags or insufficient space cause no writes. No allocation occurs.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_diagnostics_load_flags_write(
    flags: i32,
    output: *mut u8,
    len: usize,
) -> bool {
    if len > isize::MAX as usize || (output.is_null() && len != 0) {
        return false;
    }
    let bytes = if len == 0 {
        &mut []
    } else {
        // SAFETY: The caller provides the initialized unique output range.
        unsafe { std::slice::from_raw_parts_mut(output, len) }
    };
    core::write_load_flags(flags, bytes)
}

/// Format flags and transfer ownership of the returned UTF-8 allocation to C++.
///
/// # Safety
/// `out_len` must be non-null, properly aligned, and writable for one `usize`.
/// On success, the returned pointer and written length form one owned
/// allocation that must be passed exactly once to
/// `net_diagnostics_load_flags_free`. On invalid flags, this returns null and
/// leaves `out_len` unchanged.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_diagnostics_load_flags_format(
    flags: i32,
    out_len: *mut usize,
) -> *mut u8 {
    if out_len.is_null() {
        return std::ptr::null_mut();
    }
    let Some(text) = core::format_load_flags(flags) else {
        return std::ptr::null_mut();
    };
    let mut bytes = text.into_bytes().into_boxed_slice();
    let len = bytes.len();
    // SAFETY: The caller contract guarantees writable `out_len`; it cannot
    // alias this separately allocated Rust byte buffer.
    unsafe { out_len.write(len) };
    let ptr = bytes.as_mut_ptr();
    std::mem::forget(bytes);
    ptr
}

/// Release a buffer previously returned by `net_diagnostics_load_flags_format`.
///
/// # Safety
/// `ptr` and `len` must be the exact pointer/length pair returned by one
/// successful formatter call. The pair must not have been freed before and may
/// not be used after this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_diagnostics_load_flags_free(ptr: *mut u8, len: usize) {
    if ptr.is_null() {
        return;
    }
    // SAFETY: The function contract binds this pointer and length to the Rust
    // boxed-slice allocation and transfers ownership exactly once.
    drop(unsafe { Box::from_raw(std::ptr::slice_from_raw_parts_mut(ptr, len)) });
}

/// Return a process-lifetime NUL-terminated privacy mode label, or null if
/// invalid.
#[unsafe(no_mangle)]
pub extern "C" fn net_diagnostics_privacy_mode(mode: i32) -> *const c_char {
    core::privacy_mode_bytes(mode).map_or(std::ptr::null(), |b| b.as_ptr().cast())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CStr;

    #[test]
    fn formatter_ffi_ownership_and_invalid_output_contract() {
        let mut len = usize::MAX;
        let ptr = unsafe { net_diagnostics_load_flags_format(3, &mut len) };
        assert!(!ptr.is_null());
        let bytes = unsafe { std::slice::from_raw_parts(ptr, len) };
        assert_eq!(bytes, b"LOAD_VALIDATE_CACHE | LOAD_BYPASS_CACHE");
        unsafe { net_diagnostics_load_flags_free(ptr, len) };

        let mut unchanged = 0xfeedusize;
        assert!(unsafe { net_diagnostics_load_flags_format(1 << 30, &mut unchanged) }.is_null());
        assert_eq!(unchanged, 0xfeed);
        assert!(unsafe { net_diagnostics_load_flags_format(0, std::ptr::null_mut()) }.is_null());
    }

    #[test]
    fn privacy_ffi_returns_stable_static_nul_terminated_labels() {
        for (value, expected) in [
            (0, "disabled"),
            (1, "enabled"),
            (2, "enabled without client certs"),
            (3, "enabled partitioned state allowed"),
        ] {
            let first = net_diagnostics_privacy_mode(value);
            let second = net_diagnostics_privacy_mode(value);
            assert!(!first.is_null());
            assert_eq!(first, second);
            assert_eq!(unsafe { CStr::from_ptr(first) }.to_str().unwrap(), expected);
        }
        assert!(net_diagnostics_privacy_mode(-1).is_null());
        assert!(net_diagnostics_privacy_mode(4).is_null());
    }

    #[test]
    fn direct_writer_preserves_failure_and_suffix_bytes() {
        let flags = 3;
        let expected = b"LOAD_VALIDATE_CACHE | LOAD_BYPASS_CACHE";
        assert_eq!(net_diagnostics_load_flags_length(flags), expected.len());
        assert_eq!(net_diagnostics_load_flags_length(-1), usize::MAX);
        let mut output = [0xa5u8; 128];
        // SAFETY: Each call receives this live initialized unique array.
        unsafe {
            assert!(!net_diagnostics_load_flags_write(
                flags,
                output.as_mut_ptr(),
                expected.len() - 1
            ));
            assert_eq!(output, [0xa5; 128]);
            assert!(!net_diagnostics_load_flags_write(-1, output.as_mut_ptr(), output.len()));
            assert_eq!(output, [0xa5; 128]);
            assert!(net_diagnostics_load_flags_write(flags, output.as_mut_ptr(), output.len()));
            assert_eq!(&output[..expected.len()], expected);
            assert!(output[expected.len()..].iter().all(|&byte| byte == 0xa5));
            assert!(!net_diagnostics_load_flags_write(flags, std::ptr::null_mut(), 0));
            assert!(!net_diagnostics_load_flags_write(flags, std::ptr::null_mut(), 1));
            assert!(!net_diagnostics_load_flags_write(flags, std::ptr::null_mut(), usize::MAX));
        }
    }
}
