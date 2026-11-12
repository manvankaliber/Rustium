// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod port_policy_core;

#[cxx::bridge(namespace = "net::port_policy_internal")]
mod ffi {
    unsafe extern "C++" {
        include!("net/base/port_policy_ffi.h");
        fn port_feature_enabled(localhost: bool) -> bool;
        fn port_feature_param(kind: u8) -> Vec<u8>;
        fn port_invalid_token(kind: u8, token: &[u8]);
    }
    extern "Rust" {
        fn port_valid(port: i32) -> bool;
        fn port_well_known(port: i32) -> bool;
        fn port_scheme(port: i32) -> u8;
        fn port_endpoint(port: i32, enabled: bool, local: bool) -> u8;
        fn port_count() -> usize;
        fn port_reset(ports: &[u16]);
        fn port_insert(port: i32);
        fn port_erase(port: i32) -> bool;
        fn port_allowable(port: i32) -> bool;
        fn port_scoped_allowable() -> i32;
        fn port_set_allowable(port: i32);
        fn port_reload_localhost();
    }
}
use port_policy_core::{Decision, PortPolicy};
use std::collections::BTreeSet;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Mutex, MutexGuard, OnceLock};

// The legacy API relies on caller sequencing when mutating its global state.
// The mutex also permits simultaneous reads without aliased mutable references.
// Feature and logging callbacks are invoked without holding this lock.
static STATE: Mutex<PortPolicy> = Mutex::new(PortPolicy::new());
// Updated under STATE's lock after each explicit-set mutation. A false value
// lets a reader linearize before a concurrent insertion or after a removal.
static HAS_EXPLICIT: AtomicBool = AtomicBool::new(false);
// Once a reload is requested, the mutable localhost override must be read
// under STATE even after the flag is consumed.
static HAS_LOCAL_OVERRIDE: AtomicBool = AtomicBool::new(false);
fn state() -> MutexGuard<'static, PortPolicy> {
    STATE.lock().unwrap_or_else(|e| e.into_inner())
}
fn load(kind: u8) -> BTreeSet<i32> {
    let value = ffi::port_feature_param(kind);
    let (ports, invalid) = port_policy_core::parse_feature(&value);
    for token in invalid {
        ffi::port_invalid_token(kind, token);
    }
    ports
}
fn decision(d: Decision) -> u8 {
    match d {
        Decision::Allow => 0,
        Decision::Block => 1,
        Decision::AllowAndRecord => 2,
        Decision::BlockAndRecord => 3,
    }
}
fn port_valid(port: i32) -> bool {
    PortPolicy::valid(port)
}
fn port_well_known(port: i32) -> bool {
    PortPolicy::well_known(port)
}
fn port_scheme(port: i32) -> u8 {
    if !PortPolicy::valid(port) {
        return decision(Decision::Block);
    }
    let preflight = if HAS_EXPLICIT.load(Ordering::Acquire) {
        state().scheme_preflight(port)
    } else {
        PortPolicy::scheme_without_explicit(port)
    };
    if let Some(result) = preflight {
        return decision(result);
    }
    if !ffi::port_feature_enabled(false) {
        return decision(Decision::Allow);
    }
    static ABUSE: OnceLock<(BTreeSet<i32>, BTreeSet<i32>)> = OnceLock::new();
    let (restricted, monitored) = ABUSE.get_or_init(|| (load(0), load(1)));
    decision(PortPolicy::scheme_feature(port, restricted, monitored))
}
fn port_endpoint(port: i32, enabled: bool, local: bool) -> u8 {
    if !enabled {
        return decision(Decision::Allow);
    }
    if !local || (HAS_EXPLICIT.load(Ordering::Acquire) && state().endpoint_preflight(port, local)) {
        return decision(Decision::Allow);
    }
    static INITIAL: OnceLock<BTreeSet<i32>> = OnceLock::new();
    let initial = INITIAL.get_or_init(|| load(2));
    if !HAS_LOCAL_OVERRIDE.load(Ordering::Acquire) {
        return decision(if initial.contains(&port) {
            Decision::BlockAndRecord
        } else {
            Decision::Allow
        });
    }
    if state().needs_localhost_reload() {
        let ports = load(2);
        state().install_localhost_reload(ports);
    }
    if state().localhost_restricted(initial, port) {
        decision(Decision::BlockAndRecord)
    } else {
        decision(Decision::Allow)
    }
}

fn port_count() -> usize {
    state().count()
}
fn port_reset(ports: &[u16]) {
    let mut guard = state();
    guard.reset(ports);
    HAS_EXPLICIT.store(guard.count() != 0, Ordering::Release);
}
fn port_insert(port: i32) {
    let mut guard = state();
    guard.insert(port);
    HAS_EXPLICIT.store(true, Ordering::Release);
}
fn port_erase(port: i32) -> bool {
    let mut guard = state();
    let removed = guard.erase(port);
    HAS_EXPLICIT.store(guard.count() != 0, Ordering::Release);
    removed
}
fn port_allowable(port: i32) -> bool {
    state().allowable(port)
}
fn port_scoped_allowable() -> i32 {
    state().scoped_allowable()
}
fn port_set_allowable(port: i32) {
    state().set_scoped_allowable(port);
}
fn port_reload_localhost() {
    let mut guard = state();
    guard.reload_localhost();
    HAS_LOCAL_OVERRIDE.store(true, Ordering::Release);
}
