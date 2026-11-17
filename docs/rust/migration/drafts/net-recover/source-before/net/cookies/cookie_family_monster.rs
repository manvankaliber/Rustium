// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
//! CookieMonster policy/control. Native maps, iterators, tasks, observers and
//! persistence remain ownership/effect services; no native service selects a
//! cookie for acceptance, overwrite, filtering, expiry or eviction.
#![forbid(unsafe_code)]
use std::collections::{BTreeMap, BTreeSet};
use crate::{base, canonical::Cookie, status};

pub const DOMAIN_MAX: usize = 180;
pub const DOMAIN_PURGE: usize = 30;
pub const GLOBAL_MAX: usize = 3300;
pub const GLOBAL_PURGE: usize = 300;
pub const PARTITION_BYTES: usize = 10240;
pub const PARTITION_MAX: usize = 180;
pub const MAX_PURGED_KEYS: usize = 100;
pub const DEFAULT_ACCESS_UPDATE_MICROS: i64 = 60_000_000;
pub const QUOTAS: [usize; 3] = [30, 50, 70];
pub const PURGE_ROUNDS: [(i32, bool); 6] =
    [(0, true), (0, false), (1, true), (2, true), (1, false), (2, false)];

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum Op {
    Check = 2000,
    State = 2001,
    SetState = 2002,
    Argument = 2003,
    Now = 2004,
    NowTicks = 2005,
    Begin = 2006,
    End = 2007,
    Current = 2008,
    Advance = 2009,
    OpenKeyRange = 2010,
    Count = 2011,
    Select = 2012,
    Key = 2013,
    NativeKeyEqual = 2014,
    NativeKeyCompare = 2015,
    Feature = 2016,
    Sample = 2017,
    CallbackPresent = 2018,
    RunCallback = 2019,
    QueueEmpty = 2020,
    QueuePush = 2021,
    QueuePop = 2022,
    QueueErase = 2023,
    QueuePrepend = 2024,
    KeysContains = 2025,
    KeysInsert = 2026,
    KeysClear = 2027,
    LoadAll = 2028,
    LoadKey = 2029,
    StoreFlush = 2030,
    PostCallback = 2031,
    KeepSession = 2032,
    InsertNative = 2033,
    EraseNative = 2034,
    Notify = 2035,
    PersistentAdd = 2036,
    PersistentDelete = 2037,
    PersistentAccess = 2038,
    Log = 2039,
    Metric = 2040,
    Counter = 2041,
    SetCounter = 2042,
    SetDate = 2043,
    Neighbor = 2044,
    PartitionFind = 2045,
    Access = 2046,
    WriteAccess = 2047,
    Delegate = 2048,
    Include = 2049,
    SetPermitted = 2050,
    DateArithmetic = 2051,
    OutputCookie = 2052,
    OutputList = 2053,
    Predicate = 2054,
    MatchDeletion = 2055,
    ValueEqual = 2056,
    CopyCookie = 2057,
    PartitionKeys = 2058,
    PrefPresent = 2059,
    PrefReady = 2060,
    PrefRead = 2061,
    PrefInit = 2062,
    PrefContains = 2063,
    PrefSet = 2064,
    PrefRemove = 2065,
    PrefWrite = 2066,
    Sort = 2067,
    EmitKey = 2068,
    InputByte = 2069,
    OutputScalar = 2070,
    WeakFlushDelete = 2071,
    TimerBegin = 2072,
    TimerFinish = 2073,
    PostPreconnect = 2074,
    ControlCharacters = 2075,
    NativeRangeKey = 2076,
    NativePartialSort = 2077,
    DefaultSchemes = 2078,
    SetSchemes = 2079,
    SetPersist = 2080,
    InputPartition = 2081,
    PartitionEqual = 2082,
    CheckCanonical = 2083,
    InputCookie = 2084,
    AfterInsert = 2085,
    SetCreation = 2086,
    SetAccess = 2087,
    Scheme = 2088,
    AllowedScheme = 2089,
    SelectPartition = 2090,
    PrepareBase = 2091,
    PostInclude = 2092,
    PostSet = 2093,
    UrlFacts = 2095,
}

// State IDs: initialized0,started1,finished2,seen-global3,store4,
// persist-session5,earliest-access6,last-stats7,load-block-start8,load-block-set9.
// Counter IDs: total-unpartitioned0,total-partitioned1,nonced2,partition-bytes3,
// nonced-bytes4,num-keys5,purged-key-count6,partition-count7,partition-bytes-key8.
// Cursor selectors: unpartitioned0,partition-map1,key-range2,input-list3,
// current-partition4,per-key-task-map5,global-task-queue6,current-key-queue7.
// NativeKey mode: current unique0,strict1,legacy2. Its immutable native value
// storage/comparison is a named dependency, independent of the Rust key policy.
pub trait Runtime {
    fn failed(&self) -> bool;
    fn n(&mut self, op: Op, a: i64, b: i64) -> i64;
    fn bytes(&mut self, op: Op, a: i64, b: i64) -> Vec<u8>;
    fn write(&mut self, op: Op, a: i64, b: i64, bytes: &[u8]);
    fn cookie(&mut self, id: i64, with_value: bool) -> Cookie;
    fn access(&mut self, id: i64) -> base::AccessResult;
    fn access_out(&mut self, id: i64, value: base::AccessResult);
    fn include(&mut self, id: i64) -> base::AccessResult;
    fn set_permitted(&mut self, id: i64) -> base::AccessResult;
    fn equivalent(&mut self, left: i64, right: i64, kind: i32) -> bool;
    fn key_compare(&mut self, left: i64, right: i64, kind: i32) -> i32;
    fn secure_matching(&mut self, left: i64, right: i64) -> bool;
    fn web_equal(&mut self, left: i64, right: i64) -> bool;
    fn value_equal(&mut self, left: i64, right: i64) -> bool;
    fn effectively_none(&mut self, id: i64, semantics: i32) -> bool;
    fn scope_for_key(&mut self) -> i32;
    fn get_key(&mut self, domain: &[u8]) -> Vec<u8>;
}
fn state(s: &mut impl Runtime, id: i64) -> i64 {
    s.n(Op::State, id, 0)
}
fn set_state(s: &mut impl Runtime, id: i64, value: i64) {
    s.n(Op::SetState, id, value);
}
fn argument(s: &mut impl Runtime, id: i64) -> i64 {
    s.n(Op::Argument, id, 0)
}
fn counter(s: &mut impl Runtime, id: i64) -> usize {
    s.n(Op::Counter, id, 0) as usize
}
fn check(s: &mut impl Runtime, id: i64, value: bool) {
    s.n(Op::Check, id, i64::from(value));
}
fn exclude(out: &mut base::AccessResult, reason: i32) {
    out.status = status::apply(out.status, 0, reason, 0);
}
fn warn(out: &mut base::AccessResult, reason: i32) {
    out.status = status::apply(out.status, 6, reason, 0);
}
fn include(out: base::AccessResult) -> bool {
    status::query(out.status, 0, 0, 0)
}
fn has(out: base::AccessResult, reason: i32) -> bool {
    status::query(out.status, 1, reason, 0)
}
fn warning(out: base::AccessResult, reason: i32) -> bool {
    status::query(out.status, 5, reason, 0)
}
fn current(s: &mut impl Runtime, cursor: i64) -> i64 {
    s.n(Op::Current, cursor, 0)
}
fn end(s: &mut impl Runtime, cursor: i64) -> bool {
    s.failed() || s.n(Op::End, cursor, 0) != 0
}
fn next(s: &mut impl Runtime, cursor: i64) {
    s.n(Op::Advance, cursor, 0);
}
fn count(s: &mut impl Runtime, selector: i64) -> usize {
    s.n(Op::Count, selector, 0) as usize
}
fn emit_key(s: &mut impl Runtime, key: &[u8]) {
    s.write(Op::EmitKey, 0, 0, key);
}
fn range(s: &mut impl Runtime, selector: i64) -> i64 {
    s.n(Op::Begin, selector, 0)
}
fn semantics(s: &mut impl Runtime, kind: i64, id: i64) -> i64 {
    if s.n(Op::Delegate, -1, 0) != 0 {
        s.n(Op::Delegate, kind, id)
    } else {
        -1
    }
}

