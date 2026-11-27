// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Private C ABI for the transport pool. Every non-null queue
//! pointer is a uniquely owned Rust allocation. Native callers must serialize
//! access, pass valid non-overlapping output storage, and never call after free.
//! No function calls native code while its Rust reference is alive. Panics must
//! abort; an adapter must terminate on allocation error, never return it as a
//! socket result. No native Request or ConnectJob address is passed here.

#[path = "retained_queue.rs"]
mod retained_queue;
use retained_queue::{Action, Detach, Error, Inserted, RankAction, RequestQueue};

const OK: i32 = 0;
const MISSING: i32 = 1;
const INVALID: i32 = 2;
const ALLOCATION: i32 = 3;
const EXHAUSTED: i32 = 4;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Reply {
    pub token: u64,
    pub rank: usize,
    pub donor: u64,
    pub action: u32,
}

fn status(error: Error) -> i32 {
    match error {
        Error::Missing => MISSING,
        Error::InvalidPriority | Error::InvalidState => INVALID,
        Error::Allocation => ALLOCATION,
        Error::TokenExhausted => EXHAUSTED,
    }
}
fn reply(token: u64, rank: usize, action: Action) -> Reply {
    let (action, donor) = match action {
        Action::None => (0, 0),
        Action::UnassignedFront => (1, 0),
        Action::Donor(token) => (2, token),
    };
    Reply { token, rank, donor, action }
}
fn inserted(value: Inserted) -> Reply {
    reply(value.token, value.rank, value.action)
}

// Scalar views of the production rank decisions for the frozen C++ oracle.
// The component wrappers keep these private Rust symbols inside net.
const ASSIGNMENT_NONE: usize = usize::MAX;
const ASSIGNMENT_FRONT: usize = usize::MAX - 1;
const ASSIGNMENT_INVALID: usize = usize::MAX - 2;

#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_assignment_hole(
    rank: usize,
    request_count: usize,
    assigned: usize,
    unassigned: usize,
) -> usize {
    if request_count >= ASSIGNMENT_INVALID || rank >= request_count || assigned >= request_count {
        return ASSIGNMENT_INVALID;
    }
    match retained_queue::hole_action(rank, assigned, unassigned) {
        RankAction::None => ASSIGNMENT_NONE,
        RankAction::UnassignedFront => ASSIGNMENT_FRONT,
        RankAction::Donor(rank) => rank,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_assignment_release(
    request_count: usize,
    assigned: usize,
    unassigned: usize,
) -> usize {
    if request_count >= ASSIGNMENT_INVALID || assigned > request_count || unassigned == 0 {
        return ASSIGNMENT_INVALID;
    }
    retained_queue::released_job_rank(request_count, assigned).unwrap_or(ASSIGNMENT_NONE)
}

// Testable C ABI layout metadata; unknown selectors return usize::MAX.
#[unsafe(no_mangle)]
pub extern "C" fn rust_request_queue_reply_layout(field: u32) -> usize {
    match field {
        0 => std::mem::size_of::<Reply>(),
        1 => std::mem::align_of::<Reply>(),
        2 => std::mem::offset_of!(Reply, token),
        3 => std::mem::offset_of!(Reply, rank),
        4 => std::mem::offset_of!(Reply, donor),
        5 => std::mem::offset_of!(Reply, action),
        _ => usize::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_request_queue_new() -> *mut RequestQueue {
    Box::into_raw(Box::new(RequestQueue::new()))
}

/// # Safety
/// `queue` is null or a live, uniquely owned pointer returned by new; a live
/// pointer is consumed exactly once and must have no outstanding references.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_free(queue: *mut RequestQueue) {
    if !queue.is_null() {
        drop(unsafe { Box::from_raw(queue) });
    }
}

/// # Safety
/// Queue and output satisfy the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_create(
    queue: *mut RequestQueue,
    priority: u8,
    respect: u8,
    assigned: usize,
    unassigned: usize,
    out: *mut Reply,
) -> i32 {
    if out.is_null() || respect > 1 {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_mut() }) else {
        return INVALID;
    };
    match queue.create(priority, respect != 0, assigned, unassigned) {
        Ok(value) => {
            unsafe {
                out.write(inserted(value));
            }
            OK
        }
        Err(error) => status(error),
    }
}

/// # Safety
/// Queue and output satisfy the module-level contract. On success `out` is
/// 0 for no-op, 1 for detached. The retained box is never exposed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_detach(
    queue: *mut RequestQueue,
    token: u64,
    priority: u8,
    out: *mut u32,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_mut() }) else {
        return INVALID;
    };
    match queue.detach_for_priority(token, priority) {
        Ok(value) => {
            unsafe {
                out.write(u32::from(value == Detach::Detached));
            }
            OK
        }
        Err(error) => status(error),
    }
}

/// # Safety
/// Queue and output satisfy the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_finish(
    queue: *mut RequestQueue,
    token: u64,
    assigned: usize,
    unassigned: usize,
    out: *mut Reply,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_mut() }) else {
        return INVALID;
    };
    match queue.finish_priority(token, assigned, unassigned) {
        Ok(value) => {
            unsafe {
                out.write(inserted(value));
            }
            OK
        }
        Err(error) => status(error),
    }
}

