// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_upper_rust_common::Controller;
use crate::connection_jobs_rust_core::Services;
use crate::errors::*;
const MODULE: u32 = 4;
const STATE: u32 = 0;
const SET_STATE: u32 = 1;
const HAS_NESTED: u32 = 2;
const HAS_NESTED_SOCKET: u32 = 3;
const HAS_SSL_SOCKET: u32 = 4;
const NESTED_LOAD: u32 = 5;
const NESTED_ESTABLISHED: u32 = 6;
const COPY_ATTEMPTS: u32 = 7;
const COPY_RESOLVE: u32 = 8;
const COPY_RESOLUTION: u32 = 9;
const COPY_CERTIFICATE: u32 = 10;
const NEGOTIATION_STARTED: u32 = 11;
const PARAMS_CONNECTION_TYPE: u32 = 12;
const CLEAR_TCP: u32 = 13;
const CLEAR_NESTED: u32 = 14;
const HAPPY_EYEBALLS: u32 = 15;
const HAS_ECH_RETRY: u32 = 16;
const CREATE_TCP: u32 = 17;
const CREATE_TCP_RETRY: u32 = 18;
const CREATE_TRANSPORT: u32 = 19;
const CREATE_TRANSPORT_RETRY: u32 = 20;
const CONNECT_NESTED: u32 = 21;
const SAVE_RESOLVE: u32 = 22;
const SAVE_RESOLUTION: u32 = 23;
const APPEND_ATTEMPTS: u32 = 24;
const TAKE_NESTED_SOCKET: u32 = 25;
const SAVE_PEER: u32 = 26;
const SAVE_ALIASES: u32 = 27;
const SAVE_STALE: u32 = 28;
const CREATE_SOCKS: u32 = 29;
const CREATE_TUNNEL: u32 = 30;
const SAVE_CERTIFICATE: u32 = 31;
const HANDSHAKE_TIMER: u32 = 32;
const COPY_CONNECT_TIMING: u32 = 33;
const SET_NEGOTIATION: u32 = 34;
const STAMP_SSL_START: u32 = 35;
const IS_TCP: u32 = 36;
const SAVE_ENDPOINT: u32 = 37;
const SAVE_SERVICE_ENDPOINT: u32 = 38;
const CONFIG_INIT: u32 = 39;
const CONFIG_IGNORE_ERRORS: u32 = 40;
const CONFIG_NAK: u32 = 41;
const STALE: u32 = 42;
const CONFIG_NO_EARLY_DATA: u32 = 43;
const HAS_CONTEXT: u32 = 44;
const ECH_PROVIDER: u32 = 45;
const HAS_ENDPOINT: u32 = 46;
const HAS_SERVICE_ENDPOINT: u32 = 47;
const CONFIG_ECH_RETRY: u32 = 48;
const CONFIG_ECH_ENDPOINT: u32 = 49;
const CONFIG_ECH_SERVICE: u32 = 50;
const CONFIG_HAS_ECH: u32 = 51;
const EXPECT_DIRECT: u32 = 52;
const SHOULD_TRUST_IDS: u32 = 53;
const CONFIG_TRUST_IDS: u32 = 54;
const CONFIG_PADDING: u32 = 55;
const LOG_SSL: u32 = 56;
const LOG_ECH_FLAG: u32 = 57;
const LOG_ECH_BYTES: u32 = 58;
const CONFIG_HAS_TRUST: u32 = 59;
const LOG_TRUST: u32 = 60;
const CONFIG_HAS_PADDING: u32 = 61;
const LOG_PADDING: u32 = 62;
const CREATE_SSL: u32 = 63;
const CONNECT_SSL: u32 = 64;
const STAMP_SSL_END: u32 = 65;
const HAS_PEER: u32 = 66;
const APPEND_PEER_ERROR: u32 = 67;
const CLEAR_PEER: u32 = 68;
const DISABLE_STALE: u32 = 69;
const LEGACY_FALLBACK: u32 = 70;
const DISABLE_FALLBACK: u32 = 71;
const ENDPOINT_ECH: u32 = 72;
const SERVICE_ECH: u32 = 73;
const SAVE_ECH_RETRY: u32 = 74;
const EXPECT_ECH_CAPABLE: u32 = 75;
const LOG_ECH_RETRY: u32 = 76;
const RECORD_SSL_RESULT: u32 = 77;
const SET_CAPABLE: u32 = 78;
const SET_ENABLED: u32 = 79;
const PUBLISH_SSL: u32 = 80;
const CREATE_CERT_REQUEST: u32 = 81;
const GET_CERT_REQUEST: u32 = 82;
const RESET_TIMER: u32 = 83;
const CLEAR_NESTED_SOCKET: u32 = 84;
const CLEAR_SSL_SOCKET: u32 = 85;
const CLEAR_CERT: u32 = 86;
const CLEAR_NEGOTIATION: u32 = 87;
const CLEAR_STALE: u32 = 88;
const CLEAR_RESOLVE: u32 = 89;
const PRIORITY_NESTED: u32 = 90;
const BAD_STATE: u32 = 92;
const EXPECT_OK: u32 = 93;
const TRACE_DRIVE: u32 = 94;
const TRACE_SSL: u32 = 95;
const PARAMS_IS_SOCKS: u32 = 96;
const PARAMS_IS_HTTP: u32 = 97;