pub fn should_persist(cookie: &Cookie, s: &mut impl Runtime) -> bool {
    (cookie.persistent() || state(s, 5) != 0) && state(s, 4) != 0
}
pub fn fetch_if_needed(s: &mut impl Runtime) {
    if state(s, 4) != 0 && state(s, 1) == 0 {
        set_state(s, 1, 1);
        s.n(Op::LoadAll, 0, 0);
    }
}
pub fn flush(s: &mut impl Runtime) {
    s.n(Op::Check, 0, 1);
    if state(s, 0) != 0 && state(s, 4) != 0 {
        s.n(Op::StoreFlush, 0, 0);
    } else if s.n(Op::CallbackPresent, 0, 0) != 0 {
        s.n(Op::PostCallback, 0, 0);
    }
}
pub fn global_callback(s: &mut impl Runtime) {
    s.n(Op::Check, 0, 1);
    set_state(s, 0, 1);
    fetch_if_needed(s);
    set_state(s, 3, 1);
    if state(s, 2) == 0 && state(s, 4) != 0 {
        if s.n(Op::QueueEmpty, 0, 0) != 0 {
            let now = s.n(Op::NowTicks, 0, 0);
            set_state(s, 8, now);
            set_state(s, 9, 1);
        }
        s.n(Op::QueuePush, 0, 0);
        return;
    }
    s.n(Op::RunCallback, 0, 0);
}
pub fn key_callback(s: &mut impl Runtime) {
    s.n(Op::Check, 0, 1);
    set_state(s, 0, 1);
    fetch_if_needed(s);
    if state(s, 2) == 0 && state(s, 4) != 0 {
        if state(s, 3) != 0 {
            s.n(Op::QueuePush, 0, 0);
            return;
        }
        let domain = s.bytes(Op::Key, 0, 0);
        let key = s.get_key(&domain);
        emit_key(s, &key);
        if s.n(Op::KeysContains, 0, 0) == 0 {
            if s.n(Op::Count, 10, 0) == 0 {
                // Preserve original load-before-emplace ordering, including
                // synchronous completion behavior of a native backing store.
                s.n(Op::LoadKey, 0, 0);
                s.n(Op::QueuePush, 2, 0);
            }
            s.n(Op::QueuePush, 1, 0);
            return;
        }
    }
    s.n(Op::RunCallback, 0, 0);
}
pub fn key_loaded(s: &mut impl Runtime) {
    let loaded_key = s.bytes(Op::Key, 0, 0);
    store_loaded(s);
    emit_key(s, &loaded_key);
    if s.n(Op::Count, 10, 0) == 0 {
        return;
    }
    while !s.failed() && s.n(Op::QueueEmpty, 1, 0) == 0 {
        s.n(Op::QueuePop, 1, 0);
        s.n(Op::RunCallback, 1, 0);
    }
    s.n(Op::QueueErase, 1, 0);
    s.n(Op::KeysInsert, 0, 0);
}
pub fn invoke_queue(s: &mut impl Runtime) {
    set_state(s, 3, 1);
    let cursor = range(s, 5);
    while !end(s, cursor) {
        let id = current(s, cursor);
        s.n(Op::QueuePrepend, id, 0);
        next(s, cursor);
    }
    s.n(Op::QueueErase, 2, 0);
    while !s.failed() && s.n(Op::QueueEmpty, 0, 0) == 0 {
        s.n(Op::QueuePop, 0, 0);
        s.n(Op::RunCallback, 1, 0);
    }
    let none = s.n(Op::Count, 11, 0) == 0;
    check(s, 1, none);
    set_state(s, 2, 1);
    s.n(Op::KeysClear, 0, 0);
}

