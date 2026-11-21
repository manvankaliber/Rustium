// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Response-info policy. C++ owns values; each service is one synchronous
//! primitive read/write, field assignment, or dependency-owned operation.
#![forbid(unsafe_code)]
#[path = "http_response_info_rust_ids.rs"]
mod http_response_info_rust_ids;
use http_response_info_rust_ids::*;

pub trait Services {
    fn call(&mut self, op: u32, field: u32, value: i64) -> Option<i64>;
}
fn get(s: &mut impl Services, field: u32) -> i64 {
    s.call(GET, field, 0).expect("field service")
}
fn set(s: &mut impl Services, field: u32, value: i64) {
    s.call(SET, field, value);
}
fn do_op(s: &mut impl Services, op: u32, value: i64) -> Option<i64> {
    s.call(op, 0, value)
}
const CERT: u32 = 1 << 8;
const SECURITY: u32 = 1 << 9;
const CERT_STATUS: u32 = 1 << 10;
const VARY: u32 = 1 << 11;
const TRUNCATED: u32 = 1 << 12;
const SPDY: u32 = 1 << 13;
const ALPN: u32 = 1 << 14;
const SSL_CONNECTION: u32 = 1 << 16;
const PROTOCOL: u32 = 1 << 17;
const CONNECTION: u32 = 1 << 18;
const AUTH: u32 = 1 << 19;
const SCT: u32 = 1 << 20;
const UNUSED: u32 = 1 << 21;
const GROUP: u32 = 1 << 22;
const PKP: u32 = 1 << 23;
const STALE: u32 = 1 << 24;
const SIGNATURE: u32 = 1 << 25;
const RESTRICTED: u32 = 1 << 26;
const ALIASES: u32 = 1 << 27;
const ECH: u32 = 1 << 29;
const BROWSER: u32 = 1 << 30;
const EXTRA: u32 = 1 << 31;
const PROXY: u32 = 1 << 1;
const ORIGINAL: u32 = 1 << 2;
const BODY: u32 = 1 << 3;
const ZSTD: u32 = 1 << 4;

