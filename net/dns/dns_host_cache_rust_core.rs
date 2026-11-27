// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#![forbid(unsafe_code)]

// Only scalar snapshots and call-local indices cross into policy. The native
// services retain the public map, Values, clocks and URL/IP ownership.
pub trait Services {
    fn call(&self, op: u32, a: i64, b: i64, c: i64, d: i64) -> i64;
}
include!("dns_host_cache_rust_ops.rs");
macro_rules! q {
    ($s:expr,$op:ident) => {
        $s.call(Op::$op as u32, 0, 0, 0, 0)
    };
    ($s:expr,$op:ident,$a:expr) => {
        $s.call(Op::$op as u32, $a, 0, 0, 0)
    };
    ($s:expr,$op:ident,$a:expr,$b:expr) => {
        $s.call(Op::$op as u32, $a, $b, 0, 0)
    };
    ($s:expr,$op:ident,$a:expr,$b:expr,$c:expr) => {
        $s.call(Op::$op as u32, $a, $b, $c, 0)
    };
    ($s:expr,$op:ident,$a:expr,$b:expr,$c:expr,$d:expr) => {
        $s.call(Op::$op as u32, $a, $b, $c, $d)
    };
}
fn stale(s: &impl Services, e: i64, n: i64) -> bool {
    n - q!(s, Changes, e) > 0 || q!(s, ExpiredBy, e) >= 0
}
fn pinned(s: &impl Services, e: i64) -> bool {
    q!(s, Pin, e) == 1 && q!(s, Changes, e) == q!(s, Network)
}
fn prefer(s: &impl Services, a: i64, b: i64) -> i64 {
    if a < 0 {
        return b;
    }
    if b < 0 {
        return a;
    }
    let n1 = q!(s, Changes, a);
    let n2 = q!(s, Changes, b);
    if n1 != n2 {
        return if n1 > n2 { a } else { b };
    }
    q!(s, Dcheck, i64::from(q!(s, Secure, a) != q!(s, Secure, b)));
    let exp1 = q!(s, ExpiredBy, a) >= 0;
    let exp2 = q!(s, ExpiredBy, b) >= 0;
    if exp1 != exp2 {
        return if exp1 { b } else { a };
    }
    if q!(s, Secure, a) != 0 {
        a
    } else {
        b
    }
}
fn find(s: &impl Services) -> i64 {
    let a = q!(s, Find);
    if q!(s, IgnoreSecure) != 0 {
        prefer(s, a, q!(s, FindOther))
    } else {
        a
    }
}
fn hit(s: &impl Services, e: i64, is_stale: bool) {
    let total = q!(s, TotalHits, e).saturating_add(1).min(i64::from(i32::MAX));
    let stale = (q!(s, StaleHits, e) + i64::from(is_stale)).min(i64::from(i32::MAX));
    q!(s, SetHits, e, total, stale);
}
fn lookup(s: &impl Services, allow_stale: bool) -> i64 {
    if q!(s, Max) == 0 {
        return -1;
    }
    let e = find(s);
    if e < 0 {
        if q!(s, Metrics) != 0 {
            q!(s, LookupMetric, 0, -1);
        }
        return -1;
    }
    let is_stale = stale(s, e, q!(s, Network));
    if is_stale && !allow_stale {
        if q!(s, Metrics) != 0 {
            q!(s, LookupMetric, 1, e);
        }
        return -1;
    }
    hit(s, e, is_stale);
    if q!(s, Metrics) != 0 {
        q!(s, LookupMetric, if is_stale { 3 } else { 2 }, e);
    }
    if allow_stale && q!(s, StaleOutput) != 0 {
        q!(s, FillStale, e, q!(s, Network) - q!(s, Changes, e));
    }
    e
}
fn evict(s: &impl Services) -> i64 {
    let end = q!(s, Rows) + 2;
    let mut oldest = -1;
    for e in 2..end {
        if pinned(s, e) {
            continue;
        }
        if oldest < 0
            || (q!(s, Expires, e) < q!(s, Expires, oldest)
                && (stale(s, e, q!(s, Network)) || !stale(s, oldest, q!(s, Network))))
        {
            oldest = e;
        }
    }
    if oldest < 0 {
        return 0;
    }
    q!(s, EraseMetric, 0, oldest);
    q!(s, Erase, oldest);
    1
}
fn set(s: &impl Services) -> i64 {
    if q!(s, Max) == 0 {
        return 0;
    }
    let e = q!(s, Find);
    let mut active = false;
    let changed;
    if e >= 0 {
        active = pinned(s, e);
        changed = q!(s, Error, 0) == 0 && q!(s, Contents, e, 0) == 0;
        q!(s, Erase, e);
    } else {
        changed = true;
        while q!(s, Size) >= q!(s, Max) {
            if q!(s, EvictNested) == 0 {
                break;
            }
        }
    }
    let pin = q!(s, Pin, 0);
    q!(s, Insert, if pin < 0 { i64::from(active) } else { pin });
    // ScheduleWrite may delete the cache. No owner access follows this
    // callback.
    if changed && q!(s, HasDelegate) != 0 {
        q!(s, Schedule);
    }
    0
}
fn erase_all(s: &impl Services) -> i64 {
    let end = q!(s, Rows) + 2;
    for e in 2..end {
        q!(s, EraseMetric, q!(s, Outcome), e);
    }
    0
}
fn clear(s: &impl Services) -> i64 {
    q!(s, Clock);
    let end = q!(s, Rows) + 2;
    for e in 2..end {
        q!(s, EraseMetric, 1, e);
    }
    if q!(s, Size) == 0 {
        return 0;
    }
    q!(s, ClearMap);
    if q!(s, HasDelegate) != 0 {
        q!(s, Schedule);
    }
    0
}
fn clear_hosts(s: &impl Services) -> i64 {
    if q!(s, FilterNull) != 0 {
        return clear(s);
    }
    q!(s, Clock);
    let end = q!(s, Rows) + 2;
    q!(s, SnapshotFilterKeys);
    let mut changed = false;
    for e in 2..end {
        let remove = q!(s, Filter, e) != 0;
        if q!(s, OwnerAlive) == 0 {
            return 0;
        }
        if remove && q!(s, RefreshFilterRow, e) != 0 {
            q!(s, EraseMetric, 1, e);
            q!(s, Erase, e);
            changed = true;
        }
    }
    if changed && q!(s, HasDelegate) != 0 {
        q!(s, Schedule);
    }
    0
}
fn matching(s: &impl Services) -> i64 {
    let end = q!(s, Rows) + 2;
    for e in 2..end {
        if q!(s, MatchingHost, e) != 0 {
            q!(s, MatchingOut, e);
            return e;
        }
    }
    -1
}
fn contents(s: &impl Services) -> i64 {
    for op in [Op::Eq0, Op::Eq1, Op::Eq2, Op::Eq3, Op::Eq4, Op::Eq5, Op::Eq6] {
        if s.call(op as u32, 0, 0, 0, 0) == 0 {
            return 0;
        }
    }
    1
}
fn equal(s: &impl Services) -> i64 {
    if contents(s) == 0 {
        return 0;
    }
    for op in [Op::Eq7, Op::Eq8, Op::Eq9, Op::Eq10, Op::Eq11, Op::Eq12, Op::Eq13] {
        if s.call(op as u32, 0, 0, 0, 0) == 0 {
            return 0;
        }
    }
    1
}
fn key_cmp(s: &impl Services) -> i64 {
    for op in
        [Op::KeyCmp0, Op::KeyCmp1, Op::KeyCmp2, Op::KeyCmp3, Op::KeyCmp4, Op::KeyCmp5, Op::KeyCmp6]
    {
        let r = s.call(op as u32, 0, 0, 0, 0);
        if r != 0 {
            return r;
        }
    }
    0
}
fn merge(s: &impl Services) -> i64 {
    q!(s, Dcheck, i64::from(matches!(q!(s, Error, 0), 0 | -105)));
    q!(s, Dcheck, i64::from(matches!(q!(s, Error, 1), 0 | -105)));
    let error = if q!(s, Error, 0) == 0 || q!(s, Error, 1) == 0 { 0 } else { -105 };
    for op in [Op::Merge0, Op::Merge1, Op::Merge2, Op::Merge3, Op::Merge4, Op::Merge5] {
        s.call(op as u32, 0, 0, 0, 0);
    }
    q!(s, Dcheck, q!(s, Eq7));
    let t1 = q!(s, Ttl, 0);
    let t2 = q!(s, Ttl, 1);
    let ttl = if t1 >= 0 && t2 >= 0 {
        t1.min(t2)
    } else if t2 >= 0 {
        t2
    } else {
        t1
    };
    q!(
        s,
        SetMergeScalars,
        error,
        ttl,
        q!(s, Expires, 0).min(q!(s, Expires, 1)),
        q!(s, Changes, 0).max(q!(s, Changes, 1))
    );
    q!(
        s,
        SetHits,
        0,
        (q!(s, TotalHits, 0) + q!(s, TotalHits, 1)).min(i64::from(i32::MAX)),
        (q!(s, StaleHits, 0) + q!(s, StaleHits, 1)).min(i64::from(i32::MAX))
    );
    0
}
fn copy_port(s: &impl Services) -> i64 {
    for e in 0..q!(s, EndpointCount) {
        if q!(s, EndpointPort, e) == 0 {
            q!(s, SetEndpointPort, e);
        }
    }
    for h in 0..q!(s, HostCount) {
        if q!(s, HostPort, h) == 0 {
            q!(s, SetHostPort, h);
        }
    }
    0
}
fn endpoints(s: &impl Services, service: bool) -> i64 {
    if q!(s, EndpointCount) == 0 {
        return 0;
    }
    if service {
        for e in 0..q!(s, EndpointCount) {
            q!(s, ServiceIp, e, i64::from(q!(s, EndpointPort, e) == 0), q!(s, IsV6, e));
        }
    }
    if service || q!(s, CanonicalCount) == 1 {
        for m in 0..q!(s, MetadataCount) {
            if q!(s, TargetMatches, m) != 0 {
                if service {
                    q!(s, AppendService, m);
                } else {
                    q!(s, AppendEndpoint, m);
                }
            }
        }
    }
    if service {
        q!(s, AppendService, -1);
    } else {
        q!(s, AppendEndpoint, -1);
    }
    0
}
fn metadatas(s: &impl Services) -> i64 {
    for m in 0..q!(s, MetadataCount) {
        q!(s, AppendMetadata, m);
    }
    0
}
fn ttl_results(s: &impl Services) -> i64 {
    let mut min = i64::MAX;
    let mut any = false;
    for r in 0..q!(s, ResultCount) {
        any |= q!(s, ResultTtlCount, r) > 0;
        for kind in 0..2 {
            min = min.min(q!(s, ResultTtl, r, kind));
        }
    }
    q!(s, TtlOutput, i64::from(any), min);
    min
}
fn internal(s: &impl Services) -> i64 {
    let count = q!(s, ResultCount);
    let mut data = Vec::new();
    let mut aliases = Vec::new();
    let mut errors = Vec::new();
    let mut metadata = None;
    let mut ttl = None;
    let mut source = None;
    for r in 0..count {
        if q!(s, ResultTtlCount, r) > 0 {
            for kind in 0..2 {
                ttl = Some(ttl.unwrap_or(i64::MAX).min(q!(s, ResultTtl, r, kind)));
            }
        }
        let result_source = match q!(s, ResultSource, r) {
            0 => 1,
            1 => 2,
            _ => 0,
        };
        q!(s, Dcheck, i64::from(source.is_none_or(|x| x == result_source)));
        source = Some(result_source);
        match q!(s, ResultType, r) {
            0 => {
                if q!(s, ResultV6, r) != 0 {
                    data.insert(0, r)
                } else {
                    data.push(r)
                }
            }
            1 => {
                q!(s, Dcheck, i64::from(metadata.is_none()));
                metadata = Some(r)
            }
            2 => errors.push(r),
            3 => aliases.push(r),
            _ => unreachable!(),
        }
    }
    let mut error = -105;
    if !data.is_empty() || metadata.is_some() {
        error = 0;
        for &e in &errors {
            q!(s, Check, i64::from(q!(s, ResultQuery, e) != 0 && q!(s, ResultError, e) == -105));
        }
    } else if !errors.is_empty() {
        let mut cacheable = false;
        for &e in &errors {
            cacheable |= q!(s, ResultTtlCount, e) > 0;
            if q!(s, ResultError, e) != -105 || q!(s, ResultQuery, e) == 0 {
                q!(s, Check, i64::from(errors.len() == 1));
                error = q!(s, ResultError, e);
            }
        }
        if !cacheable {
            ttl = None;
        }
    } else {
        ttl = None;
    }
    if !data.is_empty() {
        for &d in &data {
            q!(s, Dcheck, q!(s, DataNonempty, d));
            q!(s, Dcheck, i64::from(q!(s, ResultTtlCount, d) > 0));
            q!(s, DataAppend, d);
        }
        for a in aliases {
            q!(s, AliasAppend, a);
        }
    }
    if let Some(m) = metadata {
        q!(s, Dcheck, i64::from(q!(s, ResultTtlCount, m) > 0));
        if q!(s, MetadataCopy, m) != 0 && data.is_empty() {
            error = -105;
        }
    }
    q!(s, InternalScalars, error, source.unwrap_or(q!(s, EmptySource)), ttl.unwrap_or(-1_000_000));
    0
}
fn lookup_metric(s: &impl Services) -> i64 {
    let outcome = q!(s, Outcome);
    q!(s, UmaEnum, 0, outcome);
    if outcome == 3 {
        q!(s, Check, q!(s, EntryPresent));
        q!(s, UmaTime, 0, q!(s, ExpiredBy, 0));
        q!(s, UmaCount, 0, q!(s, Network) - q!(s, Changes, 0));
    }
    0
}
fn erase_metric(s: &impl Services) -> i64 {
    q!(s, UmaEnum, 1, q!(s, Outcome));
    let expired = q!(s, ExpiredBy, 0);
    if stale(s, 0, q!(s, Network)) {
        q!(s, UmaTime, 1, expired);
        q!(s, UmaCount, 1, q!(s, Network) - q!(s, Changes, 0));
        q!(s, UmaCount, 2, q!(s, StaleHits, 0));
    } else {
        q!(s, UmaTime, 2, expired.saturating_neg());
    }
    0
}
// Typed Value lookup intentionally treats a wrong optional-field type as
// absent, exactly like DictValue::FindInt/FindString/FindList in the frozen
// API.
fn field(s: &impl Services, v: i64, f: i64, t: i64) -> i64 {
    let h = q!(s, Field, v, f);
    if h >= 0 && q!(s, Type, h) == t {
        h
    } else {
        -1
    }
}
fn int(s: &impl Services, v: i64, f: i64, default: i64) -> i64 {
    let h = field(s, v, f, 2);
    if h < 0 {
        default
    } else {
        q!(s, AsInt, h)
    }
}
fn boolean(s: &impl Services, v: i64, f: i64, default: i64) -> i64 {
    let h = field(s, v, f, 1);
    if h < 0 {
        default
    } else {
        q!(s, AsBool, h)
    }
}
fn set_int(s: &impl Services, v: i64, f: i64, x: i64) {
    q!(s, SetField, v, f, q!(s, IntValue, x));
}
fn entry_value(s: &impl Services) -> i64 {
    let v = q!(s, NewDict);
    let stale = q!(s, IncludeStale);
    q!(s, SetField, v, 8, q!(s, SerializeExpiration, stale));
    if stale != 0 {
        set_int(s, v, 9, q!(s, SerializeTtl));
        set_int(s, v, 11, q!(s, Changes, 0));
        let pin = q!(s, Pin, 0);
        if pin >= 0 {
            q!(s, SetField, v, 10, q!(s, BoolValue, pin));
        }
    }
    let error = q!(s, Error, 0);
    if error != 0 {
        set_int(s, v, 12, error);
    } else {
        for (kind, key) in [13, 17, 20, 22, 23, 24, 25].into_iter().enumerate() {
            let list = q!(s, NewList);
            for i in 0..q!(s, CollectionCount, kind as i64) {
                q!(s, Append, list, q!(s, CollectionValue, kind as i64, i));
            }
            q!(s, SetField, v, key, list);
        }
    }
    v
}
fn list(s: &impl Services) -> i64 {
    q!(s, OutputClear);
    let end = q!(s, Rows) + 2;
    for e in 2..end {
        q!(s, SelectRow, e);
        let nak = if q!(s, Debug) == 0 {
            let n = q!(s, NakToValue);
            if n < 0 || q!(s, TargetNetwork) != -1 {
                continue;
            }
            n
        } else {
            q!(s, NakDebug)
        };
        let v = entry_value(s);
        if q!(s, SchemeHost) != 0 {
            for f in [0, 2] {
                q!(s, SetField, v, f, q!(s, KeyValue, f));
            }
        }
        for f in [1, 3, 4, 5, 6] {
            q!(s, SetField, v, f, q!(s, KeyValue, f));
        }
        q!(s, SetField, v, 7, nak);
        if q!(s, Debug) != 0 {
            q!(s, SetField, v, 26, q!(s, TargetNetworkValue));
        }
        q!(s, OutputAppend, v);
    }
    0
}
fn restore(s: &impl Services) -> i64 {
    q!(s, ResetRestore);
    for i in 0..q!(s, InputList) {
        if q!(s, Size) == q!(s, Max) {
            break;
        }
        let v = q!(s, InputAt, i);
        if q!(s, Type, v) != 6 {
            return 0;
        }
        let hostname = field(s, v, 1, 4);
        if hostname < 0 || q!(s, ValidHostname, hostname) == 0 {
            return 0;
        }
        let scheme = field(s, v, 0, 4);
        let port = int(s, v, 2, -1);
        if scheme >= 0 && !(0..=65535).contains(&port) {
            return 0;
        }
        if scheme < 0 {
            q!(s, RestoreStringHost, hostname);
        } else if q!(s, RestoreHost, scheme, hostname, port) == 0 {
            return 0;
        }
        let exp = field(s, v, 8, 4);
        let flags = field(s, v, 4, 2);
        if exp < 0 || flags < 0 {
            return 0;
        }
        let query = int(s, v, 3, -1);
        if !matches!(query, 0..=5 | 7) {
            return 0;
        }
        let source = int(s, v, 5, 0);
        let nak = q!(s, Field, v, 7);
        if nak < 0 || q!(s, Type, nak) == 4 {
            return 0;
        }
        if q!(s, RestoreNak, nak) == 0 {
            continue;
        }
        let secure = boolean(s, v, 6, 0);
        let error = field(s, v, 12, 2);
        let pin = boolean(s, v, 10, 0);
        let mut lists = [-1; 8];
        if error < 0 {
            for (slot, key) in [13, 17, 20, 21, 22, 23, 24, 25].into_iter().enumerate() {
                lists[slot] = field(s, v, key, 7);
            }
            if (lists[5] < 0) != (lists[6] < 0) {
                return 0;
            }
        }
        if q!(s, RestoreExpiration, exp) == 0 {
            return 0;
        }
        q!(s, RestoreTime);
        q!(s, ResetRestoredEntry);
        let mut unavailable = false;
        if lists[0] >= 0 {
            for j in 0..q!(s, ListSize, lists[0]) {
                let ep = q!(s, ListAt, lists[0], j);
                if q!(s, RestoreEndpoint, ep) == 0 {
                    if q!(s, Type, ep) == 6 && field(s, ep, 16, 4) >= 0 {
                        unavailable = true;
                        break;
                    }
                    return 0;
                }
            }
        }
        if unavailable {
            continue;
        }
        if lists[1] >= 0 {
            for j in 0..q!(s, ListSize, lists[1]) {
                if q!(s, RestoreMetadata, q!(s, ListAt, lists[1], j)) == 0 {
                    return 0;
                }
            }
        }
        for (slot, target) in [(2, 0), (4, 1), (7, 2)] {
            if lists[slot] >= 0 {
                for j in 0..q!(s, ListSize, lists[slot]) {
                    let item = q!(s, ListAt, lists[slot], j);
                    if q!(s, Type, item) != 4 {
                        return 0;
                    }
                    q!(s, RestoreString, target, item);
                }
            }
        }
        if lists[3] >= 0 {
            if q!(s, RestoredEndpointCount) != 0 {
                return 0;
            }
            for j in 0..q!(s, ListSize, lists[3]) {
                let item = q!(s, ListAt, lists[3], j);
                if q!(s, Type, item) != 4 || q!(s, RestoreLegacy, item) == 0 {
                    return 0;
                }
            }
        }
        if lists[5] >= 0 {
            let n = q!(s, ListSize, lists[5]);
            if n != q!(s, ListSize, lists[6]) {
                return 0;
            }
            for j in 0..n {
                let host = q!(s, ListAt, lists[5], j);
                let port = q!(s, ListAt, lists[6], j);
                if q!(s, Type, host) != 4 || q!(s, Type, port) != 2 {
                    return 0;
                }
                let p = q!(s, AsInt, port);
                if !(0..=65535).contains(&p) {
                    return 0;
                }
                q!(s, RestoreHostRecord, host, p);
            }
        }
        q!(s, RestoreFlags, q!(s, AsInt, flags));
        q!(s, RestoreError, if error < 0 { 0 } else { q!(s, AsInt, error) });
        if q!(s, RestoreFinish, query, source, secure) == 0 {
            q!(s, RestoreInsert, pin, q!(s, Network) - 1);
        }
    }
    1
}

