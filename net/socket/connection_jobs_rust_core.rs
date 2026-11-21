// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Shared scalar service contract. Native storage never becomes a Rust reference.
#![forbid(unsafe_code)]

#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Module {
    Base = 0,
    Transport = 1,
    Tcp = 2,
    Factory = 3,
    Ssl = 4,
    Socks = 5,
    HttpProxy = 6,
    Handle = 7,
    Pool = 8,
    TransportSubJob = 9,
    TcpConnector = 10,
    ParamsFactory = 11,
    AdditionalCapacity = 12,
    StreamHandle = 13,
}
#[derive(Clone, Copy, Debug)]
pub struct Reply {
    pub value: i64,
    pub alive: bool,
}
pub trait Services {
    fn call(&mut self, module: u32, action: u32, argument: i64) -> Reply;
}
pub struct Controller<'a, S: Services> {
    pub native: &'a mut S,
    pub module: Module,
    pub alive: bool,
}
impl<'a, S: Services> Controller<'a, S> {
    pub fn new(native: &'a mut S, module: Module) -> Self {
        Self { native, module, alive: true }
    }
    pub fn call(&mut self, action: u32, argument: i64) -> Reply {
        if !self.alive {
            return Reply { value: 0, alive: false };
        }
        let reply = self.native.call(self.module as u32, action, argument);
        self.alive = reply.alive;
        reply
    }
    pub fn value(&mut self, action: u32, argument: i64) -> i64 {
        self.call(action, argument).value
    }
    pub fn flag(&mut self, action: u32, argument: i64) -> bool {
        self.value(action, argument) != 0
    }
    pub fn base(&mut self, operation: u32, argument: i64) -> i64 {
        if !self.alive {
            return 0;
        }
        let result = crate::base_run(operation, argument, self.native);
        // Refresh invocation-only liveness after a terminal native callback.
        let reply = self.native.call(self.module as u32, u32::MAX, 0);
        self.alive = reply.alive;
        result
    }
    pub fn step(&mut self, action: u32, argument: i64) -> bool {
        self.call(action, argument).alive
    }
}
