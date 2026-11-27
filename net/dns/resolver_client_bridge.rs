// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
//! One provider frame per synchronous read-only manager decision, or per query
//! when surrounding operations may replace a session/client. Production policy
//! executes
//! directly in this crate; custom native clients retain their virtual override.
use crate::{Services, Words};
struct Client<'a, S>(&'a mut S);
impl<S: Services> Services for Client<'_, S> {
    fn call(&mut self, operation: u32, input: Words) -> Words {
        self.0.call(10000 + operation, input)
    }
}
pub fn query<S: Services>(s: &mut S, event: u32, input: Words, fallback: u32) -> Words {
    let result = if s.yes(9000) {
        let mut client = Client(s);
        crate::dns_client_policy::run(&mut client, event, input)
    } else {
        let reply = s.get(fallback);
        // Legacy native capability/mode operations return their value in a.
        Words::signed(reply.a as i64)
    };
    s.act(9001);
    result
}
/// Lazily acquires one provider frame for a complete read-only decision. If
/// that decision never queries the client, it performs no frame allocation or
/// open/close callback. Nested capability queries reuse the same availability.
pub struct Scope<'a, S> {
    native: &'a mut S,
    available: Option<Words>,
}
impl<S: Services> Services for Scope<'_, S> {
    fn call(&mut self, operation: u32, input: Words) -> Words {
        match operation {
            9000 => {
                if let Some(result) = self.available {
                    return result;
                }
                let result = self.native.call(operation, input);
                self.available = Some(result);
                result
            }
            9001 => Words::default(),
            _ => self.native.call(operation, input),
        }
    }
    fn bytes(&mut self, operation: u32) -> &[u8] {
        self.native.bytes(operation)
    }
}
pub fn scope<S: Services, R>(s: &mut S, body: impl FnOnce(&mut Scope<'_, S>) -> R) -> R {
    let mut scoped = Scope { native: s, available: None };
    let result = body(&mut scoped);
    if scoped.available.is_some() {
        scoped.native.act(9001);
    }
    result
}
pub fn boolean<S: Services>(s: &mut S, event: u32, input: Words, fallback: u32) -> bool {
    query(s, event, input, fallback).x != 0
}