fn valid_hostname(s: &impl Services) -> i64 {
    i64::from(q!(s, HostnameEmpty) == 0 && q!(s, HostnameIp) == 0 && q!(s, HostnameUrlIp) == 0)
}
fn parse_endpoint(s: &impl Services) -> i64 {
    if q!(s, Type, 0) != 6 {
        return 0;
    }
    let ip = field(s, 0, 14, 4);
    let port = int(s, 0, 15, -1);
    if ip < 0 || !(0..=65535).contains(&port) || q!(s, EndpointIp, ip) == 0 {
        return 0;
    }
    let scope = q!(s, EndpointScope, 0);
    if scope >= 0 && (scope == 0 || q!(s, IpV6LinkLocal) == 0) {
        return 0;
    }
    q!(s, StoreEndpoint, port);
    1
}
fn parse_metadata(s: &impl Services) -> i64 {
    if q!(s, Type, 0) != 6 {
        return 0;
    }
    let priority = int(s, 0, 18, -1);
    let metadata = q!(s, Field, 0, 19);
    if !(0..=65535).contains(&priority) || metadata < 0 || q!(s, ParseMetadata, metadata) == 0 {
        return 0;
    }
    q!(s, StoreMetadataPair, priority);
    1
}
fn serialize_endpoint(s: &impl Services) -> i64 {
    let v = q!(s, NewDict);
    q!(s, SetField, v, 14, q!(s, EndpointAddressValue));
    set_int(s, v, 15, q!(s, EndpointNativePort));
    if q!(s, EndpointHasScope) != 0 {
        let scope = q!(s, EndpointInterfaceValue);
        if q!(s, Type, scope) != 0 {
            q!(s, SetField, v, 16, scope);
        }
    }
    v
}
fn serialize_metadata(s: &impl Services) -> i64 {
    let v = q!(s, NewDict);
    set_int(s, v, 18, q!(s, MetadataPriority));
    q!(s, SetField, v, 19, q!(s, MetadataValue));
    v
}

