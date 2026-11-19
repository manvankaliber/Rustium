// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The integer states form a small synchronous protocol with the native owner.
// Native code performs exactly the observation requested, then passes its value
// back. No pointer, socket, iterator, callback, or clock crosses this boundary.
// Usability: 0=used observation, 1=connected-and-idle, 2=connected for
// unused, 4=connected for used; 3=usable, 5=remote closed, 6=unexpected data.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_idle_usable_step(state: i32, value: bool) -> i32 {
    match (state, value) {
        (0, true) => 1,
        (0, false) => 2,
        (1, true) => 3,
        (1, false) => 4,
        (2, true) => 3,
        (2, false) | (4, false) => 5,
        (4, true) => 6,
        _ => -1,
    }
}

// Release: 0=connected-and-idle, 1=connected, 2=generation match;
// 10=reuse, 11=returned closed, 12=unexpected data, 13=stale generation.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_idle_release_step(state: i32, value: bool) -> i32 {
    match (state, value) {
        (0, false) => 1,
        (0, true) => 2,
        (1, false) => 11,
        (1, true) => 12,
        (2, true) => 10,
        (2, false) => 13,
        _ => -1,
    }
}

// TimeDelta's pinned InMicroseconds() is its exact signed internal value,
// including Min/Max sentinels; native code still performs TimeTicks subtraction.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_idle_expired(elapsed: i64, timeout: i64) -> bool {
    elapsed >= timeout
}

// 0=keep, 1=forced flush, 2=expired, 3=remote closed, 4=unexpected data.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_idle_cleanup(force: bool, expired: bool, usability: i32) -> i32 {
    match usability {
        5 => 3,
        6 => 4,
        3 if expired => 2,
        3 if force => 1,
        3 => 0,
        _ => -1,
    }
}

// Selection: phase 0 consumes usability and asks for the used observation
// (1) or discards the socket (2); phase 1 consumes used and replaces (3) or
// keeps (4) the candidate; phase 2 asks whether the list is nonempty (5)
// only if there is no candidate; phase 3 picks the oldest (6) or keeps (4).
// List iteration and destruction remain synchronous with the native owner.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_idle_selection_step(phase: i32, value: bool) -> i32 {
    match (phase, value) {
        (0, true) => 1,
        (0, false) => 2,
        (1, true) => 3,
        (1, false) => 4,
        (2, false) => 5,
        (2, true) => 4,
        (3, true) => 6,
        (3, false) => 4,
        _ => -1,
    }
}
