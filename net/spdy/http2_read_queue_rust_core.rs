// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

use std::cell::RefCell;
use std::collections::VecDeque;
use std::num::NonZeroUsize;
use std::rc::Rc;

struct Entry {
    handle: NonZeroUsize,
    remaining: usize,
}

#[derive(Default)]
pub struct Queue {
    // The oldest entry stays inline. A single-buffer queue needs no deque
    // allocation; further entries retain FIFO order in the tail. An absent
    // first entry always implies an empty tail. Removing the first promotes
    // exactly one tail entry without allocation or moving native buffers.
    first: Option<Entry>,
    tail: VecDeque<Entry>,
    total: usize,
}

#[derive(Default)]
pub struct Chunk {
    pub handle: usize,
    pub bytes: usize,
    pub detached: bool,
}

impl Queue {
    pub fn is_empty(&self) -> bool {
        debug_assert!(self.first.is_some() || self.tail.is_empty());
        debug_assert_eq!(self.first.is_none(), self.total == 0);
        self.first.is_none()
    }

    pub fn total(&self) -> usize {
        self.total
    }

    pub fn enqueue(&mut self, handle: usize, remaining: usize) {
        let handle = NonZeroUsize::new(handle).expect("null buffer handle");
        assert_ne!(remaining, 0);
        // No real collection can own more than usize::MAX bytes. Fail closed
        // rather than wrap accounting if the native contract is violated.
        self.total = self.total.checked_add(remaining).expect("queue size overflow");
        let entry = Entry { handle, remaining };
        if self.first.is_none() {
            debug_assert!(self.tail.is_empty());
            self.first = Some(entry);
        } else {
            self.tail.push_back(entry);
        }
    }

    // All decisions and accounting precede native copying and callbacks. A
    // partial chunk remains at the front; native Consume updates its offset
    // before dispatching callbacks. No callbacks run during plan construction.
    pub fn next(&mut self, limit: usize) -> Chunk {
        let Some(front) = self.first.as_mut() else {
            return Chunk::default();
        };
        if limit == 0 {
            return Chunk::default();
        }
        let bytes = limit.min(front.remaining);
        let chunk = Chunk { handle: front.handle.get(), bytes, detached: bytes == front.remaining };
        front.remaining -= bytes;
        self.total -= bytes;
        if chunk.detached {
            self.first = self.tail.pop_front();
        }
        chunk
    }

    pub fn take(&mut self) -> Self {
        std::mem::take(self)
    }

    pub fn discard(&mut self) -> usize {
        self.first.take().map_or(0, |entry| {
            self.first = self.tail.pop_front();
            self.total -= entry.remaining;
            entry.handle.get()
        })
    }
}

// The native owner and each detached Clear snapshot hold one strong reference.
// This keeps observation cleanup independent of the C++ owner's lifetime.
// RefCell guards exist only within synchronous FFI calls, never callbacks.
pub type StateHandle = RefCell<State>;

#[derive(Default)]
pub struct State {
    queue: Queue,
    observed_total: Option<usize>,
    destroying: bool,
}

impl State {
    pub fn is_drained(&self) -> bool {
        self.queue.is_empty()
    }

    pub fn total(&self) -> usize {
        self.observed_total.unwrap_or(self.queue.total())
    }

    pub fn enqueue(&mut self, handle: usize, remaining: usize) -> bool {
        self.observed_total = None;
        if self.destroying {
            return false;
        }
        self.queue.enqueue(handle, remaining);
        true
    }

    pub fn next(&mut self, limit: usize) -> Chunk {
        self.observed_total = None;
        if self.destroying {
            return Chunk::default();
        }
        self.queue.next(limit)
    }

    fn begin_destroy(&mut self) {
        self.destroying = true;
        self.observed_total = Some(self.queue.total());
    }

    pub fn destroy_next(&mut self) -> usize {
        if !self.destroying {
            self.begin_destroy();
        }
        // Preserve the original pre-destruction total throughout read-only
        // callbacks, although each returned handle leaves Rust ownership.
        self.queue.discard()
    }
}

pub struct Snapshot {
    queue: Queue,
    owner: Rc<StateHandle>,
}

impl Snapshot {
    pub fn take(owner: &Rc<StateHandle>) -> Option<Self> {
        let mut state = owner.borrow_mut();
        // A mutation ends the compatibility observation. Recursive mutation
        // during original deque destruction is unsupported: entries already
        // being destroyed must never be offered to another mutation.
        state.observed_total = None;
        if state.destroying || state.queue.is_empty() {
            return None;
        }
        state.observed_total = Some(state.queue.total());
        Some(Self { queue: state.queue.take(), owner: Rc::clone(owner) })
    }

    pub fn discard(&mut self) -> usize {
        self.queue.discard()
    }
}

impl Drop for Snapshot {
    fn drop(&mut self) {
        assert!(self.queue.is_empty());
        // May be the last reference after a callback deleted the native owner.
        // This touches only Rust-owned storage, never that deleted C++ object.
        self.owner.borrow_mut().observed_total = None;
    }
}

#[cfg(test)]
mod tests {
    use super::Queue;

    fn read(queue: &mut Queue, limit: usize) -> (usize, usize, bool, usize) {
        let chunk = queue.next(limit);
        (chunk.handle, chunk.bytes, chunk.detached, queue.total())
    }

    #[test]
    fn partial_reads_and_discard_preserve_fifo_across_front_promotion() {
        let mut queue = Queue::default();
        queue.enqueue(1, 3);
        queue.enqueue(2, 5);
        queue.enqueue(3, 7);
        assert_eq!(read(&mut queue, 0), (0, 0, false, 15));
        assert_eq!(read(&mut queue, 2), (1, 2, false, 13));
        assert_eq!(read(&mut queue, 4), (1, 1, true, 12));
        assert_eq!(read(&mut queue, 4), (2, 4, false, 8));
        assert_eq!(queue.discard(), 2);
        assert_eq!(read(&mut queue, usize::MAX), (3, 7, true, 0));
        assert!(queue.is_empty());
        assert_eq!(read(&mut queue, 1), (0, 0, false, 0));
        queue.enqueue(4, 2);
        assert_eq!(read(&mut queue, 2), (4, 2, true, 0));
        assert_eq!(queue.discard(), 0);
    }

    #[test]
    fn detached_front_and_tail_are_independent_of_new_live_entries() {
        let mut queue = Queue::default();
        queue.enqueue(1, 3);
        queue.enqueue(2, 5);
        let mut detached = queue.take();
        assert!(queue.is_empty());
        assert_eq!(queue.total(), 0);
        queue.enqueue(3, 7);
        assert_eq!(detached.discard(), 1);
        assert_eq!(detached.total(), 5);
        assert_eq!(detached.discard(), 2);
        assert_eq!(detached.discard(), 0);
        assert!(detached.is_empty());
        assert_eq!(read(&mut queue, 7), (3, 7, true, 0));
    }
}