pub fn deletion_mapping(cause: i32) -> (i32, bool) {
    const CAUSES: [(i32, bool); 16] = [
        (1, true),
        (3, true),
        (4, true),
        (5, true),
        (1, false),
        (1, false),
        (5, true),
        (5, true),
        (5, true),
        (5, true),
        (6, true),
        (5, true),
        (5, true),
        (5, true),
        (5, false),
        (1, false),
    ];
    CAUSES.get(cause as usize).copied().unwrap_or((1, false))
}
pub fn insertion_cause(observability: i32) -> i32 {
    match observability {
        0 => 7,
        1 => 0,
        2 => 8,
        _ => 0,
    }
}
pub fn delete_cookie(s: &mut impl Runtime, id: i64, sync: bool, cause: i32) {
    if s.failed() {
        return;
    }
    let cookie = s.cookie(id, false);
    let (change, notify) = deletion_mapping(cause);
    s.n(Op::OutputScalar, 3, i64::from(sync));
    s.n(Op::OutputScalar, 4, i64::from(cause));
    s.n(Op::OutputScalar, 6, i64::from(change));
    s.n(Op::Log, 0, id);
    if cause != 5 {
        s.n(Op::Log, 1, id);
    }
    if s.failed() {
        return;
    }
    if should_persist(&cookie, s) && sync {
        s.n(Op::PersistentDelete, id, 0);
    }
    if s.failed() {
        return;
    }
    let access_semantics = semantics(s, 1, id) as i32;
    let scope_semantics = semantics(s, 2, id) as i32;
    let mut deletion_access = base::AccessResult::default();
    deletion_access.access_semantics = access_semantics;
    deletion_access.scope_semantics = scope_semantics;
    deletion_access.allowed_secure = true;
    s.access_out(2, deletion_access);
    // The cookie remains in its native map through notification. A change
    // callback must not synchronously mutate the cookie it is observing.
    s.n(Op::Select, id, 0);
    s.n(Op::OutputScalar, 7, 2);
    s.n(Op::Notify, i64::from(change), i64::from(notify));
    if s.failed() {
        return;
    }
    let cookie = s.cookie(id, false);
    if cookie.partition.is_some() {
        let size = cookie.size();
        let total = counter(s, 3);
        s.n(Op::SetCounter, 3, (total - size) as i64);
        s.n(Op::Select, id, 0);
        let bytes = counter(s, 8);
        s.n(Op::SetCounter, 8, (bytes - size) as i64);
        if cookie.partition.is_some_and(|p| p.nonce) {
            let nonced = counter(s, 4);
            s.n(Op::SetCounter, 4, (nonced - size) as i64);
        }
        s.n(Op::EraseNative, id, 1);
        let total = counter(s, 1);
        s.n(Op::SetCounter, 1, (total - 1) as i64);
        if cookie.partition.is_some_and(|p| p.nonce) {
            let nonced = counter(s, 2);
            s.n(Op::SetCounter, 2, (nonced - 1) as i64);
        }
        let total = counter(s, 1);
        let nonced = counter(s, 2);
        check(s, 2, total >= nonced);
    } else {
        let prev = s.n(Op::Neighbor, id, -1) != 0;
        let next = s.n(Op::Neighbor, id, 1) != 0;
        if !prev && !next {
            let keys = counter(s, 5);
            s.n(Op::SetCounter, 5, (keys - 1) as i64);
        }
        s.n(Op::EraseNative, id, 0);
    }
}
pub fn insert_cookie(
    s: &mut impl Runtime,
    id: i64,
    sync: bool,
    dispatch: bool,
    observability: i32,
) -> i64 {
    if s.failed() {
        return 0;
    }
    let cookie = s.cookie(id, false);
    s.n(Op::OutputScalar, 3, i64::from(sync));
    s.n(Op::Log, 2, id);
    if should_persist(&cookie, s) && sync {
        s.n(Op::PersistentAdd, id, 0);
    }
    if s.failed() {
        return 0;
    }
    let cookie = s.cookie(id, false);
    let partitioned = cookie.partition.is_some();
    if partitioned {
        let size = cookie.size();
        let total = counter(s, 3);
        s.n(Op::SetCounter, 3, (total + size) as i64);
        s.n(Op::Select, id, 0);
        let bytes = counter(s, 8);
        s.n(Op::SetCounter, 8, (bytes + size) as i64);
        if cookie.partition.is_some_and(|p| p.nonce) {
            let total = counter(s, 4);
            s.n(Op::SetCounter, 4, (total + size) as i64);
        }
    }
    let inserted = s.n(Op::InsertNative, id, i64::from(partitioned));
    if s.failed() {
        return 0;
    }
    if partitioned {
        let total = counter(s, 1);
        s.n(Op::SetCounter, 1, (total + 1) as i64);
        if cookie.partition.is_some_and(|p| p.nonce) {
            let total = counter(s, 2);
            s.n(Op::SetCounter, 2, (total + 1) as i64);
        }
        let total = counter(s, 1);
        let nonced = counter(s, 2);
        check(s, 2, total >= nonced);
    } else if s.n(Op::Neighbor, inserted, -1) == 0 {
        let keys = counter(s, 5);
        s.n(Op::SetCounter, 5, (keys + 1) as i64);
    }
    if s.n(Op::Sample, 0, 0) != 0 {
        let access = s.access(0);
        let none = s.effectively_none(inserted, access.access_semantics);
        let current = s.cookie(inserted, false);
        let bits =
            stored_cookie_type(none, current.http_only, current.secure, current.persistent());
        s.n(Op::Metric, 0, i64::from(bits));
        let source_type = s.cookie(inserted, false).source_type;
        s.n(Op::Metric, 23, i64::from(source_type));
    }
    let access = s.access(0);
    check(s, 3, include(access));
    if dispatch {
        s.n(Op::Select, inserted, 0);
        s.n(Op::OutputScalar, 7, 0);
        s.n(Op::Notify, i64::from(insertion_cause(observability)), 1);
    }
    inserted
}
pub fn update_access(s: &mut impl Runtime, id: i64, current: i64) {
    let cookie = s.cookie(id, false);
    s.n(Op::Select, id, 0);
    let elapsed = s.n(Op::DateArithmetic, 0, current);
    if elapsed < argument(s, 0) {
        return;
    }
    s.n(Op::SetAccess, id, current);
    if should_persist(&cookie, s) {
        s.n(Op::PersistentAccess, id, 0);
    }
}

pub fn expired_range(s: &mut impl Runtime, cursor: i64, current_time: i64, collect: bool) -> usize {
    let mut removed = 0;
    while !end(s, cursor) {
        let id = current(s, cursor);
        next(s, cursor);
        let cookie = s.cookie(id, false);
        if cookie.expired(current_time) {
            delete_cookie(s, id, true, 2);
            removed += 1;
        } else if collect {
            s.n(Op::OutputCookie, id, 0);
        }
    }
    removed
}
pub fn find(s: &mut impl Runtime, partitioned: bool) {
    if partitioned && s.n(Op::PartitionFind, 0, 0) == 0 {
        return;
    }
    let now = s.n(Op::Now, 0, 0);
    let domain = s.bytes(Op::Key, 0, 0);
    let key = s.get_key(&domain);
    emit_key(s, &key);
    let cursor = s.n(Op::OpenKeyRange, i64::from(partitioned), 0);
    expired_range(s, cursor, now, true);
}
pub fn store_loaded(s: &mut impl Runtime) {
    let mut controls = Vec::new();
    let input = range(s, 3);
    while !end(s, input) {
        let id = current(s, input);
        next(s, input);
        let cookie = s.cookie(id, false);
        let key = s.get_key(&cookie.domain);
        emit_key(s, &key);
        let mut access = base::AccessResult::default();
        access.access_semantics = -1;
        s.access_out(0, access);
        let inserted = insert_cookie(s, id, false, false, 1);
        let current = s.cookie(inserted, true);
        if contains_control(&current.name) || contains_control(&current.value) {
            controls.push((inserted, current.partition.is_some()));
        }
        let access = s.cookie(inserted, false).access;
        let earliest = state(s, 6);
        if earliest == 0 || access < earliest {
            set_state(s, 6, access);
        }
    }
    // Original removes unpartitioned controls first, then partitioned ones.
    for partitioned in [false, true] {
        for (id, is_partitioned) in controls.iter().copied() {
            if is_partitioned == partitioned {
                delete_cookie(s, id, true, 11);
            }
        }
    }
    ensure_valid(s);
}
pub fn trim_duplicates(s: &mut impl Runtime, cursor: i64) {
    let mut groups: Vec<Vec<i64>> = Vec::new();
    while !end(s, cursor) {
        let id = current(s, cursor);
        next(s, cursor);
        if let Some(group) = groups.iter_mut().find(|group| s.equivalent(id, group[0], 0)) {
            group.push(id);
        } else {
            groups.push(vec![id]);
        }
    }
    // The original map visits UniqueCookieKey groups in key order; equal
    // creation times retain insertion order as in its multiset.
    groups.sort_by(|a, b| s.key_compare(a[0], b[0], 0).cmp(&0));
    for mut group in groups {
        if group.len() < 2 {
            continue;
        }
        group.sort_by_key(|id| std::cmp::Reverse(s.cookie(*id, false).creation));
        s.n(Op::Select, group[0], 0);
        s.n(Op::Log, 3, (group.len() - 1) as i64);
        for id in group.into_iter().skip(1) {
            delete_cookie(s, id, true, 4);
        }
    }
}
pub fn ensure_valid(s: &mut impl Runtime) {
    for selector in [0, 1] {
        let cursor = range(s, selector);
        while !end(s, cursor) {
            let sub = s.n(Op::OpenKeyRange, 3, cursor);
            trim_duplicates(s, sub);
        }
    }
}

