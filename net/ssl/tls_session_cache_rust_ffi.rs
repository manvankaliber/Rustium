// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Synchronous C ABI for the cache. The only unsafe code decodes initialized
//! foreign views and calls narrow, non-reentrant native value/crypto services.
//! SSL_SESSION_free (which may run ex-data destructors) is deferred until all
//! state borrows end. No state/owner/context is touched after the first free.
#![deny(unsafe_op_in_unsafe_fn)]
mod tls_session_cache_rust_core;
use std::{cmp::Ordering, ffi::c_void};
use tls_session_cache_rust_core::{Cache, Key, Proxy, Server, Services};

#[repr(C)]
pub struct Bytes {
    data: *const u8,
    len: usize,
}
#[repr(C)]
pub struct ServerView {
    host: Bytes,
    port: u16,
}
#[repr(C)]
pub struct ProxyView {
    server: ServerView,
    scheme: i32,
}
#[repr(C)]
pub struct KeyView {
    server: ServerView,
    ip: Bytes,
    nak: *const c_void,
    proxies: *const ProxyView,
    proxy_count: usize,
    proxy_index: usize,
    privacy: i32,
    usage: i32,
    opaque: i32,
    has_ip: bool,
    valid_proxies: bool,
    has_opaque: bool,
}
#[repr(C)]
pub struct Memory {
    percent: i32,
}
unsafe extern "C" {
    fn tls_session_cache_rust_feature(which: u32) -> bool;
    fn tls_session_cache_rust_nak_clone(nak: *const c_void) -> usize;
    fn tls_session_cache_rust_nak_drop(nak: usize);
    fn tls_session_cache_rust_nak_compare(a: usize, b: usize) -> i32;
    fn tls_session_cache_rust_now(clock: *const c_void) -> i64;
    fn tls_session_cache_rust_time(session: usize) -> u64;
    fn tls_session_cache_rust_timeout(session: usize) -> u64;
    fn tls_session_cache_rust_single(session: usize) -> bool;
    fn tls_session_cache_rust_up_ref(session: usize) -> usize;
    fn tls_session_cache_rust_copy(session: usize) -> usize;
    fn tls_session_cache_rust_free(session: usize);
}
pub struct Nak(usize);
impl Drop for Nak {
    fn drop(&mut self) {
        // SAFETY: owns exactly one native NAK allocation.
        unsafe {
            tls_session_cache_rust_nak_drop(self.0);
        }
    }
}
impl PartialEq for Nak {
    fn eq(&self, b: &Self) -> bool {
        self.cmp(b) == Ordering::Equal
    }
}
impl Eq for Nak {}
impl PartialOrd for Nak {
    fn partial_cmp(&self, b: &Self) -> Option<Ordering> {
        Some(self.cmp(b))
    }
}
impl Ord for Nak {
    fn cmp(&self, b: &Self) -> Ordering {
        // SAFETY: both opaque immutable NAKs remain owned for this call.
        unsafe { tls_session_cache_rust_nak_compare(self.0, b.0) }.cmp(&0)
    }
}
struct Native(*const c_void);
impl Services for Native {
    fn stateful_memory(&self) -> bool {
        // SAFETY: native feature lookup, queried only at original policy
        // points.
        unsafe { tls_session_cache_rust_feature(0) }
    }
    fn ignore_memory_pressure(&self) -> bool {
        // SAFETY: immutable feature query; preserve original lazy query order.
        unsafe { tls_session_cache_rust_feature(1) }
    }

