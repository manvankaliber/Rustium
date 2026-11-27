// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
pub mod backoff_rust_core;
use backoff_rust_core::{Entry, Policy, Services};
use std::ffi::c_void;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct WirePolicy {
    ignore: i32,
    initial_ms: i64,
    factor: f64,
    jitter: f64,
    maximum_ms: i64,
    lifetime_ms: i64,
    always_initial: u8,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct WireEntry {
    failures: i32,
    release: i64,
}
impl From<WirePolicy> for Policy {
    fn from(p: WirePolicy) -> Self {
        Self {
            ignore: p.ignore,
            initial_ms: p.initial_ms,
            factor: p.factor,
            jitter: p.jitter,
            maximum_ms: p.maximum_ms,
            lifetime_ms: p.lifetime_ms,
            always_initial: p.always_initial != 0,
        }
    }
}
impl From<WireEntry> for Entry {
    fn from(e: WireEntry) -> Self {
        Self { failures: e.failures, release: e.release }
    }
}
impl From<Entry> for WireEntry {
    fn from(e: Entry) -> Self {
        Self { failures: e.failures, release: e.release }
    }
}
unsafe extern "C" {
    fn backoff_rust_service_now(ctx: *const c_void) -> i64;
    fn backoff_rust_service_publish(ctx: *const c_void, failures: i32);
    fn backoff_rust_service_entry(ctx: *const c_void) -> WireEntry;
    fn backoff_rust_service_maximum(ctx: *const c_void) -> i64;
    fn backoff_rust_service_lifetime(ctx: *const c_void) -> i64;
    fn backoff_rust_service_random() -> f64;
    fn backoff_rust_service_pow(base: f64, exponent: i32) -> f64;
    fn backoff_rust_service_add(tick: i64, delta: i64) -> i64;
    fn backoff_rust_service_subtract(a: i64, b: i64) -> i64;
    fn backoff_rust_service_milliseconds(a: i64) -> i64;
    fn backoff_rust_service_in_milliseconds(a: i64) -> i64;
    fn backoff_rust_service_checked_delay(a: f64) -> i64;
}
struct Native(*const c_void);
// SAFETY: the opaque context is provided by the synchronous C++ caller, remains
// alive for the call, and is only interpreted by the C++ primitive service.
impl Services for Native {
    fn publish_failures(&mut self, failures: i32) {
        unsafe { backoff_rust_service_publish(self.0, failures) }
    }
    fn current_entry(&self) -> Entry {
        unsafe { backoff_rust_service_entry(self.0).into() }
    }
    fn current_limits(&self) -> (i64, i64) {
        unsafe { (backoff_rust_service_maximum(self.0), backoff_rust_service_lifetime(self.0)) }
    }
    fn now(&mut self) -> i64 {
        unsafe { backoff_rust_service_now(self.0) }
    }
    fn random(&mut self) -> f64 {
        unsafe { backoff_rust_service_random() }
    }
    fn pow(&mut self, a: f64, b: i32) -> f64 {
        unsafe { backoff_rust_service_pow(a, b) }
    }
    fn add_time_delta(&self, a: i64, b: i64) -> i64 {
        unsafe { backoff_rust_service_add(a, b) }
    }
    fn subtract_times(&self, a: i64, b: i64) -> i64 {
        unsafe { backoff_rust_service_subtract(a, b) }
    }
    fn milliseconds(&self, a: i64) -> i64 {
        unsafe { backoff_rust_service_milliseconds(a) }
    }
    fn in_milliseconds(&self, a: i64) -> i64 {
        unsafe { backoff_rust_service_in_milliseconds(a) }
    }
    fn checked_delay_us(&self, a: f64) -> i64 {
        unsafe { backoff_rust_service_checked_delay(a) }
    }
}
/// # Safety
/// The opaque context must be the live matching C++ entry/list context for
/// this synchronous call and every service callback; no context is retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn backoff_rust_inform(
    e: WireEntry,
    p: WirePolicy,
    ok: u8,
    ctx: *const c_void,
) -> WireEntry {
    let mut e = Entry::from(e);
    e.inform(ok != 0, &p.into(), &mut Native(ctx));
    e.into()
}
/// # Safety
/// The opaque context must be the live matching C++ entry/list context for
/// this synchronous call and every service callback; no context is retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn backoff_rust_reject(e: WireEntry, ctx: *const c_void) -> u8 {
    Entry::from(e).reject(&mut Native(ctx)) as u8
}
/// # Safety
/// The opaque context must be the live matching C++ entry/list context for
/// this synchronous call and every service callback; no context is retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn backoff_rust_until(e: WireEntry, ctx: *const c_void) -> i64 {
    Entry::from(e).until_release(&mut Native(ctx))
}
/// # Safety
/// The opaque context must be the live matching C++ entry/list context for
/// this synchronous call and every service callback; no context is retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn backoff_rust_discard(
    e: WireEntry,
    p: WirePolicy,
    ctx: *const c_void,
) -> u8 {
    Entry::from(e).discard(&p.into(), &mut Native(ctx)) as u8
}
/// # Safety
/// The opaque context must be the live matching C++ entry/list context for
/// this synchronous call and every service callback; no context is retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn backoff_rust_release_for_duration(
    duration: i64,
    p: WirePolicy,
    ctx: *const c_void,
) -> i64 {
    Entry::release_for_duration(duration, &p.into(), &mut Native(ctx))
}
#[repr(C)]
pub struct WireDecoded {
    pub failures: i32,
    pub duration: i64,
    pub absolute: i64,
    pub valid: u8,
}
#[repr(C)]
pub struct WirePair {
    pub first: i64,
    pub second: i64,
}
#[repr(C)]
pub struct WireParsed {
    pub value: i64,
    pub valid: u8,
}
unsafe extern "C" {
    fn backoff_rust_list_len(ctx: *const c_void) -> usize;
    fn backoff_rust_list_kind(ctx: *const c_void, index: usize) -> u8;
    fn backoff_rust_list_integer(ctx: *const c_void, index: usize) -> i32;
    fn backoff_rust_list_double_seconds(ctx: *const c_void, index: usize) -> i64;
    fn backoff_rust_list_parse(ctx: *const c_void, index: usize) -> WireParsed;
    fn backoff_rust_delta_subtract(a: i64, b: i64) -> i64;
    fn backoff_rust_wall_add(a: i64, b: i64) -> i64;
}
impl backoff_rust_core::SerializerServices for Native {
    fn len(&self) -> usize {
        unsafe { backoff_rust_list_len(self.0) }
    }
    fn kind(&self, i: usize) -> u8 {
        unsafe { backoff_rust_list_kind(self.0, i) }
    }
    fn integer(&self, i: usize) -> i32 {
        unsafe { backoff_rust_list_integer(self.0, i) }
    }
    fn double_seconds(&self, i: usize) -> i64 {
        unsafe { backoff_rust_list_double_seconds(self.0, i) }
    }
    fn parse_i64(&self, i: usize) -> Option<i64> {
        let p = unsafe { backoff_rust_list_parse(self.0, i) };
        (p.valid != 0).then_some(p.value)
    }
    fn delta_subtract(&self, a: i64, b: i64) -> i64 {
        unsafe { backoff_rust_delta_subtract(a, b) }
    }
    fn wall_add(&self, a: i64, b: i64) -> i64 {
        unsafe { backoff_rust_wall_add(a, b) }
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn backoff_rust_serialize_times(release: i64, now: i64, wall: i64) -> WirePair {
    let (first, second) =
        backoff_rust_core::serialize_times(release, now, wall, &Native(std::ptr::null()));
    WirePair { first, second }
}
/// # Safety
/// The opaque context must be the live matching C++ entry/list context for
/// this synchronous call and every service callback; no context is retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn backoff_rust_decode(ctx: *const c_void) -> WireDecoded {
    match backoff_rust_core::decode(&Native(ctx)) {
        Some(v) => WireDecoded {
            failures: v.failures,
            duration: v.duration,
            absolute: v.absolute,
            valid: 1,
        },
        None => WireDecoded { failures: 0, duration: 0, absolute: 0, valid: 0 },
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn backoff_rust_restore_duration(
    original: i64,
    absolute: i64,
    now: i64,
) -> WireParsed {
    match backoff_rust_core::restore_duration(original, absolute, now, &Native(std::ptr::null())) {
        Some(value) => WireParsed { value, valid: 1 },
        None => WireParsed { value: 0, valid: 0 },
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn backoff_rust_finite_release(release: i64) -> u8 {
    backoff_rust_core::finite_release(release) as u8
}
#[unsafe(no_mangle)]
pub extern "C" fn backoff_rust_reset() -> WireEntry {
    let mut entry = Entry::default();
    entry.reset();
    entry.into()
}
#[unsafe(no_mangle)]
pub extern "C" fn backoff_rust_custom(e: WireEntry, release: i64) -> WireEntry {
    let mut e = Entry::from(e);
    e.custom_release(release);
    e.into()
}
