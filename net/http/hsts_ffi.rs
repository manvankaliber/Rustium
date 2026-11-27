// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod hsts_parser;

#[cxx::bridge(namespace = "net::hsts_internal")]
mod ffi {
    extern "Rust" {
        fn parse_hsts(input: &[u8], limit: u32) -> u64;
    }
}

// A scalar result avoids allocating or constructing output objects at the
// bridge. Bit zero means valid, bit one means includeSubDomains, and the
// remaining bits hold the exact uint32 max-age. Invalid results are zero.
fn parse_hsts(input: &[u8], limit: u32) -> u64 {
    match hsts_parser::parse(input, limit) {
        Some(policy) => {
            ((policy.max_age as u64) << 2) | 1 | u64::from(policy.include_subdomains) << 1
        }
        None => 0,
    }
}
