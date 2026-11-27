// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#![forbid(unsafe_code)]
use crate::Services;

pub fn produce_buffer<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    0
}

pub fn dispatch<S: Services>(method: u32, services: &mut S) -> i64 {
    match method {
        0 => produce_buffer(services),
        _ => panic!("invalid private HTTP/2 family method"),
    }
}
