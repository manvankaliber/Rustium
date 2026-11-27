// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.

use crate::{Services, Words};
use crate::resolver_task::*;

use crate::client_bridge::{boolean as client_bool, query as client_query};
use crate::dns_request_family_ids::ClientEvent as Client;

pub fn islocaltask<S: Services>(s: &mut S) -> Words {
    Words::flag(local(s.get(1).a))
}

pub fn geteffectivesecurednsmode<S: Services>(s: &mut S) -> Words {
    let policy = s.get(1).a;
    if policy == 1 || policy == 2 {
        return Words::arg(OFF);
    }
    if s.yes(2) {
        return s.get(3);
    }
    Words::arg(OFF)
}

pub fn cacheresult<S: Services>(s: &mut S) -> Words {
    if s.yes(1) && (s.get(2).x == 0 || s.yes(3)) {
        s.act(4);
    }
    Words::default()
}

pub fn sethaveonlyloopbackaddresses<S: Services>(s: &mut S) -> Words {
    let old = s.get(1).a;
    let mask = s.get(2).a;
    let enabled = s.yes(3);
    s.call(4, Words::arg(if enabled { old | mask } else { old & !mask }));
    Words::default()
}

pub fn setinsecurednsclientenabled<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) {
        return Words::default();
    }
    let enabled = client_bool(s, Client::Insecure, Words::signed(-1), 3);
    let additional = enabled && client_bool(s, Client::Additional, Words::signed(-1), 4);
    s.act(5);
    if client_bool(s, Client::Insecure, Words::signed(-1), 3) != enabled
        || (client_bool(s, Client::Insecure, Words::signed(-1), 3)
            && client_bool(s, Client::Additional, Words::signed(-1), 4) != additional)
    {
        s.act(6);
    }
    Words::default()
}

pub fn setdnsconfigoverrides<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) && s.yes(3) {
        return Words::default();
    }
    s.act(4);
    let allowed = client_bool(s, Client::Secure, Words::default(), 5)
        || client_bool(s, Client::Insecure, Words::signed(-1), 6);
    if s.yes(7) {
        s.act(8);
        s.act(9);
        if allowed {
            s.act(10);
        }
    }
    Words::default()
}

pub fn maybeservefromcache<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) || s.get(3).a == 3 {
        return Words::default();
    }
    s.act(4);
    if s.get(5).a == 4 {
        s.act(6);
    }
    if s.get(3).a == 1 {
        s.act(7);
    } else {
        s.act(8);
    }
    if s.yes(9) {
        s.act(10);
        return Words::flag(true);
    }
    Words::default()
}

pub fn onfallbackresolve<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if client_bool(s, Client::InsecureFallback, Words::signed(-1), 2) {
        return Words::default();
    }
    s.act(3);
    if client_bool(s, Client::InsecureFallback, Words::signed(-1), 2) {
        s.act(4);
    }
    Words::default()
}

pub fn updatejobsforchangedconfig<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    if s.yes(3) {
        s.act(4);
    }
    Words::default()
}

pub fn setdnsclientfortesting<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        if !s.yes(3) {
            s.act(4);
        }
        s.act(5);
    }
    s.act(6);
    s.act(7);
    Words::default()
}

pub fn pushdnstasks<S: Services>(s: &mut S) -> Words {
    crate::client_bridge::scope(s, |s| {
        let p = s.get(11);
        let flags = s.get(12);
        push_dns_tasks(
            s,
            DnsTasks {
                allowed: p.a != 0,
                fallback: p.b != 0,
                system: p.x != 0,
                secure: p.y as u64,
                insecure: flags.a,
                cache: flags.b != 0,
                local_first: flags.x != 0,
            },
        );
        Words::default()
    })
}

