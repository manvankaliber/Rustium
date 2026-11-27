// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.

use crate::{Services, Words};
use crate::resolver_task::*;

pub fn calculateresolvepath<S: Services>(s: &mut S) -> Words {
    let task = s.get(1).a;
    let f = s.get(2);
    match fallback_path(task, f.a != 0, f.b != 0, f.x != 0) {
        Some(v) => Words { a: 1, b: v, ..Words::default() },
        None => Words::default(),
    }
}

pub fn attemptmodetotasktype<S: Services>(s: &mut S) -> Words {
    let mode = s.get(1).a;
    Words::arg(attempt_task(mode))
}

pub fn gettargetnetwork<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        let network = s.get(2).x;
        if network != -1 {
            s.act(3);
            return s.get(2);
        }
    }
    s.get(4)
}

pub fn getechmode<S: Services>(s: &mut S) -> Words {
    if !s.yes(1) {
        return Words::arg(1);
    }
    s.get(2)
}

pub fn schedule<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        s.act(3);
    } else {
        s.act(4);
    }
    if !s.yes(5) {
        s.act(6);
    }
    Words::default()
}

pub fn cancelrequest<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    if s.get(3).a > 0 {
        s.act(4);
        s.act(5);
    } else {
        s.act(6);
    }
    Words::default()
}

pub fn cancelserviceendpointrequest<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    if s.get(3).a > 0 {
        s.act(4);
        s.act(5);
    } else {
        s.act(6);
    }
    Words::default()
}

pub fn addrequest<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    s.act(5);
    Words::default()
}

pub fn addserviceendpointrequest<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    s.act(5);
    Words::default()
}

pub fn abortinsecurednstask<S: Services>(s: &mut S) -> Words {
    let fallback = s.yes(1);
    if fallback {
        s.act(2);
        while s.yes(3) {
            if matches!(s.get(4).a,DNS|PLATFORM) {
                s.act(5);
            } else {
                s.act(6);
            }
        }
    }
    if s.yes(7) && !s.yes(8) {
        if fallback {
            s.act(9);
            s.act(10);
            s.act(11);
        } else if !s.yes(12) {
            s.act(13);
        }
    }
    Words::default()
}

pub fn runnexttask<S: Services>(s: &mut S) -> Words {
    if s.get(1).a == 0 {
        let count = s.get(2).a;
        if count == 0 {
            s.act(3);
            return Words::default();
        }
        let mut i = 0_u64;
        while i.wrapping_add(1) < s.get(2).a {
            s.call(4, Words::arg(i));
            i += 1;
        }
        s.act(5);
        return Words::default();
    }
    let task = s.get(6).a;
    if !s.yes(7) && dispatched(task) {
        s.act(8);
        s.act(9);
        s.act(10);
        if s.get(11).a > s.get(12).a {
            s.act(13);
        }
        return Words::default();
    }
    if s.yes(14) {
        s.act(15);
        s.act(16);
    }
    s.act(17);
    s.act(18);
    match task {
        SYSTEM => s.act(19),
        DNS => s.act(20),
        PLATFORM => s.act(21),
        SECURE_DNS => s.act(22),
        MDNS => s.act(23),
        INSECURE_CACHE => s.act(24),
        NAT64 => s.act(25),
        _ => s.act(26),
    }
    Words::default()
}

pub fn finish<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        s.act(2);
        s.act(3);
        s.act(4);
        s.act(5);
        if s.yes(6) {
            s.act(7);
            if s.yes(8) {
                s.act(9);
            }
            s.act(10);
        }
    } else if s.yes(11) {
        s.act(12);
        if s.yes(8) {
            s.act(13);
        }
        s.act(14);
    }
    Words::default()
}

pub fn killdnstask<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        if s.yes(2) {
            while s.get(3).x > 1 || s.yes(4) {
                s.act(5);
            }
        }
        s.act(6);
    }
    s.act(7);
    Words::default()
}

