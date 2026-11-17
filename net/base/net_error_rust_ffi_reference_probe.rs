// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Narrow stateless C ABI. Input slices reference immutable native constants or
//! QUICHE literals for this call only. Output may be uninitialized: raw copies
//! never create initialized references to it. No handles, callbacks or owners
//! survive a call; cancellation/destruction/reentrancy introduce no state.
//! The Windows-native mapper invokes its diagnostic callback only after all
//! local mapping decisions; no Rust reference or borrowed native data spans it.
#[path = "net_error_rust_core.rs"]
mod core;
use core::{posix::PosixConstants, windows::WindowsConstants};
const UNKNOWN: i32 = i32::MIN;
#[unsafe(no_mangle)]
pub extern "C" fn net_error_rust_defined(error: i32) -> bool {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    core::defined(error)
}
#[unsafe(no_mangle)]
pub extern "C" fn net_error_rust_certificate(error: i32) -> bool {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    core::certificate(error)
}
#[unsafe(no_mangle)]
pub extern "C" fn net_error_rust_client_certificate(error: i32) -> bool {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    core::client_certificate(error)
}
#[unsafe(no_mangle)]
pub extern "C" fn net_error_rust_hostname(error: i32) -> i8 {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    core::hostname(error)
}
#[unsafe(no_mangle)]
pub extern "C" fn net_error_rust_blocked(error: i32) -> bool {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    core::blocked(error)
}
#[unsafe(no_mangle)]
pub extern "C" fn net_error_rust_extended(error: i32, extended: i32) -> bool {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    core::extended(error, extended)
}
#[unsafe(no_mangle)]
pub extern "C" fn net_error_rust_file(error: i32) -> i32 {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    core::file(error).unwrap_or(UNKNOWN)
}
/// # Safety
/// `constants` points to the initialized immutable named constant POD for this
/// call. Its C++ field order and widths are generated from the same schema.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_error_rust_windows(
    error: u32,
    constants: *const WindowsConstants,
) -> i32 {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    // SAFETY: caller supplies the immutable POD; no native call occurs here.
    core::windows::map(error, unsafe { &*constants }).unwrap_or(UNKNOWN)
}
/// # Safety
/// `warn_unknown` is a valid non-unwinding native diagnostic function. It may
/// reenter this stateless mapper. No references or borrowed data span the call.
#[cfg(target_os = "windows")]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_error_rust_windows_native(
    error: u32,
    warn_unknown: unsafe extern "C" fn(u32),
) -> i32 {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    match core::windows_native::map(error) {
        Some(result) => result,
        None => {
            // SAFETY: valid synchronous non-unwinding diagnostic by contract.
            // Only copied scalars and the function pointer exist at this point.
            unsafe { warn_unknown(error) };
            core::ids::ERR_FAILED
        }
    }
}
/// # Safety
/// Same POD preconditions as windows; POSIX errors and constants remain signed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_error_rust_posix(error: i32, constants: *const PosixConstants) -> i32 {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    // SAFETY: caller supplies the immutable POD; no native call occurs here.
    core::posix::map(error, unsafe { &*constants }).unwrap_or(UNKNOWN)
}
/// # Safety
/// `quic` is null (no suffix) or points to `quic_len` immutable bytes. `output`
/// is null (length query), or writable for `capacity` bytes and disjoint from
/// inputs. Output need not be initialized. No output is written on short
/// capacity.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_error_rust_format(
    error: i32,
    full: bool,
    quic: *const u8,
    quic_len: usize,
    output: *mut u8,
    capacity: usize,
) -> usize {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    let suffix = if quic.is_null() {
        None
    } else {
        // SAFETY: immutable initialized literal/bytes supplied by caller.
        Some(unsafe { std::slice::from_raw_parts(quic, quic_len) })
    };
    let parts = core::format_parts(error, full, suffix);
    let required = parts[0].len() + parts[1].len();
    if !output.is_null() && capacity >= required {
        let mut written = 0;
        for part in parts {
            // SAFETY: length checked above; raw copies accept uninitialized
            // destination storage. Inputs and output are disjoint by contract.
            unsafe {
                std::ptr::copy_nonoverlapping(part.as_ptr(), output.add(written), part.len());
            }
            written += part.len();
        }
    }
    required
}

#[cfg(test)]
#[path = "net_error_rust_ffi_tests.rs"]
mod mapping_tests;

// Enabled only in the director's isolated qualification build. Never reset
// while work is running; all threads contribute to the same monotonic count.
#[cfg(rustium_reference_probe)]
mod rustium_reference_probe {
    use std::sync::atomic::{AtomicU64, Ordering};
    static COUNT: AtomicU64 = AtomicU64::new(0);
    pub(super) fn hit() {
        COUNT.fetch_add(1, Ordering::Relaxed);
    }
    #[unsafe(no_mangle)]
    pub extern "C" fn rustium_probe_net_error_count() -> u64 {
        COUNT.load(Ordering::Relaxed)
    }
}
