// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

use std::collections::{BTreeMap, BTreeSet};

const RESTRICTED: &[i32] = &[
    0, 1, 7, 9, 11, 13, 15, 17, 19, 20, 21, 22, 23, 25, 37, 42, 43, 53, 69, 77, 79, 87, 95, 101,
    102, 103, 104, 109, 110, 111, 113, 115, 117, 119, 123, 135, 137, 139, 143, 161, 179, 389, 427,
    465, 512, 513, 514, 515, 526, 530, 531, 532, 540, 548, 554, 556, 563, 587, 601, 636, 989, 990,
    993, 995, 1719, 1720, 1723, 2049, 3659, 4045, 5060, 5061, 6000, 6566, 6665, 6666, 6667, 6668,
    6669, 6697, 10080,
];

/// Values tell the adapter whether to emit the corresponding UMA histogram.
#[derive(Debug, PartialEq, Eq)]
pub enum Decision {
    Allow,
    Block,
    AllowAndRecord,
    BlockAndRecord,
}

/// Owns policy state. The adapter must provide the same external sequencing as
/// port_util's original unsynchronized globals; a mutex is not a replacement
/// for the existing caller-side ordering requirements.
#[derive(Default)]
pub struct PortPolicy {
    explicit: BTreeMap<i32, usize>,
    scoped_allowable: i32,
    abuse: Option<(BTreeSet<i32>, BTreeSet<i32>)>,
    localhost: Option<BTreeSet<i32>>,
    reload_localhost: bool,
}

fn whitespace(c: u8) -> bool {
    matches!(c, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c)
}

/// Returns invalid trimmed nonempty tokens for the adapter's DLOG, in order.
pub fn parse_feature(input: &[u8]) -> (BTreeSet<i32>, Vec<&[u8]>) {
    let mut values = BTreeSet::new();
    let mut invalid = Vec::new();
    for raw in input.split(|&c| c == b',') {
        let start = raw.iter().position(|&c| !whitespace(c)).unwrap_or(raw.len());
        let end = raw.iter().rposition(|&c| !whitespace(c)).map_or(start, |n| n + 1);
        let token = &raw[start..end];
        if token.is_empty() {
            continue;
        }
        let valid_lead = token[0].is_ascii_digit() && !(token.len() > 1 && token[0] == b'0');
        let parsed = if valid_lead {
            token.iter().try_fold(0_i32, |n, c| {
                if !c.is_ascii_digit() {
                    return None;
                }
                n.checked_mul(10)?.checked_add(i32::from(*c - b'0'))
            })
        } else {
            None
        };
        if let Some(port) = parsed {
            values.insert(port);
        } else {
            invalid.push(token);
        }
    }
    (values, invalid)
}

