// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

use std::collections::BTreeMap;

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(C)]
pub struct Update {
    pub id: u32,
    pub parent: u32,
    pub weight: i32,
    pub exclusive: u8,
    pub has_parent: u8,
}

// Stable arena indices keep each band's insertion order while the map provides
// logarithmic ID lookup. Removed slots are reused, never exposed across FFI.
#[derive(Clone, Copy, Default)]
struct Node {
    id: u32,
    priority: u8,
    prev: Option<usize>,
    next: Option<usize>,
}

#[derive(Clone, Default)]
pub struct State {
    by_id: BTreeMap<u32, usize>,
    nodes: Vec<Node>,
    free: Option<usize>,
    heads: [Option<usize>; 8],
    tails: [Option<usize>; 8],
}

// Same float32 steps and truncation as the frozen QUICHE policy. Only 0..7
// reaches this function; invalid array indexing was undefined in the original.
fn weight(priority: u8) -> i32 {
    ((255.9_f32 / 7.0) * (7.0 - f32::from(priority))) as i32 + 1
}

impl State {
    fn lower_bound(&self, priority: u8) -> Option<usize> {
        (0..=usize::from(priority)).rev().find_map(|p| self.tails[p])
    }

    fn parent(&self, index: usize) -> Option<usize> {
        let node = self.nodes[index];
        node.prev.or_else(|| node.priority.checked_sub(1).and_then(|p| self.lower_bound(p)))
    }

    fn child(&self, index: usize) -> Option<usize> {
        let node = self.nodes[index];
        node.next.or_else(|| (usize::from(node.priority) + 1..8).find_map(|p| self.heads[p]))
    }

    fn append(&mut self, index: usize, id: u32, priority: u8) {
        let p = usize::from(priority);
        let prev = self.tails[p];
        self.nodes[index] = Node { id, priority, prev, next: None };
        if let Some(prev) = prev {
            self.nodes[prev].next = Some(index);
        } else {
            self.heads[p] = Some(index);
        }
        self.tails[p] = Some(index);
    }

    fn unlink(&mut self, index: usize) {
        let node = self.nodes[index];
        let p = usize::from(node.priority);
        if let Some(prev) = node.prev {
            self.nodes[prev].next = node.next;
        } else {
            self.heads[p] = node.next;
        }
        if let Some(next) = node.next {
            self.nodes[next].prev = node.prev;
        } else {
            self.tails[p] = node.prev;
        }
    }

    fn update_for(&self, id: u32, parent: Option<usize>, priority: u8) -> Update {
        Update {
            id,
            parent: parent.map_or(0, |i| self.nodes[i].id),
            weight: weight(priority),
            exclusive: 1,
            has_parent: u8::from(parent.is_some()),
        }
    }

    pub fn create(&mut self, id: u32, priority: u8) -> Option<Update> {
        if self.by_id.contains_key(&id) {
            return None;
        }
        assert!(priority < 8, "priority outside original defined domain");
        let result = self.update_for(id, self.lower_bound(priority), priority);
        let index = if let Some(index) = self.free {
            self.free = self.nodes[index].next;
            index
        } else {
            self.nodes.push(Node::default());
            self.nodes.len() - 1
        };
        self.append(index, id, priority);
        self.by_id.insert(id, index);
        Some(result)
    }

    pub fn destroy(&mut self, id: u32) {
        if let Some(index) = self.by_id.remove(&id) {
            self.unlink(index);
            self.nodes[index].next = self.free;
            self.free = Some(index);
        }
    }

    // A fixed result avoids allocation in Rust; the native caller owns its
    // public vector. A priority change can return zero, one, or two updates.
    pub fn update(&mut self, id: u32, priority: u8) -> ([Update; 2], usize) {
        let mut result = [Update::default(); 2];
        let Some(&index) = self.by_id.get(&id) else {
            return (result, 0);
        };
        let old_priority = self.nodes[index].priority;
        if old_priority == priority {
            return (result, 0);
        }
        assert!(priority < 8, "priority outside original defined domain");
        let old_parent = self.parent(index);
        let mut new_parent = self.lower_bound(priority);
        if new_parent == Some(index) {
            new_parent = old_parent;
        }
        let mut count = 0;
        if old_parent != new_parent {
            if let Some(child) = self.child(index) {
                let node = self.nodes[child];
                result[count] = self.update_for(node.id, old_parent, node.priority);
                count += 1;
            }
            result[count] = self.update_for(id, new_parent, priority);
            count += 1;
        }
        self.unlink(index);
        self.append(index, id, priority);
        (result, count)
    }
}