pub fn reducebyonejobslot<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        if s.yes(3) {
            s.act(4);
        }
        s.act(5);
    } else if s.get(6).x > 1 {
        if s.yes(3) {
            s.act(7);
        }
        s.act(8);
    } else {
        s.act(9);
    }
    Words::default()
}

pub fn updatepriority<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        s.act(2);
    }
    Words::default()
}

pub fn start<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    if s.get(3).x >= 2 {
        if !s.yes(4) {
            s.act(5);
            return Words::default();
        }
        s.act(6);
        s.act(7);
        if s.get(8).x >= 1 {
            s.act(9);
        }
        return Words::default();
    }
    s.act(10);
    s.act(11);
    Words::default()
}

pub fn insecurecachelookup<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    if s.yes(3) {
        s.act(4);
        s.act(5);
    } else {
        s.act(6);
    }
    Words::default()
}

pub fn startdnstask<S: Services>(s: &mut S) -> Words {
    let secure = s.yes(1);
    s.call(2, Words::flag(secure));
    s.act(3);
    s.act(4);
    if secure {
        s.act(5);
    }
    if s.yes(6) || s.yes(7) || s.yes(8) {
        s.act(9);
    }
    s.act(10);
    if secure {
        while s.get(11).x >= 1 {
            s.act(10);
        }
        s.act(12);
    } else if s.get(11).x >= 1 {
        s.act(13);
    }
    Words::default()
}

pub fn onintermediatetransactionscomplete<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        s.act(2);
        let mut unused = s.get(3).x - s.get(4).x;
        while unused > 0 && s.get(5).x > 0 {
            s.act(6);
            unused -= 1;
        }
        if s.yes(7) && s.get(5).x == 0 {
            s.act(8);
            s.act(9);
        }
        while unused > 0 {
            s.act(10);
            unused -= 1;
        }
    } else if s.get(5).x >= 1 {
        s.act(6);
    }
    if s.yes(11) && s.yes(12) {
        s.act(13);
    }
    Words::default()
}

pub fn shouldsorttransactionsindividually<S: Services>(s: &mut S) -> Words {
    if !s.yes(1) {
        return Words::flag(false);
    }
    if s.yes(2) {
        return Words::flag(true);
    }
    if s.yes(3) && !s.yes(4) {
        return Words::flag(false);
    }
    if s.yes(5) && !s.yes(6) {
        return Words::flag(false);
    }
    Words::flag(true)
}

pub fn maybecacheresult<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        s.act(2);
    }
    Words::default()
}

pub fn cancelrequests<S: Services>(s: &mut S) -> Words {
    s.act(1);
    while s.yes(2) {
        s.act(3);
        s.act(4);
        s.act(5);
        if !s.yes(6) {
            return Words::default();
        }
    }
    while s.yes(7) {
        s.act(8);
        s.act(9);
        s.act(10);
        if !s.yes(6) {
            return Words::default();
        }
    }
    Words::default()
}

pub fn completerequestswithoutcache<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        s.act(2);
        while s.yes(3) {
            if !s.yes(4) {
                s.act(5);
            }
            s.act(6);
        }
    }
    s.act(7);
    Words::default()
}

pub fn completerequests<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        let task = s.get(3).a;
        let source = match task {
            SECURE_DNS => 0,
            DNS => 1,
            SYSTEM => 2,
            PLATFORM => 3,
            MDNS => 4,
            NAT64 => 5,
            INSECURE_CACHE => 6,
            _ => {
                s.act(4);
                0
            }
        };
        s.call(5, Words::arg(source));
    }
    if s.get(6).x == 0 {
        s.act(7);
    }
    s.act(8);
    if s.yes(9) {
        s.act(10);
    }
    s.act(11);
    if s.yes(12) {
        s.act(13);
        return Words::default();
    }
    s.act(14);
    if s.yes(15) {
        s.act(16);
    }
    s.act(17);
    if s.get(6).x == 0 && s.yes(18) && s.get(19).a == 0 {
        s.act(20);
    }
    while s.yes(21) {
        s.act(22);
        s.act(23);
        s.act(24);
        if s.get(6).x == 0 && !s.yes(25) {
            s.act(26);
        }
        s.act(27);
        if !s.yes(28) {
            return Words::default();
        }
    }
    while s.yes(29) {
        s.act(30);
        s.act(31);
        s.act(32);
        if !s.yes(28) {
            return Words::default();
        }
    }
    Words::default()
}

