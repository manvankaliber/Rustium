// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_upper_rust_common::Controller;
use crate::connection_jobs_rust_core::Services;
use crate::errors::*;
const MODULE: u32 = 6;
const STATE: u32 = 0;
const SET_STATE: u32 = 1;
const HAS_NESTED: u32 = 2;
const NESTED_LOAD: u32 = 3;
const NESTED_ESTABLISHED: u32 = 4;
const ESTABLISHED: u32 = 5;
const SET_ESTABLISHED: u32 = 6;
const COPY_RESOLVE: u32 = 7;
const HAS_CERT: u32 = 8;
const COPY_CERT: u32 = 9;
const SCHEME: u32 = 10;
const STAMP_START: u32 = 11;
const ALTERNATE_TIMER: u32 = 12;
const CREATE_TRANSPORT: u32 = 13;
const TUNNEL: u32 = 14;
const FIND_EXISTING_SPDY: u32 = 15;
const CREATE_SSL: u32 = 16;
const EXPECT_HTTPS: u32 = 17;
const CONNECT_NESTED: u32 = 18;
const SAVE_RESOLVE: u32 = 19;
const FIRST_PROXY: u32 = 20;
const ERROR_LATENCY: u32 = 21;
const SAVE_CERT: u32 = 22;
const MULTI_PROXY: u32 = 23;
const MARK_PROXY_CERT: u32 = 24;
const NEGOTIATED: u32 = 25;
const SUCCESS_LATENCY: u32 = 26;
const PUBLISH_UNTUNNELED: u32 = 27;
const HANDSHAKE_TIMER: u32 = 28;
const CREATE_HTTP_SOCKET: u32 = 29;
const CLEAR_NESTED: u32 = 30;
const CONNECT_SOCKET: u32 = 31;
const POST_AUTH: u32 = 32;
const PUBLISH_SOCKET: u32 = 33;
const FIND_SPDY: u32 = 34;
const CREATE_SPDY: u32 = 35;
const SESSION_OK: u32 = 36;
const SESSION_ERROR: u32 = 37;
const TAKE_SESSION: u32 = 38;
const NEW_STREAM_REQUEST: u32 = 39;
const REQUEST_SPDY_STREAM: u32 = 40;
const CLEAR_STREAM_REQUEST: u32 = 41;
const CREATE_SPDY_SOCKET: u32 = 42;
const CREATE_QUIC_REQUEST: u32 = 43;
const REQUEST_QUIC_SESSION: u32 = 44;
const CLEAR_QUIC_REQUEST: u32 = 45;
const TAKE_QUIC_SESSION: u32 = 46;
const REQUEST_QUIC_STREAM: u32 = 47;
const CREATE_QUIC_SOCKET: u32 = 48;
const RESTART_SOCKET: u32 = 49;
const SOCKET_CONNECTED: u32 = 50;
const RESTARTED: u32 = 51;
const SET_RESTARTED: u32 = 52;
const HAS_AUTH: u32 = 53;
const AUTH_CLOSED: u32 = 54;
const CLEAR_SOCKET: u32 = 55;
const HAS_SOCKET: u32 = 56;
const PRIORITY_NESTED: u32 = 57;
const PRIORITY_SOCKET: u32 = 58;
const TIMEOUT_LATENCY: u32 = 59;
const STOP_TIMER: u32 = 60;
const NOTIFY_AUTH: u32 = 61;
const POST_RESTART: u32 = 62;
const HAS_UA: u32 = 63;
const GET_UA: u32 = 64;
const EMPTY_TEXT: u32 = 65;
const BUILD_KEY: u32 = 66;
const BAD_STATE: u32 = 68;
const EXPECT_OK: u32 = 69;
const EXPECT_NOT_PENDING: u32 = 70;
const HAS_ESTIMATOR: u32 = 71;
const READ_RTT: u32 = 72;
const READ_TIMEOUT_CONFIG: u32 = 73;
const IS_HTTPS_INPUT: u32 = 74;
const SSL_MULTIPLIER: u32 = 75;
const PLAIN_MULTIPLIER: u32 = 76;
const MULTIPLY_RTT: u32 = 77;
const BELOW_MIN: u32 = 78;
const ABOVE_MAX: u32 = 79;
const TAKE_MIN: u32 = 80;
const TAKE_MAX: u32 = 81;
const TIMEOUT_MICROS: u32 = 82;
const HTTP_VERSION_INPUT: u32 = 83;
const SCHEME_INPUT: u32 = 84;
const RESULT_INPUT: u32 = 85;
const HISTOGRAM_PREFIX: u32 = 86;
const HISTOGRAM_HTTP1: u32 = 87;
const HISTOGRAM_HTTP2: u32 = 88;
const HISTOGRAM_HTTP3: u32 = 89;
const HISTOGRAM_HTTP: u32 = 90;
const HISTOGRAM_HTTPS: u32 = 91;
const HISTOGRAM_QUIC: u32 = 92;
const HISTOGRAM_SUCCESS: u32 = 93;
const HISTOGRAM_ERROR: u32 = 94;
const HISTOGRAM_TIMEOUT: u32 = 95;
const EMIT_HISTOGRAM: u32 = 96;
const TASK_SCHEDULER: u32 = 97;
const TASK_NET: u32 = 98;
const TASK_DEFAULT: u32 = 99;
const PARAM_OVER_SSL: u32 = 100;
const PARAM_OVER_QUIC: u32 = 101;
const URL_HTTPS: u32 = 102;
const URL_HTTP: u32 = 103;
const PARAM_TUNNEL: u32 = 104;
const CREATE_AUTH: u32 = 105;
const PARAM_PRESENT: u32 = 106;
const PARAM_VALUE: u32 = 107;

