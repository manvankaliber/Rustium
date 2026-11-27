// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod http2_priority_rust_core;
use http2_priority_rust_core::{State, Update};

const _: () = assert!(std::mem::size_of::<Update>() == 16);
const _: () = assert!(std::mem::offset_of!(Update, weight) == 8);
const _: () = assert!(std::mem::offset_of!(Update, has_parent) == 13);

#[unsafe(no_mangle)]
pub extern "C" fn http2_priority_rust_new() -> *mut State {
    Box::into_raw(Box::default())
}

/// # Safety
/// `state` is a live exclusively owned handle from new/clone, consumed once.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_priority_rust_delete(state: *mut State) {
    // SAFETY: native owner transfers its unique handle back exactly once.
    unsafe { drop(Box::from_raw(state)) };
}

/// # Safety
/// `state` is live and not concurrently mutated; the new handle is independent.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_priority_rust_clone(state: *const State) -> *mut State {
    // SAFETY: no callbacks, foreign aliases, or concurrent mutations exist.
    Box::into_raw(Box::new(unsafe { &*state }.clone()))
}

/// # Safety
/// Unique live state; output points to writable (possibly uninitialized) Update
/// storage on success. On duplicate ID it may be null and is never accessed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_priority_rust_create(
    state: *mut State,
    id: u32,
    priority: u8,
    output: *mut Update,
) -> u8 {
    // SAFETY: the native owner grants this synchronous exclusive borrow. There
    // are no native callbacks and no borrowed reference escapes the call.
    let result = unsafe { &mut *state }.create(id, priority);
    if let Some(result) = result {
        // SAFETY: raw write initializes foreign storage without reading it.
        unsafe { output.write(result) };
        1
    } else {
        0
    }
}

/// # Safety
/// `state` is a unique live handle for the duration of this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_priority_rust_destroy(state: *mut State, id: u32) {
    // SAFETY: exclusive synchronous borrow; no callbacks and no references
    // escape.
    unsafe { &mut *state }.destroy(id);
}

/// # Safety
/// Unique live state; output has space for two Updates (may be uninitialized).
/// Only the returned number of entries is initialized, with raw writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_priority_rust_update(
    state: *mut State,
    id: u32,
    priority: u8,
    output: *mut Update,
) -> usize {
    // SAFETY: same exclusive handle contract, no native calls inside the
    // borrow.
    let (result, count) = unsafe { &mut *state }.update(id, priority);
    for (i, entry) in result.into_iter().take(count).enumerate() {
        // SAFETY: i < count <= 2, output points to two writable slots. Never
        // construct a slice/reference over potentially uninitialized storage.
        unsafe { output.add(i).write(entry) };
    }
    count
}
