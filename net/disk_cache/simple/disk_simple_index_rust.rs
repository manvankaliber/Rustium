// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! SimpleIndex policy. Native services own containers, tasks, clocks, IO and
//! callbacks. No native reference, iterator, callback or owner is borrowed
//! here.
#![forbid(unsafe_code)]

pub const ALIVE: u32 = 0;
pub const CACHETYPE: u32 = 1;
pub const INITIALIZED: u32 = 2;
pub const CACHESIZE: u32 = 3;
pub const MAXSIZE: u32 = 4;
pub const HIGH: u32 = 5;
pub const LOW: u32 = 6;
pub const EVICTING: u32 = 7;
pub const BACKGROUND: u32 = 8;
pub const PRIORITIZED: u32 = 9;
pub const FACTOR: u32 = 10;
pub const PERIOD: u32 = 11;
pub const SETMAX: u32 = 12;
pub const SETHIGH: u32 = 13;
pub const SETLOW: u32 = 14;
pub const SETCACHESIZE: u32 = 15;
pub const SETINITIALIZED: u32 = 16;
pub const SETINITMETHOD: u32 = 17;
pub const CONTAINS: u32 = 18;
pub const META: u32 = 19;
pub const SETMETA: u32 = 20;
pub const INSERT: u32 = 21;
pub const ERASE: u32 = 22;
pub const REMOVEDINSERT: u32 = 23;
pub const REMOVEDERASE: u32 = 24;
pub const REMOVEDBEGIN: u32 = 25;
pub const REMOVEDNEXT: u32 = 26;
pub const REMOVEDKEY: u32 = 27;
pub const REMOVEDCLEAR: u32 = 28;
pub const BEGIN: u32 = 29;
pub const NEXT: u32 = 30;
pub const KEY: u32 = 31;
pub const VALUE: u32 = 32;
pub const ADDRESS: u32 = 33;
pub const LOADEDERASE: u32 = 34;
pub const LOADEDPUT: u32 = 35;
pub const LOADEDBEGIN: u32 = 36;
pub const LOADEDNEXT: u32 = 37;
pub const LOADEDVALUE: u32 = 38;
pub const SWAPLOADED: u32 = 39;
pub const LOADMETHOD: u32 = 40;
pub const LOADFLUSH: u32 = 41;
pub const NOWSECONDS: u32 = 42;
pub const NOWSTORED: u32 = 43;
pub const TIMEINTERNAL: u32 = 44;
pub const RANGELOWER: u32 = 45;
pub const RANGEUPPER: u32 = 46;
pub const OUTPUTPUSH: u32 = 47;
pub const TIMERSTART: u32 = 48;
pub const TIMERSTOP: u32 = 49;
pub const TIMERRUNNING: u32 = 50;
pub const LOAD: u32 = 51;
pub const WRITE: u32 = 52;
pub const EVICTIONSTART: u32 = 53;
pub const DOOMPUSH: u32 = 54;
pub const DOOMSUBMIT: u32 = 55;
pub const EVICTIONTELEMETRY: u32 = 56;
pub const EVICTIONDONETELEMETRY: u32 = 57;
pub const INITTELEMETRY: u32 = 58;
pub const QUEUEREADY: u32 = 59;
pub const POSTREADY: u32 = 60;
pub const READYCOUNT: u32 = 61;
pub const POSTONE: u32 = 62;
pub const ABORTONE: u32 = 63;
pub const INVALIDSIZE: u32 = 64;
pub const METACACHETYPE: u32 = 65;
pub const METAHASPREFETCH: u32 = 66;
pub const METAREADTIME: u32 = 67;
pub const METAREADPACKED: u32 = 68;
pub const METATIMEVALUE: u32 = 69;
pub const METAPACKEDVALUE: u32 = 70;
pub const METABITS: u32 = 71;
pub const METASETBITS: u32 = 72;
pub const METASTOREDFROMINTERNAL: u32 = 73;
pub const METAINTERNALFROMSTORED: u32 = 74;
pub const METAWRITETIME: u32 = 75;
pub const METAWRITEPACKED: u32 = 76;
pub const ASSERTACCOUNTED: u32 = 77;
pub const OP_SERIALIZE: u32 = 24;
pub const OP_DESERIALIZE: u32 = 25;
pub const OP_SETMAX: u32 = 0;
pub const OP_INSERT: u32 = 1;
pub const OP_REMOVE: u32 = 2;
pub const OP_HAS: u32 = 3;
pub const OP_USE: u32 = 4;
pub const OP_GETMEMORY: u32 = 5;
pub const OP_SETMEMORY: u32 = 6;
pub const OP_GETPREFETCH: u32 = 7;
pub const OP_SETPREFETCH: u32 = 8;
pub const OP_UPDATESIZE: u32 = 9;
pub const OP_INSERTTEST: u32 = 10;
pub const OP_RANGE: u32 = 11;
pub const OP_SIZERANGE: u32 = 12;
pub const OP_LASTUSED: u32 = 13;
pub const OP_SETLASTUSED: u32 = 14;
pub const OP_PENDING: u32 = 15;
pub const OP_READY: u32 = 16;
pub const OP_INITIALIZE: u32 = 17;
pub const OP_MERGE: u32 = 18;
pub const OP_WRITE: u32 = 19;
pub const OP_POSTPONE: u32 = 20;
pub const OP_EVICT: u32 = 21;
pub const OP_EVICTIONDONE: u32 = 22;
pub const OP_SHUTDOWN: u32 = 23;

