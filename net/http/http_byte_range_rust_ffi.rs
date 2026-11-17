// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Value-only ABI: no pointers, borrowed memory, callbacks or unsafe code.
#![deny(unsafe_op_in_unsafe_fn)]
pub mod http_byte_range_rust_core;
use http_byte_range_rust_core::{Header, State};
#[repr(C)]
pub struct Bounds {
    pub state: State,
    pub success: bool,
}
#[unsafe(no_mangle)]
pub extern "C" fn http_byte_range_rust_construct(kind: u32, a: i64, b: i64) -> State {
    match kind {
        1 => State::bounded(a, b),
        2 => State::right_unbounded(a),
        3 => State::suffix(a),
        _ => State::new(),
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn http_byte_range_rust_is_suffix(suffix: i64) -> bool {
    State::suffix(suffix).is_suffix()
}
#[unsafe(no_mangle)]
pub extern "C" fn http_byte_range_rust_has_first(first: i64) -> bool {
    State::right_unbounded(first).has_first()
}
#[unsafe(no_mangle)]
pub extern "C" fn http_byte_range_rust_has_last(last: i64) -> bool {
    State::bounded(-1, last).has_last()
}
#[unsafe(no_mangle)]
pub extern "C" fn http_byte_range_rust_is_valid(first: i64, last: i64, suffix: i64) -> bool {
    State { first, last, suffix, computed: false }.is_valid()
}
#[unsafe(no_mangle)]
pub extern "C" fn http_byte_range_rust_compute(mut state: State, size: i64) -> Bounds {
    let success = state.compute_bounds(size);
    Bounds { state, success }
}
#[unsafe(no_mangle)]
pub extern "C" fn http_byte_range_rust_header(first: i64, last: i64, suffix: i64) -> Header {
    State { first, last, suffix, computed: false }.header()
}
