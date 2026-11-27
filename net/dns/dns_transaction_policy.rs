//! Stub resolver suffix search, attempts, retry/fallback and callback sequencing.
use crate::Words;
use crate::dns_request_family_services::{Services, w, wa};
use crate::dns_request_family_ids::{TransactionEvent as E, TransactionOp as O};
const OK: i64 = 0;
const PENDING: i64 = -1;
const INVALID: i64 = -4;
const NAME: i64 = -105;
const TIMEOUT: i64 = -803;
fn read<S: Services>(s: &mut S, f: i64) -> i64 {
    s.call(O::Read, w(f)).x
}
fn set<S: Services>(s: &mut S, f: i64, v: i64) {
    s.call(O::Write, Words { y: v, ..w(f) });
}
fn check<S: Services>(s: &mut S, c: i64) {
    s.call(O::Check, w(c));
}
fn count<S: Services>(s: &mut S) -> u64 {
    s.call(O::AttemptCount, w(0)).a
}
fn more<S: Services>(s: &mut S) -> bool {
    read(s, 3) == 0 && s.call(O::IteratorAvailable, w(0)).x != 0
}
fn prepare<S: Services>(s: &mut S) -> i64 {
    if s.call(O::EncodeHost, w(0)).x == 0 {
        return INVALID;
    }
    if s.call(O::HostTrailing, w(0)).x != 0 {
        s.call(O::PushBase, w(1));
        return OK;
    }
    let length = s.call(O::BaseLength, w(0)).a;
    let (mut cursor, mut labels) = (0, 0_i64);
    while cursor < length {
        let byte = s.call(O::EncodedByte, wa(cursor)).a;
        if byte == 0 {
            break;
        }
        labels += 1;
        cursor += byte + 1;
    }
    let ndots = labels - 1;
    if ndots > 0 && read(s, 8) == 0 {
        s.call(O::PushBase, w(1));
        return OK;
    }
    let mut had = false;
    if ndots >= read(s, 9) {
        s.call(O::PushBase, w(0));
        had = true;
    }
    let suffixes = s.call(O::SearchCount, w(0)).a;
    for i in 0..suffixes {
        if s.call(O::EncodeSuffix, wa(i)).x == 0 {
            continue;
        }
        if s.call(O::EncodedLength, w(0)).a == length {
            if had {
                continue;
            }
            had = true;
        }
        s.call(O::PushSuffix, w(0));
    }
    if ndots > 0 && !had {
        s.call(O::PushBase, w(1));
    }
    if s.call(O::QnamesEmpty, w(0)).x != 0 {
        -805
    } else {
        OK
    }
}
fn pending<S: Services>(s: &mut S) -> bool {
    for i in 0..count(s) {
        let h = s.call(O::AttemptAt, wa(i)).a;
        if s.call(O::AttemptPending, wa(h)).x != 0 {
            return true;
        }
    }
    false
}
fn clear<S: Services>(s: &mut S, leave: u64) {
    let mut i = 0;
    while i < count(s) {
        let h = s.call(O::AttemptAt, wa(i)).a;
        if s.call(O::AttemptPending, wa(h)).x != 0 && h != leave {
            s.call(O::ClearErase, wa(i));
        } else {
            i += 1;
        }
    }
}
fn increment<S: Services>(s: &mut S) {
    let n = read(s, 10);
    set(s, 3, n + 1);
}
fn start_attempt<S: Services>(s: &mut S, attempt: u64, number: u64, record: bool) -> Words {
    s.call(O::StartAttempt, Words { a: attempt, b: number, ..w(i64::from(record)) })
}
fn transport<S: Services>(s: &mut S, server: u64, tcp: bool) -> Words {
    check(s, if tcp { 11 } else { 10 });
    let number = count(s);
    let h = s.call(if tcp { O::ConstructTcp } else { O::ConstructUdp }, wa(server)).a;
    increment(s);
    s.call(O::LogAttempt, Words { x: if tcp { 2 } else { 0 }, ..wa(h) });
    start_attempt(s, h, number, !tcp)
}
fn classic<S: Services>(s: &mut S) -> Words {
    if read(s, 4) != 0 {
        return w(INVALID);
    }
    s.call(O::NextQueryId, w(0));
    let empty = read(s, 5) != 0;
    s.call(O::PrepareClassicQuery, w(i64::from(empty)));
    check(s, 12);
    let server = s.call(O::IteratorNext, w(0)).a;
    let number = count(s);
    let tcp = read(s, 6) != 0;
    let result = transport(s, server, tcp);
    s.call(O::RecordAttempt, w(if tcp { 1 } else { 0 }));
    if result.x == PENDING {
        s.call(O::TimerClassic, Words { a: server, b: number, ..w(0) });
    }
    result
}
fn http<S: Services>(s: &mut S) -> Words {
    if read(s, 4) != 0 {
        return w(INVALID);
    }
    check(s, 13);
    let server = s.call(O::IteratorNext, w(0)).a;
    let number = count(s);
    let empty = number == 0;
    s.call(O::PrepareHttp, w(i64::from(empty)));
    let h = s.call(O::ConstructHttp, wa(server)).a;
    increment(s);
    s.call(O::LogAttempt, Words { x: 1, ..wa(h) });
    let result = start_attempt(s, h, number, true);
    if result.x == PENDING {
        s.call(O::TimerHttp, wa(server));
    }
    // Original returns attempts_.back(), not its pre-Start pointer.
    let last = s.call(O::LastAttempt, w(0)).a;
    Words { a: last, ..result }
}
fn platform<S: Services>(s: &mut S) -> Words {
    s.call(O::RecordAttempt, w(4));
    let fallback_number = read(s, 10) as u64;
    if read(s, 15) != 0 {
        clear(s, 0);
    }
    let number = count(s);
    let server = s.call(O::IteratorNext, w(0)).a;
    let h = s.call(O::ConstructPlatform, wa(server)).a;
    increment(s);
    let record = read(s, 14) != 0;
    let result = start_attempt(s, h, number, record);
    if result.x == PENDING {
        s.call(O::TimerPlatform, Words { a: server, b: fallback_number, ..w(0) });
    }
    result
}
fn attempt<S: Services>(s: &mut S) -> Words {
    let allowed = more(s);
    s.call(O::Check, Words { y: i64::from(allowed), ..w(2) });
    match read(s, 1) {
        0 => {
            check(s, 3);
            classic(s)
        }
        1 => {
            check(s, 4);
            s.call(O::RecordAttempt, w(3));
            http(s)
        }
        2 => platform(s),
        _ => {
            check(s, 99);
            w(INVALID)
        }
    }
}
fn query<S: Services>(s: &mut S) -> Words {
    s.call(O::LogQuery, w(0));
    s.call(O::ClearAttemptsAll, w(0));
    set(s, 3, 0);
    set(s, 2, 0);
    let mode = read(s, 1);
    let retries = if mode == 2 {
        if read(s, 14) != 0 {
            read(s, 13)
        } else {
            1
        }
    } else {
        0
    };
    s.call(O::ResetIterator, Words { y: retries, ..w(mode) });
    check(s, 5);
    if s.call(O::IteratorAvailable, w(0)).x == 0 {
        return w(-20);
    }
    attempt(s)
}
fn retry<S: Services>(s: &mut S, previous: u64) -> Words {
    s.call(O::Check, Words { a: previous, ..w(6) });
    set(s, 2, 1);
    let server = s.call(O::AttemptServer, wa(previous)).a;
    s.call(O::NextQueryId, w(0));
    s.call(O::CloneRetryQuery, wa(previous));
    clear(s, 0);
    let result = transport(s, server, true);
    s.call(O::RecordAttempt, w(2));
    if result.x == PENDING {
        s.call(O::TimerDouble, w(2));
    }
    result
}
fn timeout_timer<S: Services>(s: &mut S) {
    check(s, 7);
    let mode = read(s, 1);
    s.call(O::TimerTimeout, w(mode));
}
pub fn process<S: Services>(s: &mut S, mut result: Words) -> Words {
    while result.x != PENDING {
        s.call(O::LogResponse, wa(result.a));
        match result.x {
            OK | NAME => {
                s.call(O::RecordSuccess, wa(result.a));
                s.call(O::LogQueryEnd, w(result.x));
                if result.x == OK {
                    s.call(O::Check, Words { a: result.a, ..w(8) });
                    return result;
                }
                if s.call(O::QnamesEmpty, w(0)).x == 0 {
                    s.call(O::PopQname, w(0));
                }
                if s.call(O::QnamesEmpty, w(0)).x != 0 {
                    return result;
                }
                result = query(s);
            }
            TIMEOUT => {
                s.call(O::TimerStop, w(0));
                if result.a != 0 {
                    s.call(O::Check, Words { a: result.a, ..w(9) });
                    s.call(O::RecordFailure, result);
                }
                if more(s) {
                    result = attempt(s);
                    continue;
                }
                if read(s, 2) == 0 && pending(s) {
                    timeout_timer(s);
                    return w(PENDING);
                }
                return result;
            }
            -801 => {
                result = retry(s, result.a);
            }
            INVALID | -20 => {
                s.call(O::LogQueryEnd, w(result.x));
                return result;
            }
            _ => {
                s.call(O::Check, Words { a: result.a, ..w(14) });
                if s.call(O::AttemptIsLast, wa(result.a)).x != 0 {
                    s.call(O::TimerStop, w(0));
                    s.call(O::RecordFailure, result);
                    if more(s) {
                        result = attempt(s);
                        continue;
                    }
                    if read(s, 2) != 0 {
                        return result;
                    }
                    timeout_timer(s);
                }
                if pending(s) {
                    check(s, 15);
                    return w(PENDING);
                }
                return result;
            }
        }
    }
    result
}
fn record<S: Services>(s: &mut S, error: i64) {
    s.call(O::Check, Words { y: error, ..w(16) });
    let histogram = match read(s, 11) {
        1 => 0,
        28 => 1,
        65 => 2,
        _ => return,
    };
    let mode = read(s, 1);
    let retry = read(s, 3);
    let value = Words { a: 0, b: 0, x: histogram, y: error.wrapping_neg() };
    s.call(O::RecordLookup, value);
    let lookup_kind = match mode {
        0 => 1,
        1 => 3,
        2 => 4,
        _ => {
            check(s, 99);
            return;
        }
    };
    s.call(O::RecordLookup, Words { a: lookup_kind, ..value });
    if mode == 0 && retry != 0 {
        s.call(O::RecordLookup, Words { a: 2, ..value });
    }
}
fn callback<S: Services>(s: &mut S, result: Words) {
    s.call(O::Check, Words { y: result.x, ..w(17) });
    if read(s, 0) == 0 {
        return;
    }
    let response =
        if result.a != 0 { s.call(O::AttemptResponse, wa(result.a)).x != 0 } else { false };
    s.call(O::Check, Words { y: i64::from(result.x != OK || response), ..w(18) });
    s.call(O::TimerStop, w(0));
    if result.x == OK && result.a != 0 {
        s.call(O::SetDohDetails, wa(result.a));
    }
    s.call(O::LogEnd, w(result.x));
    record(s, result.x);
    s.call(O::RunCallback, result);
}
pub fn run<S: Services>(s: &mut S, event: u32, input: Words) -> Words {
    match event {
        E::Destroy => {
            if read(s, 0) != 0 {
                s.call(O::LogEnd, w(-3));
                record(s, -3);
            }
            w(0)
        }
        E::Start => {
            check(s, 0);
            set(s, 1, 0);
            check(s, 1);
            s.call(O::SetTimerRunner, w(0));
            s.call(O::LogStart, w(0));
            s.call(O::StartElapsed, w(0));
            let mut result = w(prepare(s));
            if result.x == OK {
                let size = read(s, 17);
                set(s, 4, size);
                let first = query(s);
                result = process(s, first);
            }
            if result.x != PENDING {
                clear(s, result.a);
                s.call(O::PostCallback, result);
            }
            w(0)
        }
        E::Prepare => w(prepare(s)),
        E::Callback => {
            callback(s, input);
            w(0)
        }
        E::Attempt => attempt(s),
        E::Classic => classic(s),
        E::Http => http(s),
        E::Platform => platform(s),
        E::Udp => transport(s, input.a, false),
        E::Tcp => transport(s, input.a, true),
        E::RetryTcp => retry(s, input.a),
        E::Query => query(s),
        E::More => w(i64::from(more(s))),
        E::Process => process(s, input),
        E::Clear => {
            clear(s, input.a);
            w(0)
        }
        E::Pending => w(i64::from(pending(s))),
        E::Complete => {
            let h = s.call(O::AttemptAt, wa(input.a)).a;
            if input.b != 0 && s.call(O::AttemptResponse, wa(h)).x != 0 {
                s.call(O::RecordRtt, Words { a: h, ..input });
            }
            if read(s, 0) == 0 {
                return w(0);
            }
            let result = process(s, Words { a: h, ..input });
            if result.x != PENDING {
                callback(s, result);
            }
            w(0)
        }
        E::Fallback => {
            if read(s, 0) == 0 {
                return w(0);
            }
            check(s, 19);
            let h = s.call(O::LastAttempt, w(0)).a;
            let result = process(s, Words { a: h, ..w(TIMEOUT) });
            if result.x != PENDING {
                callback(s, result);
            }
            w(0)
        }
        E::TimeoutTimer => {
            timeout_timer(s);
            w(0)
        }
        E::Timeout => {
            if read(s, 0) != 0 {
                callback(s, w(TIMEOUT));
            }
            w(0)
        }
        E::RecordLookup => {
            record(s, input.x);
            w(0)
        }
        _ => w(INVALID),
    }
}
