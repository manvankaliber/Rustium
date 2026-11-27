// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

pub const MAX_RAW_FRAME_SIZE: usize = 0x00ff_ffff;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u8)]
pub enum CopyAction {
    Copy = 0,
    Empty = 1,
    TooLarge = 2,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ConsumeSource {
    Consume = 0,
    Discard = 1,
}

impl ConsumeSource {
    pub fn from_code(code: u32) -> Option<Self> {
        match code {
            0 => Some(Self::Consume),
            1 => Some(Self::Discard),
            _ => None,
        }
    }
}

pub const fn initial_offset() -> usize {
    0
}

pub fn copy_action(size: usize) -> CopyAction {
    if size == 0 {
        CopyAction::Empty
    } else if size > MAX_RAW_FRAME_SIZE {
        CopyAction::TooLarge
    } else {
        CopyAction::Copy
    }
}

/// A value snapshot, never a borrow of a native buffer or its storage.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Cursor {
    total: usize,
    offset: usize,
}

impl Cursor {
    pub const fn at(total: usize, offset: usize) -> Self {
        Self { total, offset }
    }

    pub fn remaining(self) -> usize {
        // Preserve native size_t arithmetic even in debug-assertion builds.
        self.total.wrapping_sub(self.offset)
    }

    pub fn next_offset(self, consumed: usize) -> usize {
        self.offset.wrapping_add(consumed)
    }

    pub fn discard_size(self) -> Option<usize> {
        let remaining = self.remaining();
        (remaining != 0).then_some(remaining)
    }
}

/// Commit the cursor and capture native callback ownership before invocation.
/// The returned snapshot count is fixed for this dispatch. Callbacks added to
/// the owner or nested consumption do not change the outer dispatch's members.
/// Nothing borrows/revisits the native owner after `prepare` returns.
pub fn consume<P, I>(
    cursor: Cursor,
    amount: usize,
    source: ConsumeSource,
    prepare: P,
    mut invoke: I,
) where
    P: FnOnce(usize) -> usize,
    I: FnMut(usize, usize, ConsumeSource),
{
    let count = prepare(cursor.next_offset(amount));
    for index in 0..count {
        invoke(index, amount, source);
    }
}

/// Rust selects whether destruction needs a discard notification and its size.
/// Native consumption then uses the same commit/snapshot/dispatch path.
pub fn discard<F>(cursor: Cursor, mut consume: F)
where
    F: FnMut(usize, ConsumeSource),
{
    if let Some(amount) = cursor.discard_size() {
        consume(amount, ConsumeSource::Discard);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::{Cell, RefCell};

    #[test]
    fn raw_copy_bounds_do_not_restrict_frame_constructor() {
        assert_eq!(ConsumeSource::from_code(0), Some(ConsumeSource::Consume));
        assert_eq!(ConsumeSource::from_code(1), Some(ConsumeSource::Discard));
        assert_eq!(ConsumeSource::from_code(2), None);
        assert_eq!(ConsumeSource::from_code(u32::MAX), None);
        assert_eq!(copy_action(0), CopyAction::Empty);
        assert_eq!(copy_action(1), CopyAction::Copy);
        assert_eq!(copy_action(MAX_RAW_FRAME_SIZE), CopyAction::Copy);
        assert_eq!(copy_action(MAX_RAW_FRAME_SIZE + 1), CopyAction::TooLarge);
        assert_eq!(copy_action(usize::MAX), CopyAction::TooLarge);
        assert_eq!(Cursor::at(0, initial_offset()).remaining(), 0);
        assert_eq!(Cursor::at(usize::MAX, 0).remaining(), usize::MAX);
    }

    #[test]
    fn cursor_and_native_wrapping_arithmetic() {
        let cursor = Cursor::at(16, 5);
        assert_eq!(cursor.remaining(), 11);
        assert_eq!(cursor.next_offset(4), 9);
        assert_eq!(Cursor::at(0, 1).remaining(), usize::MAX);
        assert_eq!(Cursor::at(usize::MAX, usize::MAX).next_offset(1), 0);
    }

    #[test]
    fn prepare_precedes_every_callback_and_membership_is_fixed() {
        let offset = Cell::new(0);
        let owner_callback_count = Cell::new(2);
        let events = RefCell::new(Vec::new());
        consume(
            Cursor::at(10, 0),
            3,
            ConsumeSource::Consume,
            |next| {
                offset.set(next);
                events.borrow_mut().push((0, next, 0));
                owner_callback_count.get()
            },
            |index, amount, source| {
                assert_eq!(offset.get(), 3);
                assert_eq!(source, ConsumeSource::Consume);
                events.borrow_mut().push((1, index, amount));
                owner_callback_count.set(3);
            },
        );
        assert_eq!(*events.borrow(), vec![(0, 3, 0), (1, 0, 3), (1, 1, 3)]);
    }

    #[test]
    fn discard_notifies_only_nonempty_remaining_bytes() {
        let events = RefCell::new(Vec::new());
        discard(Cursor::at(5, 5), |n, source| events.borrow_mut().push((n, source)));
        assert!(events.borrow().is_empty());
        discard(Cursor::at(5, 2), |n, source| events.borrow_mut().push((n, source)));
        assert_eq!(*events.borrow(), vec![(3, ConsumeSource::Discard)]);
    }

    #[test]
    fn nested_dispatch_keeps_the_outer_value_snapshot() {
        let offset = Cell::new(0);
        let events = RefCell::new(Vec::new());
        consume(
            Cursor::at(10, 0),
            2,
            ConsumeSource::Consume,
            |next| {
                offset.set(next);
                events.borrow_mut().push(0);
                2
            },
            |index, _, _| {
                events.borrow_mut().push(1 + index);
                if index == 0 {
                    consume(
                        Cursor::at(10, offset.get()),
                        3,
                        ConsumeSource::Consume,
                        |next| {
                            offset.set(next);
                            events.borrow_mut().push(3);
                            2
                        },
                        |index, _, _| events.borrow_mut().push(4 + index),
                    );
                }
            },
        );
        assert_eq!(*events.borrow(), vec![0, 1, 3, 4, 5, 2]);
        assert_eq!(offset.get(), 5);
    }
}
