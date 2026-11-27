#![forbid(unsafe_code)]
use crate::connection_jobs_rust_core::{Controller, Module, Services};
use crate::errors::*;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Init = 0,
    LoadState = 1,
    OnIO = 2,
    Loop = 3,
    Resolve = 4,
    ResolveComplete = 5,
    Resolved = 6,
    Transport = 7,
    TransportComplete = 8,
    Handle = 9,
    OnSub = 10,
    StartV4 = 11,
    Connect = 12,
    Priority = 13,
    Svcb = 14,
    Usable = 15,
    Factory = 16,
    Timeout = 17,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Action {
    State = 0,
    SetState = 1,
    CheckLoop = 2,
    CheckOk = 3,
    CheckNotPending = 4,
    NotReached = 5,
    DnsStart = 6,
    HasOverride = 7,
    OverrideEnd = 8,
    SchemeDestination = 9,
    MakeSchemeRequest = 10,
    MakePairRequest = 11,
    StartRequest = 12,
    ResolveTrace = 13,
    DnsEnd = 14,
    StoreResolveError = 15,
    FailedDnsAttempt = 16,
    HasCallback = 17,
    HostCallback = 18,
    PostResolution = 19,
    SetResultsInput = 20,
    ResultsCount = 21,
    SelectResult = 22,
    ResultIpCount = 23,
    InsertSeenIp = 24,
    AddFilteredIp = 25,
    BeginFiltered = 26,
    AppendFiltered = 27,
    CopyAliases = 28,
    CopyResolution = 29,
    ResetRequest = 30,
    EndpointCount = 31,
    SelectCurrentEndpoint = 32,
    IpFamily = 33,
    AppendV4 = 34,
    AppendV6 = 35,
    UnexpectedFamily = 36,
    V4Count = 37,
    V6Count = 38,
    MakeV4 = 39,
    MakeV6 = 40,
    HasV4 = 41,
    HasV6 = 42,
    V4Started = 43,
    V6Started = 44,
    V4Load = 45,
    V6Load = 46,
    SelectV4 = 47,
    SelectV6 = 48,
    StartSelected = 49,
    SubType = 50,
    AdoptSocket = 51,
    ResetV4 = 52,
    ResetV6 = 53,
    StopFallback = 54,
    FallbackTime = 55,
    StartFallback = 56,
    Latency = 57,
    NextEndpoint = 58,
    Notify = 59,
    FallbackEvent = 60,
    CheckV4 = 61,
    CheckNoV6 = 62,
    Priority = 63,
    Https = 64,
    EchEnabled = 65,
    ResultAlternative = 66,
    ResultEch = 67,
    AlpnCount = 68,
    AlpnSupported = 69,
    HasInitOverride = 70,
    MoveInitOverride = 71,
    CheckOverrideIps = 72,
    CheckUsable = 73,
    CheckLegacy = 74,
    HasSsl = 75,
}

