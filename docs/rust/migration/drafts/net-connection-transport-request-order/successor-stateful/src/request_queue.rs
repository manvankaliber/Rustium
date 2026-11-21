// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Standalone authoritative ordering kernel. This is not wired to Chromium.
//! Tokens are generated scalar identities, never native addresses. The native
//! owner must keep them unique across its lifetime, including detach/reinsert.

use std::collections::VecDeque;

pub type Token = u64;
const PRIORITIES: usize = 6;
const MAX_PRIORITY: u8 = 5;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Entry {
    pub token: Token,
    pub priority: u8,
    pub respect_limits: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Action {
    None,
    UnassignedFront,
    Donor(Token),
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Inserted {
    pub rank: usize,
    pub action: Action,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TakeForPriority {
    Unchanged,
    Taken(Entry),
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    InvalidToken,
    Duplicate,
    Missing,
    InvalidPriority,
    InvalidState,
    Capacity,
    Allocation,
}

pub struct RequestQueue {
    buckets: [VecDeque<Entry>; PRIORITIES],
    len: usize,
    // A harness bound, not a proposed browser limit. Integration must supply
    // a source-compatible limit and define its failure policy.
    limit: usize,
}

impl RequestQueue {
    pub fn new(limit: usize) -> Self {
        Self { buckets: std::array::from_fn(|_| VecDeque::new()), len: 0, limit }
    }

    pub fn len(&self) -> usize {
        self.len
    }
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    fn find(&self, token: Token) -> Option<(usize, usize, usize)> {
        let mut rank = 0;
        for priority in (0..PRIORITIES).rev() {
            for (offset, entry) in self.buckets[priority].iter().enumerate() {
                if entry.token == token {
                    return Some((priority, offset, rank));
                }
                rank += 1;
            }
        }
        None
    }

    pub fn token_at(&self, mut rank: usize) -> Option<Token> {
        for priority in (0..PRIORITIES).rev() {
            let bucket = &self.buckets[priority];
            if rank < bucket.len() {
                return bucket.get(rank).map(|entry| entry.token);
            }
            rank -= bucket.len();
        }
        None
    }

    pub fn front(&self) -> Option<Token> {
        self.token_at(0)
    }

    // A single caller-provided output buffer supports one bridge crossing for a
    // snapshot, rather than exporting a native pointer or calling per element.
    pub fn copy_tokens(&self, out: &mut [Token]) -> Result<usize, Error> {
        if out.len() < self.len {
            return Err(Error::Capacity);
        }
        let mut i = 0;
        for priority in (0..PRIORITIES).rev() {
            for entry in &self.buckets[priority] {
                out[i] = entry.token;
                i += 1;
            }
        }
        Ok(i)
    }

    // Pre-state has an assigned prefix. A nonempty unassigned-job list implies
    // all old requests have jobs. Mutation is committed only after validation
    // and fallible reservation, and does not call native code.
    pub fn insert(
        &mut self,
        entry: Entry,
        assigned: usize,
        unassigned: usize,
    ) -> Result<Inserted, Error> {
        if entry.token == 0 {
            return Err(Error::InvalidToken);
        }
        if entry.priority > MAX_PRIORITY
            || (!entry.respect_limits && entry.priority != MAX_PRIORITY)
        {
            return Err(Error::InvalidPriority);
        }
        if self.find(entry.token).is_some() {
            return Err(Error::Duplicate);
        }
        if assigned > self.len || (unassigned != 0 && assigned != self.len) {
            return Err(Error::InvalidState);
        }
        if self.len >= self.limit {
            return Err(Error::Capacity);
        }
        let priority = usize::from(entry.priority);
        let higher: usize = self.buckets[priority + 1..].iter().map(VecDeque::len).sum();
        let rank = higher + if entry.respect_limits { self.buckets[priority].len() } else { 0 };
        self.buckets[priority].try_reserve(1).map_err(|_| Error::Allocation)?;
        if entry.respect_limits {
            self.buckets[priority].push_back(entry);
        } else {
            self.buckets[priority].push_front(entry);
        }
        self.len += 1;
        let action = if unassigned != 0 {
            Action::UnassignedFront
        } else if rank < assigned {
            // rank < assigned <= old length, so the donor is present after
            // insertion. This is the last formerly assigned request.
            Action::Donor(self.token_at(assigned).expect("validated donor rank"))
        } else {
            Action::None
        };
        Ok(Inserted { rank, action })
    }

    // Used by cancellation, pop and auth binding. Native Request/job ownership
    // remains with the caller; this removes only authoritative order state.
    pub fn take(&mut self, token: Token) -> Result<Entry, Error> {
        let (priority, offset, _) = self.find(token).ok_or(Error::Missing)?;
        let entry = self.buckets[priority].remove(offset).expect("located entry");
        self.len -= 1;
        Ok(entry)
    }

    // No-op retains queue identity and FIFO position. A changed priority is a
    // detach, not an atomic reorder: native release/reassignment must happen
    // before a later insert. Ignore-limits changes are outside the valid native
    // contract and are rejected here; the production DCHECK mapping is open.
    pub fn take_for_priority(
        &mut self,
        token: Token,
        new_priority: u8,
    ) -> Result<TakeForPriority, Error> {
        if new_priority > MAX_PRIORITY {
            return Err(Error::InvalidPriority);
        }
        let (priority, offset, _) = self.find(token).ok_or(Error::Missing)?;
        let entry = self.buckets[priority][offset];
        if entry.priority == new_priority {
            return Ok(TakeForPriority::Unchanged);
        }
        if !entry.respect_limits {
            return Err(Error::InvalidPriority);
        }
        let removed = self.buckets[priority].remove(offset).expect("located entry");
        self.len -= 1;
        Ok(TakeForPriority::Taken(removed))
    }

    // The named request remains in the queue as the single hole after a job
    // has been released. `assigned` excludes that job even if it remains in
    // native jobs_; this is the distinct RemoveUnboundJob transient state.
    pub fn select_hole(
        &self,
        token: Token,
        assigned: usize,
        unassigned: usize,
    ) -> Result<Action, Error> {
        let (_, _, rank) = self.find(token).ok_or(Error::Missing)?;
        if assigned >= self.len || (unassigned != 0 && assigned != self.len - 1) {
            return Err(Error::InvalidState);
        }
        if unassigned != 0 {
            Ok(Action::UnassignedFront)
        } else if rank < assigned {
            Ok(Action::Donor(self.token_at(assigned).expect("validated donor rank")))
        } else {
            Ok(Action::None)
        }
    }

    // Call after native push_back of the added/released job. A target is the
    // first request outside the already-assigned prefix; native applies back.
    pub fn select_released(
        &self,
        assigned: usize,
        unassigned: usize,
    ) -> Result<Option<Token>, Error> {
        if unassigned == 0 || assigned > self.len || (assigned < self.len && unassigned != 1) {
            return Err(Error::InvalidState);
        }
        Ok(self.token_at(assigned))
    }
}

#[cfg(test)]
#[path = "request_queue_tests.rs"]
mod tests;