pub fn filter(s: &mut impl Runtime) {
    let now = s.n(Op::Now, 0, 0);
    record_stats(s, now);
    if s.n(Op::Delegate, -1, 0) != 0 {
        s.n(Op::Delegate, 0, 0);
    }
    let include_unpartitioned = include_unpartitioned(s);
    let mut names = BTreeSet::new();
    let mut results = Vec::new();
    let cursor = range(s, 8);
    while !end(s, cursor) {
        let id = current(s, cursor);
        next(s, cursor);
        let mut access = s.include(id);
        let cookie = s.cookie(id, false);
        if !include_unpartitioned && cookie.partition.is_none() {
            exclude(&mut access, 26);
        }
        let mut reduced = access;
        reduced.status = status::apply(reduced.status, 1, 4, 0);
        if include(reduced) && !warning(reduced, 9) && !warning(reduced, 8) && cookie.host_only() {
            names.insert(cookie.name.clone());
        }
        results.push((id, access));
    }
    for (id, mut access) in results {
        if s.n(Op::Sample, 0, 0) != 0 && include(access) {
            read_metrics(s, id);
        }
        let cookie = s.cookie(id, false);
        let scheme_mismatch = has(access, 21) || warning(access, 9);
        if !cookie.host_only() && !scheme_mismatch && names.contains(&cookie.name) {
            if s.n(Op::Feature, 0, 0) != 0 {
                exclude(&mut access, 22);
            } else {
                warn(&mut access, 11);
            }
        }
        s.access_out(0, access);
        if !include(access) {
            if s.n(Op::Argument, 1, 0) != 0 {
                s.n(Op::OutputCookie, id, 1);
            }
            continue;
        }
        if s.n(Op::Argument, 2, 0) != 0 {
            update_access(s, id, now);
        }
        s.n(Op::OutputCookie, id, 2);
    }
}
pub fn include_unpartitioned(s: &mut impl Runtime) -> bool {
    if s.n(Op::Count, 20, 0) == 0 || s.n(Op::PartitionKeys, 0, 0) != 0 {
        return true;
    }
    let cursor = range(s, 9);
    while !end(s, cursor) {
        let key = current(s, cursor);
        next(s, cursor);
        if s.n(Op::InputPartition, key, 0) == 0 {
            return true;
        }
    }
    false
}
pub fn cookie_sort(left: &Cookie, right: &Cookie) -> std::cmp::Ordering {
    right.path.len().cmp(&left.path.len()).then_with(|| left.creation.cmp(&right.creation))
}

pub fn maybe_overwrite(
    s: &mut impl Runtime,
    new: i64,
    allowed: bool,
    skip_http_only: bool,
    expired: bool,
    access: &mut base::AccessResult,
) -> i32 {
    let mut found = false;
    let mut candidate = None;
    let mut skipped = None;
    let legacy = semantics(s, 2, new) == 1;
    let cursor = s.n(Op::OpenKeyRange, 2, new);
    while !end(s, cursor) {
        let id = current(s, cursor);
        let old = s.cookie(id, false);
        if old.secure && !allowed && s.secure_matching(new, id) {
            skipped = Some(id);
            s.n(Op::Log, 4, id);
            *access = s.access(0);
            exclude(access, 12);
            s.access_out(0, *access);
        }
        if s.equivalent(new, id, 0) || (legacy && s.equivalent(new, id, 2)) {
            check(s, 4, !found);
            found = true;
            if skip_http_only && old.http_only {
                s.n(Op::Log, 5, id);
                *access = s.access(0);
                exclude(access, 13);
                s.access_out(0, *access);
            } else {
                candidate = Some(id);
            }
        }
        next(s, cursor);
    }
    let mut observable = 1;
    if let Some(id) = candidate {
        if s.value_equal(id, new) {
            let creation = s.cookie(id, false).creation;
            s.n(Op::OutputScalar, 0, creation);
            observable = 2;
        }
        if s.web_equal(id, new) {
            observable = 0;
        }
        if include(*access) {
            delete_cookie(s, id, true, if expired { 10 } else { 1 });
            *access = s.access(0);
        } else if has(*access, 12) {
            check(s, 5, skipped.is_some());
            s.n(Op::Log, 6, id);
        }
    }
    observable
}
pub fn set_cookie(s: &mut impl Runtime) {
    let collect = s.n(Op::Sample, 0, 0) != 0;
    let new = s.n(Op::InputCookie, 0, 0);
    if s.n(Op::Delegate, -1, 0) != 0 {
        s.n(Op::Delegate, 0, 0);
    }
    activate_legacy(s);
    let mut access = s.set_permitted(new);
    let mut cookie = s.cookie(new, false);
    let key = s.get_key(&cookie.domain);
    emit_key(s, &key);
    if cookie.creation == 0 {
        cookie.creation = s.n(Op::Now, 0, 0);
        s.n(Op::SetCreation, new, cookie.creation);
    }
    let expired = cookie.expired(cookie.creation);
    let try_duplicates = cookie.partition.is_none() || s.n(Op::PartitionFind, new, 0) != 0;
    let mut observable = 1;
    if try_duplicates {
        let skip = s.n(Op::Argument, 3, 0) != 0;
        observable = maybe_overwrite(s, new, access.allowed_secure, skip, expired, &mut access);
    }
    if has(access, 12) || has(access, 13) {
        s.n(Op::Log, 7, new);
    }
    if include(access) {
        s.n(Op::Log, 8, new);
        let none = s.effectively_none(new, -1);
        if none && collect {
            let size = s.cookie(new, false).size();
            s.n(Op::Metric, 2, size as i64);
            let partitioned = s.cookie(new, false).partition.is_some();
            s.n(Op::Metric, if partitioned { 24 } else { 25 }, size as i64);
        }
        if !expired {
            if collect {
                persistence_metrics(s, new);
                let domain_cookie = !s.cookie(new, false).host_only();
                s.n(Op::Metric, 4, i64::from(domain_cookie));
            }
            let inherit = s.n(Op::Argument, 5, 0);
            if inherit != 0 {
                s.n(Op::SetCreation, new, inherit);
            }
            s.access(0);
            insert_cookie(s, new, true, true, observable);
        } else {
            s.n(Op::Log, 9, new);
        }
        if cookie.partition.is_some() {
            garbage_collect_partition(s, cookie.creation, &key);
        } else {
            garbage_collect(s, cookie.creation, &key);
        }
        if collect {
            let local = argument(s, 64) != 0;
            let port = argument(s, 65);
            s.n(Op::Metric, if local { 5 } else { 28 }, port);
            let scheme = argument(s, 66);
            s.n(Op::Metric, 29, scheme);
        }
        access = s.access(0);
    } else {
        access.status = status::apply(access.status, 7, 12, 0);
    }
    s.access_out(0, access);
    s.n(Op::RunCallback, 2, 0);
}

