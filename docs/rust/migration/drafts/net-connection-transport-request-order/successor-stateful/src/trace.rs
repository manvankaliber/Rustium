// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Paired with oracle/original_trace.cc. Native jobs are represented only by
// counts here; this is an isolated trace harness, not a Chromium adapter.
mod request_queue;
use request_queue::{Action, Entry, RequestQueue, TakeForPriority};

fn order(queue: &RequestQueue) -> Vec<u64> {
    let mut tokens = vec![0; queue.len()];
    queue.copy_tokens(&mut tokens).unwrap();
    tokens
}
fn insert(
    queue: &mut RequestQueue,
    id: u64,
    priority: u8,
    respect_limits: bool,
    assigned: &mut usize,
    unassigned: &mut usize,
) {
    let result = queue
        .insert(Entry { token: id, priority, respect_limits }, *assigned, *unassigned)
        .unwrap();
    let action = match result.action {
        Action::None => "none".to_string(),
        Action::UnassignedFront => {
            *assigned += 1;
            *unassigned -= 1;
            "front".to_string()
        }
        Action::Donor(token) => format!("donor={token}"),
    };
    println!("insert {id} {action} {:?}", order(queue));
}
fn release(queue: &mut RequestQueue, id: u64, assigned: &mut usize, unassigned: &mut usize) {
    *assigned -= 1;
    *unassigned += 1;
    let target = queue.select_released(*assigned, *unassigned).unwrap();
    let action = match target {
        Some(token) => {
            *assigned += 1;
            *unassigned -= 1;
            format!("target={token}")
        }
        None => "none".to_string(),
    };
    println!("release {id} {action} {:?}", order(queue));
}
fn main() {
    let mut queue = RequestQueue::new(32);
    let (mut assigned, mut unassigned) = (0, 2);
    insert(&mut queue, 1, 2, true, &mut assigned, &mut unassigned);
    insert(&mut queue, 2, 2, true, &mut assigned, &mut unassigned);
    insert(&mut queue, 3, 2, true, &mut assigned, &mut unassigned);
    insert(&mut queue, 4, 4, true, &mut assigned, &mut unassigned);
    assert!(matches!(queue.take_for_priority(4, 2).unwrap(), TakeForPriority::Taken(_)));
    release(&mut queue, 4, &mut assigned, &mut unassigned);
    insert(&mut queue, 4, 2, true, &mut assigned, &mut unassigned);
    assert_eq!(queue.take_for_priority(1, 2), Ok(TakeForPriority::Unchanged));
    println!("same 1 {:?}", order(&queue));
    queue.take(1).unwrap();
    release(&mut queue, 1, &mut assigned, &mut unassigned);
    insert(&mut queue, 5, 5, false, &mut assigned, &mut unassigned);
    insert(&mut queue, 6, 5, false, &mut assigned, &mut unassigned);
    let action = queue.select_hole(6, assigned - 1, unassigned).unwrap();
    assigned -= 1;
    let action_text = match action {
        Action::None => "none".to_string(),
        Action::UnassignedFront => "front".to_string(),
        Action::Donor(token) => format!("donor={token}"),
    };
    println!("job-remove 6 {action_text} {:?}", order(&queue));
    assert_eq!(queue.take_for_priority(6, 5), Ok(TakeForPriority::Unchanged));
    println!("same 6 {:?}", order(&queue));
    let _ = assigned;
}
