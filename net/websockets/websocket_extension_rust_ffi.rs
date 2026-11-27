// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod websocket_extension_rust_core;
use std::{ffi::c_void, slice};
use websocket_extension_rust_core as core;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Bytes {
    data: *const u8,
    len: usize,
}
#[repr(C)]
pub struct Pair {
    name: Bytes,
    value: Bytes,
}
const _: () = assert!(size_of::<Pair>() == 4 * size_of::<usize>());
const _: () = assert!(align_of::<Pair>() == align_of::<usize>());

// SAFETY: Callers provide valid initialized readable call-scoped ranges. Empty
// ranges may be null. Shared ranges may overlap, including self-comparisons.
unsafe fn bytes<'a>(b: Bytes) -> &'a [u8] {
    if b.len == 0 {
        &[]
    } else {
        unsafe { slice::from_raw_parts(b.data, b.len) }
    }
}
unsafe fn pairs<'a>(p: *const Pair, len: usize) -> Vec<core::Parameter<'a>> {
    let p = if len == 0 { &[] } else { unsafe { slice::from_raw_parts(p, len) } };
    p.iter()
        .map(|p| core::Parameter {
            name: unsafe { bytes(p.name) },
            value: unsafe { bytes(p.value) },
        })
        .collect()
}
fn view(s: &[u8]) -> Bytes {
    Bytes { data: s.as_ptr(), len: s.len() }
}

/// # Safety
/// Input must be readable for the call; null is permitted for an empty range.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_extension_token(input: Bytes) -> bool {
    core::token(unsafe { bytes(input) })
}

/// # Safety
/// Inputs and descriptors are initialized, aligned and readable for this call.
/// Ranges may overlap. Nothing is written or retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_extension_equal(a: Pair, b: Pair) -> bool {
    core::Parameter { name: unsafe { bytes(a.name) }, value: unsafe { bytes(a.value) } }
        == core::Parameter { name: unsafe { bytes(b.name) }, value: unsafe { bytes(b.value) } }
}

/// # Safety
/// Descriptor arrays and their byte ranges are initialized, aligned and
/// readable for this call. They may overlap. Nothing is written or retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_extension_equivalent(
    name: Bytes,
    a: *const Pair,
    alen: usize,
    other: Bytes,
    b: *const Pair,
    blen: usize,
) -> bool {
    core::equivalent(
        unsafe { bytes(name) },
        &unsafe { pairs(a, alen) },
        unsafe { bytes(other) },
        &unsafe { pairs(b, blen) },
    )
}

/// # Safety
/// Input is readable and remains unmodified throughout callbacks. `context`
/// belongs to the caller. The callback copies each borrowed range before
/// returning, never retains it or changes input storage, and cannot unwind.
/// kind=0 starts an extension, kind=1 adds a parameter. No callbacks on
/// failure.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_extension_parse(
    input: Bytes,
    context: *mut c_void,
    emit: unsafe extern "C" fn(*mut c_void, usize, Bytes, Bytes),
) {
    if let Some(parsed) = core::parse(unsafe { bytes(input) }) {
        for e in parsed {
            unsafe { emit(context, 0, view(e.name), view(&[])) };
            for (name, value) in e.parameters {
                unsafe { emit(context, 1, view(name), view(&value)) };
            }
        }
    }
}

/// # Safety
/// Inputs follow equivalent's range contract. Callback copies the output bytes
/// before returning and cannot unwind. Output allocation is disjoint from all
/// input ranges; callback must not mutate/retain input descriptors or bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_extension_serialize(
    name: Bytes,
    p: *const Pair,
    len: usize,
    context: *mut c_void,
    emit: unsafe extern "C" fn(*mut c_void, Bytes),
) {
    let result = core::serialize(unsafe { bytes(name) }, &unsafe { pairs(p, len) });
    unsafe { emit(context, view(&result)) };
}
