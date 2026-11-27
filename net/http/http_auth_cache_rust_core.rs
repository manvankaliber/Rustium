// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Cache policy. Opaque handles identify stable native nodes; this module never
//! dereferences them. C++ owns storage and native value/clock/callback
//! services. Rust chooses matching, traversal, eviction, path updates and copy
//! policy.
#![forbid(unsafe_code)]

#[derive(Clone, Copy)]
#[repr(u32)]
pub enum Scalar {
    Count,
    CompareKey,
    Target,
    Scheme,
    PathCount,
    PathSwap,
    PathErase,
    PathPop,
    Nonce,
    SetNonce,
    LastUse,
    SetLastUse,
    Creation,
    SetCreation,
    SetCreationTicks,
    Tick,
    Clock,
    Erase,
    Insert,
    SetIdentity,
    SetValues,
    SetChallenge,
    CredentialEquals,
    UrlMatches,
    Keyed,
    SetKeyed,
    SourceFirst,
    SourceNext,
    SourceTarget,
    BindSource,
    OriginEquals,
    CredentialsEqual,
    HasMatcher,
    At,
    First,
    Next,
    WarnCacheLimit,
    WarnPathLimit,
}

#[derive(Clone, Copy)]
#[repr(u32)]
pub enum Bytes {
    QueryRealm,
    QueryPath,
    Realm,
    Path,
    Challenge,
}

pub trait Services {
    fn scalar(&mut self, op: Scalar, entry: i64, a: i64, b: i64) -> i64;
    fn bytes(&self, op: Bytes, entry: i64, index: i64) -> &[u8];
    fn push_path(&mut self, entry: i64, path: &[u8]);
}

const DIRECT: i64 = -1;
const SOURCE: i64 = -2;
const SERVER: i64 = 1;
const PROXY: i64 = 0;
const MAX_ENTRIES: i64 = 20;
const MAX_PATHS: i64 = 10;

fn get(s: &mut impl Services, op: Scalar, id: i64) -> i64 {
    s.scalar(op, id, 0, 0)
}
fn set(s: &mut impl Services, op: Scalar, id: i64, value: i64) {
    s.scalar(op, id, value, 0);
}
fn parent(path: &[u8]) -> &[u8] {
    path.iter().rposition(|&b| b == b'/').map_or(path, |i| &path[..=i])
}
fn encloses(container: &[u8], path: &[u8]) -> bool {
    if container.is_empty() {
        path.is_empty()
    } else {
        path.starts_with(container)
    }
}

fn enclosing(s: &mut impl Services, id: i64, dir: &[u8]) -> Option<usize> {
    for p in 0..get(s, Scalar::PathCount, id) {
        let path = s.bytes(Bytes::Path, id, p);
        if encloses(path, dir) {
            let length = path.len();
            if p != 0 {
                s.scalar(Scalar::PathSwap, id, p, p - 1);
            }
            return Some(length);
        }
    }
    None
}

fn add_path(s: &mut impl Services, id: i64, path: &[u8]) {
    let dir = parent(path);
    if enclosing(s, id, dir).is_some() {
        return;
    }
    let mut p = 0;
    while p < get(s, Scalar::PathCount, id) {
        if encloses(dir, s.bytes(Bytes::Path, id, p)) {
            set(s, Scalar::PathErase, id, p);
        } else {
            p += 1;
        }
    }
    if get(s, Scalar::PathCount, id) >= MAX_PATHS {
        get(s, Scalar::WarnPathLimit, id);
        get(s, Scalar::PathPop, id);
    }
    s.push_path(id, dir);
}

fn lookup(s: &mut impl Services, target: i64, scheme: i64) -> i64 {
    let keyed = target == SERVER && get(s, Scalar::Keyed, 0) != 0;
    for index in 0..get(s, Scalar::Count, 0) {
        let id = get(s, Scalar::At, index);
        if s.scalar(Scalar::CompareKey, id, i64::from(keyed), 0) == 0
            && get(s, Scalar::Scheme, id) == scheme
            && s.bytes(Bytes::Realm, id, 0) == s.bytes(Bytes::QueryRealm, 0, 0)
        {
            let ticks = get(s, Scalar::Tick, 0);
            set(s, Scalar::SetLastUse, id, ticks);
            return id;
        }
    }
    -1
}