    fn now(&self) -> i64 {
        // SAFETY: live base::Clock passed by public adapter.
        unsafe { tls_session_cache_rust_now(self.0) }
    }
    fn time(&self, s: usize) -> u64 {
        // SAFETY: owned SSL_SESSION or null (supported).
        unsafe { tls_session_cache_rust_time(s) }
    }
    fn timeout(&self, s: usize) -> u64 {
        // SAFETY: core calls only for nonnull session.
        unsafe { tls_session_cache_rust_timeout(s) }
    }
    fn single_use(&self, s: usize) -> bool {
        // SAFETY: live owned nonnull session.
        unsafe { tls_session_cache_rust_single(s) }
    }
    fn up_ref(&self, s: usize) -> usize {
        // SAFETY: obtains one additional owned ref.
        unsafe { tls_session_cache_rust_up_ref(s) }
    }
    fn without_early_data(&self, s: usize) -> usize {
        // SAFETY: immutable crypto copy.
        unsafe { tls_session_cache_rust_copy(s) }
    }
}
unsafe fn bytes(v: &Bytes) -> Vec<u8> {
    if v.len == 0 {
        return Vec::new();
    }
    // SAFETY: caller supplies initialized input bytes for this call; copy
    // immediately so no shared slice survives a later native service.
    unsafe { std::slice::from_raw_parts(v.data, v.len) }.to_vec()
}
unsafe fn server(v: &ServerView) -> Server {
    Server(v.port, unsafe { bytes(&v.host) })
}
unsafe fn key(v: &KeyView) -> Key<Nak> {
    let proxies = if v.valid_proxies {
        let mut p = Vec::with_capacity(v.proxy_count);
        for i in 0..v.proxy_count {
            // SAFETY: initialized array of proxy_count ProxyViews, call-scoped.
            let view = unsafe { &*v.proxies.add(i) };
            p.push(Proxy(view.scheme, unsafe { server(&view.server) }));
        }
        Some(p)
    } else {
        None
    };
    Key {
        server: unsafe { server(&v.server) },
        ip: if v.has_ip { Some((v.ip.len, unsafe { bytes(&v.ip) })) } else { None },
        nak: Nak(unsafe { tls_session_cache_rust_nak_clone(v.nak) }),
        privacy: v.privacy,
        usage: v.usage,
        proxies,
        opaque: if v.has_opaque { Some(v.opaque) } else { None },
        proxy_index: v.proxy_index,
    }
}
fn release(garbage: Vec<usize>) {
    for s in garbage {
        // SAFETY: each entry represents exactly one owned ref.
        unsafe {
            tls_session_cache_rust_free(s);
        }
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn tls_session_cache_rust_register_memory_consumer(
    is_mac: bool,
    is_win: bool,
) -> bool {
    tls_session_cache_rust_core::register_memory_consumer(is_mac, is_win)
}
/// # Safety
/// Returned state has a single sequence-confined owner, destroyed exactly once.
#[unsafe(no_mangle)]
pub extern "C" fn tls_session_cache_rust_new(max: usize, count: usize) -> *mut Cache<Nak> {
    Box::into_raw(Box::new(Cache::new(max, count)))
}
/// # Safety
/// State must be live, exclusively owned and not already being destroyed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn tls_session_cache_rust_delete(state: *mut Cache<Nak>) {
    // Detach and destroy state BEFORE releasing sessions, whose final frees can
    // run ex-data callbacks. Destruction reentry into the same C++ object is
    // unsupported, as in the original; no pointer/borrow is used after free.
    let mut state = unsafe { Box::from_raw(state) };
    state.flush();
    let garbage = state.take_garbage();
    drop(state);
    release(garbage);
}
/// # Safety
/// State is live and sequence-confined; no mutating invocation runs
/// concurrently.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn tls_session_cache_rust_stat(state: *const Cache<Nak>, field: u32) -> u64 {
    let s = unsafe { &*state };
    match field {
        0 => s.size() as u64,
        1 => s.max_size as u64,
        2 => s.generation,
        _ => std::process::abort(),
    }
}
/// # Safety
/// Initialized key and native views must live through this synchronous call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn tls_session_cache_rust_compare(
    a: *const KeyView,
    b: *const KeyView,
) -> i32 {
    let a = unsafe { key(&*a) };
    let b = unsafe { key(&*b) };
    match a.cmp(&b) {
        Ordering::Less => -1,
        Ordering::Equal => 0,
        Ordering::Greater => 1,
    }
}
/// # Safety
/// Session is a valid SSL_SESSION (null has BoringSSL's original semantics).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn tls_session_cache_rust_expired(session: usize, now: i64) -> bool {
    tls_session_cache_rust_core::expired(&Native(std::ptr::null()), session, now)
}
/// # Safety
/// State is exclusively sequence-owned. Input views/context are live for this
/// call, contain initialized bytes and never alias mutable Rust storage. Native
/// query/copy/up-ref services cannot reenter. session transfers one ref on
/// Insert. Other pointers may be null only when unused by the selected
/// operation.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn tls_session_cache_rust_run(
    state: *mut Cache<Nak>,
    op: u32,
    view: *const KeyView,
    session: usize,
    generation: u64,
    clock: *const c_void,
    memory: *const Memory,
    servers: *const ServerView,
    count: usize,
) -> usize {
    let (result, garbage) = {
        let s = unsafe { &mut *state };
        let native = Native(clock);
        let result = match op {
            0 => s.lookup(&unsafe { key(&*view) }, &native),
            1 => {
                s.insert(generation, unsafe { key(&*view) }, session, &native);
                0
            }
            2 => {
                s.clear_early_data(&unsafe { key(&*view) }, &native);
                0
            }
            3 => {
                let mut keys = Vec::with_capacity(count);
                for i in 0..count {
                    keys.push(unsafe { server(&*servers.add(i)) });
                }
                keys.sort();
                s.flush_servers(&keys);
                0
            }
            4 => {
                s.flush();
                0
            }
            5 => {
                let m = unsafe { &*memory };
                s.update_limit(m.percent, &native);
                0
            }
            6 => {
                let m = unsafe { &*memory };
                s.release_memory(m.percent, &native);
                0
            }
            _ => std::process::abort(),
        };
        (result, s.take_garbage())
    };
    // All owner/state/input access is complete. A native final free may reenter
    // the public API or delete its owner without leaving a dangling state
    // borrow.
    release(garbage);
    result
}
