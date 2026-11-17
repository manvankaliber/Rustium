// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! ConnectJob BASE sequencing. Native services retain no Rust reference.
#![forbid(unsafe_code)]
#[allow(dead_code)]
#[path = "../base/net_error_rust_ids.rs"]
mod errors;

#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Init = 0,
    Destroy = 1,
    Connect = 2,
    ChangePriority = 3,
    PassSocket = 4,
    SetSocket = 5,
    NotifyCompletion = 6,
    NotifyProxyAuth = 7,
    ResetTimer = 8,
    TimerIsRunning = 9,
    LogStart = 10,
    StopAndLogCompletion = 11,
    CompletionInsideTrace = 12,
    SetDoneClosure = 13,
    DefaultAttempts = 14,
    DefaultResolution = 15,
    DefaultCertificate = 16,
    DefaultEndpoint = 17,
    DefaultSslError = 18,
    DefaultStaleDns = 19,
    DefaultTimeoutHook = 20,
}
impl Operation {
    pub fn from_wire(value: u32) -> Option<Self> {
        Some(match value {
            0 => Self::Init,
            1 => Self::Destroy,
            2 => Self::Connect,
            3 => Self::ChangePriority,
            4 => Self::PassSocket,
            5 => Self::SetSocket,
            6 => Self::NotifyCompletion,
            7 => Self::NotifyProxyAuth,
            8 => Self::ResetTimer,
            9 => Self::TimerIsRunning,
            10 => Self::LogStart,
            11 => Self::StopAndLogCompletion,
            12 => Self::CompletionInsideTrace,
            13 => Self::SetDoneClosure,
            14 => Self::DefaultAttempts,
            15 => Self::DefaultResolution,
            16 => Self::DefaultCertificate,
            17 => Self::DefaultEndpoint,
            18 => Self::DefaultSslError,
            19 => Self::DefaultStaleDns,
            20 => Self::DefaultTimeoutHook,
            _ => return None,
        })
    }
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Action {
    TopLevel = 0,
    HasDelegate = 1,
    DurationMicros = 2,
    StartTimer = 3,
    StopTimer = 4,
    StampStartNow = 5,
    BeginConnectEvent = 6,
    ConnectInternal = 7,
    StoreFinalError = 8,
    StampEndNow = 9,
    EndConnectEvent = 10,
    TakeDelegate = 11,
    ClearDelegate = 12,
    InvokeCompletionDelegate = 13,
    TraceCompletion = 14,
    InvokeProxyAuthDelegate = 15,
    HasInputSocket = 16,
    LogInputSocketSource = 17,
    HasDnsAliases = 18,
    MoveDnsAliases = 19,
    MoveInputSocket = 20,
    ClearSocket = 21,
    TimedOutInternal = 22,
    TimedOutEvent = 23,
    BeginTopEvent = 24,
    EndTopEvent = 25,
    StorePriority = 26,
    ChangePriorityInternal = 27,
    PassSocket = 28,
    InstallDoneClosure = 29,
    TimerRunning = 30,
    EmptyAttempts = 31,
    NoResolution = 32,
    NoCertificate = 33,
    NoEndpoint = 34,
}

#[derive(Clone, Copy, Debug)]
pub struct Reply {
    pub value: i64,
    pub alive: bool,
}
pub trait Services {
    /// Executes one named native primitive. Deletion returns alive=false even
    /// when a primitive (notably ConnectInternal) produced a usable scalar.
    fn call(&mut self, action: Action, argument: i64) -> Reply;
}

struct Controller<'a, S> {
    services: &'a mut S,
    alive: bool,
}
impl<S: Services> Controller<'_, S> {
    fn call(&mut self, action: Action, argument: i64) -> Reply {
        if !self.alive {
            return Reply { value: 0, alive: false };
        }
        let reply = self.services.call(action, argument);
        self.alive = reply.alive;
        reply
    }
    fn step(&mut self, action: Action, argument: i64) -> bool {
        self.call(action, argument).alive
    }
    fn flag(&mut self, action: Action) -> Option<bool> {
        let reply = self.call(action, 0);
        reply.alive.then_some(reply.value != 0)
    }
    fn log_start(&mut self) {
        if self.step(Action::StampStartNow, 0) {
            self.step(Action::BeginConnectEvent, 0);
        }
    }
    fn complete(&mut self, result: i64) {
        if self.step(Action::StopTimer, 0)
            && self.step(Action::StoreFinalError, result)
            && self.step(Action::StampEndNow, 0)
        {
            self.step(Action::EndConnectEvent, result);
        }
    }
    fn notify_inside_trace(&mut self, result: i64) {
        if !self.step(Action::TakeDelegate, 0) {
            return;
        }
        self.complete(result);
        if self.alive {
            self.step(Action::InvokeCompletionDelegate, result);
        }
    }
    fn set_socket(&mut self) {
        match self.flag(Action::HasInputSocket) {
            Some(true) => {
                if !self.step(Action::LogInputSocketSource, 0) {
                    return;
                }
                if self.flag(Action::HasDnsAliases) == Some(true)
                    && !self.step(Action::MoveDnsAliases, 0)
                {
                    return;
                }
                if self.alive {
                    self.step(Action::MoveInputSocket, 0);
                }
            }
            Some(false) => {
                self.step(Action::MoveInputSocket, 0);
            }
            None => {}
        }
    }
    fn connect(&mut self) -> i64 {
        let duration = self.call(Action::DurationMicros, 0);
        if !duration.alive {
            return i64::from(errors::ERR_ABORTED);
        }
        if duration.value != 0 && !self.step(Action::StartTimer, 0) {
            return i64::from(errors::ERR_ABORTED);
        }
        self.log_start();
        if !self.alive {
            return i64::from(errors::ERR_ABORTED);
        }
        let reply = self.call(Action::ConnectInternal, 0);
        // A deleting virtual call returning IO_PENDING was defined originally:
        // the outer method only returns its local result afterward.
        let result = reply.value;
        if reply.alive && result != i64::from(errors::ERR_IO_PENDING) {
            self.complete(result);
            if self.alive {
                self.step(Action::ClearDelegate, 0);
            }
        }
        result
    }
}