pub fn onsystemtaskcomplete<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    let mut error = s.get(3).x;
    if error == 0 {
        s.act(4);
    } else {
        s.act(5);
    }
    if s.get(6).x != 0 && error == 0 {
        s.act(7);
        s.act(8);
    }
    if s.yes(9) {
        error = s.get(10).x;
    }
    s.call(11, Words { a: (error == 0) as u64, x: error, ..Words::default() });
    Words::default()
}

pub fn ondnstaskfailure<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let secure = s.yes(3);
    s.call(2, Words::flag(secure));
    if !s.yes(4) {
        return Words::default();
    }
    s.act(5);
    let ttl = s.yes(7);
    s.call(6, Words::flag(ttl));
    s.act(8);
    s.act(9);
    if !s.yes(10) {
        s.act(11);
    }
    s.act(12);
    Words::default()
}

pub fn ondnstaskcomplete<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        s.act(3);
    }
    s.act(4);
    s.act(5);
    if address_queries(s.get(6)) && s.get(7).x == 0 && s.get(8).a == 0 {
        s.act(9);
    }
    s.act(10);
    if s.get(7).x != 0 {
        s.act(11);
        return Words::default();
    }
    s.act(12);
    let secure = s.yes(13);
    s.call(14, Words::flag(secure));
    s.act(15);
    if !secure {
        s.act(16);
    }
    if s.yes(17) {
        s.act(18);
    } else {
        s.act(19);
    }
    if s.yes(20) {
        s.act(21);
        return Words::default();
    }
    s.call(22, Words::flag(secure));
    Words::default()
}

pub fn startsystemtask<S: Services>(s: &mut S) -> Words {
    s.act(1);
    if s.yes(2) {
        s.act(3);
    }
    let query = s.get(6);
    let family = address_family(query.a != 0, query.b != 0);
    s.call(4, Words::arg(family));
    s.act(5);
    Words::default()
}

pub fn startmdnstask<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let rv = s.get(2).x;
    s.act(3);
    if rv == 0 {
        s.act(4);
    } else {
        s.call(5, Words::signed(rv));
    }
    Words::default()
}

pub fn onmdnstaskcomplete<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    if s.yes(3) {
        s.act(4);
        return Words::default();
    }
    s.act(5);
    Words::default()
}

pub fn onserviceendpointsupdated<S: Services>(s: &mut S) -> Words {
    s.act(1);
    while s.yes(2) {
        s.act(3);
        s.act(4);
    }
    Words::default()
}

pub fn servefromhosts<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    let count = s.get(3).a;
    let mut found = false;
    for index in 0..count {
        if s.call(4, Words::arg(index)).a != 0 {
            found = true;
            break;
        }
    }
    if count != 0 && found {
        s.act(5);
        s.act(6);
        return Words::flag(true);
    }
    Words::flag(false)
}

pub fn addrequestcommon<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    if !s.yes(4) {
        s.act(5);
    }
    Words::default()
}

pub fn recordjobhttpshistograms<S: Services>(s: &mut S) -> Words {
    let attempted = s.yes(1) && s.yes(2) && !s.yes(3);
    s.call(4, Words::flag(attempted));
    if !attempted {
        let reason = if !s.yes(5) {
            1
        } else if !s.yes(6) {
            2
        } else if !s.yes(7) {
            3
        } else if s.yes(8) {
            4
        } else if s.yes(3) {
            5
        } else if s.yes(9) {
            6
        } else {
            0
        };
        s.call(10, Words::arg(reason));
    }
    Words::default()
}

