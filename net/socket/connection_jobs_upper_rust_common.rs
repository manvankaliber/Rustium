// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
use crate::connection_jobs_rust_core::Services;
pub fn certificate(error: i64) -> bool {
    net_error_policy_core::certificate(error as i32)
}
pub struct Controller<'a, S: Services> {
    pub module: u32,
    pub native: &'a mut S,
    pub alive: bool,
}
impl<S: Services> Controller<'_, S> {
    pub fn v(&mut self, action: u32, argument: i64) -> i64 {
        if !self.alive {
            return 0;
        }
        let reply = self.native.call(self.module, action, argument);
        self.alive = reply.alive;
        reply.value
    }
    pub fn b(&mut self, action: u32) -> bool {
        self.v(action, 0) != 0
    }
    pub fn s(&mut self, action: u32) {
        self.v(action, 0);
    }
    pub fn base(&mut self, operation: u32, argument: i64) -> i64 {
        if !self.alive {
            return 0;
        }
        let value = crate::base_run(operation, argument, self.native);
        // A terminal base callback may have deleted the derived receiver.
        // Refresh invocation-only liveness before any further upper action.
        let reply = self.native.call(self.module, u32::MAX, 0);
        self.alive = reply.alive;
        value
    }
}
