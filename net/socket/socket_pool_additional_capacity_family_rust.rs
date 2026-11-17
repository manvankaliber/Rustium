// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_upper_rust_common::Controller;
use crate::connection_jobs_rust_core::Services;
use crate::errors::*;
const MODULE: u32 = 12;
const FEATURE: u32 = 0;
const CREATE_FEATURE: u32 = 1;
const CREATE_EMPTY: u32 = 2;
const BASE_BITS: u32 = 3;
const CAPACITY: u32 = 4;
const MINIMUM_BITS: u32 = 5;
const NOISE_BITS: u32 = 6;
const ERROR: u32 = 7;
const CLEAR_BASE: u32 = 8;
const CLEAR_CAPACITY: u32 = 9;
const CLEAR_MINIMUM: u32 = 10;
const CLEAR_NOISE: u32 = 11;
const IN_USE: u32 = 12;
const SOFT_CAP: u32 = 13;
const CURRENT: u32 = 14;
const CHECK_TRANSITION: u32 = 15;
const RANDOM_DOUBLE: u32 = 16;
const RANDOM_BOOL: u32 = 17;
const SET_PERCENTAGE: u32 = 18;
const POWER: u32 = 19;
const SET_PROBABILITY: u32 = 20;
const PERCENT_ALLOCATE: u32 = 21;
const PERCENT_RELEASE: u32 = 22;
const ACTION: u32 = 23;
const ACTIONS_TAKEN: u32 = 24;
const STORE_NEXT: u32 = 25;
const NEXT: u32 = 26;
const TRANSITION_HISTOGRAM: u32 = 27;
const HISTOGRAM_PREFIX: u32 = 28;
const HISTOGRAM_ALLOCATE: u32 = 29;
const HISTOGRAM_RELEASE: u32 = 30;
const HISTOGRAM_FROM_UNCAPPED: u32 = 31;
const HISTOGRAM_FROM_CAPPED: u32 = 32;
const HISTOGRAM_TO_UNCAPPED: u32 = 33;
const HISTOGRAM_TO_CAPPED: u32 = 34;
const OTHER_BASE: u32 = 35;
const OTHER_CAPACITY: u32 = 36;
const OTHER_MINIMUM: u32 = 37;
const OTHER_NOISE: u32 = 38;