pub fn recordjobhistograms<S: Services>(s: &mut S) -> Words {
    let error = s.get(1).x;
    s.act(2);
    let ordinary = s.yes(3);
    let category = if error == 0 {
        if ordinary {
            s.act(4);
            0
        } else {
            2
        }
    } else if error == s.get(5).x || error == s.get(6).x {
        if ordinary {
            4
        } else {
            5
        }
    } else if ordinary {
        s.act(7);
        1
    } else {
        3
    };
    s.call(8, Words::arg(category));
    if category == 1 || (!s.yes(9) && category == 4) {
        if s.yes(10) {
            s.act(11);
        } else {
            s.act(12);
        }
    }
    if category == 0 && s.yes(13) {
        let mut secure = false;
        let mut classic = false;
        let mut platform = false;
        let count = s.get(14).a;
        for i in 0..count {
            match s.call(15, Words::arg(i)).a {
                0 => classic = true,
                1 => secure = true,
                2 => platform = true,
                _ => (),
            }
        }
        let task = s.get(16).a;
        if let Some(path) = fallback_path(task, secure, classic, platform) {
            s.call(17, Words::arg(path));
        }
    }
    Words::default()
}

pub fn operator_lt<S: Services>(s: &mut S) -> Words {
    s.act(7);
    for index in 0..4 {
        let a = s.call(1, Words::arg(index));
        let b = s.call(2, Words::arg(index));
        if index == 1 {
            if a.x != b.x {
                return Words::flag(a.x < b.x);
            }
        } else if a.a != b.a {
            return Words::flag(a.a < b.a);
        }
    }
    for index in 0..3 {
        if s.call(3, Words::arg(index)).a != 0 {
            return Words::flag(true);
        }
        if s.call(4, Words::arg(index)).a != 0 {
            return Words::flag(false);
        }
    }
    let a = s.get(5).x;
    let b = s.get(6).x;
    Words::flag(a < b)
}

pub fn operator_eq<S: Services>(s: &mut S) -> Words {
    if s.yes(1) {
        return Words::flag(false);
    }
    Words::flag(!s.yes(2))
}

pub fn tocachekey<S: Services>(s: &mut S) -> Words {
    let count = s.get(1).a;
    if count != 1 {
        s.act(2);
    }
    let kind = if count == 1 { s.get(3).a } else { 0 };
    if s.yes(4) {
        s.act(5);
    } else {
        s.act(6);
    }
    s.call(7, Words::arg(kind));
    Words::default()
}

pub fn destroy_job<S: Services>(s: &mut S) -> Words {
    let queued = s.yes(1);
    let running = s.yes(2);
    s.act(3);
    if running {
        s.act(4);
    } else if queued {
        s.act(5);
    }
    while s.yes(6) {
        s.act(7);
        s.act(8);
        s.act(9);
        s.act(10);
    }
    while s.yes(11) {
        s.act(12);
        s.act(13);
        s.act(14);
    }
    Words::default()
}

pub fn add<S: Services>(s: &mut S) -> Words {
    let total = s.get(1).a;
    s.call(2, Words::arg(total.wrapping_add(1)));
    let count = s.get(3).a;
    s.call(4, Words::arg(count.wrapping_add(1)));
    let current = s.get(5).a;
    let requested = s.get(6).a;
    if current < requested {
        s.call(7, Words::arg(requested));
    }
    Words::default()
}

pub fn remove<S: Services>(s: &mut S) -> Words {
    s.act(1);
    let total = s.get(2).a;
    s.call(3, Words::arg(total.wrapping_sub(1)));
    let count = s.get(4).a;
    s.call(5, Words::arg(count.wrapping_sub(1)));
    let mut highest = s.get(6).a;
    let minimum = s.get(7).a;
    while highest > minimum && s.call(8, Words::arg(highest)).a == 0 {
        highest -= 1;
    }
    s.call(9, Words::arg(highest));
    if s.get(2).a == 0 {
        s.act(10);
    }
    Words::default()
}

pub fn changerequestpriority<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    s.act(5);
    Words::default()
}

