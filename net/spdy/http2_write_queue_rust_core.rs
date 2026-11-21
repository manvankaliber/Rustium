// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

// Primitive native storage services. None invoke producers or user callbacks.
// Rust never owns or dereferences a stream; its live identity is queried anew.
pub trait Storage {
    fn size(&self, priority: usize) -> usize;
    fn frame(&self, priority: usize, index: usize) -> u8;
    fn stream(&self, priority: usize, index: usize) -> usize;
    fn stream_id(&self, priority: usize, index: usize) -> u32;
    fn append(&mut self, priority: usize);
    fn erase(&mut self, priority: usize, index: usize);
    fn move_to(&mut self, priority: usize, index: usize, destination: usize);
    fn count(&self) -> i32;
    fn set_count(&mut self, count: i32);
    fn set_removing(&mut self, removing: bool);
}

pub fn capped(frame: u8) -> bool {
    matches!(frame, 3 | 4 | 6 | 7 | 8)
}

pub fn adjust(count: i32, frame: u8, delta: i32) -> i32 {
    if capped(frame) {
        count.wrapping_add(delta)
    } else {
        count
    }
}

pub fn selected(s: &impl Storage, priorities: usize) -> Option<usize> {
    (0..priorities).rev().find(|&priority| s.size(priority) != 0)
}

pub fn empty(s: &impl Storage, priorities: usize) -> bool {
    (0..priorities).all(|priority| s.size(priority) == 0)
}

pub fn enqueue(s: &mut impl Storage, priority: usize, frame: u8) {
    s.append(priority);
    s.set_count(adjust(s.count(), frame, 1));
}

pub fn stream_in_other_priority(
    s: &impl Storage,
    priorities: usize,
    expected: usize,
    stream: usize,
) -> bool {
    (0..priorities).any(|p| p != expected && (0..s.size(p)).any(|i| s.stream(p, i) == stream))
}

pub fn remove(
    s: &mut impl Storage,
    priorities: usize,
    priority: usize,
    stream: usize,
    after: Option<u32>,
) {
    s.set_removing(true);
    let range = if after.is_some() { 0..priorities } else { priority..priority + 1 };
    for p in range {
        let mut i = 0;
        while i < s.size(p) {
            let live = s.stream(p, i);
            let erase = if let Some(last) = after {
                live != 0 && (s.stream_id(p, i) > last || s.stream_id(p, i) == 0)
            } else {
                live == stream
            };
            if erase {
                s.set_count(adjust(s.count(), s.frame(p, i), -1));
                s.erase(p, i);
            } else {
                i += 1;
            }
        }
    }
    // Native producer destruction starts only after this function has returned.
    s.set_removing(false);
}

pub fn change(s: &mut impl Storage, old: usize, new: usize, stream: usize) {
    let mut i = 0;
    while i < s.size(old) {
        if s.stream(old, i) == stream {
            s.move_to(old, i, new);
        } else {
            i += 1;
        }
    }
}

pub fn clear(s: &mut impl Storage, priorities: usize) {
    s.set_removing(true);
    for p in 0..priorities {
        while s.size(p) != 0 {
            s.erase(p, 0);
        }
    }
    s.set_removing(false);
    s.set_count(0);
}
