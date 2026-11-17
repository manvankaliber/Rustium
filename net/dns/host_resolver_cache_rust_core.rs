// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

// Draft only. Native values, key ordering providers and ownership are separate
// primitives; all cache traversal/selection/persistence decisions live here.
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct Values {
    pub a: u64,
    pub b: u64,
    pub x: i64,
    pub y: i64,
}

#[repr(u32)]
#[derive(Clone, Copy)]
pub enum Op {
    Size = 1,
    Capacity,
    Generation,
    SetGeneration,
    Canonicalize,
    CompleteCanonical,
    OpenRange,
    StepBack,
    Query,
    Entry,
    Retain,
    RetainedCount,
    CloseMatching,
    TickNow,
    WallNow,
    AssertTimed,
    Expiration,
    Difference,
    Magnitude,
    SelectResult,
    PreparePending,
    DropPending,
    EraseRetained,
    InsertPending,
    BeginEntries,
    CursorValid,
    CursorNext,
    EraseCursor,
    KeepBest,
    EraseBest,
    CheckBest,
    CheckCapacity,
    ListReset,
    DictReset,
    DictInt,
    DictBool,
    NakValue,
    NakDebugValue,
    DictNak,
    ResultValue,
    DictResult,
    Network,
    DictNetwork,
    ListAppend,
    LoggingReset,
    CapacityInt,
    LoggingInt,
    LoggingEntries,
    RestoreList,
    RestoreDict,
    RestoreNakField,
    RestoreNak,
    RestoreSource,
    RestoreSecure,
    RestoreResult,
    RestorePending,
    RestoreReset,
}

