//! DoH probe lifecycle and result policy; native owns clocks/backoff/wire/WeakPtr.
use crate::Words;
use crate::dns_request_family_services::{Services, w, wa};
use crate::dns_request_family_ids::{ProbeEvent as E, ProbeOp as O};
fn read<S: Services>(s: &mut S, f: i64) -> Words {
    s.call(O::Read, w(f))
}
fn continue_probe<S: Services>(s: &mut S, server: u64, network: i64) {
    if read(s, 0).x == 0 || read(s, 1).x == 0 {
        s.call(O::ClearAll, w(0));
        return;
    }
    if read(s, 4).x == 0 {
        return;
    }
    if s.call(O::Read, Words { a: server, ..w(5) }).x != 0 {
        s.call(O::StatsClear, wa(server));
        return;
    }
    s.call(O::Check, w(2));
    s.call(O::BackoffFailure, w(0));
    s.call(O::PostContinue, Words { a: server, ..w(network) });
    let number = read(s, 6).a;
    s.call(O::ConstructHttp, Words { a: server, b: u64::from(number == 0), ..w(0) });
    s.call(O::StartAttempt, Words { a: number, b: server, ..w(network) });
}
fn complete<S: Services>(s: &mut S, input: Words) {
    let mut success = false;
    if read(s, 4).x != 0 && read(s, 0).x != 0 && read(s, 1).x != 0 {
        if input.x != 0 {
            s.call(O::RecordFailure, input);
        } else {
            s.call(O::Check, Words { a: input.a, ..w(3) });
            if s.call(O::ResponsePresent, wa(input.a)).x != 0 {
                s.call(O::ExtractResponse, wa(input.a));
                let results = s.call(O::ResultCount, w(0));
                if results.b != 0 {
                    for i in 0..results.a {
                        if s.call(O::ResultIsData, wa(i)).x != 0
                            && s.call(O::ResultHasEndpoints, wa(i)).x != 0
                        {
                            s.call(O::RecordSuccess, Words { a: input.b, ..w(0) });
                            s.call(O::RecordRtt, Words { a: input.b, ..w(input.x) });
                            success = true;
                            break;
                        }
                    }
                }
            }
            if !success {
                s.call(O::RecordFailure, Words { a: input.a, b: input.b, x: -814, y: input.y });
            }
        }
    }
    let histogram = u64::from(success) + 2 * u64::from(input.y != 0);
    s.call(O::RecordSequence, Words { a: histogram, ..input });
}
pub fn run<S: Services>(s: &mut S, event: u32, input: Words) -> Words {
    match event {
        E::Initialize => {
            s.call(O::Check, w(0));
            s.call(O::EncodeProbe, w(0));
            let count = read(s, 2).a;
            s.call(O::Resize, wa(count));
            w(0)
        }
        E::Start => {
            s.call(O::Check, w(1));
            let count = read(s, 2).a;
            for server in 0..count {
                if s.call(O::StatsPresent, wa(server)).x != 0 {
                    continue;
                }
                s.call(O::NewStats, wa(server));
                s.call(O::ClockNow, w(0));
                continue_probe(s, server, input.x);
            }
            w(0)
        }
        E::Delay => {
            if input.a >= read(s, 3).a || s.call(O::StatsPresent, wa(input.a)).x == 0 {
                w(0)
            } else {
                s.call(O::Delay, wa(input.a))
            }
        }
        E::Continue => {
            continue_probe(s, input.a, input.x);
            w(0)
        }
        E::Complete => {
            complete(s, input);
            w(0)
        }
        _ => w(-4),
    }
}
