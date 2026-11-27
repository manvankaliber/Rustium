#![forbid(unsafe_code)]
use crate::connection_jobs_rust_core::{Controller, Module, Services};
use crate::errors::*;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Init = 0,
    LoadState = 1,
    Connect = 2,
    Priority = 3,
    Updated = 4,
    Finished = 5,
    UpdateSync = 6,
    AdvanceAsync = 7,
    Advance = 8,
    Complete = 9,
    CompleteAsync = 10,
    Slow = 11,
    AdvanceState = 12,
    Promote = 13,
    NextIp = 14,
    Usable = 15,
    Find = 16,
    UpdateSvcb = 17,
    ResetState = 18,
    StateDone = 19,
    SetDone = 20,
    GetResults = 21,
    CryptoReady = 22,
    GetAliases = 23,
    Notify = 24,
    Timeout = 25,
    Count = 26,
    Stale = 27,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Action {
    CheckNotDone = 0,
    CheckDnsIncomplete = 1,
    CheckPendingDifferent = 2,
    CheckFinalEndpointEmpty = 3,
    CheckComplete = 4,
    CheckHasFinal = 5,
    CheckCrypto = 6,
    SelectState = 7,
    HasPrimary = 8,
    HasV4 = 9,
    PrimaryDone = 10,
    V4Done = 11,
    PrimaryWaitingDns = 12,
    V4WaitingDns = 13,
    PrimaryWaitingEndpoint = 14,
    V4WaitingEndpoint = 15,
    PrimaryHasAddress = 16,
    V4HasAddress = 17,
    PrimaryLoad = 18,
    V4Load = 19,
    PrimaryV6 = 20,
    SelectPrimary = 21,
    SelectV4 = 22,
    AdvanceConnector = 23,
    HasRequest = 24,
    HasOverride = 25,
    DisableStale = 26,
    DualRace = 27,
    OptimisticEnabled = 28,
    StaleConnectorsFeature = 29,
    RequestStale = 30,
    DnsComplete = 31,
    SetDnsComplete = 32,
    DnsStart = 33,
    DnsEnd = 34,
    MakeRequest = 35,
    CacheDefault = 36,
    CacheStale = 37,
    StartRequest = 38,
    Priority = 39,
    ResolveError = 40,
    ClearAttempts = 41,
    HasCallback = 42,
    HostCallback = 43,
    WaitingDeletion = 44,
    IncrementWaiting = 45,
    DecrementWaiting = 46,
    PostAdvance = 47,
    ResetIndex = 48,
    SetIndex = 49,
    GetIndex = 50,
    IncrementIndex = 51,
    MakeStalePrimary = 52,
    MakeSecondary = 53,
    MakeInitialPrimary = 54,
    SwapConnectors = 55,
    CheckNoSecondary = 56,
    SlowEvent = 57,
    SlowRunning = 58,
    StopSlow = 59,
    FallbackDuration = 60,
    StartTimePresent = 61,
    SubtractElapsed = 62,
    DelayNegative = 63,
    ZeroDelay = 64,
    StartSlow = 65,
    ResetPrimary = 66,
    ResetV4 = 67,
    SetPreferV6 = 68,
    PreferV6 = 69,
    InputIsPrimary = 70,
    InputIsStale = 71,
    OtherExists = 72,
    OtherWaitingEndpoint = 73,
    InputDone = 74,
    SetOtherV4 = 75,
    SetOtherPrimary = 76,
    DnsResults = 77,
    OverrideResults = 78,
    DnsAliases = 79,
    OverrideAliases = 80,
    DnsCrypto = 81,
    ResultsCount = 82,
    SelectEndpoint = 83,
    Alternative = 84,
    Ech = 85,
    AlpnCount = 86,
    AlpnMatches = 87,
    Svcb = 88,
    SetSvcb = 89,
    Https = 90,
    EchEnabled = 91,
    SetFamily = 92,
    IpCount = 93,
    SelectIp = 94,
    IpAlreadyAttempted = 95,
    InsertIp = 96,
    ReturnIp = 97,
    InputIpV6 = 98,
    IpMatchesInput = 99,
    ReturnEndpoint = 100,
    PrimaryAddressToInput = 101,
    V4AddressToInput = 102,
    ConnectorHasAddress = 103,
    ConnectorAddressToInput = 104,
    IpEqualsSaved = 105,
    SaveIp = 106,
    PromotePrimary = 107,
    PromoteV4 = 108,
    StoreResolution = 109,
    CheckConnector = 110,
    PassSocket = 111,
    PassFinalEndpoint = 112,
    AttemptsEmpty = 113,
    AddEmptyAttempt = 114,
    LastAttempt = 115,
    CheckStaleUnset = 116,
    StoreStale = 117,
    ConnectorIsStale = 118,
    ResetRequest = 119,
    InvalidateTasks = 120,
    SetDone = 121,
    Notify = 122,
    CheckV2 = 123,
    CheckInitialIp = 124,
    CheckUsable = 125,
    StaleValue = 126,
    CheckStateActive = 127,
    HasSsl = 128,
}

