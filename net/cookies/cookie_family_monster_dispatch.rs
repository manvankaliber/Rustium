// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
//! Call-scoped store ABI adapter. Native callbacks never borrow a Rust store.
#![deny(unsafe_op_in_unsafe_fn)]
use crate::{
    base, canonical as c, canonical_dispatch as cd,
    ffi::{Native, Output, Request},
    monster as m, status,
    types::Url,
    util,
};
use m::{Op, Runtime};

struct Adapter<'a> {
    native: &'a mut Native,
}
struct CurrentUrl {
    scheme: Vec<u8>,
    host: Vec<u8>,
    path: Vec<u8>,
    flags: i64,
    port: i32,
    identity: usize,
}
impl CurrentUrl {
    fn view(&self) -> Url<'_> {
        Url {
            identity: self.identity,
            scheme: &self.scheme,
            host: &self.host,
            path: &self.path,
            standard: self.flags & 1 != 0,
            is_ip: self.flags & 2 != 0,
            cryptographic: self.flags & 4 != 0,
            localhost: self.flags & 8 != 0,
            host_compliant: self.flags & 16 != 0,
            effective_port: self.port,
        }
    }
}
fn current_url(native: &mut Native) -> CurrentUrl {
    let facts = native.run(Op::UrlFacts as u32, 0, 0, &[], &[], util::Calendar::default());
    // SAFETY: each initialized native URL view is copied before the next call.
    let scheme = unsafe { facts.bytes.copy() };
    let host = unsafe { facts.bytes2.copy() };
    let path = native.run(Op::UrlFacts as u32, 1, 0, &[], &[], util::Calendar::default());
    let path = unsafe { path.bytes.copy() };
    CurrentUrl {
        scheme,
        host,
        path,
        flags: facts.number,
        port: facts.number2 as i32,
        identity: facts.number3 as usize,
    }
}
impl base::SchemeProvider for Adapter<'_> {
    fn scheme_count(&mut self) -> usize {
        self.n(Op::Count, 30, 0) as usize
    }
    fn scheme(&mut self, index: usize) -> Vec<u8> {
        self.bytes(Op::AllowedScheme, index as i64, 0)
    }
}
impl Runtime for Adapter<'_> {
    fn failed(&self) -> bool {
        self.native.failed.get()
    }
    fn n(&mut self, op: Op, a: i64, b: i64) -> i64 {
        if self.failed() {
            return if matches!(op,Op::End|Op::QueueEmpty) { 1 } else { 0 };
        }
        if op == Op::Feature && a == 1 {
            return i64::from(
                self.native.number(Op::Feature as u32, 0, 0) != 0
                    || self.native.number(Op::Feature as u32, 3, 0) != 0,
            );
        }
        if op == Op::PartitionEqual {
            let right = self.native.number(Op::InputPartition as u32, 0, 1);
            return i64::from(<Native as c::Native>::partition_equal(
                self.native,
                c::Partition { identity: a as u64, nonce: false },
                c::Partition { identity: right as u64, nonce: false },
            ));
        }
        self.native.number(op as u32, a, b)
    }
    fn bytes(&mut self, op: Op, a: i64, b: i64) -> Vec<u8> {
        let result = self.native.run(op as u32, a, b, &[], &[], util::Calendar::default());
        // SAFETY: initialized native byte view, copied before another callback.
        unsafe { result.bytes.copy() }
    }
    fn write(&mut self, op: Op, a: i64, b: i64, bytes: &[u8]) {
        self.native.run(op as u32, a, b, bytes, &[], util::Calendar::default());
    }
    fn cookie(&mut self, id: i64, with_value: bool) -> c::Cookie {
        cd::read_cookie(self.native, id, with_value)
    }
    fn access(&mut self, id: i64) -> base::AccessResult {
        let out = self.native.run(Op::Access as u32, id, 0, &[], &[], util::Calendar::default());
        base::AccessResult {
            status: status::State {
                exclusions: out.number as u64,
                warnings: out.number2 as u64,
                exemption: out.number4 as i32,
                invalid: 0,
            },
            effective_same_site: out.number3 as i32,
            access_semantics: out.params.access_semantics,
            scope_semantics: out.params.scope_semantics,
            allowed_secure: out.params.delegate_trustworthy,
        }
    }
    fn access_out(&mut self, id: i64, value: base::AccessResult) {
        // Encode individual initialized scalar bytes; never expose repr(C)
        // padding, Rust object references or uninitialized output storage.
        let mut bytes = Vec::with_capacity(33);
        bytes.extend(value.status.exclusions.to_ne_bytes());
        bytes.extend(value.status.warnings.to_ne_bytes());
        bytes.extend(value.status.exemption.to_ne_bytes());
        bytes.extend(value.effective_same_site.to_ne_bytes());
        bytes.extend(value.access_semantics.to_ne_bytes());
        bytes.extend(value.scope_semantics.to_ne_bytes());
        bytes.push(u8::from(value.allowed_secure));
        self.write(Op::WriteAccess, id, 0, &bytes);
    }
    fn include(&mut self, id: i64) -> base::AccessResult {
        self.n(Op::Select, id, 0);
        let access =
            if self.n(Op::Delegate, -1, 0) != 0 { self.n(Op::Delegate, 1, id) } else { -1 };
        let scope = if self.n(Op::Delegate, -1, 0) != 0 { self.n(Op::Delegate, 2, id) } else { -1 };
        let trust = self.n(Op::Argument, 60, 0) != 0;
        self.write(Op::PrepareBase, access, scope, &[u8::from(trust)]);
        let url = current_url(self.native);
        let (out, context) = base::include(url.view(), self.native);
        self.access_out(0, out);
        if !self.failed() {
            self.n(Op::PostInclude, id, i64::from(context));
        }
        // A native virtual hook can legally mutate an originally nonconst
        // result through another alias. Return the post-hook native value.
        self.access(0)
    }
    fn set_permitted(&mut self, id: i64) -> base::AccessResult {
        self.n(Op::Select, id, 0);
        let access =
            if self.n(Op::Delegate, -1, 0) != 0 { self.n(Op::Delegate, 1, id) } else { -1 };
        let scope = if self.n(Op::Delegate, -1, 0) != 0 { self.n(Op::Delegate, 2, id) } else { -1 };
        let trust = self.n(Op::Argument, 60, 0) != 0;
        self.write(Op::PrepareBase, access, scope, &[u8::from(trust)]);
        let prior = if self.n(Op::Argument, 50, 0) != 0 { Some(self.access(1)) } else { None };
        let url = current_url(self.native);
        let allowed = base::cookieable_scheme(url.view().scheme, self);
        let out = base::set_permitted(url.view(), allowed, prior, self.native);
        self.access_out(0, out);
        if !self.failed() {
            self.n(Op::PostSet, id, 0);
        }
        self.access(0)
    }
    fn equivalent(&mut self, a: i64, b: i64, kind: i32) -> bool {
        let a = self.cookie(a, false);
        let b = self.cookie(b, false);
        let f = cd::features(self.native);
        c::key_equal(&a, &b, kind as u32, f, self.native)
    }
    fn key_compare(&mut self, a: i64, b: i64, kind: i32) -> i32 {
        let a = self.cookie(a, false);
        let b = self.cookie(b, false);
        let f = cd::features(self.native);
        c::key_compare(&a, &b, kind as u32, f, self.native)
    }
    fn secure_matching(&mut self, a: i64, b: i64) -> bool {
        let a = self.cookie(a, false);
        let b = self.cookie(b, false);
        c::secure_matching(&a, &b, self.native)
    }
    fn web_equal(&mut self, a: i64, b: i64) -> bool {
        let left = self.cookie(a, false);
        let right = self.cookie(b, false);
        let f = cd::features(self.native);
        let equivalent = c::key_equal(&left, &right, 0, f, self.native);
        let secure = (
            base::is_secure(left.secure, f.scheme_bound, left.source_scheme),
            base::is_secure(right.secure, f.scheme_bound, right.source_scheme),
        );
        c::web_equal(&left, &right, equivalent, secure, (a as u64, b as u64), self.native)
    }
    fn value_equal(&mut self, a: i64, b: i64) -> bool {
        <Native as c::Native>::value_equal(self.native, a as u64, b as u64)
    }
    fn effectively_none(&mut self, id: i64, semantics: i32) -> bool {
        self.n(Op::Select, id, 0);
        base::effective_same_site(semantics, self.native) == 0
    }
    fn scope_for_key(&mut self) -> i32 {
        if self.n(Op::Delegate, -1, 0) != 0 {
            self.n(Op::Delegate, 2, -2) as i32
        } else {
            -1
        }
    }
    fn get_key(&mut self, domain: &[u8]) -> Vec<u8> {
        let registry = util::DomainProvider::registry(self.native, domain);
        util::cookie_domain_as_host(if registry.is_empty() { domain } else { &registry }).to_vec()
    }
}
fn args(s: &mut impl Runtime, index: i64) -> i64 {
    s.n(Op::Argument, index, 0)
}
fn ids(s: &mut impl Runtime, selector: i64) -> Vec<i64> {
    let mut out = Vec::new();
    let cursor = s.n(Op::Begin, selector, 0);
    while !s.failed() && s.n(Op::End, cursor, 0) == 0 {
        out.push(s.n(Op::Current, cursor, 0));
        s.n(Op::Advance, cursor, 0);
    }
    out
}
pub(crate) fn dispatch(
    operation: u32,
    request: &Request,
    _url: Url<'_>,
    native: &mut Native,
) -> Output {
    let mut output = Output::default();
    let mut s = Adapter { native };
    let a = request.scalar;
    match operation {
        2000 => m::flush(&mut s),
        2001 => {
            if s.n(Op::State, 4, 0) != 0 {
                s.n(Op::KeepSession, 0, 0);
            }
        }
        2002 => {
            if s.n(Op::State, 0, 0) == 0 {
                s.n(Op::SetSchemes, 0, 0);
                s.n(Op::RunCallback, 5, 1);
            } else {
                s.n(Op::RunCallback, 5, 0);
            }
        }
        2003 => {
            let initialized = s.n(Op::State, 0, 0) != 0;
            s.n(Op::Check, 9, i64::from(!initialized));
            s.n(Op::SetPersist, a[0], 0);
        }
        2004 => {
            for scheme in [b"http".as_slice(), b"https", b"ws", b"wss"] {
                s.write(Op::DefaultSchemes, 0, 0, scheme);
            }
        }
        2005 => {
            s.n(Op::Check, 0, 1);
            s.n(Op::Log, 11, 0);
        }
        2006 => {
            let left = s.cookie(a[0], false);
            let right = s.cookie(a[1], false);
            output.scalar = i64::from(m::cookie_sort(&left, &right).is_lt());
        }
        2007 => m::get_all(&mut s),
        2008 => {
            for id in ids(&mut s, 3) {
                let semantics =
                    if s.n(Op::Delegate, -1, 0) != 0 { s.n(Op::Delegate, 1, id) } else { -1 };
                s.n(Op::OutputScalar, 1, semantics);
            }
            s.n(Op::RunCallback, 6, 0);
        }
        2009 => m::get_list(&mut s),
        2010 => {
            output.scalar = m::delete_matching(&mut s, true) as i64;
        }
        2011 => {
            let trust = if args(&mut s, 43) != 0 && s.n(Op::Delegate, -1, 0) != 0 {
                s.n(Op::Delegate, 0, 1) != 0
            } else {
                false
            };
            let access = if s.n(Op::Delegate, -1, 0) != 0 { s.n(Op::Delegate, 1, 0) } else { -1 };
            let scope = if s.n(Op::Delegate, -1, 0) != 0 { s.n(Op::Delegate, 2, 0) } else { -1 };
            s.write(Op::PrepareBase, access, scope, &[u8::from(trust)]);
            output.scalar = s.n(Op::MatchDeletion, 0, 0);
        }
        2012 => {
            output.scalar = m::delete_canonical(&mut s) as i64;
        }
        2013 => {
            output.scalar = m::delete_matching(&mut s, false) as i64;
        }
        2014 => {
            s.n(Op::SetState, 0, 1);
        }
        2015 => m::fetch_if_needed(&mut s),
        2016 => {
            let store = s.n(Op::State, 4, 0) != 0;
            let finished = s.n(Op::State, 2, 0) != 0;
            s.n(Op::Check, 10, i64::from(store && !finished));
            s.n(Op::LoadAll, 0, 0);
        }
        2017 => m::loaded(&mut s),
        2018 => m::key_loaded(&mut s),
        2019 => m::store_loaded(&mut s),
        2020 => m::invoke_queue(&mut s),
        2021 => m::ensure_valid(&mut s),
        2022 => {
            let cursor = s.n(Op::Begin, 2, 0);
            m::trim_duplicates(&mut s, cursor);
        }
        2023 => m::find(&mut s, false),
        2024 => m::find(&mut s, true),
        2025 => m::filter(&mut s),
        2026 => {
            let mut access = s.access(0);
            output.scalar = i64::from(m::maybe_overwrite(
                &mut s,
                a[0],
                a[1] != 0,
                a[2] != 0,
                a[3] != 0,
                &mut access,
            ));
            s.access_out(0, access);
        }
        2027 => output.scalar = i64::from(m::insertion_cause(a[0] as i32)),
        2028 => output.scalar = m::insert_cookie(&mut s, a[0], a[1] != 0, a[2] != 0, a[3] as i32),
        2029 => {
            let cookie = s.cookie(a[0], false);
            output.scalar = i64::from(m::should_persist(&cookie, &mut s));
        }
        2030 => m::set_cookie(&mut s),
        2031 => m::set_all(&mut s),
        2032 => m::set_unsafe(&mut s),
        2033 => m::update_access(&mut s, a[0], a[1]),
        2034 | 2035 => m::delete_cookie(&mut s, a[0], a[1] != 0, a[2] as i32),
        2036 => {
            let key = s.bytes(Op::Key, 0, 0);
            output.scalar = m::garbage_collect(&mut s, a[0], &key) as i64;
        }
        2037 => {
            let key = s.bytes(Op::Key, 0, 0);
            output.scalar = m::garbage_collect_partition(&mut s, a[0], &key) as i64;
        }
        2038 | 2039 => {
            let mut cookies = ids(&mut s, 8);
            let protect = if operation == 2039 { a[3] == 0 } else { a[3] != 0 };
            output.scalar = m::purge(
                &mut s,
                &mut cookies,
                a[0] as i32,
                a[1] as usize,
                a[2] as usize,
                protect,
                operation == 2039,
            ) as i64;
            s.n(Op::OutputList, 5, 0);
            for id in cookies {
                s.n(Op::OutputCookie, id, 6);
            }
        }
        2040 | 2041 => {
            let cursor = s.n(Op::Begin, 2, 0);
            output.scalar = m::expired_range(&mut s, cursor, a[0], a[1] != 0) as i64;
        }
        2042 => m::all_expired_partitioned(&mut s, a[0]),
        2043 => {
            let cookies = ids(&mut s, 8);
            output.scalar = cookies.len() as i64;
            for id in cookies {
                m::delete_cookie(&mut s, id, true, a[0] as i32);
            }
        }
        2044 => {
            let mut cookies = ids(&mut s, 8);
            let (count, earliest) = m::purge_lra(&mut s, &mut cookies, a[0], a[1] as usize);
            output.scalar = count as i64;
            s.n(Op::OutputScalar, 2, earliest);
        }
        2045 => {
            let domain = s.bytes(Op::Key, 0, 0);
            let key = s.get_key(&domain);
            s.write(Op::EmitKey, 0, 0, &key);
        }
        2046 => output.scalar = i64::from(m::has_scheme(&mut s)),
        2047 | 2048 => {
            output.scalar = if s.n(Op::Delegate, -1, 0) != 0 {
                s.n(Op::Delegate, if operation == 2047 { 1 } else { 2 }, a[0])
            } else {
                -1
            }
        }
        2049 => output.scalar = i64::from(m::activate_legacy(&mut s)),
        2050 => m::delete_aliases(&mut s),
        2051 => {
            let cursor = s.n(Op::Begin, 2, 0);
            while !s.failed() && s.n(Op::End, cursor, 0) == 0 {
                let id = s.n(Op::Current, cursor, 0);
                if s.n(Op::Argument, 70, id) == 0
                    || s.cookie(id, false).creation > s.n(Op::Argument, 71, id)
                {
                    s.n(Op::OutputCookie, id, 7);
                }
                s.n(Op::Advance, cursor, 0);
            }
        }
        2052 => m::record_stats(&mut s, a[0]),
        2053 => output.scalar = i64::from(m::stats(&mut s)),
        2054 => m::global_callback(&mut s),
        2055 => {
            let load = s.n(Op::State, 2, 0) == 0;
            s.n(Op::Metric, 40, i64::from(load));
            if load {
                s.n(Op::PostPreconnect, 0, 0);
            }
        }
        2056 | 2057 => m::key_callback(&mut s),
        2058 => {
            output.scalar = if a[0] == -1 {
                0
            } else if a[0] == -2 {
                1
            } else {
                let destination = args(&mut s, 65);
                if a[0] == destination {
                    4
                } else {
                    let default = args(&mut s, 67);
                    s.n(Op::Check, 11, i64::from(a[2] != 0));
                    i64::from(m::same_port(
                        a[0] as i32,
                        destination as i32,
                        a[2] as i32,
                        default as i32,
                    ))
                }
            };
        }
        2059 => {
            output.scalar = match m::site_in_other_partition(&mut s) {
                None => -1,
                Some(value) => i64::from(value),
            }
        }
        2060 => output.scalar = i64::from(a[0] != 0 && a[1] != 0),
        2061 => output.scalar = i64::from(a[0] == 0),
        2062 | 2069 => {
            let selector = if operation == 2062 { 2 } else { 8 };
            let mut total = 0;
            for id in ids(&mut s, selector) {
                total += s.cookie(id, false).size();
            }
            output.scalar = total as i64;
        }
        2063 => {
            output.scalar =
                i64::from(m::stored_cookie_type(a[0] != 0, a[1] != 0, a[2] != 0, a[3] != 0))
        }
        2064 => {
            let mut count = 0;
            for id in ids(&mut s, 8) {
                let cookie = s.cookie(id, false);
                if cookie.priority == a[0] as i32 && (a[1] == 0 || cookie.secure) {
                    count += 1;
                }
            }
            output.scalar = count;
        }
        2065 => {
            let cookie = s.cookie(a[0], false);
            output.scalar =
                i64::from(cookie.priority == a[1] as i32 && (a[2] == 0 || !cookie.secure));
        }
        2070 => output.scalar = m::DEFAULT_ACCESS_UPDATE_MICROS,
        2098 => output.scalar = i64::from(if a[0] == a[1] { a[2] < a[3] } else { a[0] > a[1] }),
        2099 => output.scalar = i64::from(if a[0] == a[1] { a[2] < a[3] } else { a[0] < a[1] }),
        _ => {
            s.native.number(399, i64::from(operation), 0);
        }
    }
    if s.failed() {
        Output::default()
    } else {
        output
    }
}
