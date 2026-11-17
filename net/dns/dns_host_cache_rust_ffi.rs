// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
mod dns_host_cache_rust_core;
#[cxx::bridge(namespace = "net::dns_host_cache_rust")]
mod ffi {
    // SAFETY: CXX checks the non-null reference ABI. The stack-owned service
    // frame lives until execute returns. It never exposes any object pointer
    // or initialized-memory claim to Rust. Calls retain no borrows or callback
    // continuations. ScheduleWrite is terminal and may destroy the owner.
    unsafe extern "C++" {
        include!("net/dns/dns_host_cache_rust_ffi.h");
        #[namespace = "net"]
        type DnsHostCacheRustServices;
        fn Call(self: &DnsHostCacheRustServices, op: u32, a: i64, b: i64, c: i64, d: i64) -> i64;
    }
    extern "Rust" {
        fn stale_value(expired: i64, changes: i32) -> bool;
        fn completed(error: i32) -> bool;
        fn known_ttl(ttl: i64) -> bool;
        fn execute(service: &DnsHostCacheRustServices, method: u32) -> i64;
    }
}
impl dns_host_cache_rust_core::Services for ffi::DnsHostCacheRustServices {
    fn call(&self, op: u32, a: i64, b: i64, c: i64, d: i64) -> i64 {
        self.Call(op, a, b, c, d)
    }
}
fn execute(service: &ffi::DnsHostCacheRustServices, method: u32) -> i64 {
    dns_host_cache_rust_core::run(service, method)
}

fn stale_value(expired: i64, changes: i32) -> bool {
    dns_host_cache_rust_core::stale_value(expired, changes)
}
fn completed(error: i32) -> bool {
    dns_host_cache_rust_core::completed(error)
}
fn known_ttl(ttl: i64) -> bool {
    dns_host_cache_rust_core::known_ttl(ttl)
}
