// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
use crate::{Services, Words};
use crate::client_bridge::boolean;
use crate::dns_request_family_ids::ClientEvent as Client;
pub const SYSTEM: u64 = 0;
pub const DNS: u64 = 1;
pub const SECURE_DNS: u64 = 2;
pub const MDNS: u64 = 3;
pub const CACHE: u64 = 4;
pub const INSECURE_CACHE: u64 = 5;
pub const SECURE_CACHE: u64 = 6;
pub const CONFIG: u64 = 7;
pub const NAT64: u64 = 8;
pub const HOSTS: u64 = 9;
pub const PLATFORM: u64 = 10;
pub const OFF: u64 = 0;
pub const AUTOMATIC: u64 = 1;
pub const SECURE: u64 = 2;
pub fn local(task: u64) -> bool {
    matches!(task, CACHE|INSECURE_CACHE|SECURE_CACHE|CONFIG|HOSTS)
}
pub fn dispatched(task: u64) -> bool {
    matches!(task, SYSTEM|DNS|PLATFORM|MDNS)
}
pub fn attempt_task(attempt: u64) -> u64 {
    // The native adapter maps the provider enum explicitly to these family IDs.
    match attempt {
        0 => DNS,
        1 => SECURE_DNS,
        2 => PLATFORM,
        _ => u64::MAX,
    }
}
pub fn fallback_path(task: u64, secure: bool, classic: bool, platform: bool) -> Option<u64> {
    match task {
        SECURE_DNS => Some(0),
        DNS => Some(if secure { 1 } else { 6 }),
        PLATFORM => Some(if secure { 2 } else { 7 }),
        SYSTEM => Some(if secure {
            if classic {
                4
            } else if platform {
                5
            } else {
                3
            }
        } else if classic {
            8
        } else if platform {
            9
        } else {
            10
        }),
        _ => None,
    }
}
pub fn cacheable(present: bool, error: i64, positive_ttl: bool) -> bool {
    present && (error == 0 || positive_ttl)
}
pub fn address_family(a: bool, aaaa: bool) -> u64 {
    if a && aaaa {
        0
    } else if aaaa {
        2
    } else {
        1
    }
}
pub fn address_queries(value: Words) -> bool {
    value.a != 0 || value.b != 0
}
pub fn nat64_literal(default_no_v6: bool, ipv4: bool, loopback: bool, source: i64) -> bool {
    !default_no_v6 && ipv4 && !loopback && source != 4
}
pub struct DnsTasks {
    pub allowed: bool,
    pub fallback: bool,
    pub system: bool,
    pub secure: u64,
    pub insecure: u64,
    pub cache: bool,
    pub local_first: bool,
}
/// Native ops 1/2 prepend/append a task, 3 pops, 4 reads front, 5/6
/// are live DNS-provider capability/preference facts, 7 asserts the selected
/// front, 8 reads size, 9 reads a task, 10 reports an invalid persisted mode.
pub fn push_dns_tasks<S: Services>(s: &mut S, p: DnsTasks) {
    let insecure_allowed = p.insecure != 0;
    let insecure_task = if matches!(p.insecure, 2|3) { PLATFORM } else { DNS };
    match p.secure {
        SECURE => {
            if p.cache {
                s.call(7, Words::arg(SECURE_CACHE));
            }
            if p.allowed && boolean(s, Client::Secure, Words::default(), 5) {
                s.call(2, Words::arg(SECURE_DNS));
            }
        }
        AUTOMATIC => {
            if p.cache {
                s.call(7, Words::arg(CACHE));
            }
            if boolean(s, Client::SecureFallback, Words::default(), 6) {
                if p.allowed && insecure_allowed {
                    s.call(2, Words::arg(insecure_task));
                }
            } else if p.local_first {
                if p.allowed {
                    s.call(2, Words::arg(SECURE_DNS));
                    if insecure_allowed {
                        s.call(2, Words::arg(insecure_task));
                    }
                }
            } else {
                if p.cache {
                    s.act(3);
                    s.call(2, Words::arg(SECURE_CACHE));
                }
                if p.allowed {
                    s.call(2, Words::arg(SECURE_DNS));
                }
                if p.cache {
                    s.call(2, Words::arg(INSECURE_CACHE));
                }
                if p.allowed && insecure_allowed {
                    s.call(2, Words::arg(insecure_task));
                }
            }
        }
        OFF => {
            if p.cache {
                let front = s.get(4).a;
                s.call(7, Words { a: front, b: 1, x: local(front) as i64, y: 0 });
            }
            if p.allowed && insecure_allowed {
                s.call(2, Words::arg(insecure_task));
            }
        }
        _ => s.act(10),
    }
    let mut built_in = false;
    let length = s.get(8).a;
    for index in 0..length {
        let task = s.call(9, Words::arg(index)).a;
        if matches!(task, DNS|SECURE_DNS|PLATFORM) {
            built_in = true;
            break;
        }
    }
    if p.system && p.insecure != 3 && (!built_in || p.fallback) {
        s.call(2, Words::arg(SYSTEM));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn fallback_precedence() {
        assert_eq!(fallback_path(SYSTEM,true,true,true),Some(4));
        assert_eq!(fallback_path(SYSTEM,false,true,true),Some(8));
        assert_eq!(fallback_path(MDNS,true,true,true),None);
    }
    #[test]
    fn local_and_dispatch_domains() {
        for task in 0..=10 {
            assert!(!(local(task) && dispatched(task)));
        }
        assert!(!cacheable(true,-1,false));
        assert!(cacheable(true,-1,true));
    }
}
