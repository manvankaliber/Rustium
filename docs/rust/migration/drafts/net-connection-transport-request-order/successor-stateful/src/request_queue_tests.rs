// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use super::*;

fn entry(token: Token, priority: u8, respect_limits: bool) -> Entry {
    Entry { token, priority, respect_limits }
}
fn order(queue: &RequestQueue) -> Vec<Token> {
    let mut tokens = vec![0; queue.len()];
    assert_eq!(queue.copy_tokens(&mut tokens), Ok(queue.len()));
    tokens
}

#[test]
fn preconnect_donor_detach_and_release_sequence() {
    let mut q = RequestQueue::new(32);
    assert_eq!(
        q.insert(entry(1, 2, true), 0, 2).unwrap(),
        Inserted { rank: 0, action: Action::UnassignedFront }
    );
    assert_eq!(
        q.insert(entry(2, 2, true), 1, 1).unwrap(),
        Inserted { rank: 1, action: Action::UnassignedFront }
    );
    assert_eq!(q.insert(entry(3, 2, true), 2, 0).unwrap().action, Action::None);
    assert_eq!(
        q.insert(entry(4, 4, true), 2, 0).unwrap(),
        Inserted { rank: 0, action: Action::Donor(2) }
    );
    assert_eq!(order(&q), [4, 1, 2, 3]);
    assert_eq!(q.take_for_priority(4, 2).unwrap(), TakeForPriority::Taken(entry(4, 4, true)));
    assert_eq!(q.select_released(1, 1), Ok(Some(2)));
    assert_eq!(q.insert(entry(4, 2, true), 2, 0).unwrap().action, Action::None);
    assert_eq!(order(&q), [1, 2, 3, 4]);
    assert_eq!(q.take_for_priority(1, 2), Ok(TakeForPriority::Unchanged));
    assert_eq!(q.select_hole(1, 1, 0), Ok(Action::Donor(2)));
    assert_eq!(q.take(1), Ok(entry(1, 2, true)));
    assert_eq!(q.select_released(1, 1), Ok(Some(3)));
    assert_eq!(order(&q), [2, 3, 4]);
}

#[test]
fn front_order_noop_and_failed_operations_preserve_state() {
    let mut q = RequestQueue::new(4);
    q.insert(entry(1, 5, true), 0, 0).unwrap();
    q.insert(entry(2, 5, false), 0, 0).unwrap();
    q.insert(entry(3, 5, false), 0, 0).unwrap();
    assert_eq!(order(&q), [3, 2, 1]);
    assert_eq!(q.take_for_priority(2, 5), Ok(TakeForPriority::Unchanged));
    assert_eq!(q.take_for_priority(2, 4), Err(Error::InvalidPriority));
    assert_eq!(q.insert(entry(1, 3, true), 0, 0), Err(Error::Duplicate));
    assert_eq!(q.insert(entry(4, 4, false), 0, 0), Err(Error::InvalidPriority));
    assert_eq!(q.insert(entry(4, 1, true), 0, 1), Err(Error::InvalidState));
    let mut short = [99; 2];
    assert_eq!(q.copy_tokens(&mut short), Err(Error::Capacity));
    assert_eq!(short, [99; 2]);
    assert_eq!(order(&q), [3, 2, 1]);
    q.insert(entry(4, 0, true), 0, 0).unwrap();
    assert_eq!(q.insert(entry(5, 0, true), 0, 0), Err(Error::Capacity));
    assert_eq!(order(&q), [3, 2, 1, 4]);
}

#[test]
fn hole_and_release_bounds() {
    let mut q = RequestQueue::new(8);
    assert_eq!(q.select_released(0, 1), Ok(None));
    q.insert(entry(1, 1, true), 0, 0).unwrap();
    q.insert(entry(2, 1, true), 0, 0).unwrap();
    assert_eq!(q.select_hole(1, 1, 0), Ok(Action::Donor(2)));
    assert_eq!(q.select_hole(2, 1, 0), Ok(Action::None));
    assert_eq!(q.select_hole(1, 1, 2), Ok(Action::UnassignedFront));
    assert_eq!(q.select_hole(2, 2, 0), Err(Error::InvalidState));
    assert_eq!(q.select_released(0, 2), Err(Error::InvalidState));
    assert_eq!(q.select_released(2, 1), Ok(None));
}