const NONE: i64 = 12;
fn set(c: &mut Controller<impl Services>, s: i64) {
    c.v(SET_STATE, s);
}
fn begin(c: &mut Controller<impl Services>) -> i64 {
    c.s(STAMP_START);
    c.s(ALTERNATE_TIMER);
    c.base(8, 0);
    match c.v(SCHEME, 0) {
        64 => {
            set(c, 7);
            c.s(SET_ESTABLISHED);
        }
        4 | 32 => set(c, 1),
        _ => c.s(BAD_STATE),
    }
    0
}
fn transport(c: &mut Controller<impl Services>) -> i64 {
    if c.v(SCHEME, 0) == 4 {
        c.s(CREATE_TRANSPORT);
    } else {
        c.s(EXPECT_HTTPS);
        if c.b(TUNNEL) && c.b(FIND_EXISTING_SPDY) {
            c.s(SET_ESTABLISHED);
            set(c, 5);
            return 0;
        }
        c.s(CREATE_SSL);
    }
    set(c, 2);
    c.v(CONNECT_NESTED, 0)
}
fn transport_done(c: &mut Controller<impl Services>, r: i64) -> i64 {
    c.s(SAVE_RESOLVE);
    let _scheme = c.v(SCHEME, 0);
    if r != 0 {
        if c.b(FIRST_PROXY) {
            c.s(ERROR_LATENCY);
        }
        if crate::connection_jobs_upper_rust_common::certificate(r) {
            c.s(EXPECT_HTTPS);
            return i64::from(ERR_PROXY_CERTIFICATE_INVALID);
        }
        if r == i64::from(ERR_SSL_CLIENT_AUTH_CERT_NEEDED) {
            c.s(EXPECT_HTTPS);
            c.s(SAVE_CERT);
            if c.b(MULTI_PROXY) && !c.b(HAS_CERT) {
                return r;
            }
            c.s(MARK_PROXY_CERT);
            return r;
        }
        if !c.b(FIRST_PROXY) {
            return r;
        }
        return i64::from(ERR_PROXY_CONNECTION_FAILED);
    }
    let proto = c.v(NEGOTIATED, 0);
    if c.b(FIRST_PROXY) {
        c.v(SUCCESS_LATENCY, proto);
    }
    c.s(SET_ESTABLISHED);
    if !c.b(TUNNEL) {
        c.s(PUBLISH_UNTUNNELED);
        c.base(5, 0);
        return r;
    }
    if proto == 2 {
        c.s(EXPECT_HTTPS);
        set(c, 5);
    } else {
        set(c, 3);
    }
    r
}
fn http(c: &mut Controller<impl Services>) -> i64 {
    set(c, 4);
    c.s(HANDSHAKE_TIMER);
    c.base(8, 0);
    c.s(CREATE_HTTP_SOCKET);
    c.s(CLEAR_NESTED);
    c.v(CONNECT_SOCKET, 0)
}
fn http_done(c: &mut Controller<impl Services>, r: i64) -> i64 {
    if r == i64::from(ERR_PROXY_AUTH_REQUESTED) {
        c.s(POST_AUTH);
        return i64::from(ERR_IO_PENDING);
    }
    if r == i64::from(ERR_HTTP_1_1_REQUIRED) {
        return i64::from(ERR_PROXY_HTTP_1_1_REQUIRED);
    }
    if r == i64::from(ERR_BAD_SSL_CLIENT_AUTH_CERT) {
        return i64::from(ERR_PROXY_CONNECTION_FAILED);
    }
    if r == 0 {
        c.s(PUBLISH_SOCKET);
        c.base(5, 0);
    }
    r
}
fn spdy(c: &mut Controller<impl Services>) -> i64 {
    c.s(HANDSHAKE_TIMER);
    c.base(8, 0);
    if c.b(FIND_SPDY) {
        c.s(CLEAR_NESTED);
    } else {
        c.s(CREATE_SPDY);
        c.s(CLEAR_NESTED);
        if !c.b(SESSION_OK) {
            return c.v(SESSION_ERROR, 0);
        }
        c.s(TAKE_SESSION);
    }
    set(c, 6);
    c.s(NEW_STREAM_REQUEST);
    c.v(REQUEST_SPDY_STREAM, 0)
}
fn spdy_done(c: &mut Controller<impl Services>, mut r: i64) -> i64 {
    if r < 0 {
        if r == i64::from(ERR_BAD_SSL_CLIENT_AUTH_CERT) {
            r = i64::from(ERR_PROXY_CONNECTION_FAILED);
        }
        c.s(CLEAR_STREAM_REQUEST);
        return r;
    }
    set(c, 4);
    c.s(CREATE_SPDY_SOCKET);
    c.v(CONNECT_SOCKET, 0)
}
fn quic(c: &mut Controller<impl Services>) -> i64 {
    c.s(HANDSHAKE_TIMER);
    c.base(8, 0);
    set(c, 8);
    c.s(CREATE_QUIC_REQUEST);
    c.v(REQUEST_QUIC_SESSION, 0)
}
fn quic_stream(c: &mut Controller<impl Services>, r: i64) -> i64 {
    if r < 0 {
        c.s(CLEAR_QUIC_REQUEST);
        return r;
    }
    set(c, 9);
    c.s(TAKE_QUIC_SESSION);
    c.s(CLEAR_QUIC_REQUEST);
    c.v(REQUEST_QUIC_STREAM, 0)
}
fn quic_done(c: &mut Controller<impl Services>, r: i64) -> i64 {
    if r < 0 {
        return r;
    }
    set(c, 4);
    c.s(CREATE_QUIC_SOCKET);
    c.v(CONNECT_SOCKET, 0)
}
fn restart(c: &mut Controller<impl Services>) -> i64 {
    c.s(HANDSHAKE_TIMER);
    c.base(8, 0);
    set(c, 11);
    c.v(RESTART_SOCKET, 0)
}
fn restart_done(c: &mut Controller<impl Services>, mut r: i64) -> i64 {
    c.v(EXPECT_NOT_PENDING, r);
    if r == 0 && !c.b(SOCKET_CONNECTED) {
        r = i64::from(ERR_UNABLE_TO_REUSE_CONNECTION_FOR_PROXY_AUTH);
    }
    let mut reconnect = r == i64::from(ERR_UNABLE_TO_REUSE_CONNECTION_FOR_PROXY_AUTH);
    if !c.b(RESTARTED)
        && (r == i64::from(ERR_CONNECTION_CLOSED)
            || r == i64::from(ERR_CONNECTION_RESET)
            || r == i64::from(ERR_CONNECTION_ABORTED)
            || r == i64::from(ERR_SOCKET_NOT_CONNECTED))
    {
        reconnect = true;
        c.s(SET_RESTARTED);
        if c.b(HAS_AUTH) {
            c.s(AUTH_CLOSED);
        }
    }
    if reconnect {
        c.s(CLEAR_SOCKET);
        set(c, 0);
        return 0;
    }
    set(c, 4);
    r
}
fn drive(c: &mut Controller<impl Services>, mut r: i64) -> i64 {
    while c.alive {
        let s = c.v(STATE, 0);
        set(c, NONE);
        r = match s {
            0 => {
                c.v(EXPECT_OK, r);
                begin(c)
            }
            1 => {
                c.v(EXPECT_OK, r);
                transport(c)
            }
            2 => transport_done(c, r),
            3 => {
                c.v(EXPECT_OK, r);
                http(c)
            }
            4 => http_done(c, r),
            5 => {
                c.v(EXPECT_OK, r);
                spdy(c)
            }
            6 => spdy_done(c, r),
            7 => {
                c.v(EXPECT_OK, r);
                quic(c)
            }
            8 => quic_stream(c, r),
            9 => quic_done(c, r),
            10 => {
                c.v(EXPECT_OK, r);
                restart(c)
            }
            11 => restart_done(c, r),
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
    let rv = drive(c, r);
    if rv != i64::from(ERR_IO_PENDING) {
        c.base(6, rv);
    }
    rv
}
fn load(c: &mut Controller<impl Services>) -> i64 {
    match c.v(STATE, 0) {
        2 => c.v(NESTED_LOAD, 0),
        3..=11 => 9,
        1 => {
            c.s(BAD_STATE);
            0
        }
        _ => 0,
    }
}
fn timeout(c: &mut Controller<impl Services>) -> i64 {
    if !c.b(HAS_ESTIMATOR) || !c.b(READ_RTT) {
        return 0;
    }
    c.s(READ_TIMEOUT_CONFIG);
    let multiplier =
        if c.b(IS_HTTPS_INPUT) { c.v(SSL_MULTIPLIER, 0) } else { c.v(PLAIN_MULTIPLIER, 0) };
    c.v(MULTIPLY_RTT, multiplier);
    if c.b(BELOW_MIN) {
        c.s(TAKE_MIN);
    } else if c.b(ABOVE_MAX) {
        c.s(TAKE_MAX);
    }
    c.v(TIMEOUT_MICROS, 0)
}
fn histogram(c: &mut Controller<impl Services>) -> i64 {
    c.s(HISTOGRAM_PREFIX);
    match c.v(HTTP_VERSION_INPUT, 0) {
        0 | 1 => c.s(HISTOGRAM_HTTP1),
        2 => c.s(HISTOGRAM_HTTP2),
        3 => c.s(HISTOGRAM_HTTP3),
        _ => c.s(BAD_STATE),
    }
    match c.v(SCHEME_INPUT, 0) {
        4 => c.s(HISTOGRAM_HTTP),
        32 => c.s(HISTOGRAM_HTTPS),
        64 => c.s(HISTOGRAM_QUIC),
        _ => c.s(BAD_STATE),
    }
    match c.v(RESULT_INPUT, 0) {
        0 => c.s(HISTOGRAM_SUCCESS),
        1 => c.s(HISTOGRAM_ERROR),
        2 => c.s(HISTOGRAM_TIMEOUT),
        _ => c.s(BAD_STATE),
    }
    c.s(EMIT_HISTOGRAM);
    0
}

pub fn run(operation: u32, argument: i64, native: &mut impl Services) -> i64 {
    let c = &mut Controller { module: MODULE, native, alive: true };
    match operation {
        0 => load(c),
        1 => i64::from(c.b(ESTABLISHED) || (c.b(HAS_NESTED) && c.b(NESTED_ESTABLISHED))),
        2 => {
            c.s(COPY_RESOLVE);
            0
        }
        3 => c.v(HAS_CERT, 0),
        4 => {
            c.s(COPY_CERT);
            0
        }
        5 => timeout(c),
        6 => 30_000_000,
        7 => {
            set(c, 0);
            drive(c, 0)
        }
        8 => c.v(SCHEME, 0),
        9 => io(c, argument),
        10 => {
            set(c, 10);
            c.s(POST_RESTART);
            0
        }
        11 => drive(c, argument),
        12 => begin(c),
        13 => transport(c),
        14 => transport_done(c, argument),
        15 => http(c),
        16 => http_done(c, argument),
        17 => spdy(c),
        18 => spdy_done(c, argument),
        19 => quic(c),
        20 => quic_stream(c, argument),
        21 => quic_done(c, argument),
        22 => restart(c),
        23 => restart_done(c, argument),
        24 => {
            if c.b(HAS_NESTED) {
                c.v(PRIORITY_NESTED, argument);
            }
            if c.b(HAS_SOCKET) {
                c.v(PRIORITY_SOCKET, argument);
            }
            0
        }
        25 => {
            if c.v(STATE, 0) == 2 && c.b(FIRST_PROXY) {
                c.s(TIMEOUT_LATENCY);
            }
            0
        }
        26 => {
            c.s(STOP_TIMER);
            c.base(8, 0);
            c.s(NOTIFY_AUTH);
            0
        }
        27 => {
            if c.b(HAS_UA) {
                c.s(GET_UA);
            } else {
                c.s(EMPTY_TEXT);
            }
            0
        }
        28 => {
            c.s(BUILD_KEY);
            0
        }
        29 => histogram(c),
        30 => {
            if c.b(TASK_SCHEDULER) {
                c.s(TASK_NET);
            } else {
                c.s(TASK_DEFAULT);
            }
            0
        }
        31 => {
            if c.b(PARAM_OVER_SSL) || c.b(PARAM_OVER_QUIC) {
                c.s(URL_HTTPS);
            } else {
                c.s(URL_HTTP);
            }
            0
        }
        32 => 0,
        33 => {
            if c.b(PARAM_PRESENT) {
                c.v(PARAM_VALUE, 0)
            } else {
                argument
            }
        }
        34 => {
            if c.b(PARAM_TUNNEL) {
                c.s(CREATE_AUTH);
            }
            0
        }
        _ => i64::from(ERR_UNEXPECTED),
    }
}