fn lra_sort(s: &mut impl Runtime, cookies: &mut [i64]) {
    sort_ids(s, cookies, 1);
}
fn sort_ids(s: &mut impl Runtime, cookies: &mut [i64], kind: i64) {
    // Standard-library rearrangement remains a native container primitive.
    // Its comparator is pure Rust. This preserves the configured C++ sort's
    // observable tied-element order instead of silently substituting stable sort.
    s.n(Op::OutputList, 4, 0);
    for id in cookies.iter() {
        s.n(Op::OutputCookie, *id, 3);
    }
    s.n(Op::Sort, kind, 0);
    for (index, id) in cookies.iter_mut().enumerate() {
        *id = s.n(Op::Argument, 20, index as i64);
    }
}
pub fn purge(
    s: &mut impl Runtime,
    cookies: &mut Vec<i64>,
    priority: i32,
    protect: usize,
    goal: usize,
    secure_protected: bool,
    obc: bool,
) -> usize {
    let total = cookies.iter().filter(|id| s.cookie(**id, false).priority == priority).count();
    if total <= protect {
        return 0;
    }
    let mut eligible: Vec<i64> = cookies
        .iter()
        .copied()
        .filter(|id| {
            let cookie = s.cookie(*id, false);
            cookie.priority == priority
                && if obc {
                    crate::base::is_secure(
                        cookie.secure,
                        s.n(Op::Feature, 0, 0) != 0,
                        cookie.source_scheme,
                    ) == !secure_protected
                } else {
                    !secure_protected || !cookie.secure
                }
        })
        .collect();
    let allowed = if obc {
        eligible.len().min(total - protect)
    } else if secure_protected {
        let secure = cookies
            .iter()
            .filter(|id| {
                let c = s.cookie(**id, false);
                c.priority == priority && c.secure
            })
            .count();
        total - total.min(secure.max(protect))
    } else {
        total - protect
    };
    if obc {
        eligible.sort_by_key(|id| s.cookie(*id, false).host_only());
    }
    let mut deleted = 0;
    for id in eligible.into_iter().take(goal.min(allowed)) {
        s.n(Op::OutputScalar, 8, id);
        delete_cookie(s, id, true, 6);
        if s.failed() {
            break;
        }
        s.n(Op::OutputScalar, 9, id);
        cookies.retain(|candidate| *candidate != id);
        deleted += 1;
    }
    deleted
}
pub fn garbage_collect(s: &mut impl Runtime, now: i64, key: &[u8]) -> usize {
    emit_key(s, key);
    let wall = s.n(Op::Now, 0, 0);
    let safe = s.n(Op::DateArithmetic, 1, wall);
    let partially = s.n(Op::Feature, 1, 0) != 0;
    let obc = partially && activate_legacy_impl(s, true) != 1;
    let mut deleted = 0;
    if count(s, 0) > DOMAIN_MAX {
        s.n(Op::Log, 12, 0);
        let cursor = s.n(Op::OpenKeyRange, 0, 0);
        let mut cookies = Vec::new();
        while !end(s, cursor) {
            let id = current(s, cursor);
            next(s, cursor);
            if s.cookie(id, false).expired(now) {
                delete_cookie(s, id, true, 2);
                deleted += 1;
            } else {
                cookies.push(id);
            }
        }
        if cookies.len() > DOMAIN_MAX {
            s.n(Op::Log, 13, 0);
            if counter(s, 6) < MAX_PURGED_KEYS {
                s.n(Op::KeysInsert, 1, 0);
            }
            let mut goal = cookies.len() - (DOMAIN_MAX - DOMAIN_PURGE);
            lra_sort(s, &mut cookies);
            for (priority, protect_secure) in PURGE_ROUNDS {
                if goal == 0 {
                    break;
                }
                let removed = purge(
                    s,
                    &mut cookies,
                    priority,
                    QUOTAS[priority as usize],
                    goal,
                    protect_secure,
                    obc,
                );
                goal -= removed;
                deleted += removed;
            }
            check(s, 6, goal == 0);
        }
    }
    if counter(s, 0) > GLOBAL_MAX && state(s, 6) < safe {
        s.n(Op::Log, 14, 0);
        let cursor = range(s, 0);
        let mut cookies = Vec::new();
        while !end(s, cursor) {
            let id = current(s, cursor);
            next(s, cursor);
            if s.cookie(id, false).expired(now) {
                delete_cookie(s, id, true, 2);
                deleted += 1;
            } else {
                cookies.push(id);
            }
        }
        if cookies.len() > GLOBAL_MAX {
            s.n(Op::Log, 15, 0);
            let goal = cookies.len() - (GLOBAL_MAX - GLOBAL_PURGE);
            let (mut secure, mut insecure): (Vec<i64>, Vec<i64>) =
                cookies.into_iter().partition(|id| s.cookie(*id, false).secure);
            let goal_nonsecure = goal.min(insecure.len());
            let (removed, earliest_insecure) = purge_lra(s, &mut insecure, safe, goal_nonsecure);
            deleted += removed;
            if secure.is_empty() {
                set_state(s, 6, earliest_insecure);
            } else if removed < goal {
                let goal_secure = (goal - removed).min(secure.len());
                let (removed_secure, earliest_secure) =
                    purge_lra(s, &mut secure, safe, goal_secure);
                deleted += removed_secure;
                set_state(
                    s,
                    6,
                    if earliest_insecure != 0 && earliest_insecure < earliest_secure {
                        earliest_insecure
                    } else {
                        earliest_secure
                    },
                );
            }
        }
    }
    deleted
}
pub fn purge_lra(
    s: &mut impl Runtime,
    cookies: &mut Vec<i64>,
    safe: i64,
    goal: usize,
) -> (usize, i64) {
    check(s, 7, goal <= cookies.len());
    // Keep native partial_sort as a container algorithm service with the exact
    // original chosen prefix, including its recorded off-by-one comment debt.
    // A full sort would change the original unsorted-next earliest-time result.
    s.n(Op::OutputList, 4, 0);
    for id in cookies.iter() {
        s.n(Op::OutputCookie, *id, 3);
    }
    s.n(Op::NativePartialSort, goal as i64, 0);
    for (index, id) in cookies.iter_mut().enumerate() {
        *id = s.n(Op::Argument, 20, index as i64);
    }
    let mut boundary = 0;
    while boundary < goal && s.cookie(cookies[boundary], false).access < safe {
        boundary += 1;
    }
    for id in cookies[..boundary].iter().copied() {
        delete_cookie(s, id, true, 7);
    }
    let earliest = if let Some(id) = cookies.get(boundary) {
        s.cookie(*id, false).access
    } else {
        argument(s, 72)
    };
    (boundary, earliest)
}
pub fn garbage_collect_partition(s: &mut impl Runtime, now: i64, key: &[u8]) -> usize {
    emit_key(s, key);
    if s.n(Op::PartitionFind, 0, 0) == 0 {
        return 0;
    }
    let measure = s.n(Op::OpenKeyRange, 1, 0);
    let mut total_bytes = 0;
    while !end(s, measure) {
        let id = current(s, measure);
        next(s, measure);
        total_bytes += s.cookie(id, false).size();
    }
    if total_bytes <= PARTITION_BYTES && count(s, 1) <= PARTITION_MAX {
        return 0;
    }
    let cursor = s.n(Op::OpenKeyRange, 1, 0);
    let mut cookies = Vec::new();
    let mut removed = 0;
    while !end(s, cursor) {
        let id = current(s, cursor);
        next(s, cursor);
        if s.cookie(id, false).expired(now) {
            delete_cookie(s, id, true, 2);
            removed += 1;
        } else {
            cookies.push(id);
        }
    }
    let mut bytes: usize = cookies.iter().map(|id| s.cookie(*id, false).size()).sum();
    if bytes <= PARTITION_BYTES && cookies.len() <= PARTITION_MAX {
        return removed;
    }
    let obc = s.n(Op::Feature, 1, 0) != 0 && s.scope_for_key() != 1;
    lra_sort(s, &mut cookies);
    if obc {
        cookies.sort_by_key(|id| s.cookie(*id, false).host_only());
    }
    let original = cookies.len();
    for (index, id) in cookies.into_iter().enumerate() {
        if bytes <= PARTITION_BYTES && original - index <= PARTITION_MAX {
            break;
        }
        bytes -= s.cookie(id, false).size();
        delete_cookie(s, id, true, 13);
        removed += 1;
    }
    removed
}

