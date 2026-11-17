// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]

// Call-scoped read-only views. Never the C++ layout or a persisted encoding.
#[derive(Clone, Copy, Debug)]
pub struct Url<'a> {
    pub identity: usize,
    pub scheme: &'a [u8],
    pub host: &'a [u8],
    pub path: &'a [u8],
    pub standard: bool,
    pub is_ip: bool,
    pub cryptographic: bool,
    pub localhost: bool,
    pub host_compliant: bool,
    pub effective_port: i32,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(C)]
pub struct Metadata {
    pub downgrade: i32,
    pub null_initiator: bool,
    pub redirect: i32,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(C)]
pub struct Context {
    pub context: i32,
    pub schemeful_context: i32,
    pub metadata: Metadata,
    pub schemeful_metadata: Metadata,
}

impl Context {
    pub fn inclusive() -> Self {
        Self { context: 3, schemeful_context: 3, ..Self::default() }
    }
    pub fn inclusive_for_set() -> Self {
        Self { context: 2, schemeful_context: 2, ..Self::default() }
    }
    pub fn inclusion(self, access_semantics: i32) -> i32 {
        if access_semantics == 1 {
            self.context
        } else {
            self.schemeful_context
        }
    }
    pub fn same_contexts(self, other: Self) -> bool {
        self.context == other.context && self.schemeful_context == other.schemeful_context
    }
}

#[derive(Clone, Copy, Debug)]
pub struct Features {
    pub port_bound: bool,
    pub scheme_bound: bool,
    pub compliant_host: bool,
    pub reject_non_ascii: bool,
    pub considers_redirect_chain: bool,
    pub time_limited_insecure: bool,
    pub force_third_party_blocking: bool,
    pub third_party_partitioning: bool,
}

impl Features {
    pub fn partially_origin_bound(self) -> bool {
        self.port_bound || self.scheme_bound
    }
    pub fn time_limited(self) -> bool {
        self.scheme_bound && self.time_limited_insecure
    }
    pub fn force_blocking(self) -> bool {
        self.force_third_party_blocking && self.third_party_partitioning
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(C)]
pub struct Options {
    pub exclude_httponly: bool,
    pub context: Context,
    pub update_access_time: bool,
    pub return_excluded: bool,
}

impl Options {
    pub fn original_default() -> Self {
        Self { exclude_httponly: true, update_access_time: true, ..Self::default() }
    }
    pub fn inclusive() -> Self {
        Self { context: Context::inclusive(), ..Self::default() }
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(C)]
pub struct AccessParams {
    pub access_semantics: i32,
    pub scope_semantics: i32,
    pub delegate_trustworthy: bool,
}

pub const PORT_UNSPECIFIED: i32 = -1;
pub const PORT_INVALID: i32 = -2;
