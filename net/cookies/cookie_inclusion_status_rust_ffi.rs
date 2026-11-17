// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![deny(unsafe_op_in_unsafe_fn)]
#[path = "cookie_inclusion_status_rust_core.rs"]
mod core;
use core::State;

#[unsafe(no_mangle)]
pub extern "C" fn cookie_inclusion_status_rust_query(
    s: State,
    op: u32,
    reason: i32,
    mask: u64,
) -> bool {
    core::query(s, op, reason, mask)
}
#[unsafe(no_mangle)]
pub extern "C" fn cookie_inclusion_status_rust_same(a: State, b: State) -> bool {
    core::same(a, b)
}
#[unsafe(no_mangle)]
pub extern "C" fn cookie_inclusion_status_rust_apply(
    s: State,
    op: u32,
    reason: i32,
    mask: u64,
) -> State {
    core::apply(s, op, reason, mask)
}
#[unsafe(no_mangle)]
pub extern "C" fn cookie_inclusion_status_rust_components(
    exclusions: u64,
    warnings: u64,
    exemption: i32,
) -> State {
    core::components(exclusions, warnings, exemption)
}
/// # Safety
/// A non-null buffer must point to `capacity` initialized, exclusively writable
/// bytes in one allocation. No references escape. Empty/null buffers size only.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_inclusion_status_rust_debug(
    s: State,
    output: *mut u8,
    capacity: usize,
) -> usize {
    let output = if capacity == 0 || output.is_null() {
        &mut []
    } else {
        // SAFETY: caller supplies the documented initialized exclusive range.
        unsafe { std::slice::from_raw_parts_mut(output, capacity) }
    };
    core::debug_string(s, output)
}
