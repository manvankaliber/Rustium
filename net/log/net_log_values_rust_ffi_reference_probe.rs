// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Narrow synchronous ABI: no input or sink pointer is retained.
mod net_log_values_rust_core;
use net_log_values_rust_core as core;
use std::ffi::c_void;

type Sink = unsafe extern "C" fn(*mut c_void, u8, i32, f64, *const u8, usize);

// SAFETY: Every caller supplies a live base::Value context and a C++ sink which
// copies text before returning. Numeric variants never dereference the null
// text pointer. The Rust value (including its owned storage) outlives the call.
unsafe fn emit(value: core::Value<'_>, context: *mut c_void, sink: Sink) {
    let (kind, integer, number, text) = match &value {
        core::Value::Int(v) => (0, *v, 0.0, &[][..]),
        core::Value::Double(v) => (1, 0, *v, &[][..]),
        core::Value::Bool(v) => (2, i32::from(*v), 0.0, &[][..]),
        core::Value::Text(v) => (3, 0, 0.0, v.as_ref()),
        core::Value::Decimal { digits, start } => (3, 0, 0.0, &digits[*start..]),
    };
    // SAFETY: See the synchronous sink contract above.
    unsafe { sink(context, kind, integer, number, text.as_ptr(), text.len()) };
}

/// # Safety
/// `context` and `sink` must satisfy the synchronous sink contract above.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_log_values_rust_signed(value: i64, context: *mut c_void, sink: Sink) {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    // SAFETY: Forward the caller's live synchronous sink.
    unsafe { emit(core::signed(value), context, sink) };
}

/// # Safety
/// `context` and `sink` must satisfy the synchronous sink contract above.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_log_values_rust_unsigned(
    value: u64,
    context: *mut c_void,
    sink: Sink,
) {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    // SAFETY: Forward the caller's live synchronous sink.
    unsafe { emit(core::unsigned(value), context, sink) };
}

/// # Safety
/// `context` and `sink` must satisfy the synchronous sink contract above.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_log_values_rust_bool(value: bool, context: *mut c_void, sink: Sink) {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    // SAFETY: Forward the caller's live synchronous sink.
    unsafe { emit(core::boolean(value), context, sink) };
}

/// # Safety
/// For nonzero `length`, `data` is initialized readable storage of that length
/// within one allocation. A null pointer is permitted for zero length. The sink
/// does not mutate the input; it only writes a distinct live Value context and
/// copies text synchronously. Mode: 0=byte string, 1=binary, 2=literal string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_log_values_rust_bytes(
    data: *const u8,
    length: usize,
    mode: u8,
    context: *mut c_void,
    sink: Sink,
) {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    let raw = if length == 0 {
        &[]
    } else {
        // SAFETY: The public C++ span/string_view supplies the caller's valid
        // initialized bytes. Empty/null inputs took the branch above.
        unsafe { std::slice::from_raw_parts(data, length) }
    };
    let value = match mode {
        0 => core::string(raw),
        1 => core::binary(raw),
        2 => core::literal(raw),
        _ => std::process::abort(),
    };
    // SAFETY: The input and the destination are distinct; the callback copies
    // bytes and cannot invalidate the borrow. Nothing escapes this invocation.
    unsafe { emit(value, context, sink) };
}

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
    pub extern "C" fn rustium_probe_net_log_count() -> u64 {
        COUNT.load(Ordering::Relaxed)
    }
}