pub fn createtasksequence<S: Services>(s: &mut S) -> Words {
    crate::client_bridge::scope(s, |s| {
        s.act(11);
        let ech = s.get(12).a;
        let cache = s.get(13).a != 3;
        let secure = s.get(14).a;
        if s.get(15).a == 2 {
            s.act(16);
            if cache {
                s.call(1, Words::arg(INSECURE_CACHE));
            }
            s.call(1, Words::arg(CONFIG));
            if cache {
                s.call(1, Words::arg(SECURE_CACHE));
            }
        } else if cache {
            s.call(1, Words::arg(if secure == SECURE { SECURE_CACHE } else { CACHE }));
        }
        s.call(2, Words::arg(HOSTS));
        let local_first = s.get(13).a == 1;
        let address = address_queries(s.get(17));
        let source = s.get(18).a;
        if source == 0 && s.yes(22) {
            let can_secure = client_bool(s, Client::Secure, Words::default(), 5);
            let can_insecure = client_bool(s, Client::Insecure, Words::signed(-1), 31);
            s.call(30, Words::arg(2 * (can_secure as u64) + (can_insecure as u64)));
        }
        match source {
            0 => {
                if s.yes(20) && address {
                    s.call(2, Words::arg(SYSTEM));
                } else if !s.yes(21) {
                    let system = address && secure != SECURE && ech != 2;
                    if s.yes(22) {
                        let mut insecure = 0;
                        if client_bool(s, Client::Insecure, Words::signed(ech as i64), 23)
                            && !client_bool(
                                s,
                                Client::InsecureFallback,
                                Words::signed(ech as i64),
                                24,
                            )
                            && (address
                                || client_bool(
                                    s,
                                    Client::Additional,
                                    Words::signed(ech as i64),
                                    25,
                                ))
                        {
                            insecure = client_query(s, Client::Mode, Words::signed(ech as i64), 26)
                                .x as u64;
                        }
                        let allowed = !s.yes(27);
                        let fallback = s.yes(28);
                        push_dns_tasks(
                            s,
                            DnsTasks {
                                allowed,
                                fallback,
                                system,
                                secure,
                                insecure,
                                cache,
                                local_first,
                            },
                        );
                    } else if system {
                        s.call(2, Words::arg(SYSTEM));
                    }
                } else {
                    s.call(2, Words::arg(if address { SYSTEM } else { MDNS }));
                }
            }
            1 => {
                s.call(2, Words::arg(SYSTEM));
            }
            2 => {
                if s.yes(22) {
                    let mut insecure = 0;
                    if client_bool(s, Client::Insecure, Words::signed(ech as i64), 23)
                        && (address
                            || client_bool(s, Client::Additional, Words::signed(ech as i64), 25))
                    {
                        insecure =
                            client_query(s, Client::Mode, Words::signed(ech as i64), 26).x as u64;
                    }
                    let allowed = !s.yes(27);
                    let fallback = s.yes(28);
                    push_dns_tasks(
                        s,
                        DnsTasks {
                            allowed,
                            fallback,
                            system: false,
                            secure,
                            insecure,
                            cache,
                            local_first,
                        },
                    );
                }
            }
            3 => {
                s.call(2, Words::arg(MDNS));
            }
            4 => (),
            _ => s.act(10),
        }
        if s.yes(20) {
            s.act(29);
        }
        Words::default()
    })
}

pub fn initializejobkeyandipaddress<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let ip = s.yes(2);
    s.act(3);
    let f = s.get(21);
    let mask = s.get(22);
    let mut flags = f.y as u64;
    if f.a != 0 {
        flags |= mask.a;
    }
    if f.b != 0 {
        flags |= mask.b;
    }
    if f.x != 0 {
        flags |= mask.x as u64;
    }
    s.call(23, Words::arg(flags));
    let specified = s.get(4).a;
    if specified != 0 {
        s.call(5, Words::arg(specified));
        return Words::default();
    }
    s.act(6);
    let mut local_v6 = true;
    if s.yes(7) {
        s.act(20);
        if s.yes(8) || s.yes(9) {
            local_v6 = s.yes(10);
        }
    }
    if !local_v6 && !ip && !s.yes(11) && !s.yes(12) && s.get(13).x == -1 {
        s.act(14);
    }
    if s.yes(15) && s.yes(16) {
        let scheme = s.get(17).a;
        if scheme != 0 {
            s.act(18);
        }
    }
    s.act(19);
    Words::default()
}

