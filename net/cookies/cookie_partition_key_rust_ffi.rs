// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![deny(unsafe_op_in_unsafe_fn)]

use crate::partition as core;

use std::ffi::c_void;
use core::{Primitive, Primitives, Result};

pub type Callback = unsafe extern "C" fn(*mut c_void, u32) -> i32;

struct Native {
    context: *mut c_void,
    callback: Callback,
}

impl Primitives for Native {
    fn run(&mut self, operation: Primitive) -> i32 {
        // SAFETY: callback entrypoints require a live call-scoped context and
        // primitive implementation. Native owns all pointer dereferences and
        // fully initialized scratch. The safe core controls lazy call order.
        unsafe { (self.callback)(self.context, operation as u32) }
    }
}

#[repr(C)]
pub struct Text {
    pub data: *const u8,
    pub length: usize,
}

#[repr(C)]
pub struct StreamSuffixes {
    pub nonce: Text,
    pub ancestor: Text,
}

impl From<&'static str> for Text {
    fn from(value: &'static str) -> Self {
        Self { data: value.as_ptr(), length: value.len() }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn cookie_partition_key_rust_ancestor(cross_site: bool) -> i32 {
    core::ancestor(cross_site)
}

#[unsafe(no_mangle)]
pub extern "C" fn cookie_partition_key_rust_construct_ancestor(value: i32) -> i32 {
    core::construct_ancestor(value)
}

#[unsafe(no_mangle)]
pub extern "C" fn cookie_partition_key_rust_query(
    operation: u32,
    present: bool,
    opaque: bool,
    nonce: bool,
    bit: i32,
) -> bool {
    core::query(operation, present, opaque, nonce, bit)
}

/// # Safety
/// `callback` must implement contract.json's primitive IDs for this operation,
/// own valid call-scoped native context/scratch, and neither throw nor unwind
/// or reenter this context. It may not retain Rust/native borrowed addresses.
/// Rust never dereferences `context`; the callback is synchronous. This
/// contract applies to each unsafe callback entrypoint below.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_partition_key_rust_serialize(
    present: bool,
    nonce: bool,
    bit: i32,
    context: *mut c_void,
    callback: Callback,
) -> Result {
    core::serialize(present, nonce, bit, &mut Native { context, callback })
}

/// # Safety
/// Same call-scoped callback/context contract as `cookie_partition_key_rust_serialize`.
/// Mode must be InternalStrict=0, InternalLoose=1, Storage=2, or Untrusted=3.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_partition_key_rust_parse(
    mode: u32,
    input_empty: bool,
    bit: i32,
    context: *mut c_void,
    callback: Callback,
) -> Result {
    core::parse(mode, input_empty, bit, &mut Native { context, callback })
}

/// # Safety
/// Same call-scoped callback/context contract as `cookie_partition_key_rust_serialize`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_partition_key_rust_from_nik(
    main_frame_navigation: bool,
    context: *mut c_void,
    callback: Callback,
) -> Result {
    core::from_nik(main_frame_navigation, &mut Native { context, callback })
}

/// # Safety
/// Same call-scoped callback/context contract as `cookie_partition_key_rust_serialize`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_partition_key_rust_equals(
    left_bit: i32,
    right_bit: i32,
    context: *mut c_void,
    callback: Callback,
) -> bool {
    core::equals(left_bit, right_bit, &mut Native { context, callback })
}

/// # Safety
/// Same call-scoped callback/context contract as `cookie_partition_key_rust_serialize`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_partition_key_rust_compare(
    left_bit: i32,
    right_bit: i32,
    context: *mut c_void,
    callback: Callback,
) -> i32 {
    core::compare(left_bit, right_bit, &mut Native { context, callback })
}

// Static immutable text is valid for the lifetime of the loaded Rust module.
// C++ copies or streams it immediately and never attempts to free it.
#[unsafe(no_mangle)]
pub extern "C" fn cookie_partition_key_rust_message(error: u32) -> Text {
    core::message(error).into()
}

#[unsafe(no_mangle)]
pub extern "C" fn cookie_partition_key_rust_serialized_suffix(cross_site: bool) -> Text {
    core::serialized_suffix(cross_site).into()
}

#[unsafe(no_mangle)]
pub extern "C" fn cookie_partition_key_rust_stream_suffix(nonce: bool, bit: i32) -> StreamSuffixes {
    let (nonce, ancestor) = core::stream_suffix(nonce, bit);
    StreamSuffixes { nonce: nonce.into(), ancestor: ancestor.into() }
}
