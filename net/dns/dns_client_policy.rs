//! Complete DNS-client configuration, upgrade, selection and fallback sequencing.
use crate::Words;
use crate::dns_request_family_services::{Services, w, wa};
use crate::dns_request_family_ids::{ClientEvent as E, ClientOp as O};

fn read<S: Services>(s: &mut S, field: i64) -> i64 {
    s.call(O::Read, w(field)).x
}
fn write<S: Services>(s: &mut S, field: i64, value: i64) {
    s.call(O::Write, Words { y: value, ..w(field) });
}
pub fn use_platform<S: Services>(s: &mut S, ech: i64) -> bool {
    ech == 2 && read(s, 7) != 0
}
pub fn mode<S: Services>(s: &mut S, ech: i64) -> i64 {
    if use_platform(s, ech) {
        3
    } else {
        read(s, 2)
    }
}
pub fn insecure<S: Services>(s: &mut S, ech: i64) -> bool {
    match mode(s, ech) {
        0 => false,
        2 | 3 => true,
        1 => read(s, 4) != 0 && read(s, 3) == 0 && read(s, 5) == 0,
        _ => {
            s.call(O::Check, w(0));
            false
        }
    }
}
fn metric<S: Services>(s: &mut S, kind: i64, value: i64) {
    s.call(O::Metric, Words { y: value, ..w(kind) });
}
pub fn upgrade<S: Services>(s: &mut S) {
    write(s, 4, 0);
    let mut has_doh = read(s, 12) != 0;
    if read(s, 13) == 0 && !has_doh && read(s, 14) != 0 && read(s, 15) == 1 {
        if read(s, 16) == 0 {
            s.call(O::UpgradeDot, w(0));
            has_doh = read(s, 12) != 0;
            metric(s, 0, i64::from(has_doh));
        } else {
            let mut all_local = true;
            let count = s.call(O::NameserverCount, w(0)).a;
            for i in 0..count {
                if s.call(O::NameserverPublic, wa(i)).x != 0 {
                    all_local = false;
                    break;
                }
            }
            metric(s, 1, i64::from(!all_local));
            s.call(O::UpgradeNameservers, w(0));
            has_doh = read(s, 12) != 0;
            if !has_doh {
                let fallback_provided = read(s, 17) != 0;
                let mut loopback = false;
                let mut non_loopback = false;
                for i in 0..count {
                    if s.call(O::NameserverLoopback, wa(i)).x != 0 {
                        loopback = true;
                    } else if s.call(O::NameserverPublic, wa(i)).x == 0 {
                        non_loopback = true;
                    }
                }
                metric(s, 2, i64::from(loopback) + 2 * i64::from(non_loopback));
                let local_allowed = !(loopback || non_loopback) || s.call(O::Feature, w(0)).x != 0;
                if local_allowed && fallback_provided && s.call(O::Feature, w(1)).x != 0 {
                    s.call(O::UpgradeNameservers, w(1));
                    let upgraded = read(s, 12) != 0;
                    write(s, 4, i64::from(upgraded));
                }
                has_doh = read(s, 12) != 0;
            }
            let fallback = read(s, 19);
            metric(s, 3, fallback);
            metric(s, 4, i64::from(has_doh));
        }
    } else {
        metric(s, 5, i64::from(has_doh));
        let unhandled = read(s, 13);
        metric(s, 6, unhandled);
    }
}
fn build<S: Services>(s: &mut S) {
    let all = s.call(O::HasOverridesAll, w(0)).x != 0;
    s.call(O::ApplyOverrides, w(i64::from(!all)));
    upgrade(s);
    if read(s, 13) != 0 {
        s.call(O::ClearNameservers, w(0));
    }
}
fn session<S: Services>(s: &mut S) {
    s.call(O::ClearFactory, w(0));
    s.call(O::NewSession, w(0));
    s.call(O::NewFactory, w(0));
}
fn update<S: Services>(s: &mut S) -> bool {
    build(s);
    if s.call(O::ConfigSame, w(0)).x != 0 {
        return false;
    }
    write(s, 3, 0);
    session(s);
    if read(s, 18) != 0 {
        s.call(O::LogChanged, w(0));
    }
    true
}
pub fn run<S: Services>(s: &mut S, event: u32, input: Words) -> Words {
    match event {
        E::Secure => w(i64::from(read(s, 1) != 0)),
        E::Insecure => w(i64::from(insecure(s, input.x))),
        E::Additional => {
            let usable = insecure(s, input.x);
            s.call(O::Check, w(i64::from(usable)));
            let result = use_platform(s, input.x) || read(s, 6) != 0;
            w(i64::from(result))
        }
        E::Mode => w(mode(s, input.x)),
        E::UsePlatform => w(i64::from(use_platform(s, input.x))),
        E::SecureFallback => {
            let reason = if read(s, 1) == 0 {
                1
            } else if read(s, 9) == 0 {
                4
            } else {
                0
            };
            metric(s, 7, reason);
            w(i64::from(reason != 0))
        }
        E::InsecureFallback => {
            if use_platform(s, input.x) {
                return w(0);
            }
            w(i64::from(!insecure(s, input.x) || read(s, 10) >= 16))
        }
        E::SetSystem => {
            if s.call(O::SystemSame, w(0)).x != 0 {
                return w(0);
            }
            s.call(O::StoreSystem, w(0));
            w(i64::from(update(s)))
        }
        E::SetOverrides => {
            if s.call(O::OverridesSame, w(0)).x != 0 {
                return w(0);
            }
            s.call(O::StoreOverrides, w(0));
            w(i64::from(update(s)))
        }
        E::ReplaceSession => {
            s.call(O::ApplyOverrides, w(2));
            session(s);
            w(0)
        }
        E::Session => {
            session(s);
            w(0)
        }
        E::Build => {
            build(s);
            w(0)
        }
        E::Update => w(i64::from(update(s))),
        E::Upgrade => {
            upgrade(s);
            w(0)
        }
        E::Preset => {
            if read(s, 20) == 0 {
                return w(0);
            }
            let count = s.call(O::PresetCount, w(0)).a;
            for server in 0..count {
                if s.call(O::PresetMatches, wa(server)).x == 0 {
                    continue;
                }
                // A matched server with zero addresses returns engaged empty.
                s.call(O::Write, w(5));
                let groups = s.call(O::PresetGroupCount, wa(server)).a;
                for group in 0..groups {
                    let args = Words { a: server, b: group, ..w(0) };
                    let addresses = s.call(O::PresetAddressCount, args).a;
                    for address in 0..addresses {
                        s.call(O::PresetAppend, Words { x: address as i64, ..args });
                    }
                }
                return w(1);
            }
            w(0)
        }
        E::ConfigLog => {
            s.call(O::PrepareLog, w(0));
            let secure = read(s, 1) != 0;
            s.call(O::SetLogBool, Words { y: i64::from(secure), ..w(0) });
            let usable = insecure(s, -1);
            s.call(O::SetLogBool, Words { y: i64::from(usable), ..w(1) });
            w(0)
        }
        E::SetInsecure => {
            write(s, 1, input.x);
            write(s, 2, input.y);
            w(0)
        }
        E::IncrementFailures => {
            let n = read(s, 10);
            write(s, 3, n.wrapping_add(1));
            w(0)
        }
        E::ClearFailures => {
            write(s, 3, 0);
            w(0)
        }
        _ => w(-4),
    }
}