pub fn resolvelocally<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    if !s.yes(4) {
        let valid = if s.get(5).a == 3 {
            let valid = dns_names_policy_core::valid(s.bytes(6));
            s.act(32);
            valid
        } else {
            s.yes(7)
        };
        if !valid {
            s.act(8);
            return Words::default();
        }
    }
    let length = s.get(9).a;
    if length == 0 || length > 4096 {
        s.act(8);
        return Words::default();
    }
    if s.yes(4) {
        let f = s.get(10);
        if nat64_literal(f.a != 0, f.b != 0, f.x != 0, f.y) && s.yes(11) {
            s.act(12);
        } else {
            s.act(13);
        }
        return Words::default();
    }
    s.act(14);
    if s.yes(15) {
        return Words::default();
    }
    while s.get(16).a != 0 {
        let task = s.get(17).a;
        if !local(task) {
            break;
        }
        s.act(18);
        match task {
            SECURE_CACHE | INSECURE_CACHE | CACHE => {
                s.call(
                    19,
                    Words {
                        a: (task == SECURE_CACHE) as u64,
                        b: (task == CACHE) as u64,
                        ..Words::default()
                    },
                );
                if s.yes(15) {
                    s.act(20);
                    return Words::default();
                }
                s.act(21);
            }
            CONFIG => {
                s.act(22);
                if s.yes(15) {
                    s.act(23);
                    s.act(24);
                    return Words::default();
                }
            }
            HOSTS => {
                s.act(25);
                let count = s.get(26).a;
                let mut data = false;
                for i in 0..count {
                    if s.call(27, Words::arg(i)).a != 0 {
                        data = true;
                        break;
                    }
                }
                if count != 0 && data {
                    s.act(28);
                    s.act(29);
                    return Words::default();
                }
            }
            _ => s.act(30),
        }
    }
    s.act(31);
    Words::default()
}

pub fn resolveasip<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) {
        s.act(3);
        return Words::default();
    }
    if s.yes(4) {
        s.act(5);
    }
    s.act(6);
    Words::default()
}

pub fn maybereadfromconfig<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) {
        return Words::default();
    }
    s.act(3);
    if !s.yes(4) {
        return Words::default();
    }
    s.act(5);
    if s.get(6).a == 0 {
        return Words::default();
    }
    if s.yes(7) {
        s.act(8);
    }
    s.act(9);
    Words::default()
}

pub fn startbootstrapfollowup<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    if s.get(3).a != 0 {
        return Words::default();
    }
    s.act(4);
    s.act(5);
    Words::default()
}

pub fn createandstartjob<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    let fresh = s.yes(3);
    if fresh {
        s.act(4);
    } else {
        s.act(5);
    }
    s.act(6);
    if fresh {
        s.act(7);
    }
    Words::default()
}

pub fn createandstartjobforserviceendpointrequest<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    let fresh = s.yes(3);
    if fresh {
        s.act(4);
    } else {
        s.act(5);
    }
    s.act(6);
    if fresh {
        s.act(7);
    }
    Words::default()
}

pub fn servelocalhost<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !address_queries(s.get(2)) || !s.yes(3) {
        return Words::default();
    }
    if s.yes(4) && s.yes(5) && !s.yes(6) {
        s.act(7);
    }
    s.act(8);
    Words::default()
}

pub fn servefromhosts<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) || !address_queries(s.get(3)) || (s.get(4).a != 0 && s.get(5).a == SYSTEM) {
        return Words::default();
    }
    s.act(6);
    if !s.yes(7) || s.get(8).a == 0 {
        return Words::default();
    }
    s.act(9);
    for kind in [1_u64, 0] {
        if s.call(10, Words::arg(kind)).a != 0 {
            s.call(11, Words::arg(kind));
            if s.yes(12) {
                s.call(13, Words::arg(kind));
            } else {
                s.call(14, Words::arg(kind));
            }
        }
    }
    if s.yes(15) && s.yes(16) {
        let error = s.get(17).a;
        let mut only_loopback = true;
        if error == 0 {
            let count = s.get(18).a;
            for i in 0..count {
                if s.call(19, Words::arg(i)).a == 0 {
                    only_loopback = false;
                    break;
                }
            }
        }
        if error == 1 || (error == 0 && only_loopback) {
            s.act(20);
            s.act(21);
            return Words::default();
        }
    }
    if s.yes(22) {
        s.act(23);
    }
    if s.yes(16) {
        s.act(24);
    }
    Words::default()
}

pub fn removealljobs<S: Services>(s: &mut S) -> Words {
    s.act(1);
    while s.yes(2) {
        if s.yes(3) {
            s.act(4);
        } else {
            s.act(5);
        }
    }
    s.act(6);
    Words::default()
}