pub fn changeserviceendpointrequestpriority<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    Words::default()
}

pub fn abort<S: Services>(s: &mut S) -> Words {
    s.act(1);
    Words::default()
}

pub fn onevicted<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    Words::default()
}

pub fn cancelrequestcommon<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    Words::default()
}

pub fn startnextdnstransaction<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    Words::default()
}

pub fn addtransactiontimequeued<S: Services>(s: &mut S) -> Words {
    s.act(1);
    Words::default()
}

pub fn onmdnsimmediatefailure<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    Words::default()
}

pub fn startnat64task<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    Words::default()
}

pub fn onnat64taskcomplete<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    s.act(4);
    Words::default()
}

pub fn completerequestswitherror<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    Words::default()
}

pub fn job<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    Words::default()
}

pub fn netlogjobcreationparams<S: Services>(s: &mut S) -> Words {
    s.act(1);
    s.act(2);
    s.act(3);
    while s.yes(4) {
        s.act(5);
        s.act(6);
    }
    s.act(7);
    let count = s.get(8).a;
    for i in 0..count {
        s.call(9, Words::arg(i));
    }
    s.act(10);
    s.act(11);
    s.act(12);
    Words::default()
}

pub fn containsicannnamecollisionip<S: Services>(s: &mut S) -> Words {
    let count = s.get(1).a;
    for i in 0..count {
        if s.call(2, Words::arg(i)).a != 0 {
            let word = s.call(3, Words::arg(i)).a;
            if word == 0x3535007f {
                return Words::flag(true);
            }
        }
    }
    Words::flag(false)
}

pub fn isattemptmodesecure<S: Services>(s: &mut S) -> Words {
    Words::flag(s.get(1).a == 1)
}

pub fn run<S: Services>(method: u32, s: &mut S) -> Words {
    match method {
        1 => calculateresolvepath(s),
        2 => attemptmodetotasktype(s),
        3 => gettargetnetwork(s),
        4 => getechmode(s),
        5 => schedule(s),
        6 => cancelrequest(s),
        7 => cancelserviceendpointrequest(s),
        8 => addrequest(s),
        9 => addserviceendpointrequest(s),
        10 => abortinsecurednstask(s),
        11 => runnexttask(s),
        12 => finish(s),
        13 => killdnstask(s),
        14 => reducebyonejobslot(s),
        15 => updatepriority(s),
        16 => start(s),
        17 => insecurecachelookup(s),
        18 => startdnstask(s),
        19 => onintermediatetransactionscomplete(s),
        20 => shouldsorttransactionsindividually(s),
        21 => maybecacheresult(s),
        22 => cancelrequests(s),
        23 => completerequestswithoutcache(s),
        24 => completerequests(s),
        25 => onsystemtaskcomplete(s),
        26 => ondnstaskfailure(s),
        27 => ondnstaskcomplete(s),
        28 => startsystemtask(s),
        29 => startmdnstask(s),
        30 => onmdnstaskcomplete(s),
        31 => onserviceendpointsupdated(s),
        32 => servefromhosts(s),
        33 => addrequestcommon(s),
        34 => recordjobhttpshistograms(s),
        35 => recordjobhistograms(s),
        36 => operator_lt(s),
        37 => operator_eq(s),
        38 => tocachekey(s),
        39 => destroy_job(s),
        40 => add(s),
        41 => remove(s),
        42 => changerequestpriority(s),
        43 => changeserviceendpointrequestpriority(s),
        44 => abort(s),
        45 => onevicted(s),
        46 => cancelrequestcommon(s),
        47 => startnextdnstransaction(s),
        48 => addtransactiontimequeued(s),
        49 => onmdnsimmediatefailure(s),
        50 => startnat64task(s),
        51 => onnat64taskcomplete(s),
        52 => completerequestswitherror(s),
        53 => job(s),
        54 => netlogjobcreationparams(s),
        55 => containsicannnamecollisionip(s),
        56 => isattemptmodesecure(s),
        _ => Words::default(),
    }
}
