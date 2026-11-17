// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Narrow synchronous ABI adapter. No native pointer or borrowed byte view is
//! retained across a service mutation. The entire cache policy is safe Rust.
#![deny(unsafe_op_in_unsafe_fn)]
mod http_auth_cache_rust_core;
use http_auth_cache_rust_core::{Bytes, Operation, Scalar, Services};
use std::ffi::c_void;

#[repr(C)]
pub struct View {
    data: *const u8,
    len: usize,
}
#[repr(C)]
pub struct Vtable {
    scalar: unsafe extern "C" fn(*mut c_void, u32, i64, i64, i64) -> i64,
    bytes: unsafe extern "C" fn(*const c_void, u32, i64, i64) -> View,
    push_path: unsafe extern "C" fn(*mut c_void, i64, *const u8, usize),
}
struct Native<'a> {
    context: *mut c_void,
    table: &'a Vtable,
}
impl Services for Native<'_> {
    fn scalar(&mut self, op: Scalar, id: i64, a: i64, b: i64) -> i64 {
        // SAFETY: synchronous exclusive C++ stack context; valid opaque handles
        // originate only in policy traversal. Services retain no Rust borrows.
        unsafe { (self.table.scalar)(self.context, op as u32, id, a, b) }
    }
    fn bytes(&self, op: Bytes, id: i64, index: i64) -> &[u8] {
        // SAFETY: native service returns an initialized contiguous byte view.
        // The slice borrows self, preventing a mutable service call while live.
        let view = unsafe { (self.table.bytes)(self.context, op as u32, id, index) };
        if view.len == 0 {
            return &[];
        }
        unsafe { std::slice::from_raw_parts(view.data, view.len) }
    }
    fn push_path(&mut self, id: i64, path: &[u8]) {
        // SAFETY: path is initialized and valid through this synchronous copy.
        unsafe { (self.table.push_path)(self.context, id, path.as_ptr(), path.len()) }
    }
}

/// # Safety
/// `context` and aligned `table` must be valid for the synchronous invocation.
/// Callbacks obey the lifetime and storage contracts in the matching header.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http_auth_cache_rust_run(
    context: *mut c_void,
    table: *const Vtable,
    op: u32,
    target: i64,
    scheme: i64,
    a: i64,
    b: i64,
) -> i64 {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    let op = match op {
        0 => Operation::Lookup,
        1 => Operation::LookupPath,
        2 => Operation::Add,
        3 => Operation::Remove,
        4 => Operation::ClearBetween,
        5 => Operation::ClearAll,
        6 => Operation::Stale,
        7 => Operation::SetPartition,
        8 => Operation::CopyProxy,
        9 => Operation::IncrementNonce,
        10 => Operation::EntryStale,
        11 => Operation::AddPath,
        12 => Operation::HasPath,
        13 => Operation::Equal,
        _ => std::process::abort(),
    };
    // SAFETY: table is supplied by the synchronous native adapter, never null.
    let table = unsafe { &*table };
    http_auth_cache_rust_core::run(&mut Native { context, table }, op, target, scheme, a, b)
}

// Qualification-only monotonic count; never reset while work is in flight.
#[cfg(rustium_reference_probe)]
mod rustium_reference_probe {
    use std::sync::atomic::{AtomicU64, Ordering};
    static COUNT: AtomicU64 = AtomicU64::new(0);
    pub(super) fn hit() {
        COUNT.fetch_add(1, Ordering::Relaxed);
    }
    #[unsafe(no_mangle)]
    pub extern "C" fn rustium_probe_auth_cache_count() -> u64 {
        COUNT.load(Ordering::Relaxed)
    }
}
