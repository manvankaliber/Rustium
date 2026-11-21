// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Standalone, single-threaded test-only allocation observation. No global
// allocator override is part of the production queue or ABI.
mod retained_queue;
use retained_queue::{Detach, RequestQueue};
use std::alloc::{GlobalAlloc, Layout, System};
use std::sync::atomic::{AtomicUsize, Ordering};

struct Counted;
static ALLOCATIONS: AtomicUsize = AtomicUsize::new(0);
unsafe impl GlobalAlloc for Counted {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        ALLOCATIONS.fetch_add(1, Ordering::Relaxed);
        unsafe { System.alloc(layout) }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        unsafe { System.dealloc(ptr, layout) }
    }
    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, size: usize) -> *mut u8 {
        ALLOCATIONS.fetch_add(1, Ordering::Relaxed);
        unsafe { System.realloc(ptr, layout, size) }
    }
}
#[global_allocator]
static ALLOCATOR: Counted = Counted;

fn main() {
    let mut queue = RequestQueue::new();
    let first = queue.create(2, true, 0, 0).unwrap().token;
    queue.create(2, true, 0, 0).unwrap();
    let before = ALLOCATIONS.load(Ordering::Relaxed);
    assert_eq!(queue.detach_for_priority(first, 4), Ok(Detach::Detached));
    assert!(!queue.is_scheduled(first));
    assert_eq!(queue.finish_priority(first, 0, 0).unwrap().token, first);
    let after = ALLOCATIONS.load(Ordering::Relaxed);
    assert_eq!(before, after, "detach/relink allocated");
}