pub fn activate_legacy(s: &mut impl Runtime) -> i32 {
    activate_legacy_impl(s, false)
}
fn activate_legacy_impl(s: &mut impl Runtime, use_key: bool) -> i32 {
    let domain = s.bytes(Op::Key, i64::from(use_key), 0);
    let key = s.get_key(&domain);
    emit_key(s, &key);
    let semantics = if use_key { s.scope_for_key() } else { semantics(s, 2, -1) as i32 };
    if s.n(Op::PrefPresent, 0, 0) == 0 {
        let from_pref = s.n(Op::PrefPresent, 1, 0) != 0 && s.n(Op::PrefReady, 0, 0) != 0;
        s.n(Op::PrefInit, i64::from(from_pref), 0);
    }
    let present = s.n(Op::PrefContains, 0, 0) != 0;
    if !present && semantics == 1 {
        delete_aliases(s);
        s.n(Op::PrefSet, 0, 0);
        if s.n(Op::PrefPresent, 1, 0) != 0 {
            s.n(Op::PrefWrite, 0, 0);
        }
    } else if present && semantics != 1 {
        s.n(Op::PrefRemove, 0, 0);
        if s.n(Op::PrefPresent, 1, 0) != 0 {
            s.n(Op::PrefWrite, 0, 0);
        }
    }
    semantics
}
pub fn delete_aliases(s: &mut impl Runtime) {
    let mut partitions = vec![0];
    let cursor = range(s, 1);
    while !end(s, cursor) {
        partitions.push(current(s, cursor));
        next(s, cursor);
    }
    for partition in partitions {
        let cursor = s.n(Op::OpenKeyRange, 4, partition);
        let mut most_recent: Vec<i64> = Vec::new();
        let mut ids = Vec::new();
        while !end(s, cursor) {
            let id = current(s, cursor);
            next(s, cursor);
            ids.push(id);
            if let Some(existing) =
                most_recent.iter_mut().find(|other| s.equivalent(id, **other, 2))
            {
                if s.cookie(id, false).creation > s.cookie(*existing, false).creation {
                    *existing = id;
                }
            } else {
                most_recent.push(id);
            }
        }
        for id in ids {
            let recent = most_recent.iter().find(|other| s.equivalent(id, **other, 2)).copied();
            if let Some(recent) = recent {
                if !s.equivalent(id, recent, 1) {
                    delete_cookie(s, id, true, 14);
                }
            }
        }
    }
}
pub fn record_stats(s: &mut impl Runtime, now: i64) {
    let elapsed = s.n(Op::DateArithmetic, 2, now);
    let interval = argument(s, 30);
    if elapsed <= interval {
        return;
    }
    if stats(s) {
        set_state(s, 7, now);
    }
}
pub fn stats(s: &mut impl Runtime) -> bool {
    if state(s, 1) != 0 && state(s, 2) == 0 {
        return false;
    }
    let total_count = counter(s, 0);
    s.n(Op::Metric, 10, total_count as i64);
    let mut same_site: BTreeMap<Vec<u8>, usize> = BTreeMap::new();
    let mut bytes_per_key = BTreeMap::new();
    let mut total = 0_usize;
    let cursor = range(s, 0);
    while !end(s, cursor) {
        let id = current(s, cursor);
        let cookie = s.cookie(id, false);
        let key = s.bytes(Op::NativeRangeKey, id, 0);
        let bytes = cookie.size();
        total += bytes;
        *bytes_per_key.entry(key.clone()).or_insert(0) += bytes;
        if s.effectively_none(id, -1) {
            *same_site.entry(key).or_insert(0) += 1;
        }
        next(s, cursor);
    }
    s.n(Op::Metric, 11, same_site.values().copied().max().unwrap_or(0) as i64);
    s.n(Op::Metric, 12, (total >> 10) as i64);
    let keys = counter(s, 5).max(1);
    s.n(Op::Metric, 13, (total / keys) as i64);
    s.n(Op::Metric, 14, (bytes_per_key.values().copied().max().unwrap_or(0) >> 10) as i64);
    for (metric, counter_id) in [(15, 7), (16, 1), (17, 2), (19, 3), (20, 4)] {
        let mut value = counter(s, counter_id);
        if metric >= 19 {
            value >>= 10;
        }
        s.n(Op::Metric, metric, value as i64);
    }
    let total_count = counter(s, 1);
    let nonced = counter(s, 2);
    s.n(Op::Metric, 18, (total_count - nonced) as i64);
    let total_bytes = counter(s, 3);
    let nonced_bytes = counter(s, 4);
    s.n(Op::Metric, 21, ((total_bytes - nonced_bytes) >> 10) as i64);
    let cursor = range(s, 10);
    while !end(s, cursor) {
        let bytes = current(s, cursor);
        next(s, cursor);
        s.n(Op::Metric, 22, bytes >> 10);
    }
    true
}

