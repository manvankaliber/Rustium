// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const INVALID: usize = usize::MAX;

// Rank occupies the upper bits; the low bit requests front insertion.
// RespectLimits::DISABLED is 0; every other value follows the original back
// insertion branch. The native queue supplies bucket sizes without pointers.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_request_insert_plan(
    respect_limits: i32,
    higher_count: usize,
    same_count: usize,
) -> usize {
    let front = respect_limits == 0;
    let rank = match higher_count.checked_add(if front { 0 } else { same_count }) {
        Some(value) if value < (usize::MAX >> 1) => value,
        _ => return INVALID,
    };
    (rank << 1) | usize::from(front)
}

// 0 retains the existing iterator and FIFO position; 1 removes and reinserts.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_request_reprioritize(current: i32, requested: i32) -> i32 {
    if current == requested {
        0
    } else {
        1
    }
}