pub fn run(operation: Operation, argument: i64, services: &mut impl Services) -> i64 {
    let mut c = Controller { services, alive: true };
    match operation {
        Operation::Init => {
            if c.flag(Action::TopLevel) == Some(true) {
                c.step(Action::BeginTopEvent, 0);
            }
        }
        Operation::Destroy => {
            if c.flag(Action::HasDelegate) == Some(true) {
                c.complete(i64::from(errors::ERR_ABORTED));
            }
            if c.alive && c.flag(Action::TopLevel) == Some(true) {
                c.step(Action::EndTopEvent, 0);
            }
        }
        Operation::Connect => return c.connect(),
        Operation::ChangePriority => {
            if c.step(Action::StorePriority, argument) {
                c.step(Action::ChangePriorityInternal, argument);
            }
        }
        Operation::PassSocket => {
            c.step(Action::PassSocket, 0);
        }
        Operation::SetSocket => c.set_socket(),
        Operation::NotifyCompletion => {
            c.step(Action::TraceCompletion, argument);
        }
        Operation::CompletionInsideTrace => c.notify_inside_trace(argument),
        Operation::NotifyProxyAuth => {
            c.step(Action::InvokeProxyAuthDelegate, 0);
        }
        Operation::ResetTimer => {
            if c.step(Action::StopTimer, 0) {
                let duration = c.call(Action::DurationMicros, 0);
                if duration.alive && duration.value != 0 {
                    c.step(Action::StartTimer, 0);
                }
            }
        }
        Operation::TimerIsRunning => return c.call(Action::TimerRunning, 0).value,
        Operation::LogStart => c.log_start(),
        Operation::StopAndLogCompletion => c.complete(argument),
        Operation::SetDoneClosure => {
            c.step(Action::InstallDoneClosure, 0);
        }
        Operation::DefaultAttempts => {
            c.step(Action::EmptyAttempts, 0);
        }
        Operation::DefaultResolution => {
            c.step(Action::NoResolution, 0);
        }
        Operation::DefaultCertificate => {
            c.step(Action::NoCertificate, 0);
        }
        Operation::DefaultEndpoint => {
            c.step(Action::NoEndpoint, 0);
        }
        Operation::DefaultSslError | Operation::DefaultStaleDns | Operation::DefaultTimeoutHook => {
        }
    }
    0
}

pub fn timeout(services: &mut impl Services) {
    let mut c = Controller { services, alive: true };
    if c.step(Action::ClearSocket, 0)
        && c.step(Action::TimedOutInternal, 0)
        && c.step(Action::TimedOutEvent, 0)
    {
        c.step(Action::TraceCompletion, i64::from(errors::ERR_TIMED_OUT));
    }
}

