// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_upper_rust_common::Controller;
use crate::connection_jobs_rust_core::Services;
use crate::errors::*;
const MODULE: u32 = 8;
const PRIVACY: u32 = 0;
const SECURE_DNS: u32 = 1;
const DISABLE_FETCH: u32 = 2;
const TARGET_NETWORK: u32 = 3;
const PARTITION: u32 = 4;
const PIECE: u32 = 5;
const APPEND_PIECE: u32 = 6;
const APPEND_DISABLE: u32 = 7;
const APPEND_TARGET: u32 = 8;
const APPEND_DESTINATION: u32 = 9;
const APPEND_NAK: u32 = 10;
const CLEAR_TEXT: u32 = 11;
const COPY_NAK: u32 = 12;
const EMPTY_NAK: u32 = 13;
const DIRECT_INPUT: u32 = 14;
const RANDOM_PROXY: u32 = 15;
const CREATE_CAPACITY: u32 = 16;
const EMPTY_CAPACITY: u32 = 17;
const SECONDS_INPUT: u32 = 18;
const CHECK_SECONDS: u32 = 19;
const DICT_GROUP: u32 = 20;
const LOG_REQUEST: u32 = 21;
const USING_SSL: u32 = 22;
const DIRECT_OWNER: u32 = 23;
const INSTALL_RESOLUTION_CALLBACK: u32 = 24;
const WEBSOCKETS: u32 = 25;
const CREATE_JOB: u32 = 26;
const HTTP11: u32 = 27;
const HTTP_ALL: u32 = 28;
const TUNNEL: u32 = 29;
const EXPAND_ALLOCATE: u32 = 30;
const EXPAND_RELEASE: u32 = 31;
const HAS_OVERRIDE: u32 = 32;
const OVERRIDE_CAP: u32 = 33;
const BASE_CAP: u32 = 34;
const HOST_CHECK: u32 = 35;
const HOST_LEGACY: u32 = 36;
const HOST_NOTIFY_LEGACY: u32 = 37;
const HOST_NOTIFY_SERVICE: u32 = 38;
const BAD_STATE: u32 = 39;

use std::sync::atomic::{AtomicI64, Ordering};
static USED_IDLE_TIMEOUT: AtomicI64 = AtomicI64::new(300);
fn privacy(c: &mut Controller<impl Services>, mode: i64) -> i64 {
    match mode {
        0..=3 => mode,
        _ => {
            c.s(BAD_STATE);
            0
        }
    }
}
fn secure(c: &mut Controller<impl Services>, mode: i64) -> i64 {
    match mode {
        0 => 0,
        1 => 4,
        2 => 5,
        _ => {
            c.s(BAD_STATE);
            0
        }
    }
}
fn text(c: &mut Controller<impl Services>) -> i64 {
    c.s(CLEAR_TEXT);
    if c.b(DISABLE_FETCH) {
        c.s(APPEND_DISABLE);
    }
    let mode = c.v(SECURE_DNS, 0);
    let p = secure(c, mode);
    c.v(PIECE, p);
    c.s(APPEND_PIECE);
    let mode = c.v(PRIVACY, 0);
    let p = privacy(c, mode);
    c.v(PIECE, p);
    c.s(APPEND_PIECE);
    if c.v(TARGET_NETWORK, 0) != -1 {
        c.s(APPEND_TARGET);
    }
    c.s(APPEND_DESTINATION);
    if c.b(PARTITION) {
        c.s(APPEND_NAK);
    }
    0
}
fn capacity(c: &mut Controller<impl Services>) -> i64 {
    if c.b(DIRECT_INPUT) || c.b(RANDOM_PROXY) {
        c.s(CREATE_CAPACITY);
    } else {
        c.s(EMPTY_CAPACITY);
    }
    0
}
fn create(c: &mut Controller<impl Services>) -> i64 {
    if c.b(USING_SSL) && c.b(DIRECT_OWNER) {
        c.s(INSTALL_RESOLUTION_CALLBACK);
    }
    let websocket = c.b(WEBSOCKETS);
    c.v(TUNNEL, i64::from(websocket));
    if websocket {
        c.s(HTTP11);
    } else {
        c.s(HTTP_ALL);
    }
    c.s(CREATE_JOB);
    0
}

pub fn run(operation: u32, argument: i64, native: &mut impl Services) -> i64 {
    let c = &mut Controller { module: MODULE, native, alive: true };
    match operation {
        0 => {
            c.s(HOST_CHECK);
            if c.b(HOST_LEGACY) {
                c.v(HOST_NOTIFY_LEGACY, 0)
            } else {
                c.v(HOST_NOTIFY_SERVICE, 0)
            }
        }
        1 => {
            let p = privacy(c, argument);
            c.v(PIECE, p);
            0
        }
        2 => {
            let p = secure(c, argument);
            c.v(PIECE, p);
            0
        }
        3 => text(c),
        4 => USED_IDLE_TIMEOUT.load(Ordering::Relaxed),
        5 => {
            let seconds = c.v(SECONDS_INPUT, 0);
            c.v(CHECK_SECONDS, seconds);
            USED_IDLE_TIMEOUT.store(seconds, Ordering::Relaxed);
            0
        }
        6 => {
            c.s(LOG_REQUEST);
            0
        }
        7 => {
            c.s(DICT_GROUP);
            0
        }
        8 => create(c),
        9 => {
            c.s(EXPAND_ALLOCATE);
            0
        }
        10 => {
            c.s(EXPAND_RELEASE);
            0
        }
        11 => capacity(c),
        12 => {
            if c.b(PARTITION) {
                c.s(COPY_NAK);
            } else {
                c.s(EMPTY_NAK);
            }
            0
        }
        13 => {
            if c.b(HAS_OVERRIDE) {
                c.v(OVERRIDE_CAP, 0)
            } else {
                c.v(BASE_CAP, 0)
            }
        }
        _ => i64::from(ERR_UNEXPECTED),
    }
}
