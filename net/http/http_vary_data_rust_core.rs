// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Vary policy. C++ retains storage, header containers, cryptography and
//! Pickle.
#![forbid(unsafe_code)]

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(C)]
pub struct State {
    pub digest: [u8; 32],
    pub kind: i32,
    pub valid: u32,
}

impl State {
    pub fn digest(&self) -> &[u8] {
        &self.digest[..if self.kind == 1 { 32 } else { 16 }]
    }
}

/// Returned views borrow immutable header containers throughout an operation.
pub trait Headers {
    fn wildcard(&self) -> bool;
    fn next(&self, cursor: &mut usize) -> Option<&[u8]>;
    fn request(&self, name: &[u8]) -> &[u8];
}
pub trait Crypto {
    fn begin(&self, kind: i32);
    fn update(&self, bytes: &[u8]);
    fn finish(&self, kind: i32) -> [u8; 32];
}
pub trait Pickle {
    fn reset_probe(&self);
    fn probe_read(&self, size: usize) -> bool;
    fn read_int(&self) -> Option<i32>;
    fn read(&self, out: &mut [u8]) -> bool;
    fn write_int(&self, value: i32);
    fn write(&self, bytes: &[u8]);
}

pub fn init(state: &mut State, services: &(impl Headers + Crypto), requested: i32) -> bool {
    state.valid = 0;
    if services.wildcard() {
        *state = State { digest: [0; 32], kind: 1, valid: 1 };
        return true;
    }
    let kind = i32::from(requested == 1);
    services.begin(kind);
    let mut cursor = 0;
    let mut processed = false;
    while let Some(name) = services.next(&mut cursor) {
        let value = services.request(name);
        if !value.is_empty() {
            services.update(value);
        }
        // Missing and empty fields both contribute exactly a newline. Order,
        // duplicates and enumeration of empty fields belong to the container.
        services.update(b"\n");
        processed = true;
    }
    if !processed {
        return false;
    }
    *state = State { digest: services.finish(kind), kind, valid: 1 };
    true
}

pub fn matches(state: &State, services: &(impl Headers + Crypto)) -> bool {
    if services.wildcard() {
        return false;
    }
    let mut other = *state;
    // Deliberately do not check validity: the original public API compares the
    // retained digest even after a failed initialization.
    init(&mut other, services, state.kind)
        && other.kind == state.kind
        && other.digest() == state.digest()
}

pub fn load(state: &mut State, pickle: &impl Pickle) -> bool {
    state.valid = 0;
    pickle.reset_probe();
    let kind = if pickle.probe_read(16) && !pickle.probe_read(1) {
        0
    } else {
        match pickle.read_int() {
            Some(kind @ (0 | 1)) => kind,
            _ => return false,
        }
    };
    let mut digest = [0; 32];
    if !pickle.read(&mut digest[..if kind == 1 { 32 } else { 16 }]) {
        return false;
    }
    *state = State { digest, kind, valid: 1 };
    true
}

pub fn persist(state: &State, pickle: &impl Pickle) {
    pickle.write_int(state.kind);
    pickle.write(state.digest());
}
