// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The core has no unsafe code. This ABI accepts readable byte ranges for the
// duration of each call and writable, disjoint, initialized byte buffers.
// Empty inputs may be null. No pointers or borrows survive a call. Outputs are
// staged in owned Rust values before the adapter writes to C++ buffers.
mod auth_challenge_core;
use std::slice;

#[repr(C)]
pub struct Tokenized {
    scheme_len: usize,
    params_begin: usize,
    params_len: usize,
    has_params: usize,
}
const _: () = assert!(std::mem::size_of::<Tokenized>() == 4 * std::mem::size_of::<usize>());
const _: () = assert!(std::mem::align_of::<Tokenized>() == std::mem::align_of::<usize>());

// SAFETY: callers supply a readable byte range for the duration of the call.
unsafe fn bytes<'a>(ptr: *const u8, len: usize) -> &'a [u8] {
    if len == 0 {
        &[]
    } else {
        unsafe { slice::from_raw_parts(ptr, len) }
    }
}

/// # Safety
/// Input must be readable for len bytes, or null when len is zero. The output
/// must be disjoint and writable for len initialized bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_tokenize(
    input: *const u8,
    len: usize,
    scheme_out: *mut u8,
) -> Tokenized {
    let parsed = auth_challenge_core::tokenize(unsafe { bytes(input, len) });
    // `scheme_out` has at least len initialized bytes and does not overlap
    // input.
    if !parsed.scheme.is_empty() {
        unsafe {
            std::ptr::copy_nonoverlapping(parsed.scheme.as_ptr(), scheme_out, parsed.scheme.len())
        };
    }
    let (begin, length, has_params) = match parsed.params {
        Some(r) => (r.start, r.len(), 1),
        None => (0, 0, 0),
    };
    Tokenized {
        scheme_len: parsed.scheme.len(),
        params_begin: begin,
        params_len: length,
        has_params,
    }
}

/// # Safety
/// Input must be readable for len bytes, or null when len is zero.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_encoded_len(input: *const u8, len: usize) -> usize {
    auth_challenge_core::encoded_len(unsafe { bytes(input, len) })
}

/// # Safety
/// Both input ranges must be readable for their lengths, or null when empty.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_first(
    scheme: *const u8,
    scheme_len: usize,
    expected: *const u8,
    expected_len: usize,
    encoded_len: usize,
) -> i32 {
    let s = unsafe { bytes(scheme, scheme_len) };
    let e = unsafe { bytes(expected, expected_len) };
    // Only emptiness affects first-round policy.
    match auth_challenge_core::first_round(s, e, if encoded_len == 0 { &[] } else { &[0] }) {
        auth_challenge_core::RoundResult::Accept => 0,
        _ => 3,
    }
}

// Returns 3 for mismatch, 1 for empty token, 4 to continue decoding.
/// # Safety
/// Both input ranges must be readable for their lengths, or null when empty.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_later_decision(
    scheme: *const u8,
    scheme_len: usize,
    expected: *const u8,
    expected_len: usize,
    encoded_len: usize,
) -> i32 {
    let s = unsafe { bytes(scheme, scheme_len) };
    let e = unsafe { bytes(expected, expected_len) };
    auth_challenge_core::later_decision(s, e, encoded_len)
}

// Returns SIZE_MAX on failure, otherwise decoded length. The destination has
// at least `len` initialized bytes and does not overlap the input.
/// # Safety
/// Input must be readable for len bytes, or null when empty. Output must be
/// disjoint and writable for len initialized bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_decode(input: *const u8, len: usize, out: *mut u8) -> usize {
    match auth_challenge_core::decode_strict(unsafe { bytes(input, len) }) {
        None => usize::MAX,
        Some(decoded) => {
            if !decoded.is_empty() {
                unsafe { std::ptr::copy_nonoverlapping(decoded.as_ptr(), out, decoded.len()) };
            }
            decoded.len()
        }
    }
}