type C<'a, S> = Controller<'a, S>;
const PENDING: i64 = ERR_IO_PENDING as i64;
const OKAY: i64 = OK as i64;
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
fn dual<S: Services>(c: &mut C<'_, S>) -> bool {
    b!(c, OptimisticEnabled) && b!(c, StaleConnectorsFeature)
}
fn state<S: Services>(c: &mut C<'_, S>, id: i64) -> i64 {
    v!(c, SelectState, id)
}
fn results<S: Services>(c: &mut C<'_, S>) {
    if b!(c, HasRequest) {
        s!(c, DnsResults);
    } else {
        s!(c, OverrideResults);
    }
}
fn aliases<S: Services>(c: &mut C<'_, S>) {
    if b!(c, HasRequest) {
        s!(c, DnsAliases);
    } else {
        s!(c, OverrideAliases);
    }
}
fn crypto<S: Services>(c: &mut C<'_, S>) -> bool {
    if b!(c, HasRequest) {
        b!(c, DnsCrypto)
    } else {
        true
    }
}
fn state_done<S: Services>(c: &mut C<'_, S>, id: i64) -> bool {
    let saved = state(c, id);
    let done = (!b!(c, HasPrimary) || b!(c, PrimaryDone)) && (!b!(c, HasV4) || b!(c, V4Done));
    state(c, saved);
    done
}
fn usable<S: Services>(c: &mut C<'_, S>) -> bool {
    if !b!(c, Alternative) {
        return b!(c, Svcb);
    }
    for i in 0..v!(c, AlpnCount) {
        if v!(c, AlpnMatches, i) != 0 {
            return c.alive;
        }
        if !c.alive {
            return false;
        }
    }
    false
}
fn update_svcb<S: Services>(c: &mut C<'_, S>) {
    let mut optional = true;
    if b!(c, Https) && b!(c, HasSsl) && b!(c, EchEnabled) {
        results(c);
        let mut alternative = false;
        let mut all_ech = true;
        for i in 0..v!(c, ResultsCount) {
            s!(c, SelectEndpoint, i);
            if b!(c, Alternative) {
                alternative = true;
                if !b!(c, Ech) {
                    all_ech = false;
                    break;
                }
            }
        }
        optional = !(alternative && all_ech);
    }
    s!(c, SetSvcb, i64::from(optional));
}
fn reset<S: Services>(c: &mut C<'_, S>, id: i64) {
    let saved = state(c, id);
    s!(c, StopSlow);
    s!(c, ResetPrimary);
    s!(c, ResetV4);
    s!(c, ResetIndex);
    s!(c, SetPreferV6);
    state(c, saved);
}
// The saved native IP is a value in the invocation context, never a Rust view.
fn saved_ip_is_fresh<S: Services>(c: &mut C<'_, S>) -> bool {
    for ep in 0..v!(c, ResultsCount) {
        s!(c, SelectEndpoint, ep);
        for family in [false, true] {
            s!(c, SetFamily, i64::from(family));
            for ip in 0..v!(c, IpCount) {
                if v!(c, IpEqualsSaved, ip) != 0 {
                    return c.alive;
                }
            }
        }
    }
    false
}
fn promote<S: Services>(c: &mut C<'_, S>) {
    if !dual(c) || !b!(c, HasRequest) || b!(c, RequestStale) {
        return;
    }
    let saved = state(c, 1);
    s!(c, StopSlow);
    results(c);
    for secondary in [false, true] {
        state(c, 1);
        let stale_live = if secondary {
            b!(c, HasV4) && !b!(c, V4Done)
        } else {
            b!(c, HasPrimary) && !b!(c, PrimaryDone)
        };
        if !stale_live {
            continue;
        }
        state(c, 0);
        let replaceable = if secondary {
            !b!(c, HasV4)
                || b!(c, V4WaitingEndpoint)
                || b!(c, V4WaitingDns)
                || b!(c, V4Done)
                || !b!(c, V4HasAddress)
        } else {
            !b!(c, HasPrimary)
                || b!(c, PrimaryWaitingEndpoint)
                || b!(c, PrimaryWaitingDns)
                || b!(c, PrimaryDone)
                || !b!(c, PrimaryHasAddress)
        };
        if !replaceable {
            continue;
        }
        state(c, 1);
        let address = if secondary { b!(c, V4HasAddress) } else { b!(c, PrimaryHasAddress) };
        if address {
            if secondary {
                s!(c, V4AddressToInput);
            } else {
                s!(c, PrimaryAddressToInput);
            }
            s!(c, SaveIp);
            if saved_ip_is_fresh(c) {
                if secondary {
                    s!(c, PromoteV4);
                } else {
                    s!(c, PromotePrimary);
                }
            }
        }
    }
    state(c, saved);
}
fn finish<S: Services>(c: &mut C<'_, S>, mut result: i64) -> i64 {
    s!(c, CheckNotDone);
    s!(c, CheckFinalEndpointEmpty);
    if b!(c, HasRequest) {
        s!(c, StoreResolution);
    }
    if !c.alive {
        return PENDING;
    }
    if result == OKAY {
        let ready = crypto(c);
        s!(c, CheckCrypto, i64::from(ready));
        s!(c, CheckConnector);
        aliases(c);
        s!(c, PassSocket);
        if c.alive {
            c.base(5, 0);
        }
        s!(c, PassFinalEndpoint);
        s!(c, CheckHasFinal);
        let valid = usable(c);
        s!(c, CheckUsable, i64::from(valid));
        if !c.alive {
            return PENDING;
        }
        let stale = if b!(c, DisableStale) {
            false
        } else if b!(c, StaleConnectorsFeature) {
            b!(c, ConnectorIsStale)
        } else if b!(c, HasRequest) {
            if b!(c, RequestStale) {
                true
            } else if b!(c, ConnectorHasAddress) {
                s!(c, ConnectorAddressToInput);
                s!(c, SaveIp);
                results(c);
                !saved_ip_is_fresh(c)
            } else {
                false
            }
        } else {
            false
        };
        s!(c, CheckStaleUnset);
        s!(c, StoreStale, i64::from(stale));
    } else {
        if b!(c, AttemptsEmpty) {
            s!(c, AddEmptyAttempt, result);
        }
        result = v!(c, LastAttempt);
    }
    reset(c, 0);
    reset(c, 1);
    s!(c, ResetRequest);
    s!(c, InvalidateTasks);
    s!(c, SetDone);
    result
}
fn complete<S: Services>(c: &mut C<'_, S>, result: i64) -> i64 {
    s!(c, CheckPendingDifferent, result);
    if result == OKAY || result == ERR_NETWORK_IO_SUSPENDED as i64 {
        return finish(c, result);
    }
    let fresh_done = state_done(c, 0) && {
        let saved = state(c, 0);
        let ready = b!(c, HasPrimary) || b!(c, DnsComplete);
        state(c, saved);
        ready
    };
    if fresh_done && state_done(c, 1) {
        finish(c, result)
    } else {
        PENDING
    }
}
fn advance_state<S: Services>(c: &mut C<'_, S>, id: i64) -> i64 {
    let done = state_done(c, id);
    s!(c, CheckStateActive, i64::from(!done));
    let saved = state(c, id);
    if b!(c, HasPrimary) && !b!(c, PrimaryDone) {
        s!(c, SelectPrimary);
        let rv = v!(c, AdvanceConnector);
        if !c.alive {
            return rv;
        }
        if rv != PENDING {
            let rv = complete(c, rv);
            if rv != PENDING {
                state(c, saved);
                return rv;
            }
        }
    }
    // Re-read after the primary: its completion can cancel both connectors.
    state(c, id);
    if b!(c, HasV4) && !b!(c, V4Done) {
        s!(c, SelectV4);
        let rv = v!(c, AdvanceConnector);
        if !c.alive {
            return rv;
        }
        if rv != PENDING {
            let rv = complete(c, rv);
            if rv != PENDING {
                state(c, saved);
                return rv;
            }
        }
    }
    s!(c, CheckNotDone);
    if state_done(c, id) {
        state(c, saved);
        return PENDING;
    }
    state(c, id);
    if (id == 0 || !b!(c, HasRequest) || b!(c, RequestStale))
        && !b!(c, HasV4)
        && !b!(c, SlowRunning)
        && b!(c, HasPrimary)
        && b!(c, PrimaryHasAddress)
    {
        fallback(c.native);
        s!(c, FallbackDuration);
        if b!(c, StartTimePresent) {
            s!(c, SubtractElapsed);
            if b!(c, DelayNegative) {
                s!(c, ZeroDelay);
            }
        }
        s!(c, StartSlow);
    }
    state(c, saved);
    PENDING
}
fn advance<S: Services>(c: &mut C<'_, S>) -> i64 {
    if v!(c, WaitingDeletion) != 0 {
        return PENDING;
    }
    s!(c, CheckNotDone);
    if dual(c) {
        if b!(c, HasRequest) && b!(c, RequestStale) {
            let saved = state(c, 1);
            if !b!(c, HasPrimary) {
                s!(c, MakeStalePrimary);
            }
            state(c, saved);
        }
        if !state_done(c, 1) {
            let rv = advance_state(c, 1);
            if rv != PENDING {
                return rv;
            }
        }
    }
    if !state_done(c, 0) {
        advance_state(c, 0)
    } else {
        PENDING
    }
}
fn updated<S: Services>(c: &mut C<'_, S>, final_result: Option<i64>) -> i64 {
    s!(c, CheckNotDone);
    s!(c, CheckDnsIncomplete);
    promote(c);
    state(c, 0);
    s!(c, ResetIndex);
    state(c, 1);
    s!(c, ResetIndex);
    if dual(c) && (!b!(c, HasRequest) || !b!(c, RequestStale)) {
        results(c);
        let count = v!(c, ResultsCount);
        s!(c, SetIndex, count);
    }
    let failed = if let Some(rv) = final_result {
        s!(c, CheckPendingDifferent, rv);
        s!(c, SetDnsComplete);
        rv != OKAY
    } else {
        false
    };
    state(c, 0);
    if failed || (b!(c, PrimaryWaitingDns) && (!b!(c, HasV4) || b!(c, V4WaitingDns))) {
        s!(c, DnsEnd);
    }
    if failed {
        s!(c, ResolveError);
        s!(c, ClearAttempts);
        return finish(c, final_result.unwrap_or(OKAY));
    }
    if !b!(c, HasOverride) {
        update_svcb(c);
    }
    if b!(c, HasCallback) {
        results(c);
        aliases(c);
        if b!(c, HostCallback) {
            s!(c, IncrementWaiting);
            s!(c, PostAdvance, 1);
            return PENDING;
        }
    }
    if !c.alive {
        return PENDING;
    }
    advance(c)
}
fn connect<S: Services>(c: &mut C<'_, S>) -> i64 {
    s!(c, DnsStart);
    let mut rv = OKAY;
    if !b!(c, HasOverride) {
        let cache = if b!(c, OptimisticEnabled) && !b!(c, DisableStale) {
            v!(c, CacheStale)
        } else {
            v!(c, CacheDefault)
        };
        s!(c, MakeRequest, cache);
        rv = v!(c, StartRequest);
        if !c.alive || rv == PENDING {
            return rv;
        }
    }
    updated(c, Some(rv))
}
fn notify<S: Services>(c: &mut C<'_, S>, result: i64) {
    if result != PENDING && c.alive {
        s!(c, CheckComplete);
        c.base(6, result);
    }
}
fn next_ip<S: Services>(c: &mut C<'_, S>) -> i64 {
    let stale = b!(c, InputIsStale);
    if stale && (!b!(c, HasRequest) || !b!(c, RequestStale)) {
        return ERR_NAME_NOT_RESOLVED as i64;
    }
    if dual(c) && !stale && b!(c, HasRequest) && b!(c, RequestStale) {
        return PENDING;
    }
    state(c, i64::from(stale));
    if v!(c, WaitingDeletion) != 0 {
        return PENDING;
    }
    results(c);
    s!(c, CheckNotDone);
    let input_done = b!(c, InputDone);
    s!(c, CheckCrypto, i64::from(!input_done));
    let primary = b!(c, InputIsPrimary);
    if primary {
        s!(c, SetOtherV4);
    } else {
        s!(c, SetOtherPrimary);
    }
    let prefer_v6 = if b!(c, HasV4) { primary } else { b!(c, PreferV6) };
    let other = b!(c, OtherExists);
    let only_preferred = other && !b!(c, DnsComplete);
    let mut posted = false;
    while c.alive {
        let index = v!(c, GetIndex);
        if index >= v!(c, ResultsCount) {
            break;
        }
        s!(c, SelectEndpoint, index);
        if usable(c) {
            for family in [prefer_v6, !prefer_v6] {
                if family != prefer_v6 && only_preferred {
                    break;
                }
                s!(c, SetFamily, i64::from(family));
                for ip in 0..v!(c, IpCount) {
                    s!(c, SelectIp, ip);
                    if b!(c, InsertIp) {
                        s!(c, ReturnIp);
                        return OKAY;
                    }
                }
            }
            if other {
                if !b!(c, OtherWaitingEndpoint) {
                    return PENDING;
                }
                if only_preferred {
                    s!(c, SetFamily, i64::from(!prefer_v6));
                    for ip in (0..v!(c, IpCount)).rev() {
                        s!(c, SelectIp, ip);
                        if !b!(c, IpAlreadyAttempted) {
                            return PENDING;
                        }
                    }
                }
            }
        }
        s!(c, IncrementIndex);
        if other && !posted {
            posted = true;
            s!(c, PostAdvance, 0);
        }
    }
    if b!(c, DnsComplete) {
        ERR_NAME_NOT_RESOLVED as i64
    } else {
        PENDING
    }
}
fn find<S: Services>(c: &mut C<'_, S>) {
    let v6 = b!(c, InputIpV6);
    results(c);
    for ep in 0..v!(c, ResultsCount) {
        s!(c, SelectEndpoint, ep);
        if !usable(c) {
            continue;
        }
        s!(c, SetFamily, i64::from(v6));
        for ip in 0..v!(c, IpCount) {
            s!(c, SelectIp, ip);
            if b!(c, IpMatchesInput) {
                s!(c, ReturnEndpoint);
                return;
            }
        }
    }
}
pub fn run<S: Services>(op: u32, arg: i64, native: &mut S) -> i64 {
    let mut c = Controller::new(native, Module::Tcp);
    match op {
        0 => {
            s!(c, MakeInitialPrimary);
            s!(c, CheckV2);
            if b!(c, HasOverride) {
                update_svcb(&mut c);
                results(&mut c);
                s!(c, SelectEndpoint, 0);
                s!(c, CheckInitialIp);
                let valid = usable(&mut c);
                s!(c, CheckUsable, i64::from(valid));
            }
            0
        }
        1 => {
            state(&mut c, 0);
            if !b!(c, HasPrimary) {
                return 0;
            }
            let mut load = v!(c, PrimaryLoad);
            if b!(c, HasV4) && load != 11 {
                load = v!(c, V4Load);
            }
            state(&mut c, 1);
            if b!(c, HasPrimary) && load == 10 {
                let old = v!(c, PrimaryLoad);
                if old == 11 {
                    load = old;
                }
            }
            load
        }
        2 => connect(&mut c),
        3 => {
            if b!(c, HasRequest) {
                s!(c, Priority, arg);
            }
            0
        }
        4 => {
            let rv = updated(&mut c, None);
            notify(&mut c, rv);
            0
        }
        5 => {
            let rv = updated(&mut c, Some(arg));
            notify(&mut c, rv);
            0
        }
        6 => updated(&mut c, Some(arg)),
        7 => {
            if arg != 0 {
                s!(c, DecrementWaiting);
            }
            let rv = advance(&mut c);
            notify(&mut c, rv);
            0
        }
        8 => advance(&mut c),
        9 => complete(&mut c, arg),
        10 => {
            let rv = complete(&mut c, arg);
            if rv != PENDING && c.alive {
                c.base(6, rv);
            }
            0
        }
        11 => {
            s!(c, CheckNotDone);
            state(&mut c, arg);
            s!(c, CheckNoSecondary);
            s!(c, SlowEvent);
            s!(c, MakeSecondary);
            if !b!(c, PrimaryV6) {
                s!(c, SwapConnectors);
            }
            let rv = advance(&mut c);
            notify(&mut c, rv);
            0
        }
        12 => advance_state(&mut c, arg),
        13 => {
            promote(&mut c);
            0
        }
        14 => next_ip(&mut c),
        15 => i64::from(usable(&mut c)),
        16 => {
            find(&mut c);
            0
        }
        17 => {
            update_svcb(&mut c);
            0
        }
        18 => {
            reset(&mut c, arg);
            0
        }
        19 => i64::from(state_done(&mut c, arg)),
        20 => finish(&mut c, arg),
        21 => {
            results(&mut c);
            0
        }
        22 => i64::from(crypto(&mut c)),
        23 => {
            aliases(&mut c);
            0
        }
        24 => {
            notify(&mut c, arg);
            0
        }
        25 => 240_000_000,
        26 => {
            s!(c, CheckNotDone);
            state(&mut c, arg);
            i64::from(b!(c, HasPrimary)) + i64::from(b!(c, HasV4))
        }
        27 => v!(c, StaleValue),
        28 => updated(&mut c, None),
        _ => ERR_UNEXPECTED as i64,
    }
}