fn add(s: &mut impl Services, target: i64, scheme: i64) -> i64 {
    // Preserve the caller's clock sequence, including the lookup clock on a hit
    // and the eviction clock on a full miss. Last-use is the FIRST reading.
    let ticks = get(s, Scalar::Tick, 0);
    let mut id = lookup(s, target, scheme);
    if id == -1 {
        let count = get(s, Scalar::Count, 0);
        if count >= MAX_ENTRIES {
            get(s, Scalar::WarnCacheLimit, 0);
            let mut oldest = -1;
            let mut time = get(s, Scalar::Tick, 0);
            // The caller's TickClock can reenter and remove other entries.
            // Match the original live traversal AFTER its clock read.
            let mut i = get(s, Scalar::First, 0);
            while i != -1 {
                let used = get(s, Scalar::LastUse, i);
                if used < time || oldest == -1 {
                    oldest = i;
                    time = used;
                }
                i = get(s, Scalar::Next, i);
            }
            assert_ne!(oldest, -1, "auth cache eviction requires nonempty storage");
            get(s, Scalar::Erase, oldest);
        }
        let keyed = target == SERVER && get(s, Scalar::Keyed, 0) != 0;
        id = s.scalar(Scalar::Insert, 0, i64::from(keyed), 0);
        get(s, Scalar::SetIdentity, id);
        set(s, Scalar::SetCreationTicks, id, ticks);
        let time = get(s, Scalar::Clock, 0);
        set(s, Scalar::SetCreation, id, time);
    }
    get(s, Scalar::SetValues, id);
    set(s, Scalar::SetNonce, id, 1);
    let path = s.bytes(Bytes::QueryPath, 0, 0).to_vec();
    add_path(s, id, &path);
    set(s, Scalar::SetLastUse, id, ticks);
    id
}

#[derive(Clone, Copy)]
#[repr(u32)]
pub enum Operation {
    Lookup,
    LookupPath,
    Add,
    Remove,
    ClearBetween,
    ClearAll,
    Stale,
    SetPartition,
    CopyProxy,
    IncrementNonce,
    EntryStale,
    AddPath,
    HasPath,
    Equal,
}