type C<'a, S> = Controller<'a, S>;
const PENDING: i64 = ERR_IO_PENDING as i64;
const OKAY: i64 = OK as i64;
const NONE: i64 = 5;
macro_rules! v {
    ($c:expr,$a:ident) => { $c.value(Action::$a as u32,0) };
    ($c:expr,$a:ident,$x:expr) => { $c.value(Action::$a as u32,$x) };
}
macro_rules! b {
    ($c:expr,$a:ident) => { v!($c,$a) != 0 };
}
macro_rules! s {
    ($c:expr,$a:ident) => { $c.step(Action::$a as u32,0) };
    ($c:expr,$a:ident,$x:expr) => { $c.step(Action::$a as u32,$x) };
}
fn usable<S: Services>(c: &mut C<'_, S>, optional: bool) -> bool {
    if !b!(c, ResultAlternative) {
        return optional && c.alive;
    }
    let count = v!(c, AlpnCount);
    for i in 0..count {
        if v!(c, AlpnSupported, i) != 0 {
            return c.alive;
        }
        if !c.alive {
            return false;
        }
    }
    false
}
fn optional<S: Services>(c: &mut C<'_, S>) -> bool {
    if !b!(c, Https) || !b!(c, HasSsl) || !b!(c, EchEnabled) {
        return true;
    }
    let mut any_alternative = false;
    for i in 0..v!(c, ResultsCount) {
        s!(c, SelectResult, i);
        if !c.alive {
            return true;
        }
        if b!(c, ResultAlternative) {
            any_alternative = true;
            if !b!(c, ResultEch) {
                return true;
            }
        }
    }
    !any_alternative
}
fn resolve<S: Services>(c: &mut C<'_, S>) -> i64 {
    s!(c, DnsStart);
    if b!(c, HasOverride) {
        s!(c, OverrideEnd);
        s!(c, SetState, 3);
        return OKAY;
    }
    s!(c, SetState, 1);
    if b!(c, SchemeDestination) {
        s!(c, MakeSchemeRequest);
    } else {
        s!(c, MakePairRequest);
    }
    v!(c, StartRequest)
}
fn resolve_complete<S: Services>(c: &mut C<'_, S>, result: i64) -> i64 {
    s!(c, DnsEnd);
    s!(c, StoreResolveError);
    if !c.alive {
        return PENDING;
    }
    if result != OKAY {
        s!(c, FailedDnsAttempt, result);
        return result;
    }
    s!(c, SetState, 2);
    if b!(c, HasCallback) && b!(c, HostCallback) {
        s!(c, PostResolution);
        return PENDING;
    }
    if c.alive {
        result
    } else {
        PENDING
    }
}
fn resolved<S: Services>(c: &mut C<'_, S>) -> i64 {
    s!(c, SetResultsInput);
    let optional = optional(c);
    for i in 0..v!(c, ResultsCount) {
        s!(c, SelectResult, i);
        if !usable(c, optional) {
            continue;
        }
        s!(c, BeginFiltered);
        let mut nonempty = false;
        for ip in 0..v!(c, ResultIpCount) {
            if v!(c, InsertSeenIp, ip) != 0 {
                s!(c, AddFilteredIp, ip);
                nonempty = true;
            }
        }
        if nonempty {
            s!(c, AppendFiltered);
        }
        if !c.alive {
            return PENDING;
        }
    }
    s!(c, CopyAliases);
    s!(c, CopyResolution);
    s!(c, ResetRequest);
    if !c.alive {
        return PENDING;
    }
    if v!(c, EndpointCount) == 0 {
        return ERR_NAME_NOT_RESOLVED as i64;
    }
    s!(c, SetState, 3);
    OKAY
}
fn handle<S: Services>(c: &mut C<'_, S>, mut result: i64) -> i64 {
    s!(c, CheckNotPending, result);
    if result == OKAY {
        s!(c, AdoptSocket);
        if c.alive {
            c.base(5, 0);
        }
        return result;
    }
    if result == ERR_NETWORK_IO_SUSPENDED as i64 {
        return result;
    }
    match v!(c, SubType) {
        0 => {
            s!(c, ResetV4);
        }
        1 => {
            s!(c, ResetV6);
            if b!(c, HasV4) && !b!(c, V4Started) {
                s!(c, StopFallback);
                s!(c, SelectV4);
                result = v!(c, StartSelected);
                if !c.alive {
                    return result;
                }
                if result != PENDING {
                    return handle(c, result);
                }
            }
        }
        _ => {
            s!(c, NotReached);
        }
    }
    if b!(c, HasV4) || b!(c, HasV6) {
        PENDING
    } else {
        result
    }
}
fn transport<S: Services>(c: &mut C<'_, S>) -> i64 {
    s!(c, SetState, 4);
    s!(c, SelectCurrentEndpoint);
    for ip in 0..v!(c, ResultIpCount) {
        match v!(c, IpFamily, ip) {
            1 => {
                s!(c, AppendV4, ip);
            }
            2 => {
                s!(c, AppendV6, ip);
            }
            family => {
                s!(c, UnexpectedFamily, family);
            }
        }
    }
    if v!(c, V4Count) != 0 {
        s!(c, MakeV4);
    }
    if v!(c, V6Count) != 0 {
        s!(c, MakeV6);
        s!(c, SelectV6);
        let result = v!(c, StartSelected);
        if !c.alive {
            return result;
        }
        if result != PENDING {
            return handle(c, result);
        }
        if b!(c, HasV4) {
            crate::tcp_connect_job_rust_core::fallback(c.native);
            s!(c, FallbackTime);
            s!(c, StartFallback);
        }
        return PENDING;
    }
    s!(c, CheckNoV6);
    s!(c, CheckV4);
    s!(c, SelectV4);
    let result = v!(c, StartSelected);
    if !c.alive || result == PENDING {
        return result;
    }
    handle(c, result)
}
fn transport_complete<S: Services>(c: &mut C<'_, S>, result: i64) -> i64 {
    s!(c, ResetV4);
    s!(c, ResetV6);
    s!(c, StopFallback);
    if !c.alive {
        return PENDING;
    }
    if result == OKAY {
        s!(c, Latency);
    } else if result != ERR_NETWORK_IO_SUSPENDED as i64 {
        let index = v!(c, NextEndpoint);
        if index < v!(c, EndpointCount) {
            s!(c, SetState, 3);
            return OKAY;
        }
    }
    result
}
fn run_loop<S: Services>(c: &mut C<'_, S>, mut result: i64) -> i64 {
    s!(c, CheckLoop);
    loop {
        let state = v!(c, State);
        s!(c, SetState, NONE);
        result = match state {
            0 => {
                s!(c, CheckOk, result);
                resolve(c)
            }
            1 => v!(c, ResolveTrace, result),
            2 => {
                s!(c, CheckOk, result);
                resolved(c)
            }
            3 => {
                s!(c, CheckOk, result);
                transport(c)
            }
            4 => transport_complete(c, result),
            _ => {
                s!(c, NotReached);
                ERR_UNEXPECTED as i64
            }
        };
        if !c.alive || result == PENDING || v!(c, State) == NONE {
            return result;
        }
    }
}
fn on_io<S: Services>(c: &mut C<'_, S>, result: i64) {
    let result = run_loop(c, result);
    if c.alive && result != PENDING {
        c.base(6, result);
    }
}
fn on_sub<S: Services>(c: &mut C<'_, S>, result: i64) {
    let result = handle(c, result);
    if c.alive && result != PENDING {
        on_io(c, result);
    }
}
pub fn run<S: Services>(op: u32, arg: i64, native: &mut S) -> i64 {
    let mut c = Controller::new(native, Module::Transport);
    match op {
        0 => {
            s!(c, CheckLegacy);
            if b!(c, HasInitOverride) {
                s!(c, MoveInitOverride);
                s!(c, CheckOverrideIps);
                let opt = optional(&mut c);
                s!(c, SelectResult, 0);
                let valid = usable(&mut c, opt);
                s!(c, CheckUsable, i64::from(valid));
            }
            0
        }
        1 => match v!(c, State) {
            0..=2 => 10,
            3..=4 => {
                let mut load = 0;
                if b!(c, HasV6) && b!(c, V6Started) {
                    load = v!(c, V6Load);
                }
                if b!(c, HasV4) && b!(c, V4Started) && load != 11 {
                    load = v!(c, V4Load);
                }
                load
            }
            _ => 0,
        },
        2 => {
            on_io(&mut c, arg);
            0
        }
        3 => run_loop(&mut c, arg),
        4 => resolve(&mut c),
        5 => resolve_complete(&mut c, arg),
        6 => resolved(&mut c),
        7 => transport(&mut c),
        8 => transport_complete(&mut c, arg),
        9 => handle(&mut c, arg),
        10 => {
            on_sub(&mut c, arg);
            0
        }
        11 => {
            s!(c, CheckV4);
            s!(c, FallbackEvent);
            s!(c, SelectV4);
            let rv = v!(c, StartSelected);
            if c.alive && rv != PENDING {
                on_sub(&mut c, rv);
            }
            0
        }
        12 => {
            s!(c, SetState, 0);
            run_loop(&mut c, OKAY)
        }
        13 => {
            if v!(c, State) == 1 {
                s!(c, Priority, arg);
            }
            0
        }
        14 => i64::from(optional(&mut c)),
        15 => i64::from(usable(&mut c, arg != 0)),
        16 => i64::from(arg != 0),
        17 => 240_000_000,
        _ => ERR_UNEXPECTED as i64,
    }
}
