// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Each nibble is an eligible group's native RequestPriority in native GroupId
// map order. `previous` is -1 when no earlier batch has a winner. Returning
// -1 leaves that earlier winner untouched. No pointer crosses this boundary.
#[unsafe(no_mangle)]
pub extern "C" fn rust_transport_stalled_select(
    packed: u64,
    count: u32,
    previous: i32,
) -> i32 {
    if count > 16 {
        return -1;
    }
    let mut best = previous;
    let mut selected = -1;
    for i in 0..count {
        let priority = ((packed >> (i * 4)) & 0xf) as i32;
        if priority > best {
            best = priority;
            selected = i as i32;
        }
    }
    selected
}
