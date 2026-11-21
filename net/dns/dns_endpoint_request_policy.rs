//! Service endpoint requests, including stale refresh and destruction callbacks.
use crate::Words;
use crate::dns_request_family_services::{Services, w, wa};
use crate::dns_request_family_ids::{EndpointEvent as E, EndpointOp as O};
const OK: i64 = 0;
const PENDING: i64 = -1;
const MISS: i64 = -804;
const NAME: i64 = -105;
const CONTEXT: i64 = -26;
fn read<S: Services>(s: &mut S, field: i64) -> i64 {
    s.call(O::Read, w(field)).x
}
fn set<S: Services>(s: &mut S, field: i64, value: i64) {
    s.call(O::Write, Words { y: value, ..w(field) });
}
fn check<S: Services>(s: &mut S, field: i64) {
    s.call(O::Check, w(field));
}
fn clear_stale<S: Services>(s: &mut S) {
    if read(s, 3) == 2 && read(s, 4) != 0 {
        s.call(O::ResetStale, w(0));
    }
}
fn finalize<S: Services>(s: &mut S, rv: i64) {
    check(s, 7);
    if rv == OK && read(s, 11) == 0 {
        s.call(O::FinalizeResult, w(0));
    } else {
        s.call(O::MakeEmpty, w(0));
    }
}
fn ipv6<S: Services>(s: &mut S) -> i64 {
    set(s, 1, 2);
    if read(s, 2) != 0 {
        if s.call(O::StartIPv6, w(1)).x == PENDING {
            set(s, 1, 0);
            s.call(O::MakeEmpty, w(0));
            s.call(O::SetError, w(NAME));
            return NAME;
        }
        OK
    } else {
        s.call(O::StartIPv6, w(0)).x
    }
}
fn local<S: Services>(s: &mut S) -> i64 {
    s.call(O::InitializeKey, w(0));
    let swr = read(s, 3) == 2;
    let usage = if swr {
        if read(s, 2) != 0 {
            0
        } else {
            1
        }
    } else {
        read(s, 3)
    };
    s.call(O::ResolveLocal, w(usage));
    let mut stale = read(s, 4) != 0 && read(s, 5) != 0;
    if stale && swr {
        check(s, 8);
        s.call(O::ClearTasks, w(0));
        s.call(O::ResolveFresh, w(0));
        check(s, 9);
        let fresh_error = s.call(O::FreshError, w(0)).x;
        if fresh_error != MISS || read(s, 10) != 0 {
            s.call(O::AdoptFresh, w(0));
            stale = false;
        }
        check(s, 10);
    }
    let rv = s.call(O::LocalError, w(0)).x;
    if stale && swr {
        if rv == OK && s.call(O::ResultNetwork, w(0)).x == s.call(O::CurrentNetwork, w(0)).x {
            s.call(O::ConvertStale, w(0));
        } else {
            s.call(O::ResetStale, w(1));
        }
        if read(s, 7) != 0 {
            s.call(O::LogStale, w(0));
            s.call(O::PostStale, w(0));
        }
        check(s, 11);
    } else if rv != MISS || read(s, 2) != 0 || read(s, 10) != 0 {
        if rv == OK {
            let source = if read(s, 4) != 0 { 1 } else { 2 };
            s.call(O::SetDetails, w(source));
        }
        finalize(s, rv);
        s.call(O::SetError, w(rv));
        s.call(O::ReleaseLocal, w(0));
        return rv;
    }
    set(s, 1, 4);
    s.call(O::ReleaseLocal, w(0));
    OK
}
fn start_job<S: Services>(s: &mut S) -> i64 {
    s.call(O::StartJob, w(0));
    PENDING
}
pub fn loop_request<S: Services>(s: &mut S, mut rv: i64) -> i64 {
    loop {
        let state = read(s, 9);
        set(s, 1, 0);
        rv = match state {
            1 => ipv6(s),
            2 => {
                set(s, 1, if rv == OK { 3 } else { 0 });
                rv
            }
            3 => local(s),
            4 => start_job(s),
            _ => {
                check(s, 12);
                rv
            }
        };
        if read(s, 9) == 0 || rv == PENDING {
            return rv;
        }
    }
}
fn notify<S: Services>(s: &mut S) {
    if read(s, 6) != 0 {
        return;
    }
    check(s, 6);
    s.call(O::RunUpdated, w(0));
}
pub fn run<S: Services>(s: &mut S, event: u32, input: Words) -> Words {
    let rv = match event {
        E::Destroy => {
            if read(s, 0) == 0 {
                return w(0);
            }
            s.call(O::LogCancel, w(0));
            set(s, 4, 0);
            s.call(O::CancelJob, w(0));
            check(s, 1);
            0
        }
        E::Start => {
            check(s, 0);
            if read(s, 1) == 0 {
                s.call(O::SetError, w(CONTEXT));
                return s.call(O::Squash, w(CONTEXT));
            }
            set(s, 3, 0);
            set(s, 1, 1);
            let result = loop_request(s, OK);
            s.call(O::Squash, w(result)).x
        }
        E::Loop => loop_request(s, input.x),
        E::IPv6 => ipv6(s),
        E::IPv6Complete => {
            set(s, 1, if input.x == OK { 3 } else { 0 });
            input.x
        }
        E::ResolveLocal => local(s),
        E::StartJob => start_job(s),
        E::IOComplete => {
            let result = if read(s, 1) == 0 {
                set(s, 1, 0);
                s.call(O::MakeEmpty, w(0));
                s.call(O::SetError, w(CONTEXT));
                CONTEXT
            } else {
                loop_request(s, input.x)
            };
            if result != PENDING {
                check(s, 2);
                let squashed = s.call(O::Squash, w(result)).x;
                s.call(O::RunFinished, w(squashed));
            }
            return w(0);
        }
        E::ChangePriority => {
            if read(s, 0) == 0 {
                set(s, 2, input.x);
            } else {
                s.call(O::ChangeJobPriority, input);
            }
            0
        }
        E::Completed => {
            check(s, 3);
            s.call(O::ResetJob, w(0));
            if input.x == OK {
                s.call(O::SetDetails, w(0));
            }
            finalize(s, input.x);
            clear_stale(s);
            s.call(
                O::SetError,
                Words { y: i64::from(input.y != 0 && input.x != OK), ..w(input.x) },
            );
            let squashed = s.call(O::Squash, w(input.x)).x;
            s.call(O::RunFinished, w(squashed));
            return w(0);
        }
        E::Cancelled => {
            check(s, 4);
            s.call(O::ResetJob, w(0));
            if read(s, 8) == 0 {
                return w(0);
            }
            s.call(O::LogCancel, w(0));
            s.call(O::MakeEmpty, w(0));
            s.call(O::SetError, w(-810));
            let squashed = s.call(O::Squash, w(-810)).x;
            s.call(O::RunFinished, w(squashed));
            return w(0);
        }
        E::Changed => {
            if read(s, 6) != 0 {
                return w(0);
            }
            clear_stale(s);
            check(s, 6);
            s.call(O::RunUpdated, w(0));
            return w(0);
        }
        E::Finalize => {
            finalize(s, input.x);
            0
        }
        E::ClearStale => {
            clear_stale(s);
            0
        }
        E::Notify => {
            notify(s);
            return w(0);
        }
        E::EndpointSource => {
            if read(s, 6) != 0 {
                1
            } else if read(s, 7) != 0 {
                2
            } else if read(s, 0) != 0 && s.call(O::HasTaskManager, w(0)).x != 0 {
                3
            } else {
                0
            }
        }
        E::AliasSource => {
            if read(s, 6) != 0 {
                1
            } else if read(s, 0) != 0 && s.call(O::HasTaskManager, w(0)).x != 0 {
                3
            } else {
                0
            }
        }
        E::CryptoReady => {
            if read(s, 6) != 0 {
                read(s, 12)
            } else if read(s, 0) != 0 && s.call(O::HasTaskManager, w(0)).x != 0 {
                s.call(O::GetCryptoReady, w(0)).x
            } else {
                0
            }
        }
        E::IsStaleRefreshing => i64::from(read(s, 3) == 2 && read(s, 4) != 0 && read(s, 5) != 0),
        E::Debug => {
            s.call(O::DebugAppend, wa(0));
            let count = s.call(O::DebugTaskCount, w(0)).a;
            for i in 0..count {
                let task = s.call(O::DebugTask, wa(i)).x;
                s.call(O::DebugAppend, Words { x: 1, y: task, ..wa(0) });
                s.call(O::DebugAppend, wa(1));
            }
            s.call(O::DebugAppend, wa(2));
            let job = read(s, 0);
            s.call(O::DebugAppend, Words { x: 1, y: job, ..wa(0) });
            if job != 0 {
                s.call(O::DebugAppend, wa(3));
                let manager = s.call(O::HasTaskManager, w(0)).x;
                s.call(O::DebugAppend, Words { x: 1, y: manager, ..wa(0) });
            }
            0
        }
        _ => -4,
    };
    w(rv)
}
