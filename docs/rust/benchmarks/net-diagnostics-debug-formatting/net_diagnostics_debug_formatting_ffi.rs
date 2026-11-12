// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#[path = "net_diagnostics_debug_formatting_core.rs"]
mod core;
use std::ffi::c_char;

/// Format flags and transfer ownership of the returned UTF-8 allocation to C++.
///
/// # Safety
/// `out_len` must be non-null, properly aligned, and writable for one `usize`.
/// On success, the returned pointer and written length form one owned allocation
/// that must be passed exactly once to `net_diagnostics_load_flags_free`. On
/// invalid flags, this returns null and leaves `out_len` unchanged.
#[no_mangle]
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
#[no_mangle]
pub unsafe extern "C" fn net_diagnostics_load_flags_free(ptr: *mut u8, len: usize) {
    if ptr.is_null() {
        return;
    }
    // SAFETY: The function contract binds this pointer and length to the Rust
    // boxed-slice allocation and transfers ownership exactly once.
    drop(unsafe { Box::from_raw(std::ptr::slice_from_raw_parts_mut(ptr, len)) });
}

/// Return a process-lifetime NUL-terminated privacy mode label, or null if invalid.
#[no_mangle]
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
}