fn f(c: &mut Controller<impl Services>, action: u32) -> f64 {
    f64::from_bits(c.v(action, 0) as u64)
}
fn invalid(x: f64) -> bool {
    x < 0.0 || x > 1.0 || x.is_nan()
}
fn validate(c: &mut Controller<impl Services>) -> i64 {
    let mut bad = false;
    if invalid(f(c, BASE_BITS)) {
        c.v(ERROR, 0);
        bad = true;
    }
    if (c.v(CAPACITY, 0) as u64) > 256 {
        c.v(ERROR, 1);
        bad = true;
    }
    if invalid(f(c, MINIMUM_BITS)) {
        c.v(ERROR, 2);
        bad = true;
    }
    if invalid(f(c, NOISE_BITS)) {
        c.v(ERROR, 3);
        bad = true;
    }
    if bad {
        c.s(CLEAR_BASE);
        c.s(CLEAR_CAPACITY);
        c.s(CLEAR_MINIMUM);
        c.s(CLEAR_NOISE);
    }
    0
}
fn common(c: &mut Controller<impl Services>, used: u64, soft: u64) -> Option<i64> {
    if soft > u64::from(u16::MAX) {
        c.v(ERROR, 4);
        return Some(1);
    }
    if used > u64::from(u16::MAX) {
        c.v(ERROR, 5);
        return Some(1);
    }
    let capacity = c.v(CAPACITY, 0) as u64;
    // Both counts <=u16::MAX and validated capacity<=256: no overflow.
    if used > soft + capacity {
        c.v(ERROR, 6);
        return Some(1);
    }
    if used < soft {
        return Some(0);
    }
    if used == soft + capacity {
        return Some(1);
    }
    None
}
fn transition(c: &mut Controller<impl Services>, action: i64, taken: u64) -> bool {
    c.v(CHECK_TRANSITION, taken as i64);
    let percentage = taken as f64 / (c.v(CAPACITY, 0) as u64) as f64;
    let noise = f(c, NOISE_BITS);
    let random = f(c, RANDOM_DOUBLE);
    let sign = if c.b(RANDOM_BOOL) { 1.0 } else { -1.0 };
    let multiple = 1.0 + noise * random * sign;
    c.v(SET_PERCENTAGE, percentage.to_bits() as i64);
    let power = f(c, POWER);
    let unbound = multiple * power;
    // Exact std::max(minimum,std::min(1.0,unbound)) operand/tie/NaN order.
    let upper = if unbound < 1.0 { unbound } else { 1.0 };
    let minimum = f(c, MINIMUM_BITS);
    let bound = if minimum < upper { upper } else { minimum };
    c.v(SET_PROBABILITY, bound.to_bits() as i64);
    if action == 0 {
        c.s(PERCENT_ALLOCATE);
    } else {
        c.s(PERCENT_RELEASE);
    }
    f(c, RANDOM_DOUBLE) < bound
}
fn next(c: &mut Controller<impl Services>, allocation: bool) -> i64 {
    let used = c.v(IN_USE, 0) as u64;
    let soft = c.v(SOFT_CAP, 0) as u64;
    if let Some(result) = common(c, used, soft) {
        return result;
    }
    let current = c.v(CURRENT, 0);
    if allocation {
        if current == 1 {
            return 1;
        }
        let capacity = c.v(CAPACITY, 0) as u64;
        if transition(c, 0, soft + capacity - used) {
            1
        } else {
            0
        }
    } else {
        if current == 0 {
            return 0;
        }
        if transition(c, 1, used - soft) {
            0
        } else {
            1
        }
    }
}
fn log(c: &mut Controller<impl Services>) -> i64 {
    c.s(HISTOGRAM_PREFIX);
    if c.v(ACTION, 0) == 0 {
        c.s(HISTOGRAM_ALLOCATE);
    } else {
        c.s(HISTOGRAM_RELEASE);
    }
    if c.v(CURRENT, 0) == 0 {
        c.s(HISTOGRAM_FROM_UNCAPPED);
    } else {
        c.s(HISTOGRAM_FROM_CAPPED);
    }
    if c.v(NEXT, 0) == 0 {
        c.s(HISTOGRAM_TO_UNCAPPED);
    } else {
        c.s(HISTOGRAM_TO_CAPPED);
    }
    c.s(TRANSITION_HISTOGRAM);
    0
}
fn update(c: &mut Controller<impl Services>, allocation: bool) -> i64 {
    let result = next(c, allocation);
    c.v(STORE_NEXT, result);
    log(c);
    result
}

pub fn run(operation: u32, _argument: i64, native: &mut impl Services) -> i64 {
    let c = &mut Controller { module: MODULE, native, alive: true };
    match operation {
        0 => {
            if c.b(FEATURE) {
                c.s(CREATE_FEATURE);
            } else {
                c.s(CREATE_EMPTY);
            }
            0
        }
        1 => {
            c.s(CREATE_EMPTY);
            0
        }
        2 => validate(c),
        3 => update(c, true),
        4 => update(c, false),
        5 => next(c, true),
        6 => next(c, false),
        7 => {
            let used = c.v(IN_USE, 0) as u64;
            let soft = c.v(SOFT_CAP, 0) as u64;
            common(c, used, soft).unwrap_or(-1)
        }
        8 => {
            let action = c.v(ACTION, 0);
            let taken = c.v(ACTIONS_TAKEN, 0) as u64;
            i64::from(transition(c, action, taken))
        }
        9 => log(c),
        10 => i64::from(
            f(c, BASE_BITS) == f(c, OTHER_BASE)
                && c.v(CAPACITY, 0) == c.v(OTHER_CAPACITY, 0)
                && f(c, MINIMUM_BITS) == f(c, OTHER_MINIMUM)
                && f(c, NOISE_BITS) == f(c, OTHER_NOISE),
        ),
        _ => i64::from(ERR_UNEXPECTED),
    }
}
