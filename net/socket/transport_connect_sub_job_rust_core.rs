#![forbid(unsafe_code)]
use crate::connection_jobs_rust_core::{Controller, Module, Services};
use crate::errors::*;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Start = 0,
    Load = 1,
    Io = 2,
    Loop = 3,
    Connect = 4,
    Complete = 5,
    Started = 6,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Action {
    State = 0,
    SetState = 1,
    CheckNone = 2,
    CheckLoop = 3,
    CheckOk = 4,
    NotReached = 5,
    HasWatcherFactory = 6,
    MakeWatcher = 7,
    MakeSocket = 8,
    AttemptEvent = 9,
    ApplyTag = 10,
    HasWebsocket = 11,
    WrapWebsocket = 12,
    Connect = 13,
    ResetSocket = 14,
    RecordAttempt = 15,
    Index = 16,
    Count = 17,
    Next = 18,
    ParentComplete = 19,
}

type C<'a, S> = Controller<'a, S>;
const PENDING: i64 = ERR_IO_PENDING as i64;
macro_rules! v {
    ($c:expr,$a:ident) => {$c.value(Action::$a as u32,0)};
}
macro_rules! s {
    ($c:expr,$a:ident) => {$c.step(Action::$a as u32,0)};
    ($c:expr,$a:ident,$x:expr) => {$c.step(Action::$a as u32,$x)};
}
fn connect<S: Services>(c: &mut C<'_, S>) -> i64 {
    s!(c,SetState,2);
    if v!(c,HasWatcherFactory) != 0 {
        s!(c,MakeWatcher);
    }
    s!(c,MakeSocket);
    s!(c,AttemptEvent);
    s!(c,ApplyTag);
    if v!(c,HasWebsocket) != 0 {
        s!(c,WrapWebsocket);
    }
    v!(c,Connect)
}
fn complete<S: Services>(c: &mut C<'_, S>, result: i64) -> i64 {
    s!(c,SetState,3);
    if result != OK as i64 {
        s!(c,ResetSocket);
        s!(c,RecordAttempt,result);
        if result != ERR_NETWORK_IO_SUSPENDED as i64
            && v!(c,Index).checked_add(1).is_some_and(|next| next < v!(c,Count))
        {
            s!(c,SetState,1);
            s!(c,Next);
            return OK as i64;
        }
    }
    result
}
fn run_loop<S: Services>(c: &mut C<'_, S>, mut result: i64) -> i64 {
    s!(c,CheckLoop);
    loop {
        let state = v!(c,State);
        s!(c,SetState,0);
        result = match state {
            1 => {
                s!(c,CheckOk,result);
                connect(c)
            }
            2 => complete(c, result),
            _ => {
                s!(c,NotReached);
                ERR_UNEXPECTED as i64
            }
        };
        if !c.alive || result == PENDING {
            return result;
        }
        if matches!(v!(c,State),0|3) {
            return result;
        }
    }
}
pub fn run<S: Services>(op: u32, arg: i64, native: &mut S) -> i64 {
    let mut c = Controller::new(native, Module::TransportSubJob);
    match op {
        0 => {
            s!(c,CheckNone);
            s!(c,SetState,1);
            run_loop(&mut c, OK as i64)
        }
        1 => match v!(c,State) {
            1..=3 => 11,
            0 => 0,
            _ => {
                s!(c,NotReached);
                0
            }
        },
        2 => {
            let rv = run_loop(&mut c, arg);
            if c.alive && rv != PENDING {
                s!(c,ParentComplete,rv);
            }
            0
        }
        3 => run_loop(&mut c, arg),
        4 => connect(&mut c),
        5 => complete(&mut c, arg),
        6 => i64::from(arg != 0),
        _ => ERR_UNEXPECTED as i64,
    }
}
