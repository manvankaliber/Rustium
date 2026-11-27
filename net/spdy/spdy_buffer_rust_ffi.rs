// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Stateless scalar bridge. No native memory is borrowed or retained by Rust.
use http2_policy_core::buffer as policy;
use std::ffi::c_void;

#[unsafe(no_mangle)]
pub extern "C" fn spdy_buffer_rust_initial_offset() -> usize {
    policy::initial_offset()
}

#[unsafe(no_mangle)]
pub extern "C" fn spdy_buffer_rust_copy_action(size: usize) -> policy::CopyAction {
    policy::copy_action(size)
}

#[unsafe(no_mangle)]
pub extern "C" fn spdy_buffer_rust_remaining(total: usize, offset: usize) -> usize {
    policy::Cursor::at(total, offset).remaining()
}

/// # Safety
/// `source` is 0 (CONSUME) or 1 (DISCARD). The opaque native stack context stays
/// alive for the complete call. `prepare` commits the next offset, then copies
/// callback ownership to a local snapshot and returns its exact length.
/// `invoke` accepts every index below that length and may reenter or destroy the
/// buffer owner, but must not invalidate the stack snapshot/context. Callbacks
/// must not unwind. Rust never accesses the owner or holds native references.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn spdy_buffer_rust_consume(
    total: usize,
    offset: usize,
    amount: usize,
    source: u32,
    context: *mut c_void,
    prepare: unsafe extern "C" fn(*mut c_void, usize) -> usize,
    invoke: unsafe extern "C" fn(*mut c_void, usize, usize, u32),
) {
    let source = policy::ConsumeSource::from_code(source).expect("invalid consume source");
    policy::consume(
        policy::Cursor::at(total, offset),
        amount,
        source,
        |next| {
            // SAFETY: valid synchronous non-unwinding primitive by contract.
            unsafe { prepare(context, next) }
        },
        |index, amount, source| {
            // SAFETY: index is bounded by the captured native snapshot count.
            unsafe { invoke(context, index, amount, source as u32) }
        },
    );
}

/// # Safety
/// `context` remains valid until the one optional `consume` primitive returns.
/// That primitive accepts the selected amount/source and has the same defined
/// lifetime/reentry requirements as native ConsumeHelper. It must not unwind.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn spdy_buffer_rust_discard(
    total: usize,
    offset: usize,
    context: *mut c_void,
    consume: unsafe extern "C" fn(*mut c_void, usize, u32),
) {
    policy::discard(policy::Cursor::at(total, offset), |amount, source| {
        // SAFETY: scalar decision invokes the valid native primitive at most once.
        unsafe { consume(context, amount, source as u32) }
    });
}
