// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Narrow ABI adapter: no cookie policy and no retained pointers.
mod parsed_cookie_rust_core;
use parsed_cookie_rust_core as core;
use std::slice;

// SAFETY: callers provide readable initialized bytes for this call. Empty
// ranges can be null. Shared input ranges may overlap one another.
unsafe fn bytes<'a>(ptr: *const u8, len: usize) -> &'a [u8] {
    if len == 0 {
        &[]
    } else {
        unsafe { slice::from_raw_parts(ptr, len) }
    }
}

/// # Safety
/// Input is readable for len bytes, or null if empty. op selects the documented
/// pure byte query; all returned offsets are inside the supplied range.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_query(op: usize, ptr: *const u8, len: usize) -> usize {
    let input = unsafe { bytes(ptr, len) };
    match op {
        0 => core::terminator(input),
        1 => usize::from(core::valid_chars(input, true)),
        2 => usize::from(core::valid_chars(input, false)),
        3 => usize::from(len <= 1024),
        4 => usize::from(core::string_scan(input, false).len == len),
        5 => core::priority(input) as usize,
        _ => 0,
    }
}

/// # Safety
/// Input must be readable (empty may be null). token selects name vs value;
/// string selects terminator-aware helpers vs raw iterator APIs.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_scan(
    ptr: *const u8,
    len: usize,
    token: bool,
    string: bool,
    aliases: usize,
) -> core::Scan {
    let input = unsafe { bytes(ptr, len) };
    if string {
        core::string_scan(input, token)
    } else {
        core::scan_aliased(input, token, aliases)
    }
}

/// # Safety
/// Input is readable for len bytes. position <= len; first identifies the
/// constructor's initial pair. Input remains unchanged across pair calls.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_next(
    ptr: *const u8,
    len: usize,
    position: usize,
    first: bool,
) -> core::Pair {
    core::next_pair(unsafe { bytes(ptr, len) }, position, first)
}

/// # Safety
/// Both ranges must be readable for the call (empty may be null).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_valid_pair(
    name: *const u8,
    name_len: usize,
    value: *const u8,
    value_len: usize,
) -> usize {
    core::valid_pair(unsafe { bytes(name, name_len) }, unsafe { bytes(value, value_len) })
}

/// # Safety
/// All three ranges must be readable for the call (empty may be null).
/// Inputs can alias; no input borrow survives this call or a C++ storage write.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_mutation(
    field: usize,
    ptr: *const u8,
    len: usize,
    name: *const u8,
    name_len: usize,
    value: *const u8,
    value_len: usize,
    enabled: bool,
) -> core::Mutation {
    core::mutation(
        field,
        unsafe { bytes(ptr, len) },
        unsafe { bytes(name, name_len) },
        unsafe { bytes(value, value_len) },
        enabled,
    )
}

/// # Safety
/// ptr is exclusively writable and initialized for len bytes; no shared slice
/// to that range may remain live. Empty may be null.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_lower(ptr: *mut u8, len: usize) {
    if len != 0 {
        core::lower(unsafe { slice::from_raw_parts_mut(ptr, len) });
    }
}

/// # Safety
/// indices points to nine initialized, aligned, exclusively writable usize
/// values, disjoint from all other borrowed memory. removed is a stored index.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_clear(indices: *mut usize, removed: usize) {
    core::clear_indices(unsafe { slice::from_raw_parts_mut(indices, 9) }, removed);
}

/// # Safety
/// Input must be readable for len bytes (empty may be null).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_same_site(
    ptr: *const u8,
    len: usize,
    present: bool,
) -> core::SameSite {
    core::same_site(unsafe { bytes(ptr, len) }, present)
}

/// # Safety
/// Input is readable for len bytes (empty may be null). Booleans describe
/// the current serializer storage state; result bits select separator/value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_cookie_serialization(
    ptr: *const u8,
    len: usize,
    first: bool,
    output_empty: bool,
) -> usize {
    core::serialization_flags(unsafe { bytes(ptr, len) }, first, output_empty)
}
const _: () = {
    assert!(std::mem::size_of::<core::Scan>() == 4 * std::mem::size_of::<usize>());
    assert!(std::mem::size_of::<core::Pair>() == 10 * std::mem::size_of::<usize>());
    assert!(std::mem::size_of::<core::Mutation>() == 3 * std::mem::size_of::<usize>());
    assert!(std::mem::size_of::<core::SameSite>() == 2 * std::mem::size_of::<i32>());
    assert!(std::mem::align_of::<core::Pair>() == std::mem::align_of::<usize>());
};