pub fn done_error(has_error: bool, error: i32) -> i32 {
    if has_error {
        error
    } else {
        errors::ERR_ABORTED
    }
}
pub fn top_level(has_log: bool) -> bool {
    !has_log
}
pub fn invalid_operation() -> i64 {
    i64::from(errors::ERR_UNEXPECTED)
}

#[cfg(test)]
mod tests {
    use super::*;
    struct Native {
        alive: bool,
        delegate: bool,
        duration: i64,
        connect_result: i64,
        delete_in_connect: bool,
        delete_on_notify: bool,
        events: Vec<Action>,
        end_saw_delegate: Option<bool>,
        notifications: usize,
    }
    impl Native {
        fn new() -> Self {
            Self {
                alive: true,
                delegate: true,
                duration: 0,
                connect_result: 0,
                delete_in_connect: false,
                delete_on_notify: false,
                events: Vec::new(),
                end_saw_delegate: None,
                notifications: 0,
            }
        }
    }
    impl Services for Native {
        fn call(&mut self, action: Action, argument: i64) -> Reply {
            assert!(self.alive, "no native call after receiver deletion");
            self.events.push(action);
            let mut value = 0;
            match action {
                Action::HasDelegate => value = i64::from(self.delegate),
                Action::DurationMicros => value = self.duration,
                Action::ConnectInternal => {
                    value = self.connect_result;
                    if self.delete_in_connect {
                        self.alive = false;
                    }
                }
                Action::TakeDelegate | Action::ClearDelegate => self.delegate = false,
                Action::EndConnectEvent => self.end_saw_delegate = Some(self.delegate),
                Action::InvokeCompletionDelegate => {
                    self.notifications += 1;
                    if self.delete_on_notify {
                        self.alive = false;
                    }
                }
                Action::TraceCompletion => {
                    run(Operation::CompletionInsideTrace, argument, self);
                }
                _ => {}
            }
            Reply { value, alive: self.alive }
        }
    }
    #[test]
    fn synchronous_and_async_delegate_observation_differ() {
        let mut sync = Native::new();
        assert_eq!(run(Operation::Connect, 0, &mut sync), 0);
        assert_eq!(sync.end_saw_delegate, Some(true));
        assert!(!sync.delegate);
        assert_eq!(sync.notifications, 0);
        let end = sync.events.iter().position(|x| *x == Action::EndConnectEvent).unwrap();
        let clear = sync.events.iter().position(|x| *x == Action::ClearDelegate).unwrap();
        assert!(end < clear);

        let mut asynchronous = Native::new();
        asynchronous.delete_on_notify = true;
        run(Operation::NotifyCompletion, 0, &mut asynchronous);
        assert_eq!(asynchronous.end_saw_delegate, Some(false));
        assert_eq!(asynchronous.notifications, 1);
        assert!(!asynchronous.alive);
        assert_eq!(asynchronous.events.last(), Some(&Action::InvokeCompletionDelegate));
    }
    #[test]
    fn deleting_pending_virtual_call_still_returns_its_result() {
        let mut native = Native::new();
        native.connect_result = i64::from(errors::ERR_IO_PENDING);
        native.delete_in_connect = true;
        assert_eq!(run(Operation::Connect, 0, &mut native), i64::from(errors::ERR_IO_PENDING));
        assert_eq!(native.events.last(), Some(&Action::ConnectInternal));
        assert!(!native.events.contains(&Action::StopTimer));
        assert!(!native.events.contains(&Action::ClearDelegate));
    }
    #[test]
    fn reset_zero_stops_but_negative_duration_is_not_disabled() {
        let mut zero = Native::new();
        run(Operation::ResetTimer, 0, &mut zero);
        assert_eq!(zero.events, [Action::StopTimer, Action::DurationMicros]);
        let mut negative = Native::new();
        negative.duration = -1;
        run(Operation::ResetTimer, 0, &mut negative);
        assert_eq!(negative.events, [Action::StopTimer, Action::DurationMicros, Action::StartTimer]);
    }
    #[test]
    fn done_error_and_source_selection_use_actual_presence() {
        assert_eq!(done_error(false, 0), errors::ERR_ABORTED);
        assert_eq!(done_error(false, errors::ERR_TIMED_OUT), errors::ERR_ABORTED);
        assert_eq!(done_error(true, 0), 0);
        assert_eq!(done_error(true, errors::ERR_TIMED_OUT), errors::ERR_TIMED_OUT);
        assert!(top_level(false));
        assert!(!top_level(true));
        assert_eq!(Operation::from_wire(u32::MAX), None);
    }
}