pub trait Services {
    fn call(&mut self, op: Op, args: Values, bytes: &'static [u8]) -> Values;
}

fn call(s: &mut impl Services, op: Op, a: u64, b: u64, x: i64, y: i64) -> Values {
    s.call(op, Values { a, b, x, y }, b"")
}

fn field(s: &mut impl Services, op: Op, key: &'static [u8], a: u64, x: i64) -> Values {
    s.call(op, Values { a, x, ..Values::default() }, key)
}

const NAK: &[u8] = b"network_anonymization_key";
const SOURCE: &[u8] = b"source";
const SECURE: &[u8] = b"secure";
const RESULT: &[u8] = b"result";
const GENERATION: &[u8] = b"staleness_generation";
const MAX_ENTRIES: &[u8] = b"max_entries";
const ENTRIES: &[u8] = b"entries";
const NETWORK: &[u8] = b"target_network";
const NAK_DEBUG_PREFIX: &[u8] = b"Non-persistable network anonymization key: ";

#[derive(Clone, Copy)]
struct Query {
    kind: u64,
    secure: u64,
    source: i64,
}
#[derive(Clone, Copy)]
struct Entry {
    kind: u64,
    secure: bool,
    source: i64,
    generation: i32,
}

fn query(s: &mut impl Services) -> Query {
    let v = call(s, Op::Query, 0, 0, 0, 0);
    Query { kind: v.a, secure: v.b, source: v.x }
}
fn entry(s: &mut impl Services, id: u64) -> Entry {
    let v = call(s, Op::Entry, id, 0, 0, 0);
    Entry { kind: v.a, secure: v.b != 0, source: v.x, generation: v.y as i32 }
}
fn matches(q: Query, e: Entry) -> bool {
    (q.kind == 0 || e.kind == 0 || q.kind == e.kind)
        && (q.source == 0 || q.source == e.source)
        && (q.secure == 0 || (q.secure == 2) == e.secure)
}

pub fn collect_matches(s: &mut impl Services) {
    if call(s, Op::Size, 0, 0, 0, 0).a == 0 {
        return;
    }
    let q = query(s);
    let family = call(s, Op::Canonicalize, 0, 0, 0, 0).x;
    let use_canonical = family == 0; // Native guard binds URL's NEUTRAL ID.
    if use_canonical {
        call(s, Op::CompleteCanonical, 0, 0, 0, 0);
    }
    call(s, Op::OpenRange, u64::from(use_canonical), 0, 0, 0);
    while call(s, Op::StepBack, 0, 0, 0, 0).a != 0 {
        if matches(q, entry(s, 0)) {
            call(s, Op::Retain, 0, 0, 0, 0);
        }
    }
    // Canonicalization output lifetime ends before any clock callback.
    call(s, Op::CloseMatching, 0, 0, 0, 0);
}

fn time_until(s: &mut impl Services, id: u64, ticks: i64, wall: i64) -> i64 {
    let has_ticks = call(s, Op::Expiration, id, 0, 0, 0).a != 0;
    if !has_ticks {
        call(s, Op::AssertTimed, id, 0, 0, 0);
    }
    call(s, Op::Difference, id, u64::from(has_ticks), ticks, wall).x
}
fn stale(s: &mut impl Services, id: u64, ticks: i64, wall: i64, generation: i32) -> bool {
    entry(s, id).generation != generation || time_until(s, id, ticks, wall) < 0
}
fn current_generation(s: &mut impl Services) -> i32 {
    call(s, Op::Generation, 0, 0, 0, 0).x as i32
}

pub fn lookup(s: &mut impl Services) -> Values {
    collect_matches(s);
    let ticks = call(s, Op::TickNow, 0, 0, 0, 0).x;
    let wall = call(s, Op::WallNow, 0, 0, 0, 0).x;
    let q = query(s);
    let count = call(s, Op::RetainedCount, 0, 0, 0, 0).a;
    let mut best = 0;
    for id in 1..=count {
        call(s, Op::AssertTimed, id, 0, 0, 0);
        let generation = current_generation(s);
        if stale(s, id, ticks, wall, generation) {
            continue;
        }
        if entry(s, id).secure || q.secure == 1 {
            best = id;
            break;
        }
        if best == 0 {
            best = id;
        }
    }
    if best != 0 {
        call(s, Op::SelectResult, best, 0, 0, 0);
    }
    Values { a: u64::from(best != 0), ..Values::default() }
}

pub fn lookup_stale(s: &mut impl Services) -> Values {
    collect_matches(s);
    let ticks = call(s, Op::TickNow, 0, 0, 0, 0).x;
    let wall = call(s, Op::WallNow, 0, 0, 0, 0).x;
    let q = query(s);
    let count = call(s, Op::RetainedCount, 0, 0, 0, 0).a;
    let mut best = 0;
    let mut best_delta = 0;
    for id in 1..=count {
        call(s, Op::AssertTimed, id, 0, 0, 0);
        let delta = time_until(s, id, ticks, wall);
        let generation = current_generation(s);
        let candidate = entry(s, id);
        if !stale(s, id, ticks, wall, generation) && (candidate.secure || q.secure == 1) {
            best = id;
            best_delta = delta;
            break;
        }
        let replace = if best == 0 {
            true
        } else {
            let selected = entry(s, best);
            (!stale(s, id, ticks, wall, generation) && stale(s, best, ticks, wall, generation))
                || candidate.generation > selected.generation
                || (candidate.generation == selected.generation && delta > best_delta)
                || (candidate.generation == selected.generation
                    && delta == best_delta
                    && candidate.secure
                    && !selected.secure)
        };
        if replace {
            best = id;
            best_delta = delta;
        }
    }
    if best == 0 {
        return Values::default();
    }
    let expired = best_delta < 0;
    let expired_by = if expired { call(s, Op::Magnitude, 0, 0, best_delta, 0).x } else { 0 };
    let stale_generation = entry(s, best).generation != current_generation(s);
    call(s, Op::SelectResult, best, 0, 0, 0);
    Values { a: 1, b: u64::from(expired), x: expired_by, y: i64::from(stale_generation) }
}

pub fn set(s: &mut impl Services, replace: bool, generation: i32) {
    call(s, Op::PreparePending, 0, 0, 0, 0);
    collect_matches(s);
    let count = call(s, Op::RetainedCount, 0, 0, 0, 0).a;
    if count != 0 && !replace {
        call(s, Op::DropPending, 0, 0, 0, 0);
        return;
    }
    for id in 1..=count {
        call(s, Op::EraseRetained, id, 0, 0, 0);
    }
    call(s, Op::InsertPending, 0, 0, i64::from(generation), 0);
    if call(s, Op::Size, 0, 0, 0, 0).a > call(s, Op::Capacity, 0, 0, 0, 0).a {
        evict(s);
    }
}

pub fn evict(s: &mut impl Services) {
    let ticks = call(s, Op::TickNow, 0, 0, 0, 0).x;
    let wall = call(s, Op::WallNow, 0, 0, 0, 0).x;
    let mut stale_found = false;
    let mut best_delta = i64::MAX;
    let mut have_best = false;
    call(s, Op::BeginEntries, 0, 0, 0, 0);
    while call(s, Op::CursorValid, 0, 0, 0, 0).a != 0 {
        let generation = current_generation(s);
        if stale(s, 0, ticks, wall, generation) {
            stale_found = true;
            call(s, Op::EraseCursor, 0, 0, 0, 0);
        } else {
            let delta = time_until(s, 0, ticks, wall);
            let replace = !have_best
                || delta < best_delta
                || (delta == best_delta && entry(s, u64::MAX).secure && !entry(s, 0).secure);
            if replace {
                call(s, Op::KeepBest, 0, 0, 0, 0);
                have_best = true;
                best_delta = delta;
            }
            call(s, Op::CursorNext, 0, 0, 0, 0);
        }
    }
    if !stale_found {
        call(s, Op::CheckBest, 0, 0, 0, 0);
        call(s, Op::EraseBest, 0, 0, 0, 0);
    }
    call(s, Op::CheckCapacity, 0, 0, 0, 0);
}

pub fn serialize_entries(s: &mut impl Services, logging: bool) {
    call(s, Op::ListReset, 0, 0, 0, 0);
    call(s, Op::BeginEntries, 0, 0, 0, 0);
    while call(s, Op::CursorValid, 0, 0, 0, 0).a != 0 {
        call(s, Op::DictReset, 0, 0, 0, 0);
        if logging {
            let g = entry(s, 0).generation;
            field(s, Op::DictInt, GENERATION, 0, i64::from(g));
        }
        if call(s, Op::NakValue, 0, 0, 0, 0).a == 0 {
            if !logging {
                call(s, Op::CursorNext, 0, 0, 0, 0);
                continue;
            }
            s.call(Op::NakDebugValue, Values::default(), NAK_DEBUG_PREFIX);
        }
        let network = call(s, Op::Network, 0, 0, 0, 0).x;
        if !logging && network != -1 {
            call(s, Op::CursorNext, 0, 0, 0, 0);
            continue;
        }
        field(s, Op::DictNak, NAK, 0, 0);
        let e = entry(s, 0);
        field(s, Op::DictInt, SOURCE, 0, e.source);
        field(s, Op::DictBool, SECURE, u64::from(e.secure), 0);
        call(s, Op::ResultValue, 0, 0, 0, 0);
        field(s, Op::DictResult, RESULT, 0, 0);
        if logging {
            field(s, Op::DictNetwork, NETWORK, 0, network);
        }
        call(s, Op::ListAppend, 0, 0, 0, 0);
        call(s, Op::CursorNext, 0, 0, 0, 0);
    }
}

pub fn serialize_logging(s: &mut impl Services) {
    call(s, Op::LoggingReset, 0, 0, 0, 0);
    let max = call(s, Op::CapacityInt, 0, 0, 0, 0).x;
    field(s, Op::LoggingInt, MAX_ENTRIES, 0, max);
    let generation = current_generation(s);
    field(s, Op::LoggingInt, GENERATION, 0, i64::from(generation));
    serialize_entries(s, true);
    field(s, Op::LoggingEntries, ENTRIES, 0, 0);
}

pub fn restore(s: &mut impl Services) -> bool {
    let list = call(s, Op::RestoreList, 0, 0, 0, 0);
    if list.a == 0 {
        return false;
    }
    for index in 0..list.b {
        if call(s, Op::Size, 0, 0, 0, 0).a == call(s, Op::Capacity, 0, 0, 0, 0).a {
            return true;
        }
        if call(s, Op::RestoreDict, index, 0, 0, 0).a == 0 {
            return false;
        }
        if field(s, Op::RestoreNakField, NAK, 0, 0).a == 0 {
            return false;
        }
        if call(s, Op::RestoreNak, 0, 0, 0, 0).a == 0 {
            call(s, Op::RestoreReset, 0, 0, 0, 0);
            continue;
        }
        let source = field(s, Op::RestoreSource, SOURCE, 0, 0);
        if source.a == 0 || !(0..=4).contains(&source.x) {
            return false;
        }
        let secure = field(s, Op::RestoreSecure, SECURE, 0, 0);
        if secure.a == 0 {
            return false;
        }
        let result = field(s, Op::RestoreResult, RESULT, 0, 0);
        if result.a == 0 || result.b == 0 {
            return false;
        }
        call(s, Op::RestorePending, secure.b, 0, source.x, -1);
        let generation = current_generation(s).wrapping_sub(1);
        set(s, false, generation);
        call(s, Op::RestoreReset, 0, 0, 0, 0);
    }
    call(s, Op::CheckCapacity, 0, 0, 0, 0);
    true
}

pub fn key_less(
    domain: i32,
    nak: impl FnOnce() -> i32,
    left_network: i64,
    right_network: i64,
) -> bool {
    if domain != 0 {
        return domain < 0;
    }
    let key = nak();
    if key != 0 {
        return key < 0;
    }
    left_network < right_network
}

pub fn stale_result(stale_by_generation: bool, has_expired_by: bool) -> bool {
    stale_by_generation || has_expired_by
}

pub fn run(s: &mut impl Services, method: u32, args: Values) -> Values {
    match method {
        1 => {
            collect_matches(s);
            Values::default()
        }
        2 => lookup(s),
        3 => lookup_stale(s),
        4 => {
            set(s, args.a != 0, args.x as i32);
            Values::default()
        }
        5 => {
            evict(s);
            Values::default()
        }
        6 => {
            serialize_entries(s, args.a != 0);
            Values::default()
        }
        7 => {
            serialize_logging(s);
            Values::default()
        }
        8 => Values { a: u64::from(restore(s)), ..Values::default() },
        9 => {
            let next = current_generation(s).wrapping_add(1);
            call(s, Op::SetGeneration, 0, 0, i64::from(next), 0);
            Values::default()
        }
        10 => Values { x: time_until(s, 0, args.a as i64, args.b as i64), ..Values::default() },
        11 => Values {
            a: u64::from(stale(s, 0, args.a as i64, args.b as i64, args.x as i32)),
            ..Values::default()
        },
        _ => unreachable!("private cache operation id"),
    }
}
