// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_upper_rust_common::Controller;
use crate::connection_jobs_rust_core::Services;
use crate::errors::*;
const MODULE: u32 = 13;
const MOVE_SOCKET: u32 = 0;
const PASS_SOCKET: u32 = 1;
const HAS_SOCKET: u32 = 2;
const STAMP_SOURCE_AND_REUSE: u32 = 3;
const COPY_TIMING: u32 = 4;

fn timing(c: &mut Controller<impl Services>, reused: bool) -> i64 {
    if !c.b(HAS_SOCKET) {
        return 0;
    }
    c.v(STAMP_SOURCE_AND_REUSE, i64::from(reused));
    if reused {
        return 1;
    }
    c.s(COPY_TIMING);
    1
}

pub fn run(operation: u32, argument: i64, native: &mut impl Services) -> i64 {
    let c = &mut Controller { module: MODULE, native, alive: true };
    match operation {
        0 => {
            c.s(MOVE_SOCKET);
            0
        }
        1 => {
            c.s(PASS_SOCKET);
            0
        }
        2 => timing(c, argument != 0),
        _ => i64::from(ERR_UNEXPECTED),
    }
}
