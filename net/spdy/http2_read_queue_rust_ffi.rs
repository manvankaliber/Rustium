// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod http2_read_queue_rust_core;
use http2_read_queue_rust_core::{Snapshot, StateHandle};
use std::mem::ManuallyDrop;
use std::rc::Rc;

/// # Safety
/// `queue` is live and not being mutated concurrently.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_read_queue_rust_total(queue: *const StateHandle) -> usize {
    // SAFETY: synchronous read; guard ends before any native callback.
    unsafe { &*queue }.borrow().total()
}

/// # Safety
/// `queue` is valid, aligned native owner storage initialized to null or a live
/// strong reference, with serialized access. It does not overlap Rust state.
/// `handle` identifies a non-null native buffer of `remaining` positive bytes.
/// Ownership transfers ONLY on true. No native callback runs in this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_read_queue_rust_enqueue(
    queue: *mut *const StateHandle,
    handle: usize,
    remaining: usize,
) -> bool {
    // SAFETY: initialized owner storage is readable; no callback can invalidate
    // it while this synchronous call creates and publishes the first state.
    let state = unsafe { queue.read() };
    if state.is_null() {
        let owner: Rc<StateHandle> = Rc::default();
        let accepted = owner.borrow_mut().enqueue(handle, remaining);
        assert!(accepted);
        // SAFETY: publish the one owner reference only after initialization.
        unsafe { queue.write(Rc::into_raw(owner)) };
        true
    } else {
        // SAFETY: no calls into native code; the guard ends on return.
        unsafe { &*state }.borrow_mut().enqueue(handle, remaining)
    }
}

/// # Safety
/// `queue` has serialized access; outputs are distinct, valid, aligned writable
/// objects outside queue storage. They may be uninitialized. The caller
/// finishes the plan and native copies before running callbacks.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_read_queue_rust_next(
    queue: *const StateHandle,
    limit: usize,
    bytes: *mut usize,
    detached: *mut u8,
) -> usize {
    // SAFETY: no reference/RefCell guard spans a native callback.
    let chunk = unsafe { &*queue }.borrow_mut().next(limit);
    // SAFETY: raw writes initialize foreign outputs without reading them.
    unsafe {
        bytes.write(chunk.bytes);
        detached.write(u8::from(chunk.detached));
    }
    chunk.handle
}

/// # Safety
/// `queue` owns a live strong reference with serialized access.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_read_queue_rust_take(queue: *const StateHandle) -> *mut Snapshot {
    // SAFETY: temporarily view the owner's Rc without releasing its reference.
    // Snapshot::take clones it only for a nonempty snapshot. No native code
    // runs.
    let owner = ManuallyDrop::new(unsafe { Rc::from_raw(queue) });
    Snapshot::take(&owner)
        .map_or(std::ptr::null_mut(), |snapshot| Box::into_raw(Box::new(snapshot)))
}

/// # Safety
/// `snapshot` is live with serialized access. Native code reclaims each
/// returned buffer exactly once; callbacks follow this return.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_read_queue_rust_discard(snapshot: *mut Snapshot) -> usize {
    // SAFETY: the borrow ends before the returned native buffer is destroyed.
    unsafe { &mut *snapshot }.discard()
}

/// # Safety
/// `snapshot` is a live unique Box returned by take, drained and freed once.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_read_queue_rust_free_snapshot(snapshot: *mut Snapshot) {
    // SAFETY: its own Rc makes observation cleanup safe after owner deletion.
    drop(unsafe { Box::from_raw(snapshot) });
}

/// # Safety
/// `queue` owns one live strong reference with serialized access. Native code
/// must reclaim each returned buffer exactly once; callbacks follow the return.
/// The first call establishes the destructor observation. A zero result drains
/// and RELEASES the owner's reference; the pointer must not be used afterward.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_read_queue_rust_destroy_next(queue: *const StateHandle) -> usize {
    // SAFETY: no guard spans either native callbacks or the owner's Rc release.
    let handle = unsafe { &*queue }.borrow_mut().destroy_next();
    if handle == 0 {
        // SAFETY: exactly the native owner's reference is returned once, only
        // after the FIFO has drained. Detached snapshots retain their own Rc.
        let owner = unsafe { Rc::from_raw(queue) };
        assert!(owner.borrow().is_drained());
    }
    handle
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lazy_owner_initialization_and_final_release() {
        let mut state = std::ptr::null();
        // SAFETY: initialized, exclusive owner storage; integer buffer handles
        // are never dereferenced by Rust and every returned handle is
        // reclaimed.
        unsafe {
            assert!(http2_read_queue_rust_enqueue(&mut state, 1, 7));
            assert!(!state.is_null());
            let owner = ManuallyDrop::new(Rc::from_raw(state));
            let weak = Rc::downgrade(&owner);
            assert!(http2_read_queue_rust_enqueue(&mut state, 2, 5));
            assert_eq!(http2_read_queue_rust_destroy_next(state), 1);
            assert_eq!(http2_read_queue_rust_total(state), 12);
            assert_eq!(http2_read_queue_rust_destroy_next(state), 2);
            assert_eq!(http2_read_queue_rust_total(state), 12);
            assert_eq!(http2_read_queue_rust_destroy_next(state), 0);
            // The zero return released the sole native owner reference.
            assert!(weak.upgrade().is_none());
        }
    }

    #[test]
    fn snapshot_retains_state_after_owner_release() {
        let mut state = std::ptr::null();
        // SAFETY: exclusive live owner/snapshot pointers with no callbacks.
        // Returned integer handles model buffers reclaimed by the native side.
        unsafe {
            assert!(http2_read_queue_rust_enqueue(&mut state, 1, 7));
            let owner = ManuallyDrop::new(Rc::from_raw(state));
            let weak = Rc::downgrade(&owner);
            let snapshot = http2_read_queue_rust_take(state);
            assert!(!snapshot.is_null());
            assert_eq!(http2_read_queue_rust_destroy_next(state), 0);
            assert!(weak.upgrade().is_some());
            assert_eq!(http2_read_queue_rust_discard(snapshot), 1);
            assert_eq!(http2_read_queue_rust_discard(snapshot), 0);
            http2_read_queue_rust_free_snapshot(snapshot);
            assert!(weak.upgrade().is_none());
        }
    }
}
