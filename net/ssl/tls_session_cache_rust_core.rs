// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! SSLClientSessionCache policy. Native objects are opaque integer handles.
//! Session releases are deferred until the ABI has ended its state borrow.
#![forbid(unsafe_code)]
use std::collections::BTreeMap;
use std::rc::Rc;

// Original HostPortPair declaration orders port before host.
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct Server(pub u16, pub Vec<u8>);
#[derive(PartialEq, Eq, PartialOrd, Ord)]
pub struct Proxy(pub i32, pub Server);
#[derive(PartialEq, Eq, PartialOrd, Ord)]
pub struct Key<N: Ord> {
    pub server: Server,
    // IPAddress orders size before bytes; None differs from Some(empty).
    pub ip: Option<(usize, Vec<u8>)>,
    pub nak: N,
    pub privacy: i32,
    pub usage: i32,
    pub proxies: Option<Vec<Proxy>>,
    pub opaque: Option<i32>,
    pub proxy_index: usize,
}

pub fn register_memory_consumer(is_mac: bool, is_win: bool) -> bool {
    !is_mac && !is_win
}

pub trait Services {
    fn stateful_memory(&self) -> bool;
    fn ignore_memory_pressure(&self) -> bool;
    fn now(&self) -> i64;
    fn time(&self, session: usize) -> u64;
    fn timeout(&self, session: usize) -> u64;
    fn single_use(&self, session: usize) -> bool;
    fn up_ref(&self, session: usize) -> usize;
    fn without_early_data(&self, session: usize) -> usize;
}
pub fn expired(s: &impl Services, session: usize, now: i64) -> bool {
    if now < 0 {
        return true;
    }
    let now = now as u64;
    let start = s.time(session);
    now < start.wrapping_sub(1) || now >= start.wrapping_add(s.timeout(session))
}
struct Node<N: Ord> {
    key: Rc<Key<N>>,
    sessions: [usize; 2],
    prev: Option<usize>,
    next: Option<usize>,
}
pub struct Cache<N: Ord> {
    index: BTreeMap<Rc<Key<N>>, usize>,
    nodes: Vec<Option<Node<N>>>,
    free: Vec<usize>,
    head: Option<usize>,
    tail: Option<usize>,
    pub generation: u64,
    pub max_size: usize,
    configured_max: usize,
    expiration_count: usize,
    lookups: usize,
    // Ownership of each nonzero handle moves here on removal, never freed
    // while the cache is borrowed. take_garbage() transfers that ownership.
    garbage: Vec<usize>,
}
impl<N: Ord> Cache<N> {
    pub fn new(max_size: usize, expiration_count: usize) -> Self {
        Self {
            index: BTreeMap::new(),
            nodes: Vec::new(),
            free: Vec::new(),
            head: None,
            tail: None,
            generation: 0,
            max_size,
            configured_max: max_size,
            expiration_count,
            lookups: 0,
            garbage: Vec::new(),
        }
    }
    pub fn size(&self) -> usize {
        self.index.len()
    }
    fn node(&self, id: usize) -> &Node<N> {
        self.nodes[id].as_ref().unwrap()
    }
    fn node_mut(&mut self, id: usize) -> &mut Node<N> {
        self.nodes[id].as_mut().unwrap()
    }
    fn retire(&mut self, session: usize) {
        if session != 0 {
            self.garbage.push(session);
        }
    }
    fn unlink(&mut self, id: usize) {
        let (prev, next) = (self.node(id).prev, self.node(id).next);
        if let Some(p) = prev {
            self.node_mut(p).next = next;
        } else {
            self.head = next;
        }
        if let Some(n) = next {
            self.node_mut(n).prev = prev;
        } else {
            self.tail = prev;
        }
    }
    fn promote(&mut self, id: usize) {
        if self.head == Some(id) {
            return;
        }
        self.unlink(id);
        self.node_mut(id).prev = None;
        self.node_mut(id).next = self.head;
        if let Some(h) = self.head {
            self.node_mut(h).prev = Some(id);
        } else {
            self.tail = Some(id);
        }
        self.head = Some(id);
    }
    fn find(&mut self, key: &Key<N>) -> Option<usize> {
        let id = self.index.get(key).copied()?;
        self.promote(id);
        Some(id)
    }
    fn erase(&mut self, id: usize) {
        self.unlink(id);
        let node = self.nodes[id].take().unwrap();
        self.index.remove(&node.key);
        // Entry's members are destroyed from sessions[1] to sessions[0].
        self.retire(node.sessions[1]);
        self.retire(node.sessions[0]);
        self.free.push(id);
    }
    fn trim(&mut self) {
        while self.size() > self.max_size {
            self.erase(self.tail.unwrap());
        }
        self.release_empty_arena();
    }
    fn release_empty_arena(&mut self) {
        if self.size() == 0 {
            self.nodes = Vec::new();
            self.free = Vec::new();
        }
    }
    pub fn insert(&mut self, generation: u64, key: Key<N>, session: usize, s: &impl Services) {
        if generation != self.generation {
            self.retire(session);
            return;
        }
        let id = if let Some(id) = self.find(&key) {
            id
        } else {
            // base::LRUCache::Put evicts BEFORE inserting its new entry.
            if self.max_size == 0 {
                self.retire(session);
                return;
            }
            while self.size() >= self.max_size {
                self.erase(self.tail.unwrap());
            }
            let id = self.free.pop().unwrap_or_else(|| {
                self.nodes.push(None);
                self.nodes.len() - 1
            });
            let key = Rc::new(key);
            self.index.insert(Rc::clone(&key), id);
            self.nodes[id] = Some(Node { key, sessions: [0; 2], prev: None, next: self.head });
            if let Some(h) = self.head {
                self.node_mut(h).prev = Some(id);
            } else {
                self.tail = Some(id);
            }
            self.head = Some(id);
            id
        };
        let [first, second] = self.node(id).sessions;
        if first != 0 && s.single_use(first) {
            self.retire(second);
            self.node_mut(id).sessions = [session, first];
        } else {
            self.retire(first);
            self.node_mut(id).sessions[0] = session;
        }
    }
    fn expire_entry(&mut self, id: usize, now: i64, s: &impl Services) {
        let [first, second] = self.node(id).sessions;
        if first == 0 || expired(s, first, now) {
            self.erase(id);
        } else if second != 0 && expired(s, second, now) {
            self.node_mut(id).sessions[1] = 0;
            self.retire(second);
        }
    }
    pub fn prune(&mut self, s: &impl Services) {
        let now = s.now();
        let mut next = self.head;
        while let Some(id) = next {
            next = self.node(id).next;
            self.expire_entry(id, now, s);
        }
    }
    pub fn lookup(&mut self, key: &Key<N>, s: &impl Services) -> usize {
        self.lookups = self.lookups.wrapping_add(1);
        if self.lookups >= self.expiration_count {
            self.lookups = 0;
            self.prune(s);
        }
        let Some(id) = self.find(key) else {
            return 0;
        };
        let now = s.now();
        let [first, second] = self.node(id).sessions;
        let result = if first == 0 { 0 } else { s.up_ref(first) };
        if first != 0 && s.single_use(first) {
            self.node_mut(id).sessions = [second, 0];
            self.retire(first);
        }
        self.expire_entry(id, now, s);
        // Original policy queries BoringSSL even for a null Pop result.
        if expired(s, result, now) {
            self.retire(result);
            0
        } else {
            result
        }
    }
    pub fn clear_early_data(&mut self, key: &Key<N>, s: &impl Services) {
        if let Some(id) = self.find(key) {
            for slot in 0..2 {
                let old = self.node(id).sessions[slot];
                if old != 0 {
                    let new = s.without_early_data(old);
                    self.node_mut(id).sessions[slot] = new;
                    self.retire(old);
                }
            }
        }
    }
    pub fn flush_servers(&mut self, servers: &[Server]) {
        self.generation = self.generation.wrapping_add(1);
        let mut next = self.head;
        while let Some(id) = next {
            next = self.node(id).next;
            if servers.binary_search(&self.node(id).key.server).is_ok() {
                self.erase(id);
            }
        }
    }
    pub fn flush(&mut self) {
        self.generation = self.generation.wrapping_add(1);
        // LRUCache::Clear destroys its underlying std::list in MRU order.
        while let Some(id) = self.head {
            self.erase(id);
        }
        self.release_empty_arena();
    }
    fn scaled_limit(&self, percent: i32) -> usize {
        // Original MemoryLimit::Scale: divide percentage in double precision,
        // multiply the baseline in double precision, truncate and saturate.
        (self.configured_max as f64 * (f64::from(percent) / 100.0)) as usize
    }
    pub fn update_limit(&mut self, percent: i32, s: &impl Services) {
        if s.stateful_memory() {
            self.max_size = self.size().max(self.scaled_limit(percent));
        }
    }
    pub fn release_memory(&mut self, percent: i32, s: &impl Services) {
        if s.ignore_memory_pressure() {
            return;
        }
        if s.stateful_memory() {
            self.max_size = self.scaled_limit(percent);
            self.trim();
        } else if percent <= 0 {
            self.flush();
        } else if percent <= 50 {
            self.prune(s);
        }
    }
    pub fn take_garbage(&mut self) -> Vec<usize> {
        std::mem::take(&mut self.garbage)
    }
}
