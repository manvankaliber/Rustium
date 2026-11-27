//! ResolveHostRequest sequencing. Callback invocation is always a terminal action.
use crate::Words;
use crate::dns_request_family_services::{Services, w};
use crate::dns_request_family_ids::{RequestEvent as E, RequestOp as O};
const OK: i64 = 0;
const PENDING: i64 = -1;
const NAME: i64 = -105;
const CONTEXT: i64 = -26;
const CACHE_MISS: i64 = -804;
fn read<S: Services>(s: &mut S, field: i64) -> i64 {
    s.call(O::Read, w(field)).x
}
fn write<S: Services>(s: &mut S, field: i64, value: i64) {
    s.call(O::Write, Words { y: value, ..w(field) });
}
fn check<S: Services>(s: &mut S, code: i64) {
    s.call(O::Check, w(code));
}
fn error<S: Services>(s: &mut S, value: i64, secure: i64) {
    s.call(O::Write, Words { a: secure as u64, y: value, ..w(6) });
}
fn fix<S: Services>(s: &mut S) {
    check(s, 4);
    s.call(O::CopyEndpoints, w(0));
    s.call(O::CopyAliases, w(0));
    if read(s, 8) != 0 {
        check(s, 5);
    } else {
        s.call(O::FixAliases, w(0));
    }
    s.call(O::CopyLegacy, w(0));
}
fn ipv6<S: Services>(s: &mut S) -> i64 {
    write(s, 1, 1);
    if read(s, 2) != 0 {
        let rv = s.call(O::StartIPv6, w(1)).x;
        if rv == PENDING {
            write(s, 1, 5);
            return NAME;
        }
        return OK;
    }
    s.call(O::StartIPv6, w(0)).x
}
fn parameters<S: Services>(s: &mut S) -> i64 {
    s.call(O::InitializeKey, w(0));
    let facts = s.call(O::Nat64Facts, w(0));
    if crate::resolver_task::nat64_literal(facts.a != 0, facts.b != 0, facts.y != 0, facts.x)
        && s.call(O::LastIPv6, w(0)).x != 0
    {
        write(s, 1, 2);
        return s.call(O::StartGlobal, w(0)).x;
    }
    write(s, 1, 3);
    OK
}
fn local<S: Services>(s: &mut S) -> i64 {
    s.call(O::ResolveLocal, w(0));
    let rv = s.call(O::LocalError, w(0)).x;
    if rv != CACHE_MISS || read(s, 2) != 0 || read(s, 7) != 0 {
        if rv == OK && read(s, 3) == 0 {
            let source = if s.call(O::HasStale, w(0)).x != 0 { 1 } else { 2 };
            s.call(O::CopyLocalResults, w(source));
            fix(s);
        }
        if s.call(O::HasStale, w(0)).x != 0 && read(s, 3) == 0 {
            s.call(O::MoveStale, w(0));
        }
        write(s, 1, 5);
        s.call(O::ReleaseLocal, w(0));
        return rv;
    }
    write(s, 1, 4);
    s.call(O::ReleaseLocal, w(0));
    OK
}
fn start_job<S: Services>(s: &mut S) -> i64 {
    s.call(O::StartJob, w(0));
    check(s, 3);
    s.call(O::ResetResolver, w(0));
    PENDING
}
fn finish<S: Services>(s: &mut S, rv: i64) -> i64 {
    check(s, 6);
    write(s, 2, 1);
    error(s, rv, 0);
    let result = s.call(O::Squash, w(rv)).x;
    s.call(O::LogFinish, w(result));
    result
}
pub fn loop_request<S: Services>(s: &mut S, mut rv: i64) -> i64 {
    loop {
        let state = read(s, 4);
        write(s, 1, 6);
        rv = match state {
            0 => ipv6(s),
            1 => {
                s.call(O::Check, Words { y: rv, ..w(7) });
                parameters(s)
            }
            2 => {
                write(s, 1, 3);
                write(s, 3, i64::from(rv == -2));
                OK
            }
            3 => local(s),
            4 => start_job(s),
            5 => finish(s, rv),
            _ => {
                check(s, 8);
                rv
            }
        };
        if read(s, 4) == 6 || rv == PENDING {
            return rv;
        }
    }
}
pub fn run<S: Services>(s: &mut S, event: u32, input: Words) -> Words {
    let result = match event {
        E::Destroy => {
            if read(s, 0) != 0 {
                s.call(O::CancelJob, w(0));
                s.call(O::LogCancel, w(0));
            }
            0
        }
        E::Start => {
            check(s, 0);
            if read(s, 1) == 0 {
                write(s, 2, 1);
                s.call(O::ResetResolver, w(0));
                error(s, CONTEXT, 0);
                return w(NAME);
            }
            s.call(O::LogStart, w(0));
            write(s, 1, 0);
            write(s, 5, 0);
            loop_request(s, OK)
        }
        E::Loop => loop_request(s, input.x),
        E::IPv6 => ipv6(s),
        E::Parameters => parameters(s),
        E::ParametersComplete => {
            write(s, 1, 3);
            write(s, 3, i64::from(input.x == -2));
            OK
        }
        E::ResolveLocal => local(s),
        E::StartJob => start_job(s),
        E::Finish => finish(s, input.x),
        E::IOComplete => {
            let rv = loop_request(s, input.x);
            if rv != PENDING && read(s, 5) != 0 {
                s.call(O::RunCallback, w(rv));
            }
            return w(0);
        }
        E::ChangePriority => {
            if read(s, 0) == 0 {
                write(s, 4, input.x);
            } else {
                s.call(O::ChangeJobPriority, input);
            }
            0
        }
        E::Cancelled => {
            check(s, 10);
            s.call(O::ResetJob, w(0));
            check(s, 11);
            s.call(O::ResetCallback, w(0));
            check(s, 12);
            s.call(O::LogCancel, w(0));
            0
        }
        E::Completed => {
            error(s, input.x, input.y);
            check(s, 10);
            s.call(O::ResetJob, w(0));
            check(s, 3);
            write(s, 2, 1);
            s.call(O::LogFinish, Words { y: 1, ..w(input.x) });
            check(s, 13);
            let rv = s.call(O::Squash, w(input.x)).x;
            s.call(O::RunCallback, w(rv));
            return w(0);
        }
        E::FixResults => {
            fix(s);
            0
        }
        _ => -4,
    };
    w(result)
}
