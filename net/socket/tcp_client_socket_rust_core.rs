// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct State {
    pub index: i64,
    pub phase: i64,
    pub previous: i64,
    pub received: i64,
    pub used: i64,
    pub suspended: i64,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Operation {
    Init = 0,
    Bind = 1,
    Connect = 2,
    CompleteConnect = 3,
    Disconnect = 4,
    Suspend = 5,
    Read = 6,
    ReadReady = 7,
    Write = 8,
    CompleteRead = 9,
    CompleteWrite = 10,
    CancelRead = 11,
    LocalAddress = 12,
    Open = 13,
    Timeout = 14,
}
impl Operation {
    pub fn from_wire(n: u32) -> Option<Self> {
        Some(match n {
            0 => Self::Init,
            1 => Self::Bind,
            2 => Self::Connect,
            3 => Self::CompleteConnect,
            4 => Self::Disconnect,
            5 => Self::Suspend,
            6 => Self::Read,
            7 => Self::ReadReady,
            8 => Self::Write,
            9 => Self::CompleteRead,
            10 => Self::CompleteWrite,
            11 => Self::CancelRead,
            12 => Self::LocalAddress,
            13 => Self::Open,
            14 => Self::Timeout,
            _ => return None,
        })
    }
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Action {
    Save = 0,
    Valid = 1,
    Count = 2,
    HasBind = 3,
    Open = 4,
    BindInput = 5,
    BindSaved = 6,
    SaveBind = 7,
    ClearBind = 8,
    Close = 9,
    Invalidate = 10,
    Defaults = 11,
    HasNetwork = 12,
    BindNetwork = 13,
    PortAllowed = 14,
    Before = 15,
    Watcher = 16,
    HasBefore = 17,
    Notify = 18,
    StartAttempt = 19,
    HasAttempt = 20,
    Emit = 21,
    StopTimer = 22,
    TimeoutEnabled = 23,
    HasRtt = 24,
    MinTimeout = 26,
    MaxTimeout = 27,
    AdaptiveTimeout = 28,
    StartTimer = 29,
    Connect = 30,
    LogStart = 31,
    LogEnd = 32,
    ResetCallbacks = 33,
    StoreConnect = 34,
    Read = 35,
    ReadReady = 36,
    Write = 37,
    StoreRead = 38,
    StoreWrite = 39,
    ResetRead = 40,
    CancelRead = 41,
    TakeWrite = 42,
    RunWrite = 43,
    CompleteRead = 44,
    ReadPending = 45,
    InvokeConnect = 46,
    InvokeRead = 47,
    InvokeWrite = 48,
    LocalAddress = 49,
    CopyBind = 50,
    ValidateConnect = 51,
}
// Each call commits the by-value state before executing exactly one native
// primitive. The response snapshots any reentrant changes. None means owner
// destruction or an intervening Disconnect invalidated this invocation.
pub trait Services {
    fn call(&mut self, state: State, action: Action, value: i64) -> Option<(State, i64)>;
}
pub const OK: i64 = 0;
pub const PENDING: i64 = -1;
pub const ABORTED: i64 = -3;
pub const SUSPENDED: i64 = -331;
pub const UNSAFE_PORT: i64 = -312;
pub const NOT_CONNECTED: i64 = -15;
const CONNECT: i64 = 0;
const COMPLETE: i64 = 1;
const NONE: i64 = 2;
struct Controller<'a, S> {
    state: State,
    native: &'a mut S,
}
impl<S: Services> Controller<'_, S> {
    fn call(&mut self, a: Action, v: i64) -> Option<i64> {
        let (state, result) = self.native.call(self.state, a, v)?;
        self.state = state;
        Some(result)
    }
    fn query(&mut self, a: Action) -> Option<bool> {
        Some(self.call(a, 0)? != 0)
    }
    fn emit_attempt(&mut self, result: i64) -> Option<()> {
        if self.query(Action::HasAttempt)? {
            self.call(Action::Emit, result)?;
            self.call(Action::StopTimer, 0)?;
        }
        Some(())
    }
    fn disconnect(&mut self, full: bool) -> Option<()> {
        self.emit_attempt(ABORTED)?;
        self.state.received = 0;
        self.state.previous = i64::from(self.query(Action::Valid)? && self.state.index >= 0);
        self.call(Action::Close, 0)?;
        self.call(Action::Invalidate, 0)?;
        if full {
            // Cancellation retires the connect continuation as well as its
            // callback. A later suspend must not complete a cancelled connect.
            self.state.phase = NONE;
            self.state.index = -1;
            self.call(Action::ClearBind, 0)?;
            self.state.suspended = 0;
            self.call(Action::ResetCallbacks, 0)?;
        }
        Some(())
    }
    fn open(&mut self, family: i64) -> Option<i64> {
        let rv = self.call(Action::Open, family)?;
        if rv != OK {
            return Some(rv);
        }
        if self.query(Action::HasNetwork)? {
            let rv = self.call(Action::BindNetwork, 0)?;
            if rv != OK {
                self.call(Action::Close, 0)?;
                return Some(rv);
            }
        }
        self.call(Action::Defaults, 0)?;
        Some(OK)
    }
    fn timeout(&mut self) -> Option<i64> {
        if !self.query(Action::TimeoutEnabled)? {
            return Some(i64::MAX);
        }
        let has_rtt = self.query(Action::HasRtt)?;
        // AdaptiveTimeout is solely Chromium's saturating TimeDelta multiply.
        // Native captures the optional RTT once; no estimator policy callback.
        let min = self.call(Action::MinTimeout, 0)?;
        let max = self.call(Action::MaxTimeout, 0)?;
        if !has_rtt {
            return Some(max);
        }
        let adaptive = self.call(Action::AdaptiveTimeout, 0)?;
        Some(if adaptive <= min {
            min
        } else if adaptive >= max {
            max
        } else {
            adaptive
        })
    }
    fn attempt(&mut self) -> Option<i64> {
        assert!(self.state.index >= 0 && self.state.index < self.call(Action::Count, 0)?);
        if self.state.previous != 0 {
            self.state.used = 0;
            self.state.previous = 0;
        }
        self.state.phase = COMPLETE;
        if !self.query(Action::PortAllowed)? {
            return Some(UNSAFE_PORT);
        }
        if !self.query(Action::Valid)? {
            let rv = self.open(-1)?; // -1 selects current endpoint's family.
            if rv != OK {
                return Some(rv);
            }
            if self.query(Action::HasBind)? {
                let rv = self.call(Action::BindSaved, 0)?;
                if rv != OK {
                    self.call(Action::Close, 0)?;
                    return Some(rv);
                }
            }
        }
        if self.query(Action::HasBefore)? {
            let rv = self.call(Action::Before, 0)?;
            debug_assert_ne!(rv, PENDING);
            if rv != OK {
                return Some(rv);
            }
        }
        if self.query(Action::Watcher)? && self.state.index != 0 {
            self.call(Action::Notify, 0)?;
        }
        self.call(Action::StartAttempt, 0)?;
        let timeout = self.timeout()?;
        if timeout != i64::MAX {
            self.call(Action::StartTimer, timeout)?;
        }
        self.call(Action::Connect, 0)
    }
    fn connect_loop(&mut self, mut rv: i64) -> Option<i64> {
        loop {
            let phase = self.state.phase;
            self.state.phase = NONE;
            match phase {
                CONNECT => {
                    debug_assert_eq!(rv, OK);
                    rv = self.attempt()?;
                }
                COMPLETE => {
                    self.emit_attempt(rv)?;
                    if rv != OK && rv != SUSPENDED {
                        self.disconnect(false)?;
                        if self.state.index + 1 < self.call(Action::Count, 0)? {
                            self.state.phase = CONNECT;
                            self.state.index += 1;
                            rv = OK;
                        }
                    }
                }
                _ => panic!("invalid TCP connect state"),
            }
            if rv == PENDING || self.state.phase == NONE {
                return Some(rv);
            }
        }
    }
    fn io_result(&mut self, rv: i64, read: bool, asynchronous: bool) -> Option<i64> {
        if !asynchronous && rv == PENDING {
            self.call(if read { Action::StoreRead } else { Action::StoreWrite }, 0)?;
        } else if rv > 0 {
            self.state.used = 1;
            if read {
                self.state.received += rv;
            }
        }
        Some(rv)
    }
    fn run(&mut self, op: Operation, value: i64) -> Option<i64> {
        use Operation::*;
        match op {
            Init => {
                if self.query(Action::Valid)? {
                    self.call(Action::Defaults, 0)?;
                }
            }
            Open => return self.open(value),
            Timeout => return self.timeout(),
            Bind => {
                assert!(self.state.index < 0 && !self.query(Action::HasBind)?);
                if !self.query(Action::Valid)? {
                    let rv = self.open(value)?;
                    if rv != OK {
                        return Some(rv);
                    }
                }
                let rv = self.call(Action::BindInput, 0)?;
                if rv != OK {
                    return Some(rv);
                }
                self.call(Action::SaveBind, 0)?;
            }
            Connect => {
                if self.query(Action::Valid)? && self.state.index >= 0 {
                    return Some(OK);
                }
                self.call(Action::ValidateConnect, 0)?;
                if self.state.suspended != 0 {
                    self.disconnect(true)?;
                }
                self.call(Action::LogStart, 0)?;
                self.state.phase = CONNECT;
                self.state.index = 0;
                let rv = self.connect_loop(OK)?;
                self.call(if rv == PENDING { Action::StoreConnect } else { Action::LogEnd }, rv)?;
                return Some(rv);
            }
            CompleteConnect => {
                debug_assert_eq!(self.state.phase, COMPLETE);
                debug_assert_ne!(value, PENDING);
                let rv = self.connect_loop(value)?;
                if rv != PENDING {
                    self.call(Action::LogEnd, rv)?;
                    self.call(Action::InvokeConnect, rv)?;
                }
                return Some(rv);
            }
            Disconnect => self.disconnect(true)?,
            Suspend => {
                if self.state.phase != NONE {
                    self.call(Action::Close, 0)?;
                    return self.run(CompleteConnect, SUSPENDED);
                }
                if !self.query(Action::Valid)? {
                    return Some(OK);
                }
                self.call(Action::Close, 0)?;
                self.state.suspended = 1;
                self.call(Action::TakeWrite, 0)?;
                if self.query(Action::ReadPending)? {
                    self.call(Action::CompleteRead, SUSPENDED)?;
                }
                self.call(Action::RunWrite, SUSPENDED)?;
            }
            Read | ReadReady | Write => {
                if self.state.suspended != 0 {
                    return Some(SUSPENDED);
                }
                let read = !matches!(op, Write);
                let action = match op {
                    Read => Action::Read,
                    ReadReady => Action::ReadReady,
                    _ => Action::Write,
                };
                let rv = self.call(action, 0)?;
                return self.io_result(rv, read, false);
            }
            CompleteRead | CompleteWrite => {
                let read = matches!(op, CompleteRead);
                self.io_result(value, read, true)?;
                self.call(if read { Action::InvokeRead } else { Action::InvokeWrite }, value)?;
            }
            CancelRead => {
                self.call(Action::ResetRead, 0)?;
                return self.call(Action::CancelRead, 0);
            }
            LocalAddress => {
                if !self.query(Action::Valid)? {
                    if self.query(Action::HasBind)? {
                        return self.call(Action::CopyBind, 0);
                    }
                    return Some(NOT_CONNECTED);
                }
                return self.call(Action::LocalAddress, 0);
            }
        }
        Some(OK)
    }
}
pub fn run<S: Services>(state: State, native: &mut S, op: Operation, value: i64) -> i64 {
    let mut c = Controller { state, native };
    let Some(result) = c.run(op, value) else {
        return ABORTED;
    };
    // Publish scalar updates even when the final operation was purely local.
    if c.call(Action::Save, 0).is_none() {
        return ABORTED;
    }
    result
}