#[repr(u32)]
pub enum FallbackAction {
    Default = 1000,
    AdjustEnabled,
    ReadAdjusted,
    RttEnabled,
    HasProperties,
    DestinationIsScheme,
    SchemeDestination,
    PairDestination,
    LookupStats,
    StatsNonzero,
    RttFromStats,
    HasNqe,
    NqeRtt,
    HasRtt,
    Multiply,
    BelowMinimum,
    Minimum,
    AboveMaximum,
    Maximum,
}
pub fn fallback<S: Services>(native: &mut S) -> i64 {
    let mut c = Controller::new(native, Module::Tcp);
    c.step(FallbackAction::Default as u32, 0);
    if c.flag(FallbackAction::AdjustEnabled as u32, 0) {
        c.step(FallbackAction::ReadAdjusted as u32, 0);
    }
    if c.flag(FallbackAction::RttEnabled as u32, 0) {
        let mut found = false;
        if c.flag(FallbackAction::HasProperties as u32, 0) {
            let action = if c.flag(FallbackAction::DestinationIsScheme as u32, 0) {
                FallbackAction::SchemeDestination
            } else {
                FallbackAction::PairDestination
            };
            c.step(action as u32, 0);
            if c.flag(FallbackAction::LookupStats as u32, 0)
                && c.flag(FallbackAction::StatsNonzero as u32, 0)
            {
                c.step(FallbackAction::RttFromStats as u32, 0);
                found = true;
            }
        }
        if !found && c.flag(FallbackAction::HasNqe as u32, 0) {
            c.step(FallbackAction::NqeRtt as u32, 0);
        }
        if c.flag(FallbackAction::HasRtt as u32, 0) {
            c.step(FallbackAction::Multiply as u32, 0);
            if c.flag(FallbackAction::BelowMinimum as u32, 0) {
                c.step(FallbackAction::Minimum as u32, 0);
            } else if c.flag(FallbackAction::AboveMaximum as u32, 0) {
                c.step(FallbackAction::Maximum as u32, 0);
            }
        }
    }
    0
}
