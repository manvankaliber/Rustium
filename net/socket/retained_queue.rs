// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Authoritative request-order kernel. No Chromium ABI is implemented here.
//! Only Rust-generated scalar tokens identify requests; no request pointer is
//! stored. A boxed Rust node stays allocated while detached and while the map
//! rehashes. No node borrow is held across a call to native code.

use std::collections::HashMap;

pub type Token = u64;
const PRIORITIES: usize = 6;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Action {
    None,
    UnassignedFront,
    Donor(Token),
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Inserted {
    pub token: Token,
    pub rank: usize,
    pub action: Action,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Detach {
    Unchanged,
    Detached,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    InvalidPriority,
    InvalidState,
    Missing,
    TokenExhausted,
    Allocation,
}

struct Node {
    priority: u8,
    respect_limits: bool,
    prev: Option<Token>,
    next: Option<Token>,
    // None means scheduled; Some holds the requested priority during detach.
    pending: Option<u8>,
}

pub struct RequestQueue {
    nodes: HashMap<Token, Box<Node>>,
    heads: [Option<Token>; PRIORITIES],
    tails: [Option<Token>; PRIORITIES],
    sizes: [usize; PRIORITIES],
    scheduled: usize,
    preconnect_claims: usize,
    next_token: Token,
}

impl RequestQueue {
    pub fn new() -> Self {
        Self {
            nodes: HashMap::new(),
            heads: [None; PRIORITIES],
            tails: [None; PRIORITIES],
            sizes: [0; PRIORITIES],
            scheduled: 0,
            preconnect_claims: 0,
            next_token: 1,
        }
    }
    pub fn len(&self) -> usize {
        self.scheduled
    }
    #[cfg(test)]
    pub fn is_empty(&self) -> bool {
        self.scheduled == 0
    }
    #[cfg(test)]
    pub fn is_scheduled(&self, token: Token) -> bool {
        self.nodes.get(&token).is_some_and(|node| node.pending.is_none())
    }
    pub fn scheduled_priority(&self, token: Token) -> Result<u8, Error> {
        let node = self.nodes.get(&token).ok_or(Error::Missing)?;
        if node.pending.is_some() {
            return Err(Error::Missing);
        }
        Ok(node.priority)
    }

    #[cfg(test)]
    pub fn front(&self) -> Option<Token> {
        (0..PRIORITIES).rev().find_map(|priority| self.heads[priority])
    }

    // No native pointer is returned. The caller owns the output buffer, and
    // this returns before the caller resolves any token to a native object.
    pub fn copy_tokens(&self, out: &mut [Token]) -> Result<usize, Error> {
        if out.len() < self.scheduled {
            return Err(Error::InvalidState);
        }
        let mut used = 0;
        for priority in (0..PRIORITIES).rev() {
            let mut next = self.heads[priority];
            while let Some(token) = next {
                out[used] = token;
                used += 1;
                next = self.nodes.get(&token).expect("linked node").next;
            }
        }
        Ok(used)
    }

    // Copies no more than the requested initialized output capacity.
    pub fn copy_prefix(&self, out: &mut [Token]) -> usize {
        let mut used = 0;
        for priority in (0..PRIORITIES).rev() {
            let mut next = self.heads[priority];
            while let Some(token) = next {
                if used == out.len() {
                    return used;
                }
                out[used] = token;
                used += 1;
                next = self.nodes.get(&token).expect("linked node").next;
            }
        }
        used
    }

    pub fn token_at(&self, mut rank: usize) -> Option<Token> {
        for priority in (0..PRIORITIES).rev() {
            let size = self.sizes[priority];
            if rank >= size {
                rank -= size;
                continue;
            }
            if rank <= size / 2 {
                let mut token = self.heads[priority]?;
                for _ in 0..rank {
                    token = self.nodes.get(&token)?.next?;
                }
                return Some(token);
            }
            let mut token = self.tails[priority]?;
            for _ in 0..(size - 1 - rank) {
                token = self.nodes.get(&token)?.prev?;
            }
            return Some(token);
        }
        None
    }

    fn rank(&self, token: Token) -> Result<usize, Error> {
        let node = self.nodes.get(&token).ok_or(Error::Missing)?;
        if node.pending.is_some() {
            return Err(Error::Missing);
        }
        let priority = usize::from(node.priority);
        let mut rank: usize = self.sizes[priority + 1..].iter().sum();
        let mut cursor = self.heads[priority];
        while let Some(current) = cursor {
            if current == token {
                return Ok(rank);
            }
            rank += 1;
            cursor = self.nodes.get(&current).expect("linked node").next;
        }
        Err(Error::InvalidState)
    }

    fn validate_counts(&self, assigned: usize, unassigned: usize) -> Result<(), Error> {
        if assigned > self.scheduled || (unassigned != 0 && assigned != self.scheduled) {
            return Err(Error::InvalidState);
        }
        Ok(())
    }

    fn insertion_rank(&self, priority: usize, respect: bool) -> usize {
        self.sizes[priority + 1..].iter().sum::<usize>()
            + if respect { self.sizes[priority] } else { 0 }
    }

    // Internal mutation only: all neighbors are scalar tokens. No allocations.
    fn link(&mut self, token: Token, priority: usize, front: bool) {
        let neighbor = if front { self.heads[priority] } else { self.tails[priority] };
        {
            let node = self.nodes.get_mut(&token).expect("retained node");
            node.priority = priority as u8;
            node.pending = None;
            node.prev = if front { None } else { neighbor };
            node.next = if front { neighbor } else { None };
        }
        if let Some(other) = neighbor {
            let node = self.nodes.get_mut(&other).expect("linked neighbor");
            if front {
                node.prev = Some(token);
            } else {
                node.next = Some(token);
            }
        } else {
            if front {
                self.tails[priority] = Some(token);
            } else {
                self.heads[priority] = Some(token);
            }
        }
        if front {
            self.heads[priority] = Some(token);
        } else {
            self.tails[priority] = Some(token);
        }
        self.sizes[priority] += 1;
        self.scheduled += 1;
    }

    // Unlink preserves the box and never allocates. Caller sets its phase.
    fn unlink(&mut self, token: Token) {
        let node = self.nodes.get(&token).expect("linked node");
        let (priority, prev, next) = (usize::from(node.priority), node.prev, node.next);
        if let Some(previous) = prev {
            self.nodes.get_mut(&previous).expect("previous").next = next;
        } else {
            self.heads[priority] = next;
        }
        if let Some(following) = next {
            self.nodes.get_mut(&following).expect("next").prev = prev;
        } else {
            self.tails[priority] = prev;
        }
        let node = self.nodes.get_mut(&token).expect("retained node");
        node.prev = None;
        node.next = None;
        self.sizes[priority] -= 1;
        self.scheduled -= 1;
    }

    fn selected_action(&self, rank: usize, assigned: usize, unassigned: usize) -> Action {
        if unassigned != 0 {
            Action::UnassignedFront
        } else if rank < assigned {
            Action::Donor(self.token_at(assigned).expect("donor rank"))
        } else {
            Action::None
        }
    }

    // New requests may allocate, like the original native list insertion. The
    // native adapter must treat Allocation as fatal, not as a socket error.
    pub fn create(
        &mut self,
        priority: u8,
        respect: bool,
        assigned: usize,
        unassigned: usize,
    ) -> Result<Inserted, Error> {
        if usize::from(priority) >= PRIORITIES {
            return Err(Error::InvalidPriority);
        }
        self.validate_counts(assigned, unassigned)?;
        let token = self.next_token;
        let next = token.checked_add(1).ok_or(Error::TokenExhausted)?;
        self.nodes.try_reserve(1).map_err(|_| Error::Allocation)?;
        let node = Box::new(Node {
            priority,
            respect_limits: respect,
            prev: None,
            next: None,
            pending: None,
        });
        let rank = self.insertion_rank(usize::from(priority), respect);
        assert!(self.nodes.insert(token, node).is_none());
        self.next_token = next;
        self.link(token, usize::from(priority), !respect);
        Ok(Inserted { token, rank, action: self.selected_action(rank, assigned, unassigned) })
    }

    // Same priority does not detach or change FIFO position. The native
    // ignore-limits DCHECK remains at its original later point in Group.
    pub fn detach_for_priority(&mut self, token: Token, new_priority: u8) -> Result<Detach, Error> {
        if usize::from(new_priority) >= PRIORITIES {
            return Err(Error::InvalidPriority);
        }
        let node = self.nodes.get(&token).ok_or(Error::Missing)?;
        if node.pending.is_some() {
            return Err(Error::InvalidState);
        }
        if node.priority == new_priority {
            return Ok(Detach::Unchanged);
        }
        self.unlink(token);
        self.nodes.get_mut(&token).expect("retained node").pending = Some(new_priority);
        Ok(Detach::Detached)
    }

    // Relinks exactly the retained box. Validation, rank calculation, link and
    // selection contain no allocation. Native reassignment must precede this.
    pub fn finish_priority(
        &mut self,
        token: Token,
        assigned: usize,
        unassigned: usize,
    ) -> Result<Inserted, Error> {
        self.validate_counts(assigned, unassigned)?;
        let node = self.nodes.get(&token).ok_or(Error::Missing)?;
        let priority = node.pending.ok_or(Error::InvalidState)?;
        let respect = node.respect_limits;
        let rank = self.insertion_rank(usize::from(priority), respect);
        self.link(token, usize::from(priority), !respect);
        Ok(Inserted { token, rank, action: self.selected_action(rank, assigned, unassigned) })
    }

    // Ordinary cancellation/pop discards a scheduled node. Detached nodes are
    // invisible to normal queries and need explicit adapter teardown handling.
    pub fn remove(&mut self, token: Token) -> Result<(), Error> {
        let node = self.nodes.get(&token).ok_or(Error::Missing)?;
        if node.pending.is_some() {
            return Err(Error::Missing);
        }
        self.unlink(token);
        self.nodes.remove(&token);
        Ok(())
    }

    pub fn discard_detached(&mut self, token: Token) -> Result<(), Error> {
        let node = self.nodes.get(&token).ok_or(Error::Missing)?;
        if node.pending.is_none() {
            return Err(Error::InvalidState);
        }
        self.nodes.remove(&token);
        Ok(())
    }

    pub fn select_hole(
        &self,
        token: Token,
        assigned: usize,
        unassigned: usize,
    ) -> Result<Action, Error> {
        let rank = self.rank(token)?;
        if assigned >= self.scheduled || (unassigned != 0 && assigned != self.scheduled - 1) {
            return Err(Error::InvalidState);
        }
        Ok(self.selected_action(rank, assigned, unassigned))
    }

    pub fn preconnect_claims(&self) -> usize {
        self.preconnect_claims
    }

    pub fn claim_preconnect(&mut self) -> bool {
        if self.preconnect_claims == 0 {
            return false;
        }
        self.preconnect_claims -= 1;
        true
    }

    // A removed job need not be individually identified as a preconnect.
    // The original capped the aggregate count at the remaining unbound jobs.
    pub fn trim_preconnect_claims(&mut self, job_count: usize) {
        self.preconnect_claims = self.preconnect_claims.min(job_count);
    }

    // A freshly created preconnect adds a claim. Returning a previously
    // assigned job for reuse must never restore a claim.
    pub fn release_job(
        &mut self,
        assigned: usize,
        unassigned: usize,
        added_preconnect: bool,
    ) -> Result<Option<Token>, Error> {
        let token = self.select_released(assigned, unassigned)?;
        if added_preconnect {
            self.preconnect_claims += 1;
        }
        Ok(token)
    }

    fn select_released(
        &self,
        assigned: usize,
        unassigned: usize,
    ) -> Result<Option<Token>, Error> {
        if unassigned == 0
            || assigned > self.scheduled
            || (assigned < self.scheduled && unassigned != 1)
        {
            return Err(Error::InvalidState);
        }
        Ok(self.token_at(assigned))
    }
}

#[cfg(test)]
#[path = "retained_queue_tests.rs"]
mod tests;