pub trait Services {
    fn call(&mut self, op: u32, a: u64, b: u64) -> u64;
    fn get(&mut self, op: u32) -> u64 {
        self.call(op, 0, 0)
    }
}

pub fn size_chunks(size: u64) -> Option<u32> {
    let rounded = size.wrapping_add(255) >> 8;
    (rounded >> 30 == 0).then_some(rounded as u32)
}

// Metadata wire: low 32 bits time/prefetch; upper 30 bits chunks and 2 hints.
fn size(meta: u64) -> u64 {
    ((meta >> 32) & 0x3fff_ffff) << 8
}
fn with_time(meta: u64, time: u64) -> u64 {
    (meta & !0xffff_ffff) | (time & 0xffff_ffff)
}
fn with_size(meta: u64, chunks: u32) -> u64 {
    (meta & 0xc000_0000_ffff_ffff) | (u64::from(chunks) << 32)
}
pub fn metadata(op: u32, a: u64, b: u64, c: u64) -> u64 {
    match op {
        0 => size_chunks(a).map_or(u64::MAX, u64::from),
        1 => (a << 8) | b, // pickle representation
        2 => a >> 8,
        3 => a & 3,
        4 => {
            if a != 0 {
                0
            } else {
                (b as i64).clamp(1, i64::from(u32::MAX)) as u64
            }
        }
        5 => {
            if b == 0 {
                a
            } else {
                b
            }
        } // zero prefetch is ignored
        6 => u64::from(a >> 38 == 0),
        7 => {
            // deserialize prefetch: invalid/absent/zero preserves the field
            if c != 0 && b > 0 && b <= u64::from(u32::MAX) {
                b
            } else {
                a
            }
        }
        _ => 0,
    }
}

fn postpone(s: &mut impl Services) {
    if s.get(INITIALIZED) != 0 {
        let delay = if s.get(BACKGROUND) != 0 { 100 } else { 20_000 };
        s.call(TIMERSTART, delay, 0);
    }
}
fn write(s: &mut impl Services, reason: u64) {
    if s.get(INITIALIZED) != 0 {
        s.get(TIMERSTOP);
        // Terminal service: an IO provider may synchronously reenter/delete.
        s.call(WRITE, reason, 0);
    }
}
fn update(s: &mut impl Services, key: u64, requested: u64) -> bool {
    let old = s.call(META, key, 0);
    s.call(ASSERTACCOUNTED, size(old), 0);
    let Some(chunks) = size_chunks(requested) else {
        s.call(INVALIDSIZE, requested, 0);
        return false;
    };
    let new = with_size(old, chunks);
    s.call(SETMETA, key, new);
    let total = s.get(CACHESIZE).wrapping_sub(size(old)).wrapping_add(size(new));
    s.call(SETCACHESIZE, total, 0);
    size(old) != size(new)
}
fn evict(s: &mut impl Services) {
    if s.get(EVICTING) != 0 || s.get(CACHESIZE) <= s.get(HIGH) {
        return;
    }
    s.get(EVICTIONSTART);
    let ty = s.get(CACHETYPE);
    let use_size = ty != 5 && ty != 7;
    let prioritized = s.get(PRIORITIZED) != 0;
    let period = s.get(PERIOD);
    let factor = s.get(FACTOR);
    let now = s.get(NOWSECONDS) as u32;
    let mut entries = Vec::new();
    s.get(BEGIN);
    while s.get(NEXT) != 0 {
        let meta = s.get(VALUE);
        let age = u64::from(now.wrapping_sub(meta as u32));
        let mut score = age;
        if use_size {
            score = score.wrapping_mul(size(meta).wrapping_add(512));
            if prioritized && age < period && meta >> 62 & 2 == 2 {
                // Configured feature param is the original signed int converted
                // to uint64_t by C++ usual arithmetic conversions.
                score /= factor;
            }
        }
        entries.push((u64::MAX - score, s.get(ADDRESS), s.get(KEY), size(meta)));
    }
    entries.sort_unstable_by_key(|e| (e.0, e.1));
    let amount = s.get(CACHESIZE).wrapping_sub(s.get(LOW));
    let mut total = 0u64;
    for (_, _, key, bytes) in entries {
        if total >= amount {
            break;
        }
        total = total.wrapping_add(bytes);
        s.call(DOOMPUSH, key, 0);
    }
    s.get(EVICTIONTELEMETRY);
    // No service after DoomSubmit: callback may remove entries, complete
    // synchronously, start another eviction, or delete the entire index.
    s.get(DOOMSUBMIT);
}

