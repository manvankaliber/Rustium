// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const NONE: usize = usize::MAX;
const FRONT: usize = usize::MAX - 1;
const INVALID: usize = usize::MAX - 2;

// The queue has exactly one unassigned hole. `assigned` excludes that hole,
// even when the corresponding removed job is still temporarily in jobs_.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_assignment_hole(
    rank: usize,
    request_count: usize,
    assigned: usize,
    unassigned: usize,
) -> usize {
    if request_count >= INVALID || rank >= request_count || assigned >= request_count {
        return INVALID;
    }
    if unassigned != 0 {
        FRONT
    } else if rank < assigned {
        assigned
    } else {
        NONE
    }
}

// Called after the new/released job has been pushed to the native deque.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_assignment_release(
    request_count: usize,
    assigned: usize,
    unassigned: usize,
) -> usize {
    if request_count >= INVALID || assigned > request_count || unassigned == 0 {
        return INVALID;
    }
    if assigned < request_count {
        assigned
    } else {
        NONE
    }
}
