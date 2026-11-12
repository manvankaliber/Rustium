// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// The core is safe Rust. All slices are call-scoped, and the two writable
// buffers below are initialized and disjoint from every live input range.
mod auth_challenge_core;
use std::slice;
#[repr(C)]
pub struct Tokenized {
    scheme_begin: usize,
    scheme_len: usize,
    params_begin: usize,
    params_len: usize,
    has_params: usize,
}
#[repr(C)]
pub struct Later {
    encoded_len: usize,
    decision: usize,
}
const _: () = assert!(std::mem::size_of::<Tokenized>() == 5 * std::mem::size_of::<usize>());
const _: () = assert!(std::mem::align_of::<Tokenized>() == std::mem::align_of::<usize>());
const _: () = assert!(std::mem::size_of::<Later>() == 2 * std::mem::size_of::<usize>());
// SAFETY: the caller supplies a readable range for the duration of the call.
unsafe fn bytes<'a>(ptr: *const u8, len: usize) -> &'a [u8] {
    if len == 0 {
        &[]
    } else {
        unsafe { slice::from_raw_parts(ptr, len) }
    }
}
// SAFETY: the caller supplies an initialized, exclusively writable range.
unsafe fn bytes_mut<'a>(ptr: *mut u8, len: usize) -> &'a mut [u8] {
    if len == 0 {
        &mut []
    } else {
        unsafe { slice::from_raw_parts_mut(ptr, len) }
    }
}
/// # Safety
/// Input is readable for len bytes; scratch is disjoint and writable for cap
/// initialized bytes. Empty input or scratch may be null.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_tokenize(
    input: *const u8,
    len: usize,
    scratch: *mut u8,
    cap: usize,
) -> Tokenized {
    let input = unsafe { bytes(input, len) };
    let parsed = auth_challenge_core::tokenize(input);
    if parsed.scheme.len() <= cap {
        let out = unsafe { bytes_mut(scratch, parsed.scheme.len()) };
        auth_challenge_core::lowercase_into(&input[parsed.scheme.clone()], out);
    }
    let (begin, length, has_params) = match parsed.params {
        Some(r) => (r.start, r.len(), 1),
        None => (0, 0, 0),
    };
    Tokenized {
        scheme_begin: parsed.scheme.start,
        scheme_len: parsed.scheme.len(),
        params_begin: begin,
        params_len: length,
        has_params,
    }
}
/// # Safety
/// The range must be initialized and exclusively writable for len bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_lower_in_place(input: *mut u8, len: usize) {
    auth_challenge_core::lowercase_in_place(unsafe { bytes_mut(input, len) });
}
/// # Safety
/// Input is readable for len bytes, or null when empty.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_encoded_len(input: *const u8, len: usize) -> usize {
    auth_challenge_core::encoded_len(unsafe { bytes(input, len) })
}
/// # Safety
/// Scheme and expected ranges must be readable; params must be readable when
/// the scheme matches. Empty ranges may be null.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_first(
    scheme: *const u8,
    scheme_len: usize,
    expected: *const u8,
    expected_len: usize,
    params: *const u8,
    params_len: usize,
) -> i32 {
    let s = unsafe { bytes(scheme, scheme_len) };
    let e = unsafe { bytes(expected, expected_len) };
    if !auth_challenge_core::scheme_matches(s, e) {
        return 3;
    }
    auth_challenge_core::first_round(s, e, unsafe { bytes(params, params_len) })
}
/// # Safety
/// Scheme and expected ranges must be readable; params must be readable when
/// the scheme matches. Empty ranges may be null.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_later_decision(
    scheme: *const u8,
    scheme_len: usize,
    expected: *const u8,
    expected_len: usize,
    params: *const u8,
    params_len: usize,
) -> Later {
    let s = unsafe { bytes(scheme, scheme_len) };
    let e = unsafe { bytes(expected, expected_len) };
    if !auth_challenge_core::scheme_matches(s, e) {
        return Later { encoded_len: 0, decision: 3 };
    }
    let (decision, encoded_len) =
        auth_challenge_core::later_decision(s, e, unsafe { bytes(params, params_len) });
    Later { encoded_len, decision: decision as usize }
}
/// # Safety
/// Input is readable for len bytes; out is disjoint and writable for out_len
/// initialized bytes. Empty ranges may be null. Failure may modify out only.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_auth_decode(
    input: *const u8,
    len: usize,
    out: *mut u8,
    out_len: usize,
) -> usize {
    let input = unsafe { bytes(input, len) };
    let out = unsafe { bytes_mut(out, out_len) };
    auth_challenge_core::decode_strict_into(input, out).unwrap_or(usize::MAX)
}
