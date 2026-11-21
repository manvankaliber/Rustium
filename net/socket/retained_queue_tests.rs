// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use super::*;

fn order(q: &RequestQueue) -> Vec<Token> {
    let mut out = vec![0; q.len()];
    assert_eq!(q.copy_tokens(&mut out), Ok(q.len()));
    out
}

#[test]
fn detach_is_invisible_and_relink_retains_box_and_token() {
    let mut q = RequestQueue::new();
    let a = q.create(2, true, 0, 0).unwrap().token;
    let b = q.create(2, true, 0, 0).unwrap().token;
    let node_address = &**q.nodes.get(&a).unwrap() as *const Node;
    assert_eq!(q.detach_for_priority(a, 4), Ok(Detach::Detached));
    assert!(!q.is_scheduled(a));
    assert_eq!(q.front(), Some(b));
    assert_eq!(q.remove(a), Err(Error::Missing));
    assert_eq!(order(&q), [b]);
    // Rehash while detached: the retained Rust box remains the same node.
    for _ in 0..64 {
        q.create(0, true, 0, 0).unwrap();
    }
    assert_eq!(node_address, &**q.nodes.get(&a).unwrap() as *const Node);
    let capacity_before = q.nodes.capacity();
    let relinked = q.finish_priority(a, 0, 0).unwrap();
    assert_eq!(relinked.token, a);
    assert_eq!(relinked.rank, 0);
    assert_eq!(q.nodes.capacity(), capacity_before);
    assert_eq!(node_address, &**q.nodes.get(&a).unwrap() as *const Node);
    assert_eq!(q.front(), Some(a));
}

#[test]
fn original_actions_and_temporary_hole() {
    let mut q = RequestQueue::new();
    let a = q.create(2, true, 0, 2).unwrap();
    assert_eq!(a.action, Action::UnassignedFront);
    let b = q.create(2, true, 1, 1).unwrap();
    assert_eq!(b.action, Action::UnassignedFront);
    let c = q.create(2, true, 2, 0).unwrap();
    assert_eq!(c.action, Action::None);
    let d = q.create(4, true, 2, 0).unwrap();
    assert_eq!(d.action, Action::Donor(b.token));
    assert_eq!(order(&q), [d.token, a.token, b.token, c.token]);
    assert_eq!(q.detach_for_priority(d.token, 2), Ok(Detach::Detached));
    assert_eq!(q.select_released(1, 1), Ok(Some(b.token)));
    assert_eq!(q.finish_priority(d.token, 2, 0).unwrap().action, Action::None);
    assert_eq!(order(&q), [a.token, b.token, c.token, d.token]);
    assert_eq!(q.detach_for_priority(a.token, 2), Ok(Detach::Unchanged));
    assert_eq!(q.select_hole(a.token, 1, 0), Ok(Action::Donor(b.token)));
    q.remove(a.token).unwrap();
    assert_eq!(q.select_released(1, 1), Ok(Some(c.token)));
}

#[test]
fn priority_front_order_and_relink_after_interleaving() {
    let mut q = RequestQueue::new();
    let a = q.create(5, true, 0, 0).unwrap().token;
    let b = q.create(5, false, 0, 0).unwrap().token;
    let c = q.create(5, false, 0, 0).unwrap().token;
    assert_eq!(order(&q), [c, b, a]);
    assert_eq!(q.detach_for_priority(b, 5), Ok(Detach::Unchanged));
    assert_eq!(q.detach_for_priority(b, 1), Ok(Detach::Detached));
    let d = q.create(1, true, 0, 0).unwrap().token;
    assert_eq!(q.finish_priority(b, 0, 0).unwrap().token, b);
    // This preserves release-build front semantics; Group's original debug
    // DCHECK for invalid ignore-limits priority changes stays native.
    assert_eq!(order(&q), [c, a, b, d]);
}

#[test]
fn errors_do_not_publish_a_detached_node() {
    let mut q = RequestQueue::new();
    let a = q.create(1, true, 0, 0).unwrap().token;
    assert_eq!(q.detach_for_priority(a, 2), Ok(Detach::Detached));
    assert!(q.is_empty());
    assert_eq!(q.finish_priority(a, 1, 0), Err(Error::InvalidState));
    assert!(q.is_empty());
    assert!(!q.is_scheduled(a));
    assert_eq!(q.select_hole(a, 0, 0), Err(Error::Missing));
    assert_eq!(q.finish_priority(a, 0, 0).unwrap().token, a);
    assert_eq!(q.detach_for_priority(a, 3), Ok(Detach::Detached));
    assert_eq!(q.discard_detached(a), Ok(()));
    assert_eq!(q.finish_priority(a, 0, 0), Err(Error::Missing));
}

#[test]
fn prefix_snapshot_is_ordered_and_bounded() {
    let mut q = RequestQueue::new();
    let a = q.create(1, true, 0, 0).unwrap().token;
    let b = q.create(2, true, 0, 0).unwrap().token;
    let c = q.create(2, true, 0, 0).unwrap().token;
    let mut prefix = [99; 2];
    assert_eq!(q.copy_prefix(&mut prefix), 2);
    assert_eq!(prefix, [b, c]);
    let mut all = [99; 4];
    assert_eq!(q.copy_prefix(&mut all), 3);
    assert_eq!(all, [b, c, a, 99]);
    assert_eq!(q.copy_prefix(&mut []), 0);
}

#[test]
fn preconnect_claims_are_consumed_capped_and_cleared() {
    let mut q = RequestQueue::new();
    assert!(!q.claim_preconnect());
    assert_eq!(Ok(None), q.release_job(0, 1, true));
    assert_eq!(Ok(None), q.release_job(0, 2, true));
    assert_eq!(Ok(None), q.release_job(0, 3, false));
    assert_eq!(2, q.preconnect_claims());
    q.trim_preconnect_claims(1);
    assert!(q.claim_preconnect());
    assert!(!q.claim_preconnect());
    assert_eq!(Ok(None), q.release_job(0, 1, false));
    assert_eq!(0, q.preconnect_claims());
    assert_eq!(Ok(None), q.release_job(0, 2, true));
    q.trim_preconnect_claims(0);
    assert!(!q.claim_preconnect());
}