/// # Safety
/// Queue satisfies the module-level contract. `detached` is 0 for normal
/// removal and 1 only for explicitly reviewed retained-node teardown.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_remove(
    queue: *mut RequestQueue,
    token: u64,
    detached: u8,
) -> i32 {
    if detached > 1 {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_mut() }) else {
        return INVALID;
    };
    let result = if detached == 0 { queue.remove(token) } else { queue.discard_detached(token) };
    result.map_or_else(status, |_| OK)
}

/// # Safety
/// Queue and output satisfy the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_hole(
    queue: *const RequestQueue,
    token: u64,
    assigned: usize,
    unassigned: usize,
    out: *mut Reply,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_ref() }) else {
        return INVALID;
    };
    match queue.select_hole(token, assigned, unassigned) {
        Ok(action) => {
            unsafe {
                out.write(reply(token, 0, action));
            }
            OK
        }
        Err(error) => status(error),
    }
}

/// # Safety
/// Queue and output satisfy the module-level contract. `out` is zero when no
/// request is selected; generated request tokens are always nonzero.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_release(
    queue: *mut RequestQueue,
    assigned: usize,
    unassigned: usize,
    added_preconnect: u8,
    out: *mut u64,
) -> i32 {
    if out.is_null() || added_preconnect > 1 {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_mut() }) else {
        return INVALID;
    };
    match queue.release_job(assigned, unassigned, added_preconnect != 0) {
        Ok(token) => {
            unsafe {
                out.write(token.unwrap_or(0));
            }
            OK
        }
        Err(error) => status(error),
    }
}

/// # Safety
/// Queue and output satisfy the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_preconnect_claims(
    queue: *const RequestQueue,
    out: *mut usize,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_ref() }) else {
        return INVALID;
    };
    unsafe {
        out.write(queue.preconnect_claims());
    }
    OK
}

/// # Safety
/// Queue and output satisfy the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_claim_preconnect(
    queue: *mut RequestQueue,
    out: *mut u8,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_mut() }) else {
        return INVALID;
    };
    unsafe {
        out.write(u8::from(queue.claim_preconnect()));
    }
    OK
}

/// # Safety
/// Queue satisfies the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_trim_preconnect_claims(
    queue: *mut RequestQueue,
    job_count: usize,
) -> i32 {
    let Some(queue) = (unsafe { queue.as_mut() }) else {
        return INVALID;
    };
    queue.trim_preconnect_claims(job_count);
    OK
}

/// # Safety
/// Queue and output satisfy the module-level contract. Returns zero token for
/// an empty queue or a rank beyond its end.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_at(
    queue: *const RequestQueue,
    rank: usize,
    out: *mut u64,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_ref() }) else {
        return INVALID;
    };
    unsafe {
        out.write(queue.token_at(rank).unwrap_or(0));
    }
    OK
}

/// # Safety
/// Queue and output satisfy the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_priority(
    queue: *const RequestQueue,
    token: u64,
    out: *mut u8,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_ref() }) else {
        return INVALID;
    };
    match queue.scheduled_priority(token) {
        Ok(priority) => {
            unsafe {
                out.write(priority);
            }
            OK
        }
        Err(error) => status(error),
    }
}

/// # Safety
/// Queue and output satisfy the module-level contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_len(
    queue: *const RequestQueue,
    out: *mut usize,
) -> i32 {
    if out.is_null() {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_ref() }) else {
        return INVALID;
    };
    unsafe {
        out.write(queue.len());
    }
    OK
}

/// # Safety
/// Queue is live; `out` is aligned, initialized writable storage for all
/// `capacity` u64 values (or null only when capacity is zero). `written` is
/// separately valid and non-overlapping. No output aliases any queue allocation.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_snapshot(
    queue: *const RequestQueue,
    out: *mut u64,
    capacity: usize,
    written: *mut usize,
) -> i32 {
    if capacity > (isize::MAX as usize) / std::mem::size_of::<u64>() {
        return INVALID;
    }
    if written.is_null() || (out.is_null() && capacity != 0) {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_ref() }) else {
        return INVALID;
    };
    let buffer = if capacity == 0 {
        &mut []
    } else {
        unsafe { std::slice::from_raw_parts_mut(out, capacity) }
    };
    match queue.copy_tokens(buffer) {
        Ok(count) => {
            unsafe {
                written.write(count);
            }
            OK
        }
        Err(error) => status(error),
    }
}

/// # Safety
/// `out` is aligned, initialized, writable and nonaliased for all `capacity`
/// u64 values (or null at zero capacity); `written` is separately valid.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_request_queue_snapshot_prefix(
    queue: *const RequestQueue,
    out: *mut u64,
    capacity: usize,
    written: *mut usize,
) -> i32 {
    if capacity > (isize::MAX as usize) / std::mem::size_of::<u64>() {
        return INVALID;
    }
    if written.is_null() || (out.is_null() && capacity != 0) {
        return INVALID;
    }
    let Some(queue) = (unsafe { queue.as_ref() }) else {
        return INVALID;
    };
    let buffer = if capacity == 0 {
        &mut []
    } else {
        unsafe { std::slice::from_raw_parts_mut(out, capacity) }
    };
    let count = queue.copy_prefix(buffer);
    unsafe {
        written.write(count);
    }
    OK
}