pub fn same_port(
    source: i32,
    destination: i32,
    source_scheme: i32,
    destination_default: i32,
) -> i32 {
    if source == -1 {
        return 0;
    }
    if source == -2 {
        return 1;
    }
    if source == destination {
        return 4;
    }
    let source_default = if source_scheme == 2 { 443 } else { 80 };
    if destination == destination_default && source == source_default {
        3
    } else {
        2
    }
}
pub fn scheme_allowed(scheme: &[u8], allowed: &[Vec<u8>]) -> bool {
    allowed.iter().any(|item| item == scheme)
}
pub fn stored_cookie_type(
    same_site_none: bool,
    http_only: bool,
    secure: bool,
    persistent: bool,
) -> i32 {
    i32::from(!same_site_none)
        | (i32::from(http_only) << 1)
        | (i32::from(secure) << 2)
        | (i32::from(persistent) << 3)
}
pub fn contains_control(input: &[u8]) -> bool {
    input.iter().any(|byte| *byte < 0x20 || *byte == 0x7f)
}
pub fn persistence_metrics(s: &mut impl Runtime, id: i64) {
    let creation = s.cookie(id, false).creation;
    let persistent = s.cookie(id, false).persistent();
    s.n(Op::Metric, 3, i64::from(persistent));
    if !s.cookie(id, false).persistent() {
        return;
    }
    s.n(Op::Select, id, 0);
    s.n(Op::OutputScalar, 5, creation);
    let minutes = s.n(Op::DateArithmetic, 3, 0);
    let secure = s.cookie(id, false).secure;
    s.n(Op::Metric, if secure { 26 } else { 27 }, minutes);
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn notification_causes_separate_visibility() {
        assert_eq!(insertion_cause(0),7);
        assert_eq!(insertion_cause(1),0);
        assert_eq!(insertion_cause(2),8);
        assert_eq!(deletion_mapping(10),(6,true));
        assert_eq!(deletion_mapping(14),(5,false));
        assert_eq!(deletion_mapping(5),(1,false));
    }
    #[test]
    fn control_bytes_include_del_but_not_high_bit() {
        for byte in 0..=255u8 {
            assert_eq!(contains_control(&[byte]),byte<32||byte==127);
        }
    }
    #[test]
    fn sort_path_then_creation_with_exact_ties() {
        let mut a = Cookie::default();
        let mut b = Cookie::default();
        a.path = b"/long".to_vec();
        b.path = b"/".to_vec();
        assert!(cookie_sort(&a,&b).is_lt());
        b.path = a.path.clone();
        a.creation = 3;
        b.creation = 4;
        assert!(cookie_sort(&a,&b).is_lt());
        b.creation = 3;
        assert!(cookie_sort(&a,&b).is_eq());
    }
    #[test]
    fn source_port_sentinels_and_default_transitions() {
        assert_eq!(same_port(-1,443,0,443),0);
        assert_eq!(same_port(-2,443,0,443),1);
        assert_eq!(same_port(80,80,1,80),4);
        assert_eq!(same_port(80,443,1,443),3);
        assert_eq!(same_port(81,443,1,443),2);
        assert_eq!(same_port(443,80,2,80),3);
    }
    #[test]
    fn stored_type_bits_are_orthogonal() {
        assert_eq!(stored_cookie_type(true,false,false,false),0);
        assert_eq!(stored_cookie_type(false,false,false,false),1);
        assert_eq!(stored_cookie_type(true,true,true,true),14);
        assert_eq!(stored_cookie_type(false,true,true,true),15);
    }
    #[test]
    fn scheme_matching_is_exact_bytes() {
        let schemes = vec![b"http".to_vec(),b"https".to_vec()];
        assert!(scheme_allowed(b"https",&schemes));
        assert!(!scheme_allowed(b"HTTPS",&schemes));
        assert!(!scheme_allowed(b"http\0",&schemes));
    }
}
fn read_metrics(s: &mut impl Runtime, id: i64) {
    let destination = argument(s, 65);
    let local = argument(s, 64) != 0;
    s.n(Op::Metric, if local { 6 } else { 7 }, destination);
    let cookie = s.cookie(id, false);
    let default = argument(s, 67) as i32;
    if cookie.source_port != -1
        && cookie.source_port != -2
        && cookie.source_port != destination as i32
    {
        check(s, 11, cookie.source_scheme != 0);
    }
    let same = same_port(cookie.source_port, destination as i32, cookie.source_scheme, default);
    s.n(Op::Metric, if local { 8 } else { 9 }, i64::from(same));
    let cookie = s.cookie(id, false);
    if !cookie.host_only() {
        let current_destination = argument(s, 65) as i32;
        let current_default = argument(s, 67) as i32;
        let same = same_port(
            cookie.source_port,
            current_destination,
            cookie.source_scheme,
            current_default,
        );
        s.n(Op::Metric, 32, i64::from(same));
    }
}