pub fn abortjobswithouttargetnetwork<S: Services>(s: &mut S) -> Words {
    s.act(1);
    while s.yes(2) {
        if !s.yes(3) && (!s.yes(4) || s.yes(5)) {
            s.act(6);
        } else {
            s.act(7);
        }
    }
    s.act(8);
    s.act(9);
    s.act(10);
    let mut i = 0;
    while s.yes(11) && i < s.get(12).a {
        s.call(13, Words::arg(i));
        i += 1;
    }
    if s.yes(11) {
        s.act(14);
    }
    Words::default()
}

pub fn abortinsecurednstasks<S: Services>(s: &mut S) -> Words {
    s.act(1);
    while s.yes(2) {
        s.act(3);
        s.act(4);
    }
    s.act(5);
    s.act(6);
    let count = s.get(7).a;
    for i in 0..count {
        s.call(8, Words::arg(i));
    }
    s.act(9);
    Words::default()
}

pub fn tryservingalljobsfromhosts<S: Services>(s: &mut S) -> Words {
    if !s.yes(1) {
        return Words::default();
    }
    s.act(2);
    s.act(3);
    while s.yes(4) && s.yes(5) {
        s.act(6);
        s.act(7);
    }
    Words::default()
}

pub fn onsystemdnsconfigchanged<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let mut changed = false;
    let mut allowed = false;
    if s.yes(2) {
        allowed = client_bool(s, Client::Secure, Words::default(), 3)
            || client_bool(s, Client::Insecure, Words::signed(-1), 4);
        changed = s.yes(5);
    }
    s.act(6);
    if changed && allowed {
        s.act(7);
    }
    Words::default()
}

pub fn updateconnectiontype<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) && s.yes(3) {
        s.act(4);
        s.act(5);
    }
    Words::default()
}

pub fn invalidatecaches<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    while s.yes(4) {
        if s.yes(5) {
            s.act(6);
        } else {
            s.act(7);
        }
        s.act(8);
    }
    s.act(9);
    s.act(10);
    Words::default()
}

pub fn onipaddresschanged<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    s.act(5);
    Words::default()
}

pub fn createdohproberunner<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) || !client_bool(s, Client::Secure, Words::default(), 3) {
        return Words::default();
    }
    s.act(4);
    Words::default()
}

pub fn getorcreatemdnsclient<S: Services>(s: &mut S) -> Words {
    if !s.yes(1) {
        if !s.yes(2) {
            s.act(3);
        }
        s.act(4);
    }
    let mut rv = 0;
    if !s.yes(5) {
        rv = s.get(6).x;
    }
    s.call(7, Words::signed(rv));
    if rv == 0 {
        s.act(8);
    }
    Words::signed(rv)
}

pub fn finishgloballyreachablecheck<S: Services>(s: &mut S) -> Words {
    if s.get(1).x != 0 {
        return Words::flag(false);
    }
    if s.get(2).x != 0 {
        return Words::flag(false);
    }
    if s.yes(3) {
        return Words::flag(false);
    }
    if s.yes(4) && s.yes(5) {
        return Words::flag(false);
    }
    Words::flag(true)
}

pub fn setlastipv6proberesult<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    Words::default()
}

pub fn finishipv6reachabilitycheck<S: Services>(s: &mut S) -> Words {
    let ok = s.get(2).x == 0;
    s.call(1, Words::flag(ok));
    s.act(3);
    if s.get(4).a != 0 {
        s.act(5);
        let count = s.get(6).a;
        for i in 0..count {
            s.call(7, Words::arg(i));
        }
    }
    Words::default()
}

pub fn startipv6reachabilitycheck<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.get(2).x != -1 {
        return Words::signed(0);
    }
    if !s.yes(3) && s.yes(4) {
        s.act(5);
        return Words::signed(0);
    }
    if s.yes(6) {
        s.act(7);
        return s.get(8);
    }
    let mut rv = 0;
    let mut cached = true;
    if s.yes(9) || s.get(10).x > 1000 {
        s.act(11);
        rv = s.get(12).x;
        if rv != s.get(8).x {
            s.call(13, Words::flag(rv == 0));
            rv = 0;
        }
        cached = false;
    }
    s.call(14, Words::flag(cached));
    Words::signed(rv)
}

pub fn startgloballyreachablecheck<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let mut rv = s.get(2).x;
    if rv != s.get(3).x {
        rv = if s.yes(4) { 0 } else { s.get(5).x };
    }
    Words::signed(rv)
}