const NONE: i64 = 8;
fn set(c: &mut Controller<impl Services>, s: i64) {
    c.v(SET_STATE, s);
}
fn initial(c: &mut Controller<impl Services>, kind: i64) -> i64 {
    match kind {
        0 => 0,
        1 => 2,
        2 => 4,
        _ => {
            c.s(BAD_STATE);
            NONE
        }
    }
}
fn ech(c: &mut Controller<impl Services>) -> bool {
    c.b(HAS_CONTEXT) && c.b(ECH_PROVIDER)
}
fn restart(c: &mut Controller<impl Services>) {
    c.s(RESET_TIMER);
    c.base(8, 0);
    c.s(CLEAR_TCP);
    c.s(CLEAR_NESTED);
    c.s(CLEAR_NESTED_SOCKET);
    c.s(CLEAR_SSL_SOCKET);
    c.s(CLEAR_CERT);
    c.s(CLEAR_NEGOTIATION);
    c.s(CLEAR_STALE);
    c.s(CLEAR_RESOLVE);
    c.s(CLEAR_PEER);
}
fn restart_initial(c: &mut Controller<impl Services>) {
    let kind = c.v(PARAMS_CONNECTION_TYPE, 0);
    let s = initial(c, kind);
    set(c, s);
}
fn transport(c: &mut Controller<impl Services>) -> i64 {
    set(c, 1);
    if c.b(HAPPY_EYEBALLS) {
        if c.b(HAS_ECH_RETRY) {
            c.s(CREATE_TCP_RETRY);
        } else {
            c.s(CREATE_TCP);
        }
    } else if c.b(HAS_ECH_RETRY) {
        c.s(CREATE_TRANSPORT_RETRY);
    } else {
        c.s(CREATE_TRANSPORT);
    }
    c.v(CONNECT_NESTED, 0)
}
fn transport_done(c: &mut Controller<impl Services>, r: i64) -> i64 {
    c.s(SAVE_RESOLVE);
    c.s(SAVE_RESOLUTION);
    c.s(APPEND_ATTEMPTS);
    if r == 0 {
        set(c, 6);
        c.s(TAKE_NESTED_SOCKET);
        c.s(SAVE_PEER);
        c.s(SAVE_ALIASES);
        c.s(SAVE_STALE);
    }
    r
}
fn socks(c: &mut Controller<impl Services>) -> i64 {
    set(c, 3);
    c.s(CREATE_SOCKS);
    c.v(CONNECT_NESTED, 0)
}
fn socks_done(c: &mut Controller<impl Services>, r: i64) -> i64 {
    c.s(SAVE_RESOLVE);
    c.s(SAVE_RESOLUTION);
    if r == 0 {
        set(c, 6);
        c.s(TAKE_NESTED_SOCKET);
    }
    r
}
fn tunnel(c: &mut Controller<impl Services>) -> i64 {
    set(c, 5);
    c.s(CREATE_TUNNEL);
    c.v(CONNECT_NESTED, 0)
}
fn tunnel_done(c: &mut Controller<impl Services>, r: i64) -> i64 {
    c.s(SAVE_RESOLVE);
    c.s(SAVE_RESOLUTION);
    c.s(TAKE_NESTED_SOCKET);
    if r < 0 {
        if r == i64::from(ERR_SSL_CLIENT_AUTH_CERT_NEEDED) {
            c.s(SAVE_CERTIFICATE);
        }
        return r;
    }
    set(c, 6);
    r
}
fn log_ssl(c: &mut Controller<impl Services>) -> i64 {
    let enabled = ech(c);
    c.v(LOG_ECH_FLAG, i64::from(enabled));
    c.s(LOG_ECH_BYTES);
    if c.b(CONFIG_HAS_TRUST) {
        c.s(LOG_TRUST);
    }
    if c.b(CONFIG_HAS_PADDING) {
        c.s(LOG_PADDING);
    }
    0
}
fn ssl(c: &mut Controller<impl Services>) -> i64 {
    set(c, 7);
    c.s(HANDSHAKE_TIMER);
    c.base(8, 0);
    c.s(COPY_CONNECT_TIMING);
    c.s(SET_NEGOTIATION);
    c.s(STAMP_SSL_START);
    if c.b(IS_TCP) {
        c.s(SAVE_SERVICE_ENDPOINT);
    } else {
        c.s(SAVE_ENDPOINT);
    }
    c.s(CONFIG_INIT);
    c.s(CONFIG_IGNORE_ERRORS);
    c.s(CONFIG_NAK);
    if c.b(STALE) {
        c.s(CONFIG_NO_EARLY_DATA);
    }
    if ech(c) {
        if c.b(HAS_ECH_RETRY) {
            c.s(CONFIG_ECH_RETRY);
        } else if c.b(HAS_ENDPOINT) {
            c.s(CONFIG_ECH_ENDPOINT);
        } else if c.b(HAS_SERVICE_ENDPOINT) {
            c.s(CONFIG_ECH_SERVICE);
        }
        if c.b(CONFIG_HAS_ECH) {
            c.s(EXPECT_DIRECT);
        }
    }
    if c.b(SHOULD_TRUST_IDS) {
        c.s(CONFIG_TRUST_IDS);
    }
    c.s(CONFIG_PADDING);
    c.s(LOG_SSL);
    c.s(CREATE_SSL);
    c.s(CLEAR_TCP);
    c.s(CLEAR_NESTED);
    c.v(CONNECT_SSL, 0)
}
fn ssl_done(c: &mut Controller<impl Services>, r: i64) -> i64 {
    c.s(STAMP_SSL_END);
    if r != 0 && c.b(HAS_PEER) {
        c.v(APPEND_PEER_ERROR, r);
        c.s(CLEAR_PEER);
    }
    if r != 0 && c.b(STALE) {
        restart(c);
        c.s(DISABLE_STALE);
        restart_initial(c);
        return 0;
    }
    if c.b(LEGACY_FALLBACK)
        && matches!(r,x if x==i64::from(ERR_CONNECTION_CLOSED)||x==i64::from(ERR_CONNECTION_RESET)
  ||x==i64::from(ERR_SSL_PROTOCOL_ERROR)||x==i64::from(ERR_SSL_VERSION_OR_CIPHER_MISMATCH))
    {
        restart(c);
        c.s(DISABLE_FALLBACK);
        restart_initial(c);
        return 0;
    }
    let capable =
        (c.b(HAS_ENDPOINT) && c.b(ENDPOINT_ECH)) || (c.b(HAS_SERVICE_ENDPOINT) && c.b(SERVICE_ECH));
    let enabled = ech(c);
    if !c.b(HAS_ECH_RETRY) && r == i64::from(ERR_ECH_NOT_NEGOTIATED) && enabled {
        c.v(EXPECT_ECH_CAPABLE, i64::from(capable));
        c.s(SAVE_ECH_RETRY);
        c.s(LOG_ECH_RETRY);
        restart(c);
        restart_initial(c);
        return 0;
    }
    c.v(SET_CAPABLE, i64::from(capable));
    c.v(SET_ENABLED, i64::from(enabled));
    c.v(RECORD_SSL_RESULT, r);
    if r == 0 || crate::connection_jobs_upper_rust_common::certificate(r) {
        c.s(PUBLISH_SSL);
        c.base(5, 0);
    } else if r == i64::from(ERR_SSL_CLIENT_AUTH_CERT_NEEDED) {
        c.s(CREATE_CERT_REQUEST);
        c.s(GET_CERT_REQUEST);
    }
    r
}
fn drive(c: &mut Controller<impl Services>, mut r: i64) -> i64 {
    while c.alive {
        let s = c.v(STATE, 0);
        set(c, NONE);
        r = match s {
            0 => {
                c.v(EXPECT_OK, r);
                transport(c)
            }
            1 => transport_done(c, r),
            2 => {
                c.v(EXPECT_OK, r);
                socks(c)
            }
            3 => socks_done(c, r),
            4 => {
                c.v(EXPECT_OK, r);
                tunnel(c)
            }
            5 => tunnel_done(c, r),
            6 => {
                c.v(EXPECT_OK, r);
                c.v(TRACE_SSL, 0)
            }
            7 => ssl_done(c, r),
            _ => {
                c.s(BAD_STATE);
                return r;
            }
        };
        if !c.alive || r == i64::from(ERR_IO_PENDING) || c.v(STATE, 0) == NONE {
            break;
        }
    }
    r
}
fn io(c: &mut Controller<impl Services>, r: i64) -> i64 {
    let rv = c.v(TRACE_DRIVE, r);
    if rv != i64::from(ERR_IO_PENDING) {
        c.base(6, rv);
    }
    rv
}
fn load(c: &mut Controller<impl Services>) -> i64 {
    match c.v(STATE, 0) {
        0 | 2 | 4 => 0,
        1 | 3 => c.v(NESTED_LOAD, 0),
        5 => {
            if c.b(HAS_NESTED_SOCKET) {
                9
            } else {
                c.v(NESTED_LOAD, 0)
            }
        }
        6 | 7 => 12,
        _ => {
            c.s(BAD_STATE);
            0
        }
    }
}

