// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_upper_rust_common::Controller;
use crate::connection_jobs_rust_core::Services;
use crate::errors::*;
const MODULE: u32 = 7;
const INVALIDATE_WEAK: u32 = 0;
const REQUEST_SOURCE: u32 = 1;
const CHECK_INPUT_GROUP: u32 = 2;
const STORE_POOL: u32 = 3;
const STORE_GROUP: u32 = 4;
const REQUEST_SOCKET: u32 = 5;
const STORE_CALLBACK: u32 = 6;
const HAS_SOCKET: u32 = 7;
const HAS_POOL: u32 = 8;
const POOL_PRIORITY: u32 = 9;
const INITIALIZED: u32 = 10;
const DISCONNECT: u32 = 11;
const LOAD_DIAGNOSTICS: u32 = 12;
const POOL_LOAD: u32 = 13;
const POOL_STALLED: u32 = 14;
const CHECK_ADD_HIGHER: u32 = 15;
const ADD_HIGHER: u32 = 16;
const STORE_HIGHER: u32 = 17;
const CHECK_REMOVE_HIGHER: u32 = 18;
const REMOVE_HIGHER: u32 = 19;
const CLEAR_HIGHER: u32 = 20;
const HAS_HIGHER: u32 = 21;
const CURRENT_HIGHER_INPUT: u32 = 22;
const CLOSE_IDLE: u32 = 23;
const SAVE_ATTEMPTS: u32 = 24;
const SAVE_RESOLVE: u32 = 25;
const SAVE_SSL_ERROR: u32 = 26;
const SAVE_CERT: u32 = 27;
const TAKE_CALLBACK: u32 = 28;
const RESET_CALLBACK: u32 = 29;
const INVOKE_CALLBACK: u32 = 30;
const EXPECT_NOT_PENDING: u32 = 31;
const SET_INITIALIZED: u32 = 32;
const CHECK_GENERATION: u32 = 33;
const BEGIN_SOCKET_USE: u32 = 34;
const VALID_GROUP: u32 = 35;
const CHECK_POOL: u32 = 36;
const END_SOCKET_USE: u32 = 37;
const RELEASE_SOCKET: u32 = 38;
const CANCEL_REQUEST: u32 = 39;
const BAD_STATE: u32 = 40;
const DROP_SOCKET: u32 = 41;
const CLEAR_GROUP: u32 = 42;
const CLEAR_REUSE: u32 = 43;
const CLEAR_POOL: u32 = 44;
const CLEAR_IDLE: u32 = 45;
const CLEAR_TIMING: u32 = 46;
const CLEAR_RESOLUTION: u32 = 47;
const CLEAR_GENERATION: u32 = 48;
const RESET_RESOLVE: u32 = 49;
const RESET_SSL_ERROR: u32 = 50;
const RESET_CERT: u32 = 51;
const CANCEL_INPUT: u32 = 52;
const CANCEL_JOB_INPUT: u32 = 53;
const RESET_DIAGNOSTICS: u32 = 54;

