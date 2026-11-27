#![forbid(unsafe_code)]
use crate::connection_jobs_rust_core::{Controller, Module, Services};
use crate::errors::*;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Advance = 0,
    Load = 1,
    Io = 2,
    Loop = 3,
    Obtain = 4,
    Connect = 5,
    Complete = 6,
    Verify = 7,
    Failed = 8,
    Done = 9,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Action {
    State = 0,
    SetState = 1,
    CheckNotDone = 2,
    CheckLoop = 3,
    CheckResult = 4,
    CheckOk = 5,
    CheckNoAddress = 6,
    CheckAddress = 7,
    CheckNoMore = 8,
    NotReached = 9,
    GetIp = 10,
    IpError = 11,
    TakeIp = 12,
    FindEndpoint = 13,
    HasEndpoint = 14,
    IsStale = 15,
    SaveMetadata = 16,
    ClearMetadata = 17,
    HasMetadata = 18,
    UseMetadata = 19,
    StartNull = 20,
    StampStart = 21,
    HasWatcher = 22,
    MakeWatcher = 23,
    MakeSocket = 24,
    ApplyTag = 25,
    HasWebsocket = 26,
    WrapWebsocket = 27,
    StartEvent = 28,
    Connect = 29,
    CompleteEvent = 30,
    CompleteErrorEvent = 31,
    Established = 32,
    Crypto = 33,
    UsableEvent = 34,
    FinalMetadata = 35,
    AddressV6 = 36,
    FreshPrefer = 37,
    StalePrefer = 38,
    RecordAttempt = 39,
    ResetSocket = 40,
    ResetAddress = 41,
    DoneEvent = 42,
    DoneErrorEvent = 43,
    ParentComplete = 44,
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
fn done<S: Services>(c: &mut C<'_, S>, result: i64) {
    s!(c,CheckNotDone);
    s!(c,SetState,7);
    if result != 0 {
        s!(c,DoneErrorEvent,result);
    } else {
        s!(c,DoneEvent);
    }
}
fn failed<S: Services>(c: &mut C<'_, S>, error: i64) -> i64 {
    s!(c,CheckResult,error);
    s!(c,CheckAddress);
    let prefer = v!(c,AddressV6) == 0;
    if v!(c,IsStale) != 0 {
        s!(c,StalePrefer,i64::from(prefer));
    } else {
        s!(c,FreshPrefer,i64::from(prefer));
    }
    if error != ERR_NAME_NOT_RESOLVED as i64 {
        s!(c,RecordAttempt,error);
    }
    s!(c,ResetSocket);
    s!(c,ResetAddress);
    if error == ERR_NETWORK_IO_SUSPENDED as i64 {
        done(c, error);
        return error;
    }
    s!(c,SetState,2);
    OK as i64
}
fn obtain<S: Services>(c: &mut C<'_, S>) -> i64 {
    s!(c,CheckNoAddress);
    let found = v!(c,GetIp) != 0;
    if !c.alive {
        return PENDING;
    }
    if !found {
        let error = v!(c,IpError);
        if error == PENDING {
            s!(c,SetState,1);
            return PENDING;
        }
        s!(c,CheckNoMore,error);
        done(c, error);
        return error;
    }
    s!(c,TakeIp);
    s!(c,FindEndpoint);
    if v!(c,IsStale) != 0 && v!(c,HasEndpoint) != 0 {
        s!(c,SaveMetadata);
    } else {
        s!(c,ClearMetadata);
    }
    s!(c,SetState,3);
    OK as i64
}
fn connect<S: Services>(c: &mut C<'_, S>) -> i64 {
    s!(c,CheckAddress);
    if v!(c,StartNull) != 0 {
        s!(c,StampStart);
    }
    s!(c,SetState,4);
    if v!(c,HasWatcher) != 0 {
        s!(c,MakeWatcher);
    }
    s!(c,MakeSocket);
    s!(c,ApplyTag);
    if v!(c,HasWebsocket) != 0 {
        s!(c,WrapWebsocket);
    }
    s!(c,StartEvent);
    v!(c,Connect)
}
fn complete<S: Services>(c: &mut C<'_, S>, result: i64) -> i64 {
    if result != 0 {
        s!(c,CompleteErrorEvent,result);
    } else {
        s!(c,CompleteEvent);
    }
    if !c.alive {
        return PENDING;
    }
    if result != OK as i64 {
        return failed(c, result);
    }
    s!(c,Established);
    s!(c,SetState,6);
    OK as i64
}
fn verify<S: Services>(c: &mut C<'_, S>) -> i64 {
    if v!(c,Crypto) == 0 {
        s!(c,SetState,5);
        return PENDING;
    }
    s!(c,FindEndpoint);
    if v!(c,HasEndpoint) == 0 && v!(c,IsStale) != 0 && v!(c,HasMetadata) != 0 {
        s!(c,UseMetadata);
    }
    let usable = v!(c,HasEndpoint) != 0;
    s!(c,UsableEvent,i64::from(usable));
    if !c.alive {
        return PENDING;
    }
    if !usable {
        return failed(c, ERR_NAME_NOT_RESOLVED as i64);
    }
    s!(c,FinalMetadata);
    done(c, OK as i64);
    OK as i64
}
fn run_loop<S: Services>(c: &mut C<'_, S>, mut result: i64) -> i64 {
    loop {
        s!(c,CheckResult,result);
        s!(c,CheckLoop);
        s!(c,CheckNotDone);
        let state = v!(c,State);
        s!(c,SetState,0);
        result = match state {
            2 => {
                s!(c,CheckOk,result);
                obtain(c)
            }
            3 => {
                s!(c,CheckOk,result);
                connect(c)
            }
            4 => complete(c, result),
            6 => {
                s!(c,CheckOk,result);
                verify(c)
            }
            _ => {
                s!(c,NotReached);
                ERR_UNEXPECTED as i64
            }
        };
        if !c.alive || result == PENDING || v!(c,State) == 7 {
            break;
        }
    }
    s!(c,CheckLoop);
    result
}
pub fn state_flag(kind: u32, state: i64) -> bool {
    match kind {
        0 => state == 1,
        1 => matches!(state,1|5),
        2 => state == 7,
        _ => false,
    }
}
pub fn run<S: Services>(op: u32, arg: i64, native: &mut S) -> i64 {
    let mut c = Controller::new(native, Module::TcpConnector);
    match op {
        0 => {
            s!(c,CheckNotDone);
            match v!(c,State) {
                1 => {
                    s!(c,SetState,2);
                }
                5 => {
                    s!(c,SetState,6);
                }
                _ => return PENDING,
            };
            run_loop(&mut c, OK as i64)
        }
        1 => match v!(c,State) {
            1 | 2 | 5 | 6 => 10,
            3 | 4 | 7 => 11,
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
        4 => obtain(&mut c),
        5 => connect(&mut c),
        6 => complete(&mut c, arg),
        7 => verify(&mut c),
        8 => failed(&mut c, arg),
        9 => {
            done(&mut c, arg);
            0
        }
        _ => ERR_UNEXPECTED as i64,
    }
}