pub fn destroy_hostresolvermanager<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    if s.get(4).x == -1 {
        s.act(5);
        s.act(6);
    }
    if s.yes(7) {
        s.act(8);
    }
    Words::default()
}

pub fn getdnsconfigasvalue<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        s.act(3);
    }
    Words::default()
}

pub fn registerresolvecontext<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        s.act(3);
    } else {
        s.act(4);
    }
    Words::default()
}

pub fn createmdnslistener<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    let rv = s.get(3).x;
    if rv == 0 {
        s.act(4);
    } else {
        s.call(5, Words::signed(rv));
    }
    Words::default()
}

pub fn shouldforcesystemresolverduetotestoverride<S: Services>(s: &mut S) -> Words {
    if s.yes(1) && s.yes(2) {
        s.act(3);
    }
    Words::flag(!s.yes(4) && s.yes(1) && !s.yes(2))
}

pub fn runfinishgloballyreachablecheck<S: Services>(s: &mut S) -> Words {
    let ok = s.yes(1);
    s.call(2, Words::flag(ok));
    Words::default()
}

pub fn getportforgloballyreachablecheck<S: Services>(s: &mut S) -> Words {
    if !s.yes(1) {
        return Words::signed(443);
    }
    s.get(2)
}

pub fn recorddnsclientcapabilitymetrics<S: Services>(s: &mut S) -> Words {
    crate::client_bridge::scope(s, |s| {
        if !s.yes(1) {
            return Words::default();
        }
        let secure = client_bool(s, Client::Secure, Words::default(), 2);
        let insecure = client_bool(s, Client::Insecure, Words::signed(-1), 3);
        s.call(4, Words::arg(2 * (secure as u64) + (insecure as u64)));
        Words::default()
    })
}

pub fn resolvelocalhostname<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) {
        return Words::flag(false);
    }
    s.act(3);
    s.act(4);
    Words::flag(true)
}

pub fn resemblesmulticastdnsname<S: Services>(s: &mut S) -> Words {
    let bytes = s.bytes(1);
    Words::flag(bytes.ends_with(b".local") || bytes.ends_with(b".local."))
}

pub fn configureasyncdnsnofallbackfieldtrial<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let group = s.bytes(2);
    let prefix = b"AsyncDnsNoFallback";
    Words::flag(group.len() >= prefix.len() && group[..prefix.len()].eq_ignore_ascii_case(prefix))
}

pub fn requestwillusewifi<S: Services>(s: &mut S) -> Words {
    let network = s.get(1).x;
    let kind = if network == -1 { s.get(2).x } else { s.get(3).x };
    Words::flag(kind == s.get(4).x)
}

pub fn netlogresults<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    while s.yes(3) {
        s.act(4);
        s.act(5);
    }
    s.act(6);
    Words::default()
}

pub fn filteraddresses<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let query = s.get(2);
    let family = address_family(query.a != 0, query.b != 0) as i64;
    if family == s.get(3).x {
        return Words::default();
    }
    let count = s.get(4).a;
    let mut out = 0;
    for i in 0..count {
        if s.call(5, Words::arg(i)).x == family {
            if out != i {
                s.call(6, Words { a: out, b: i, ..Words::default() });
            }
            out += 1;
        }
    }
    s.call(7, Words::arg(out));
    Words::default()
}

pub fn start<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) {
        return s.get(3);
    }
    s.act(4);
    s.act(5);
    s.get(6)
}

pub fn startrunner<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if !s.yes(2) {
        return Words::default();
    }
    if !s.yes(3) {
        s.act(4);
    }
    if s.yes(3) {
        s.act(5);
    }
    Words::default()
}

pub fn cancelrunner<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        s.act(2);
    }
    s.act(3);
    Words::default()
}