fn error_reset(c: &mut Controller<impl Services>) {
    c.s(RESET_RESOLVE);
    c.s(RESET_SSL_ERROR);
    c.s(RESET_CERT);
}
fn remove_higher(c: &mut Controller<impl Services>) {
    c.s(CHECK_REMOVE_HIGHER);
    if c.b(HAS_POOL) {
        c.s(REMOVE_HIGHER);
        c.s(CLEAR_HIGHER);
    }
}
fn reset(c: &mut Controller<impl Services>, cancel: bool, cancel_job: bool) {
    if c.b(VALID_GROUP) {
        c.s(CHECK_POOL);
        if c.b(INITIALIZED) {
            if c.b(HAS_SOCKET) {
                c.s(END_SOCKET_USE);
                c.s(RELEASE_SOCKET);
            } else {
                c.s(BAD_STATE);
            }
        } else if cancel {
            c.v(CANCEL_REQUEST, i64::from(cancel_job));
        }
    }
    c.v(SET_INITIALIZED, 0);
    c.s(DROP_SOCKET);
    c.s(CLEAR_GROUP);
    c.s(CLEAR_REUSE);
    c.s(RESET_CALLBACK);
    if c.b(HAS_HIGHER) {
        c.s(CURRENT_HIGHER_INPUT);
        remove_higher(c);
    }
    c.s(CLEAR_POOL);
    c.s(CLEAR_IDLE);
    c.s(CLEAR_TIMING);
    c.s(CLEAR_RESOLUTION);
    c.s(CLEAR_GENERATION);
}
fn complete(c: &mut Controller<impl Services>, r: i64) {
    c.v(EXPECT_NOT_PENDING, r);
    if r != 0 {
        if !c.b(HAS_SOCKET) {
            reset(c, false, false);
        } else {
            c.v(SET_INITIALIZED, 1);
        }
        return;
    }
    c.v(SET_INITIALIZED, 1);
    c.s(CHECK_GENERATION);
    c.s(BEGIN_SOCKET_USE);
}
fn init(c: &mut Controller<impl Services>) -> i64 {
    c.s(REQUEST_SOURCE);
    c.s(CHECK_INPUT_GROUP);
    reset(c, true, false);
    error_reset(c);
    c.s(STORE_POOL);
    c.s(STORE_GROUP);
    let r = c.v(REQUEST_SOCKET, 0);
    if r == i64::from(ERR_IO_PENDING) {
        c.s(STORE_CALLBACK);
    } else {
        complete(c, r);
    }
    r
}
fn io(c: &mut Controller<impl Services>, r: i64) -> i64 {
    c.s(TAKE_CALLBACK);
    c.s(RESET_CALLBACK);
    complete(c, r);
    c.v(INVOKE_CALLBACK, r);
    0
}

pub fn run(operation: u32, argument: i64, native: &mut impl Services) -> i64 {
    let c = &mut Controller { module: MODULE, native, alive: true };
    match operation {
        0 => {
            c.s(INVALIDATE_WEAK);
            reset(c, true, false);
            error_reset(c);
            0
        }
        1 => init(c),
        2 => {
            if !c.b(HAS_SOCKET) && c.b(HAS_POOL) {
                c.v(POOL_PRIORITY, argument);
            }
            0
        }
        3 => {
            reset(c, true, false);
            error_reset(c);
            0
        }
        4 => {
            if c.b(INITIALIZED) && c.b(HAS_SOCKET) {
                c.s(DISCONNECT);
            }
            reset(c, true, true);
            error_reset(c);
            0
        }
        5 => {
            c.s(LOAD_DIAGNOSTICS);
            if c.b(HAS_POOL) {
                c.v(POOL_LOAD, 0)
            } else {
                0
            }
        }
        6 => i64::from(c.b(HAS_POOL) && c.b(POOL_STALLED)),
        7 => {
            c.s(CHECK_ADD_HIGHER);
            if c.b(HAS_POOL) {
                c.s(ADD_HIGHER);
                c.s(STORE_HIGHER);
            }
            0
        }
        8 => {
            remove_higher(c);
            0
        }
        9 => {
            if c.b(HAS_POOL) {
                c.s(CLOSE_IDLE);
            }
            0
        }
        10 => {
            c.s(SAVE_ATTEMPTS);
            c.s(SAVE_RESOLVE);
            c.s(SAVE_SSL_ERROR);
            c.s(SAVE_CERT);
            0
        }
        11 => io(c, argument),
        12 => {
            complete(c, argument);
            0
        }
        13 => {
            c.s(RESET_DIAGNOSTICS);
            let cancel = c.b(CANCEL_INPUT);
            let job = c.b(CANCEL_JOB_INPUT);
            reset(c, cancel, job);
            0
        }
        14 => {
            error_reset(c);
            0
        }
        _ => i64::from(ERR_UNEXPECTED),
    }
}
