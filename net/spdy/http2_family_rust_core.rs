// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#![forbid(unsafe_code)]

/// An ephemeral native service context. Each method's operation table is bound
/// in primitive-bindings.json. No borrowed native object crosses this trait.
/// Calls may reenter/delete an owner; policy must obey its original weak/life
/// guards and finish immediately after terminal callbacks.
pub trait Services {
    fn call(&mut self, operation: u32, argument: i64) -> i64;
}

#[path = "spdy_buffer_rust_core.rs"]
pub mod buffer;
#[path = "spdy_stream_rust_core.rs"]
pub mod stream;
#[path = "spdy_http_stream_rust_core.rs"]
pub mod http_stream;
#[path = "spdy_buffer_producer_rust_core.rs"]
pub mod producer;

// Parent adds the partner's session/session_pool path declarations together
// with its dependency fragment; their service traits can remain independent.

pub fn dispatch<S: Services>(family: u32, method: u32, services: &mut S) -> i64 {
    match family {
        1 => stream::dispatch(method, services),
        2 => http_stream::dispatch(method, services),
        3 => producer::dispatch(method, services),
        _ => panic!("invalid private HTTP/2 family dispatch"),
    }
}

#[cfg(test)]
#[path = "http2_family_rust_tests.rs"]
mod tests;

// Parent-merged session/pool family modules; source-only declaration merge.
#[path = "spdy_session_rust_core.rs"]
pub mod session;
#[path = "spdy_session_pool_rust_core.rs"]
pub mod session_pool;
#[cfg(test)]
#[path = "http2_session_family_rust_tests.rs"]
mod session_family_tests;