pub fn run(s: &mut impl Services, op: u32, a: u64, b: u64) -> u64 {
    match op {
        OP_SERIALIZE => {
            let bits = s.get(METABITS);
            let raw = bits & 0xffff_ffff;
            let time = if s.get(METACACHETYPE) == 3 {
                raw
            } else {
                s.call(METAINTERNALFROMSTORED, raw, 0)
            };
            s.call(METAWRITETIME, time, 0);
            s.call(METAWRITEPACKED, metadata(1, (bits >> 32) & 0x3fff_ffff, bits >> 62, 0), 0);
        }
        OP_DESERIALIZE => {
            if s.get(METAREADTIME) == 0 || s.get(METAREADPACKED) == 0 {
                return 0;
            }
            let packed = s.get(METAPACKEDVALUE);
            if metadata(6, packed, 0, 0) == 0 {
                return 0;
            }
            let time = s.get(METATIMEVALUE);
            let raw = if s.get(METACACHETYPE) == 3 {
                let old = s.get(METABITS) & 0xffff_ffff;
                let has_prefetch = s.get(METAHASPREFETCH);
                metadata(7, old, time, has_prefetch)
            } else {
                s.call(METASTOREDFROMINTERNAL, time, 0)
            };
            let bits = raw | (metadata(2, packed, 0, 0) << 32) | (metadata(3, packed, 0, 0) << 62);
            s.call(METASETBITS, bits, 0);
            return 1;
        }
        OP_SETMAX => {
            let shrinking = a < s.get(MAXSIZE);
            s.call(SETMAX, a, 0);
            s.call(SETHIGH, a - a / 20, 0);
            s.call(SETLOW, a - 2 * (a / 20), 0);
            if shrinking {
                evict(s);
            }
        }
        OP_INSERT => {
            let time = if s.get(CACHETYPE) == 3 { u64::from(u32::MAX) } else { s.get(NOWSTORED) };
            let inserted = s.call(INSERT, a, time);
            if s.get(INITIALIZED) == 0 {
                s.call(REMOVEDERASE, a, 0);
            }
            if inserted != 0 {
                postpone(s);
            }
        }
        OP_REMOVE => {
            let found = s.call(CONTAINS, a, 0) != 0;
            if found {
                update(s, a, 0);
                s.call(ERASE, a, 0);
            }
            if s.get(INITIALIZED) == 0 {
                s.call(REMOVEDINSERT, a, 0);
            }
            if found {
                postpone(s);
            }
        }
        OP_HAS => return u64::from(s.get(INITIALIZED) == 0 || s.call(CONTAINS, a, 0) != 0),
        OP_USE => {
            if s.call(CONTAINS, a, 0) == 0 {
                return u64::from(s.get(INITIALIZED) == 0);
            }
            if s.get(CACHETYPE) != 3 {
                let meta = s.call(META, a, 0);
                let now = s.get(NOWSTORED);
                s.call(SETMETA, a, with_time(meta, now));
                postpone(s);
            }
            return 1;
        }
        OP_GETMEMORY => return s.call(META, a, 0) >> 62,
        OP_SETMEMORY => {
            if s.call(CONTAINS, a, 0) != 0 {
                let meta = s.call(META, a, 0);
                s.call(SETMETA, a, (meta & !(3 << 62)) | (b << 62));
            }
        }
        OP_GETPREFETCH => {
            return if s.call(CONTAINS, a, 0) == 0 {
                u64::MAX
            } else {
                s.call(META, a, 0) & 0xffff_ffff
            }
        }
        OP_SETPREFETCH => {
            if s.call(CONTAINS, a, 0) != 0 && b as u32 != 0 {
                let meta = s.call(META, a, 0);
                let new = with_time(meta, b);
                s.call(SETMETA, a, new);
                if new != meta {
                    postpone(s);
                }
            }
        }
        OP_UPDATESIZE => {
            if s.call(CONTAINS, a, 0) == 0 {
                return 0;
            }
            if update(s, a, b) {
                postpone(s);
                evict(s);
            }
            return 1; // even a rejected size preserves the original true result
        }
        OP_INSERTTEST => {
            if s.call(INSERT, a, b) != 0 {
                let total = s.get(CACHESIZE).wrapping_add(size(b));
                s.call(SETCACHESIZE, total, 0);
            }
        }
        OP_RANGE | OP_SIZERANGE => {
            let lower = s.call(RANGELOWER, a, 0) as i64;
            let upper = s.call(RANGEUPPER, b, 0) as i64;
            let mut total = 0u64;
            s.get(BEGIN);
            while s.get(NEXT) != 0 {
                let meta = s.get(VALUE);
                let time = s.call(TIMEINTERNAL, meta as u32 as u64, 0) as i64;
                if lower <= time && time < upper {
                    if op == OP_RANGE {
                        let key = s.get(KEY);
                        s.call(OUTPUTPUSH, key, 0);
                    }
                    total = total.wrapping_add(size(meta));
                }
            }
            return total;
        }
        OP_LASTUSED => return s.call(META, a, 0) & 0xffff_ffff,
        OP_SETLASTUSED => {
            let old = s.call(META, a, 0);
            s.call(SETMETA, a, with_time(old, b));
        }
        OP_PENDING => return s.get(TIMERRUNNING),
        OP_READY => {
            let action = if s.get(INITIALIZED) != 0 { POSTREADY } else { QUEUEREADY };
            s.get(action);
        }
        OP_INITIALIZE => {
            s.call(LOAD, a, 0);
        }
        OP_MERGE => {
            s.get(REMOVEDBEGIN);
            while s.get(REMOVEDNEXT) != 0 {
                let key = s.get(REMOVEDKEY);
                s.call(LOADEDERASE, key, 0);
            }
            s.get(REMOVEDCLEAR);
            s.get(BEGIN);
            while s.get(NEXT) != 0 {
                let key = s.get(KEY);
                let meta = s.get(VALUE);
                s.call(LOADEDPUT, key, meta);
            }
            let mut total = 0u64;
            s.get(LOADEDBEGIN);
            while s.get(LOADEDNEXT) != 0 {
                total = total.wrapping_add(size(s.get(LOADEDVALUE)));
            }
            s.get(SWAPLOADED);
            s.call(SETCACHESIZE, total, 0);
            s.call(SETINITIALIZED, 1, 0);
            let method = s.get(LOADMETHOD);
            s.call(SETINITMETHOD, method, 0);
            if s.get(LOADFLUSH) != 0 {
                write(s, 1);
            }
            if s.get(ALIVE) == 0 {
                return 0;
            }
            s.get(INITTELEMETRY);
            while s.get(READYCOUNT) != 0 {
                s.get(POSTONE);
                if s.get(ALIVE) == 0 {
                    return 0;
                }
            }
        }
        OP_WRITE => write(s, a),
        OP_POSTPONE => postpone(s),
        OP_EVICT => evict(s),
        OP_EVICTIONDONE => {
            s.get(EVICTIONDONETELEMETRY);
        }
        OP_SHUTDOWN => {
            // Pop before invoking: callback may reenter and append callbacks.
            // Each owned callback is consumed exactly once; no borrow survives.
            while s.get(READYCOUNT) != 0 {
                s.get(ABORTONE);
            }
        }
        _ => unreachable!("private operation vocabulary"),
    }
    0
}