pub fn time_range_contains(start: i64, end: i64, value: i64) -> bool {
    if start != 0 && start == end {
        return value == start;
    }
    (start == 0 || start <= value) && (end == 0 || value < end)
}
pub fn all_expired_partitioned(s: &mut impl Runtime, now: i64) {
    let partitions = range(s, 1);
    while !end(s, partitions) {
        let partition = current(s, partitions);
        next(s, partitions);
        s.n(Op::SelectPartition, partition, 0);
        let cursor = range(s, 4);
        expired_range(s, cursor, now, false);
    }
}
pub fn get_all(s: &mut impl Runtime) {
    let now = s.n(Op::Now, 0, 0);
    let cursor = range(s, 0);
    expired_range(s, cursor, now, false);
    let now = s.n(Op::Now, 0, 0);
    all_expired_partitioned(s, now);
    let mut ids = Vec::new();
    let cursor = range(s, 0);
    while !end(s, cursor) {
        ids.push(current(s, cursor));
        next(s, cursor);
    }
    let partitions = range(s, 1);
    while !end(s, partitions) {
        let partition = current(s, partitions);
        next(s, partitions);
        s.n(Op::SelectPartition, partition, 0);
        let cursor = range(s, 4);
        while !end(s, cursor) {
            ids.push(current(s, cursor));
            next(s, cursor);
        }
    }
    sort_ids(s, &mut ids, 0);
    s.n(Op::OutputList, 0, ids.len() as i64);
    for id in ids {
        s.n(Op::OutputCookie, id, 4);
    }
    s.n(Op::RunCallback, 3, 0);
}
pub fn has_scheme(s: &mut impl Runtime) -> bool {
    let scheme = s.bytes(Op::Scheme, 0, 0);
    let count = count(s, 30);
    for index in 0..count {
        let allowed = s.bytes(Op::AllowedScheme, index as i64, 0);
        let allowed = allowed.split(|byte| *byte == 0).next().unwrap_or_default();
        check(s, 12, allowed.is_ascii());
        check(s, 13, !allowed.iter().any(u8::is_ascii_uppercase));
        if allowed == scheme {
            return true;
        }
    }
    s.n(Op::Log, 10, 0);
    false
}
pub fn get_list(s: &mut impl Runtime) {
    let timer = s.n(Op::Sample, 0, 0) != 0;
    if timer {
        s.n(Op::TimerBegin, 0, 0);
    }
    s.n(Op::OutputList, 1, 0);
    if has_scheme(s) {
        activate_legacy(s);
        find(s, false);
        if count(s, 20) != 0 {
            let all = s.n(Op::PartitionKeys, 0, 0) != 0;
            let cursor = range(s, if all { 1 } else { 9 });
            while !end(s, cursor) {
                let partition = current(s, cursor);
                next(s, cursor);
                s.n(Op::SelectPartition, partition, 0);
                find(s, true);
            }
        }
        let cursor = range(s, 8);
        let mut ids = Vec::new();
        while !end(s, cursor) {
            ids.push(current(s, cursor));
            next(s, cursor);
        }
        sort_ids(s, &mut ids, 0);
        s.n(Op::OutputList, 2, ids.len() as i64);
        for id in ids {
            s.n(Op::OutputCookie, id, 5);
        }
        filter(s);
    }
    s.n(Op::RunCallback, 4, 0);
    if timer {
        s.n(Op::TimerFinish, 0, 0);
    }
}
pub fn delete_matching(s: &mut impl Runtime, time_range: bool) -> usize {
    let start = argument(s, 40);
    let finish = argument(s, 41);
    let cause = if time_range { 0 } else { argument(s, 42) as i32 };
    let mut deleted = 0;
    for selector in [0, 1] {
        let outer = range(s, selector);
        while !end(s, outer) {
            let cursor = if selector == 0 {
                outer
            } else {
                let partition = current(s, outer);
                next(s, outer);
                s.n(Op::SelectPartition, partition, 0);
                range(s, 4)
            };
            while !end(s, cursor) {
                let id = current(s, cursor);
                next(s, cursor);
                let matches = if time_range {
                    let creation = s.cookie(id, false).creation;
                    check(s, 8, creation != 0);
                    time_range_contains(start, finish, creation)
                } else {
                    s.n(Op::Predicate, id, 0) != 0
                };
                if matches {
                    delete_cookie(s, id, true, cause);
                    deleted += 1;
                }
            }
            if selector == 0 {
                break;
            }
        }
    }
    s.n(Op::WeakFlushDelete, deleted as i64, 0);
    flush(s);
    deleted
}
pub fn delete_canonical(s: &mut impl Runtime) -> usize {
    s.n(Op::TimerBegin, 1, 0);
    let target = s.n(Op::InputCookie, 0, 0);
    let partitioned = s.cookie(target, false).partition.is_some();
    let mut deleted = 0;
    if !partitioned || s.n(Op::PartitionFind, target, 0) != 0 {
        let domain = s.cookie(target, false).domain;
        let key = s.get_key(&domain);
        emit_key(s, &key);
        let cursor = s.n(Op::OpenKeyRange, i64::from(partitioned), 0);
        if !end(s, cursor) {
            // Decrypt the target once before scanning, exactly as original.
            s.n(Op::Select, target, 0);
            s.n(Op::CopyCookie, target, 2);
            while !end(s, cursor) {
                let id = current(s, cursor);
                if s.equivalent(id, target, 0) && s.value_equal(id, target) {
                    delete_cookie(s, id, true, 0);
                    deleted = 1;
                    break;
                }
                next(s, cursor);
            }
        }
    }
    s.n(Op::WeakFlushDelete, deleted, 0);
    flush(s);
    s.n(Op::TimerFinish, 1, 0);
    deleted as usize
}
pub fn set_all(s: &mut impl Runtime) {
    while !s.failed() && counter(s, 0) != 0 {
        let cursor = range(s, 0);
        let id = current(s, cursor);
        delete_cookie(s, id, true, 0);
    }
    let cursor = range(s, 3);
    while !end(s, cursor) {
        let source = current(s, cursor);
        next(s, cursor);
        let cookie = s.cookie(source, false);
        let key = s.get_key(&cookie.domain);
        emit_key(s, &key);
        if cookie.expired(cookie.creation) {
            continue;
        }
        if s.n(Op::Sample, 0, 0) != 0 {
            persistence_metrics(s, source);
        }
        let id = s.n(Op::CopyCookie, source, 0);
        let mut access = base::AccessResult::default();
        access.access_semantics = semantics(s, 1, source) as i32;
        s.access_out(0, access);
        insert_cookie(s, id, true, true, 1);
        if cookie.partition.is_some() {
            garbage_collect_partition(s, cookie.creation, &key);
        } else {
            garbage_collect(s, cookie.creation, &key);
        }
    }
    s.access_out(0, base::AccessResult::default());
    s.n(Op::RunCallback, 2, 0);
}
pub fn set_unsafe(s: &mut impl Runtime) {
    let id = s.n(Op::InputCookie, 0, 0);
    let cookie = s.cookie(id, false);
    let key = s.get_key(&cookie.domain);
    emit_key(s, &key);
    if cookie.creation == 0 {
        let now = s.n(Op::Now, 0, 0);
        s.n(Op::SetCreation, id, now);
    }
    s.access_out(0, base::AccessResult::default());
    insert_cookie(s, id, true, true, 1);
    s.n(Op::RunCallback, 2, 0);
}
pub fn loaded(s: &mut impl Runtime) {
    store_loaded(s);
    let now = s.n(Op::NowTicks, 0, 0);
    s.n(Op::Metric, 30, now);
    let blocked = if state(s, 9) != 0 { s.n(Op::DateArithmetic, 4, now) } else { 0 };
    s.n(Op::Metric, 31, blocked);
    invoke_queue(s);
}
pub fn site_in_other_partition(s: &mut impl Runtime) -> Option<bool> {
    let domain = s.bytes(Op::Key, 0, 0);
    emit_key(s, &domain);
    if state(s, 4) != 0 && state(s, 2) == 0 && s.n(Op::KeysContains, 0, 0) == 0 {
        return None;
    }
    let cursor = range(s, 1);
    while !end(s, cursor) {
        let partition = current(s, cursor);
        next(s, cursor);
        if s.n(Op::PartitionEqual, partition, 0) == 0
            && s.n(Op::InputPartition, partition, 0) == 0
            && s.n(Op::Count, 31, partition) != 0
        {
            return Some(true);
        }
    }
    Some(false)
}