fn load(s: &mut impl Services) -> Option<()> {
    let flags = do_op(s, READINT, 0)? as u32;
    let extra = if flags & EXTRA != 0 { do_op(s, READINT, 0)? as u32 } else { 0 };
    if flags & 255 != 3 {
        do_op(s, UNEXPECTEDVERSION, i64::from(flags & 255));
        return None;
    }
    let v = do_op(s, READINT64, 0)?;
    set(s, F_REQUEST, v);
    set(s, F_CACHED, 1);
    let v = do_op(s, READINT64, 0)?;
    set(s, F_RESPONSE, v);
    if extra & ORIGINAL != 0 {
        let v = do_op(s, READINT64, 0)?;
        set(s, F_ORIGINAL, v);
    }
    if do_op(s, READHEADERS, 0)? == -1 {
        return None;
    }
    if flags & CERT != 0 {
        do_op(s, READCERT, 0)?;
    }
    if flags & CERT_STATUS != 0 {
        let v = do_op(s, READUINT32, 0)?;
        set(s, F_CERT_STATUS, v);
    }
    if flags & SECURITY != 0 {
        do_op(s, READINT, 0)?;
    }
    if flags & SSL_CONNECTION != 0 {
        let v = do_op(s, READINT, 0)?;
        if (v >> 20) & 7 == 2 {
            return None;
        }
        set(s, F_SSL_CONNECTION, v);
    }
    if flags & SCT != 0 {
        let count = do_op(s, READINT, 0)?;
        for _ in 0..count {
            do_op(s, READSCT, 0)?;
            do_op(s, READUINT16, 0)?;
        }
    }
    if flags & VARY != 0 {
        do_op(s, READVARY, 0)?;
    }
    do_op(s, READHOST, 0)?;
    let port = do_op(s, READUINT16, 0)?;
    do_op(s, SETENDPOINT, port)?;
    if flags & PROTOCOL != 0 {
        do_op(s, READALPN, 0)?;
    }
    if flags & CONNECTION != 0 {
        let v = do_op(s, READINT, 0)?;
        if (1..=42).contains(&v) {
            set(s, F_CONNECTION, v);
        }
    }
    if flags & GROUP != 0 {
        let v = do_op(s, READINT, 0)?;
        let status = get(s, F_SSL_CONNECTION);
        if (status >> 20) & 7 >= 6 || do_op(s, CIPHERECDHE, status)?.ne(&0) {
            // Preserve the original narrowing modulo 2^16, including negatives.
            set(s, F_GROUP, i64::from(v as u16));
        }
    }
    if flags & STALE != 0 {
        let v = do_op(s, READINT64, 0)?;
        set(s, F_STALE, v);
    }
    for (bit, field) in [(SPDY, F_SPDY), (ALPN, F_ALPN)] {
        set(s, field, i64::from(flags & bit != 0));
    }
    do_op(s, SETTRUNCATED, i64::from(flags & TRUNCATED != 0))?;
    for (bit, field) in
        [(AUTH, F_AUTH), (UNUSED, F_UNUSED), (RESTRICTED, F_RESTRICTED), (PKP, F_PKP)]
    {
        set(s, field, i64::from(flags & bit != 0));
    }
    if flags & SIGNATURE != 0 {
        let v = do_op(s, READINT, 0)?;
        let v = u16::try_from(v).ok()?;
        set(s, F_SIGNATURE, i64::from(v));
    }
    if flags & ALIASES != 0 {
        let count = do_op(s, READINT, 0)?;
        if count < 0 {
            return None;
        }
        // Do not reserve untrusted counts: reading has the same result/state
        // without the original potentially unbounded speculative allocation.
        for _ in 0..count {
            do_op(s, READALIAS, 0)?;
        }
        do_op(s, COMMITALIASES, 0)?;
    }
    set(s, F_ECH, i64::from(flags & ECH != 0));
    if flags & BROWSER != 0 {
        do_op(s, READINT64, 0)?;
    }
    set(s, F_DICTIONARY, 0);
    if extra & PROXY != 0 {
        do_op(s, READPROXY, 0)?;
    }
    if extra & BODY != 0 {
        let size = do_op(s, READINT64, 0)?;
        if size >= 0 {
            set(s, F_BODY, size);
        }
    }
    if extra & ZSTD != 0 {
        let size = do_op(s, READINT64, 0)?;
        if size < 0 {
            return None;
        }
        set(s, F_ZSTD, size);
    }
    Some(())
}
fn persist(s: &mut impl Services, skip: bool, truncated: bool) {
    let mut flags = 3 | EXTRA;
    let mut extra = ORIGINAL;
    if get(s, F_CERT_VALID) != 0 {
        flags |= CERT | CERT_STATUS;
        for (field, bit) in
            [(F_GROUP, GROUP), (F_SSL_CONNECTION, SSL_CONNECTION), (F_SIGNATURE, SIGNATURE)]
        {
            if get(s, field) != 0 {
                flags |= bit;
            }
        }
    }
    for (field, bit) in [
        (F_VARY_VALID, VARY),
        (F_SPDY, SPDY),
        (F_ALPN, ALPN | PROTOCOL),
        (F_CONNECTION, CONNECTION),
        (F_AUTH, AUTH),
        (F_UNUSED, UNUSED),
        (F_RESTRICTED, RESTRICTED),
        (F_PKP, PKP),
        (F_STALE, STALE),
        (F_ALIAS_COUNT, ALIASES),
        (F_ECH, ECH),
    ] {
        if get(s, field) != 0 {
            flags |= bit;
        }
    }
    if truncated {
        flags |= TRUNCATED;
    }
    for (field, bit) in [(F_PROXY_VALID, PROXY), (F_BODY_PRESENT, BODY), (F_ZSTD_PRESENT, ZSTD)] {
        if get(s, field) != 0 {
            extra |= bit;
        }
    }
    // Preserve signed 32-bit Pickle representation of the high flag bit.
    do_op(s, WRITEINT, i64::from(flags as i32));
    do_op(s, WRITEINT, i64::from(extra));
    for field in [F_REQUEST, F_RESPONSE, F_ORIGINAL] {
        let v = get(s, field);
        do_op(s, WRITEINT64, v);
    }
    do_op(s, WRITEHEADERS, if skip { 63 } else { -1 });
    if flags & CERT != 0 {
        do_op(s, WRITECERT, 0);
        let v = get(s, F_CERT_STATUS);
        do_op(s, WRITEUINT32, v);
        if flags & SSL_CONNECTION != 0 {
            let v = get(s, F_SSL_CONNECTION);
            do_op(s, WRITEINT, v);
        }
    }
    if flags & VARY != 0 {
        do_op(s, WRITEVARY, 0);
    }
    do_op(s, WRITEHOST, 0);
    if flags & PROTOCOL != 0 {
        do_op(s, WRITEALPN, 0);
    }
    for (bit, field, op) in [
        (CONNECTION, F_CONNECTION, WRITEINT),
        (GROUP, F_GROUP, WRITEINT),
        (STALE, F_STALE, WRITEINT64),
        (SIGNATURE, F_SIGNATURE, WRITEINT),
    ] {
        if flags & bit != 0 {
            let v = get(s, field);
            do_op(s, op, v);
        }
    }
    if flags & ALIASES != 0 {
        let count = get(s, F_ALIAS_COUNT);
        do_op(s, WRITEINT, i64::from(count as i32));
        for i in 0..count {
            do_op(s, WRITEALIAS, i);
        }
    }
    if extra & PROXY != 0 {
        do_op(s, WRITEPROXY, 0);
    }
    for (bit, field) in [(BODY, F_BODY), (ZSTD, F_ZSTD)] {
        if extra & bit != 0 {
            let v = get(s, field);
            do_op(s, WRITEINT64, v);
        }
    }
}
pub fn execute(s: &mut impl Services, operation: u32, first: bool, second: bool) -> bool {
    match operation {
        0 => {
            // Dependency-owned nontrivial default values are constructed by
            // C++. All response-info scalar defaults are specified here too.
            for field in [
                F_CACHED,
                F_NETWORK,
                F_CACHE,
                F_ASYNC,
                F_SENT,
                F_SPDY,
                F_ALPN,
                F_AUTH,
                F_UNUSED,
                F_RESTRICTED,
                F_DICTIONARY,
                F_REQUEST,
                F_RESPONSE,
                F_ORIGINAL,
                F_STALE,
                F_CONNECTION,
            ] {
                set(s, field, 0);
            }
            set(s, F_ALTERNATE, 7);
            true
        }
        1 => {
            if !first {
                for field in 0..29 {
                    s.call(COPYFIELD, field, 0);
                }
            }
            true
        }
        2 => load(s).is_some(),
        3 => {
            persist(s, first, second);
            true
        }
        4 => matches!(get(s, F_CONNECTION), 5 | 10..=42),
        5 => get(s, F_PROXY_VALID) != 0 && get(s, F_PROXY_DIRECT) == 0,
        _ => false,
    }
}
