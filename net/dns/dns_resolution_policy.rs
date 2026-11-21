// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#![forbid(unsafe_code)]

/// All replies are initialized values. Native ownership never crosses this API.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Words {
    pub a: u64,
    pub b: u64,
    pub x: i64,
    pub y: i64,
}
impl Words {
    pub const fn arg(a: u64) -> Self {
        Self { a, b: 0, x: 0, y: 0 }
    }
    pub const fn signed(x: i64) -> Self {
        Self { a: 0, b: 0, x, y: 0 }
    }
    pub const fn flag(a: bool) -> Self {
        Self::arg(a as u64)
    }
}
pub trait Services {
    fn call(&mut self, operation: u32, input: Words) -> Words;
    fn get(&mut self, operation: u32) -> Words {
        self.call(operation, Words::default())
    }
    fn yes(&mut self, operation: u32) -> bool {
        self.get(operation).a != 0
    }
    fn act(&mut self, operation: u32) {
        let _ = self.get(operation);
    }
    /// Immutable native input, valid until the next service call. The caller
    /// must finish the byte-only algorithm before invoking another service.
    fn bytes(&mut self, _operation: u32) -> &[u8] {
        panic!("this service does not expose native input bytes")
    }
}
#[path = "resolver_task_policy.rs"]
pub mod resolver_task;
#[path = "resolver_client_bridge.rs"]
pub mod client_bridge;
// Shared request/client/transaction modules are bound into this one safe root.
pub mod dns_request_family_ids;
pub mod dns_request_family_services;
pub mod dns_client_policy;
pub mod dns_request_policy;
pub mod dns_endpoint_request_policy;
pub mod dns_transaction_policy;
pub mod dns_probe_policy;
#[cfg(test)]
mod dns_request_family_tests;
#[path = "host_resolver_cache_rust_core.rs"]
pub mod cache;
#[path = "host_resolver_manager_rust_policy.rs"]
pub mod manager;
#[path = "host_resolver_manager_job_rust_policy.rs"]
pub mod job;
#[cfg(test)]
#[path = "resolver_family_policy_tests.rs"]
mod family_tests;