pub fn getdispatcherlimits<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.get(2).a != s.get(3).a {
        return Words::default();
    }
    s.call(4, Words::arg(64));
    s.act(5);
    let parts = {
        let group = s.bytes(6);
        if group.is_empty() {
            return Words::default();
        }
        let mut offsets = Vec::new();
        let mut begin = 0;
        for end in 0..=group.len() {
            if end == group.len() || group[end] == b':' {
                let mut a = begin;
                let mut b = end;
                while a < b && matches!(group[a],b' '|b'\t'|b'\n'|b'\x0b'|b'\x0c'|b'\r') {
                    a += 1;
                }
                while b > a && matches!(group[b-1],b' '|b'\t'|b'\n'|b'\x0b'|b'\x0c'|b'\r') {
                    b -= 1;
                }
                offsets.push((a as u64, (b - a) as u64));
                begin = end + 1;
            }
        }
        offsets
    };
    let priorities = s.get(7).a;
    if parts.len() as u64 != priorities + 1 {
        s.act(8);
    }
    let mut parsed = Vec::with_capacity(parts.len());
    for (a, b) in parts {
        let reply = s.call(9, Words { a, b, ..Words::default() });
        if reply.b == 0 {
            s.act(8);
        }
        parsed.push(reply.a);
    }
    let total = parsed.pop().unwrap_or(0);
    let sum = parsed.iter().fold(0_usize, |a, b| a.wrapping_add(*b as usize)) as u64;
    let minimum = s.get(10).a as usize;
    if sum > total || (sum == total && parsed[minimum] == 0) {
        s.act(8);
    }
    s.call(4, Words::arg(total));
    s.act(11);
    for value in parsed {
        s.call(12, Words::arg(value));
    }
    Words::default()
}

pub fn hostresolvermanager<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    s.act(5);
    if !s.yes(6) {
        s.act(7);
        s.act(8);
    }
    if s.yes(9) {
        s.act(10);
    }
    s.act(11);
    if s.yes(6) {
        s.act(12);
    } else {
        s.act(13);
    }
    s.act(14);
    s.act(15);
    s.act(16);
    Words::default()
}

pub fn destroy_proberequestimpl<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        s.act(2);
    }
    Words::default()
}

pub fn createserviceendpointrequest<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        s.act(3);
        s.act(4);
    }
    s.act(5);
    s.act(6);
    Words::default()
}

pub fn run<S: Services>(method: u32, s: &mut S) -> Words {
    match method {
        1 => islocaltask(s),
        2 => geteffectivesecurednsmode(s),
        3 => cacheresult(s),
        4 => sethaveonlyloopbackaddresses(s),
        5 => setinsecurednsclientenabled(s),
        6 => setdnsconfigoverrides(s),
        7 => maybeservefromcache(s),
        8 => onfallbackresolve(s),
        9 => updatejobsforchangedconfig(s),
        10 => setdnsclientfortesting(s),
        11 => pushdnstasks(s),
        12 => createtasksequence(s),
        13 => initializejobkeyandipaddress(s),
        14 => resolvelocally(s),
        15 => resolveasip(s),
        16 => maybereadfromconfig(s),
        17 => startbootstrapfollowup(s),
        18 => createandstartjob(s),
        19 => createandstartjobforserviceendpointrequest(s),
        20 => servelocalhost(s),
        21 => servefromhosts(s),
        22 => removealljobs(s),
        23 => abortjobswithouttargetnetwork(s),
        24 => abortinsecurednstasks(s),
        25 => tryservingalljobsfromhosts(s),
        26 => onsystemdnsconfigchanged(s),
        27 => updateconnectiontype(s),
        28 => invalidatecaches(s),
        29 => onipaddresschanged(s),
        30 => createdohproberunner(s),
        31 => getorcreatemdnsclient(s),
        32 => finishgloballyreachablecheck(s),
        33 => setlastipv6proberesult(s),
        34 => finishipv6reachabilitycheck(s),
        35 => startipv6reachabilitycheck(s),
        36 => startgloballyreachablecheck(s),
        37 => destroy_hostresolvermanager(s),
        38 => getdnsconfigasvalue(s),
        39 => registerresolvecontext(s),
        40 => createmdnslistener(s),
        41 => shouldforcesystemresolverduetotestoverride(s),
        42 => runfinishgloballyreachablecheck(s),
        43 => getportforgloballyreachablecheck(s),
        44 => recorddnsclientcapabilitymetrics(s),
        45 => resolvelocalhostname(s),
        46 => resemblesmulticastdnsname(s),
        47 => configureasyncdnsnofallbackfieldtrial(s),
        48 => requestwillusewifi(s),
        49 => netlogresults(s),
        50 => filteraddresses(s),
        51 => start(s),
        52 => startrunner(s),
        53 => cancelrunner(s),
        54 => getdispatcherlimits(s),
        55 => hostresolvermanager(s),
        56 => destroy_proberequestimpl(s),
        57 => createserviceendpointrequest(s),
        _ => Words::default(),
    }
}