pub fn run(s: &mut impl Services, op: Operation, target: i64, scheme: i64, a: i64, b: i64) -> i64 {
    match op {
        Operation::Lookup => lookup(s, target, scheme),
        Operation::Add => add(s, target, scheme),
        Operation::LookupPath => {
            let path = s.bytes(Bytes::QueryPath, 0, 0).to_vec();
            let dir = parent(&path);
            let keyed = target == SERVER && get(s, Scalar::Keyed, 0) != 0;
            let mut best = -1;
            let mut length = 0;
            for index in 0..get(s, Scalar::Count, 0) {
                let id = get(s, Scalar::At, index);
                if s.scalar(Scalar::CompareKey, id, i64::from(keyed), 0) != 0 {
                    continue;
                }
                if let Some(len) = enclosing(s, id, dir)
                    && (best == -1 || len > length)
                {
                    best = id;
                    length = len;
                }
            }
            if best != -1 {
                let ticks = get(s, Scalar::Tick, 0);
                set(s, Scalar::SetLastUse, best, ticks);
            }
            best
        }
        Operation::Remove => {
            let id = lookup(s, target, scheme);
            if id != -1 && get(s, Scalar::CredentialEquals, id) != 0 {
                get(s, Scalar::Erase, id);
                1
            } else {
                0
            }
        }
        Operation::Stale => {
            let id = lookup(s, target, scheme);
            if id == -1 {
                return 0;
            }
            get(s, Scalar::SetChallenge, id);
            set(s, Scalar::SetNonce, id, 1);
            let ticks = get(s, Scalar::Tick, 0);
            set(s, Scalar::SetLastUse, id, ticks);
            1
        }
        Operation::ClearAll => {
            let count = get(s, Scalar::Count, 0);
            for _ in 0..count {
                let id = get(s, Scalar::First, 0);
                get(s, Scalar::Erase, id);
            }
            i64::from(count != 0)
        }
        Operation::ClearBetween => {
            if a == i64::MIN && b == i64::MAX && get(s, Scalar::HasMatcher, 0) == 0 {
                return run(s, Operation::ClearAll, target, scheme, 0, 0);
            }
            let count_before = get(s, Scalar::Count, 0);
            let mut id = get(s, Scalar::First, 0);
            while id != -1 {
                let time = get(s, Scalar::Creation, id);
                if time >= a && time < b && get(s, Scalar::UrlMatches, id) != 0 {
                    id = get(s, Scalar::Erase, id);
                } else {
                    id = get(s, Scalar::Next, id);
                }
            }
            i64::from(get(s, Scalar::Count, 0) != count_before)
        }
        Operation::SetPartition => {
            if get(s, Scalar::Keyed, 0) != a {
                set(s, Scalar::SetKeyed, 0, a);
                let mut id = get(s, Scalar::First, 0);
                while id != -1 {
                    if get(s, Scalar::Target, id) == SERVER {
                        id = get(s, Scalar::Erase, id);
                    } else {
                        id = get(s, Scalar::Next, id);
                    }
                }
            }
            0
        }
        Operation::CopyProxy => {
            let mut src = get(s, Scalar::SourceFirst, 0);
            while src != -1 {
                if get(s, Scalar::SourceTarget, src) != PROXY {
                    src = get(s, Scalar::SourceNext, src);
                    continue;
                }
                let scheme = get(s, Scalar::BindSource, src);
                let id = add(s, PROXY, scheme);
                // Read LIVE paths, after Add has promoted the last path. This
                // deliberately preserves legacy CopyProxyEntriesFrom(*this),
                // including its in-place swaps and nonce reset to one.
                let count = get(s, Scalar::PathCount, SOURCE);
                for p in (0..count - 1).rev() {
                    let path = s.bytes(Bytes::Path, SOURCE, p).to_vec();
                    add_path(s, id, &path);
                }
                let nonce = get(s, Scalar::Nonce, SOURCE);
                set(s, Scalar::SetNonce, id, nonce);
                src = get(s, Scalar::SourceNext, src);
            }
            0
        }
        Operation::IncrementNonce => {
            let nonce = (get(s, Scalar::Nonce, DIRECT) as i32).wrapping_add(1);
            set(s, Scalar::SetNonce, DIRECT, i64::from(nonce));
            i64::from(nonce)
        }
        Operation::EntryStale => {
            get(s, Scalar::SetChallenge, DIRECT);
            set(s, Scalar::SetNonce, DIRECT, 1);
            0
        }
        Operation::AddPath => {
            let path = s.bytes(Bytes::QueryPath, 0, 0).to_vec();
            add_path(s, DIRECT, &path);
            0
        }
        Operation::HasPath => {
            let path = s.bytes(Bytes::QueryPath, 0, 0).to_vec();
            enclosing(s, DIRECT, &path).map_or(-1, |n| n as i64)
        }
        Operation::Equal => {
            if get(s, Scalar::OriginEquals, DIRECT) == 0
                || s.bytes(Bytes::Realm, DIRECT, 0) != s.bytes(Bytes::Realm, SOURCE, 0)
                || get(s, Scalar::Scheme, DIRECT) != get(s, Scalar::Scheme, SOURCE)
                || s.bytes(Bytes::Challenge, DIRECT, 0) != s.bytes(Bytes::Challenge, SOURCE, 0)
                || get(s, Scalar::CredentialsEqual, DIRECT) == 0
            {
                return 0;
            }
            fn paths(s: &mut impl Services, id: i64) -> Vec<Vec<u8>> {
                let mut paths: Vec<_> = (0..get(s, Scalar::PathCount, id))
                    .map(|p| s.bytes(Bytes::Path, id, p).to_vec())
                    .collect();
                paths.sort_unstable();
                paths.dedup();
                paths
            }
            i64::from(paths(s, DIRECT) == paths(s, SOURCE))
        }
    }
}