impl PortPolicy {
    pub fn valid(port: i32) -> bool {
        (0..=65535).contains(&port)
    }
    pub fn well_known(port: i32) -> bool {
        (0..1024).contains(&port)
    }
    pub fn count(&self) -> usize {
        self.explicit.values().sum()
    }
    pub fn reset(&mut self, ports: &[u16]) {
        self.explicit.clear();
        for &port in ports {
            self.insert(i32::from(port));
        }
    }
    pub fn insert(&mut self, port: i32) {
        *self.explicit.entry(port).or_default() += 1;
    }
    /// False maps to NOTREACHED in the C++ destructor.
    pub fn erase(&mut self, port: i32) -> bool {
        if let Some(count) = self.explicit.get_mut(&port) {
            *count -= 1;
            if *count == 0 {
                self.explicit.remove(&port);
            }
            true
        } else {
            false
        }
    }
    pub fn allowable(&self, port: i32) -> bool {
        self.scoped_allowable == port
    }
    /// Return previous value for the adapter's DCHECK_EQ(previous, 0).
    pub fn scoped_allowable(&self) -> i32 {
        self.scoped_allowable
    }
    pub fn set_scoped_allowable(&mut self, port: i32) -> i32 {
        let previous = self.scoped_allowable;
        self.scoped_allowable = port;
        previous
    }
    pub fn reload_localhost(&mut self) {
        self.reload_localhost = true;
    }
    /// `load` is called once, only after valid, explicit and built-in checks,
    /// and only while the feature is enabled. It must discover restrict then monitor.
    pub fn scheme(
        &mut self,
        port: i32,
        enabled: bool,
        load: impl FnOnce() -> (BTreeSet<i32>, BTreeSet<i32>),
    ) -> Decision {
        if !Self::valid(port) {
            return Decision::Block;
        }
        if self.explicit.contains_key(&port) {
            return Decision::Allow;
        }
        if RESTRICTED.contains(&port) {
            return Decision::Block;
        }
        if enabled {
            let (restricted, monitored) = self.abuse.get_or_insert_with(load);
            if restricted.contains(&port) {
                return Decision::BlockAndRecord;
            }
            if monitored.contains(&port) {
                return Decision::AllowAndRecord;
            }
        }
        Decision::Allow
    }
    /// `local` is supplied by the IPAddress metadata adapter (including mapped
    /// IPv4 and unspecified addresses). Loading happens after all early exits.
    pub fn endpoint(
        &mut self,
        port: i32,
        enabled: bool,
        local: bool,
        mut load: impl FnMut() -> BTreeSet<i32>,
    ) -> Decision {
        if !enabled || !local || self.explicit.contains_key(&port) {
            return Decision::Allow;
        }
        if self.localhost.is_none() {
            self.localhost = Some(load());
        }
        if self.reload_localhost {
            self.localhost = Some(load());
            self.reload_localhost = false;
        }
        if self.localhost.as_ref().is_some_and(|ports| ports.contains(&port)) {
            Decision::BlockAndRecord
        } else {
            Decision::Allow
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn set(s: &[i32]) -> BTreeSet<i32> {
        s.iter().copied().collect()
    }
    #[test]
    fn tokens_and_boundaries() {
        let (v, bad) = parse_feature(b" 0,01, 123 ,+1,-1,2147483647,2147483648,  ,1\0,\t42\n");
        assert_eq!(v, set(&[0, 42, 123, i32::MAX]));
        assert_eq!(bad, vec![b"01".as_slice(), b"+1", b"-1", b"2147483648", b"1\0"]);
        assert!(!PortPolicy::valid(-1));
        assert!(PortPolicy::valid(65535));
        assert!(!PortPolicy::valid(65536));
        assert!(PortPolicy::well_known(1023));
        assert!(!PortPolicy::well_known(1024));
    }
    #[test]
    fn duplicates_reset_and_scoped() {
        let mut p = PortPolicy::default();
        p.reset(&[1, 1, 2]);
        p.insert(1);
        assert_eq!(p.count(), 4);
        assert!(p.erase(1));
        assert_eq!(p.count(), 3);
        p.reset(&[2]);
        assert!(!p.erase(1));
        assert_eq!(p.count(), 1);
        assert!(p.allowable(0));
        assert_eq!(p.set_scoped_allowable(23), 0);
        assert_eq!(p.set_scoped_allowable(24), 23);
        assert!(p.allowable(24));
        p.set_scoped_allowable(0);
        assert!(p.allowable(0));
    }
    #[test]
    fn precedence_and_lazy_reload() {
        let mut p = PortPolicy::default();
        assert_eq!(p.scheme(22, true, || panic!("early")), Decision::Block);
        p.insert(22);
        assert_eq!(p.scheme(22, true, || panic!("early")), Decision::Allow);
        assert_eq!(p.scheme(444, true, || (set(&[444]), set(&[444]))), Decision::BlockAndRecord);
        assert_eq!(p.scheme(445, true, || panic!("cached")), Decision::Allow);
        p.reload_localhost();
        p.insert(444);
        assert_eq!(p.endpoint(444, true, true, || panic!("explicit")), Decision::Allow);
        p.erase(444);
        let mut loads = 0;
        assert_eq!(
            p.endpoint(444, true, true, || {
                loads += 1;
                set(&[444])
            }),
            Decision::BlockAndRecord
        );
        assert_eq!(loads, 2);
        assert_eq!(p.endpoint(444, true, true, || panic!("cached")), Decision::BlockAndRecord);
        p.reload_localhost();
        assert_eq!(p.endpoint(444, true, false, || panic!("nonlocal")), Decision::Allow);
        assert_eq!(p.endpoint(444, true, true, BTreeSet::new), Decision::Allow);
    }
}
