// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// One bit per observed request, in native order. Only the contiguous prefix
// before the first zero is relevant. The caller supplies at most 16 bits.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_assignment_prefix(bits: u32, count: u32) -> i32 {
    if count > 16 {
        return -1;
    }
    let mut i = 0;
    while i < count && bits & (1 << i) != 0 {
        i += 1;
    }
    i as i32
}

// An available unassigned job is always consumed from the native deque front.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_assignment_use_front(has_unassigned: bool) -> bool {
    has_unassigned
}