// Dispatch table
pub fn run(s: &impl Services, method: u32) -> i64 {
    match method {
        0 => lookup(s, false),
        1 => lookup(s, true),
        2 => prefer(s, 0, 1),
        3 => find(s),
        4 => set(s),
        5 => evict(s),
        6 => clear(s),
        7 => clear_hosts(s),
        8 => erase_all(s),
        9 => matching(s),
        10 => contents(s),
        11 => equal(s),
        12 => key_cmp(s),
        13 => merge(s),
        14 => copy_port(s),
        15 => endpoints(s, false),
        16 => endpoints(s, true),
        17 => metadatas(s),
        18 => ttl_results(s),
        19 => internal(s),
        20 => lookup_metric(s),
        21 => erase_metric(s),
        22 => entry_value(s),
        23 => list(s),
        24 => restore(s),
        25 => i64::from(stale(s, 0, q!(s, NetworkArg))),
        26 => {
            q!(s, FillStale, 0, q!(s, NetworkArg) - q!(s, Changes, 0));
            0
        }
        27 => {
            hit(s, 0, q!(s, HitArg) != 0);
            0
        }
        28 => i64::from(pinned(s, 0)),
        29 => q!(s, Invalidate),
        30 => valid_hostname(s),
        31 => parse_endpoint(s),
        32 => parse_metadata(s),
        33 => serialize_endpoint(s),
        34 => serialize_metadata(s),
        _ => unreachable!("invalid native method"),
    }
}

pub fn stale_value(expired: i64, changes: i32) -> bool {
    changes > 0 || expired >= 0
}
pub fn completed(error: i32) -> bool {
    error != -21 && error != -119
}
pub fn known_ttl(ttl: i64) -> bool {
    ttl >= 0
}