pub fn run(operation: u32, argument: i64, native: &mut impl Services) -> i64 {
    let c = &mut Controller { module: MODULE, native, alive: true };
    match operation {
        0 => i64::from(ech(c)),
        1 => {
            if c.b(PARAMS_IS_SOCKS) {
                1
            } else if c.b(PARAMS_IS_HTTP) {
                2
            } else {
                0
            }
        }
        2 => {
            c.s(CLEAR_TCP);
            c.s(CLEAR_NESTED);
            0
        }
        3 => load(c),
        4 => i64::from(if c.b(HAS_NESTED) {
            c.b(NESTED_ESTABLISHED)
        } else {
            c.b(HAS_NESTED_SOCKET) || c.b(HAS_SSL_SOCKET)
        }),
        5 => {
            c.s(COPY_ATTEMPTS);
            0
        }
        6 => {
            c.s(COPY_RESOLVE);
            0
        }
        7 => {
            c.s(COPY_RESOLUTION);
            0
        }
        8 => {
            c.s(COPY_CERTIFICATE);
            0
        }
        9 => c.v(NEGOTIATION_STARTED, 0),
        10 => 30_000_000,
        11 => io(c, argument),
        12 => drive(c, argument),
        13 => transport(c),
        14 => transport_done(c, argument),
        15 => socks(c),
        16 => socks_done(c, argument),
        17 => tunnel(c),
        18 => tunnel_done(c, argument),
        19 => ssl(c),
        20 => ssl_done(c, argument),
        21 => initial(c, argument),
        22 => {
            restart_initial(c);
            c.v(TRACE_DRIVE, 0)
        }
        23 => {
            restart(c);
            0
        }
        24 => {
            if c.b(HAS_NESTED) {
                c.v(PRIORITY_NESTED, argument);
            }
            0
        }
        25 => log_ssl(c),
        26 => drive(c, argument),
        27 => ssl(c),
        _ => i64::from(ERR_UNEXPECTED),
    }
}
