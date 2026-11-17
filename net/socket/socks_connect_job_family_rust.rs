// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_upper_rust_common::Controller;
use crate::connection_jobs_rust_core::Services;
use crate::errors::*;
const MODULE: u32 = 5;
const GET_STATE: u32 = 0;
const SET_STATE: u32 = 1;
const HAS_TRANSPORT: u32 = 2;
const TRANSPORT_LOAD: u32 = 3;
const COPY_RESOLVE: u32 = 4;
const CLEAR_TRANSPORT: u32 = 5;
const CREATE_TRANSPORT: u32 = 6;
const CONNECT_TRANSPORT: u32 = 7;
const SAVE_TRANSPORT_RESOLVE: u32 = 8;
const RESET_HANDSHAKE_TIMER: u32 = 9;
const IS_V5: u32 = 10;
const CREATE_V5: u32 = 11;
const CREATE_V4: u32 = 12;
const CONNECT_SOCKET: u32 = 13;
const SAVE_V4_RESOLVE: u32 = 14;
const DISCONNECT_SOCKET: u32 = 15;
const PUBLISH_SOCKET: u32 = 16;
const PRIORITY: u32 = 18;
const BAD_STATE: u32 = 19;
const EXPECT_OK: u32 = 20;

const NONE: i64 = 4;
fn set(c: &mut Controller<impl Services>, state: i64) {
    c.v(SET_STATE, state);
}
fn transport(c: &mut Controller<impl Services>) -> i64 {
    set(c, 1);
    c.s(CREATE_TRANSPORT);
    c.v(CONNECT_TRANSPORT, 0)
}
fn transport_done(c: &mut Controller<impl Services>, result: i64) -> i64 {
    c.s(SAVE_TRANSPORT_RESOLVE);
    if result != 0 {
        return i64::from(ERR_PROXY_CONNECTION_FAILED);
    }
    c.s(RESET_HANDSHAKE_TIMER);
    c.base(8, 0);
    set(c, 2);
    result
}
fn socks(c: &mut Controller<impl Services>) -> i64 {
    set(c, 3);
    if c.b(IS_V5) {
        c.s(CREATE_V5);
    } else {
        c.s(CREATE_V4);
    }
    c.s(CLEAR_TRANSPORT);
    c.v(CONNECT_SOCKET, 0)
}
fn socks_done(c: &mut Controller<impl Services>, result: i64) -> i64 {
    if !c.b(IS_V5) {
        c.s(SAVE_V4_RESOLVE);
    }
    if result != 0 {
        c.s(DISCONNECT_SOCKET);
    } else {
        c.s(PUBLISH_SOCKET);
        c.base(5, 0);
    }
    result
}
fn drive(c: &mut Controller<impl Services>, mut result: i64) -> i64 {
    while c.alive {
        let state = c.v(GET_STATE, 0);
        set(c, NONE);
        result = match state {
            0 => {
                c.v(EXPECT_OK, result);
                transport(c)
            }
            1 => transport_done(c, result),
            2 => {
                c.v(EXPECT_OK, result);
                socks(c)
            }
            3 => socks_done(c, result),
            _ => {
                c.s(BAD_STATE);
                return result;
            }
        };
        if !c.alive || result == i64::from(ERR_IO_PENDING) || c.v(GET_STATE, 0) == NONE {
            break;
        }
    }
    result
}
fn io(c: &mut Controller<impl Services>, result: i64) -> i64 {
    let rv = drive(c, result);
    if rv != i64::from(ERR_IO_PENDING) {
        c.base(6, rv);
    }
    rv
}
fn load(c: &mut Controller<impl Services>) -> i64 {
    match c.v(GET_STATE, 0) {
        0 => 0,
        1 => c.v(TRANSPORT_LOAD, 0),
        2 | 3 => 11,
        _ => {
            c.s(BAD_STATE);
            0
        }
    }
}

pub fn run(operation: u32, argument: i64, native: &mut impl Services) -> i64 {
    let c = &mut Controller { module: MODULE, native, alive: true };
    match operation {
        0 => {
            c.s(CLEAR_TRANSPORT);
            0
        }
        1 => load(c),
        2 => {
            let s = c.v(GET_STATE, 0);
            i64::from(s == 2 || s == 3)
        }
        3 => {
            c.s(COPY_RESOLVE);
            0
        }
        4 => 30_000_000,
        5 => io(c, argument),
        6 => drive(c, argument),
        7 => transport(c),
        8 => transport_done(c, argument),
        9 => socks(c),
        10 => socks_done(c, argument),
        11 => {
            set(c, 0);
            drive(c, 0)
        }
        12 => {
            if c.b(HAS_TRANSPORT) {
                c.v(PRIORITY, argument);
            }
            0
        }
        _ => i64::from(ERR_UNEXPECTED),
    }
}
