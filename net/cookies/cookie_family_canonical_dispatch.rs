// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
//! Initialized family ABI adapter for the complete canonical policy.
#![deny(unsafe_op_in_unsafe_fn)]
use crate::{
    canonical as c,
    ffi::{Buffer, Native, Output, Request},
    partition, status,
    types::Url,
    util,
};

struct PartitionServices<'a>(&'a mut Native);
impl partition::Primitives for PartitionServices<'_> {
    fn run(&mut self, operation: partition::Primitive) -> i32 {
        self.0.number(1142, operation as i64, 0) as i32
    }
}
fn select_partition(native: &mut Native, a: c::Partition, b: c::Partition) -> (i32, i32) {
    native.number(1140, a.identity as i64, b.identity as i64);
    let left = native.number(1141, 0, 0) as i32;
    let right = native.number(1141, 1, 0) as i32;
    (left, right)
}

impl c::Native for Native {
    fn should_sample(&mut self) -> bool {
        self.number(1100, 0, 0) != 0
    }
    fn now(&mut self) -> i64 {
        self.number(1101, 0, 0)
    }
    fn add_time(&mut self, a: i64, b: i64) -> i64 {
        self.number(1102, a, b)
    }
    fn subtract_time(&mut self, a: i64, b: i64) -> i64 {
        self.number(1103, a, b)
    }
    fn seconds(&mut self, a: i64) -> i64 {
        self.number(1104, a, 0)
    }
    fn encode_path(&mut self, a: &[u8]) -> Vec<u8> {
        self.bytes(1105, a)
    }
    fn http_date(&mut self, a: i64) -> Vec<u8> {
        let result = self.run(1106, a, 0, &[], &[], util::Calendar::default());
        // SAFETY: native result view is initialized and live until next call.
        unsafe { result.bytes.copy() }
    }
    fn partition_equal(&mut self, a: c::Partition, b: c::Partition) -> bool {
        let (left, right) = select_partition(self, a, b);
        partition::equals(left, right, &mut PartitionServices(self))
    }
    fn partition_compare(&mut self, a: c::Partition, b: c::Partition) -> i32 {
        let (left, right) = select_partition(self, a, b);
        partition::compare(left, right, &mut PartitionServices(self))
    }
    fn value_equal(&mut self, a: u64, b: u64) -> bool {
        if self.number(1120, a as i64, 17) != self.number(1120, b as i64, 17) {
            return false;
        }
        let left = bytes(self, a as i64, 1);
        let right = bytes(self, b as i64, 1);
        left == right
    }
    fn metric(&mut self, metric: c::Metric) {
        use c::Metric::*;
        let (id, a, b) = match metric {
            NameOrValueHtab(a) => (0, i64::from(a), 0),
            DomainNonAscii(a) => (1, i64::from(a), 0),
            EmptyName(a) => (2, i64::from(a), 0),
            EmptyNameAmbiguous(a) => (3, i64::from(a), 0),
            Prefix(a) => (4, i64::from(a), 0),
            PartitionedValid(a) => (5, i64::from(a), 0),
            SameSiteAttribute(a) => (6, i64::from(a), 0),
            NamelessParse { kind, source_type } => (7, kind as i64, i64::from(source_type)),
            NameNonAscii(a) => (8, i64::from(a), 0),
            ValueNonAscii(a) => (9, i64::from(a), 0),
            DoubleUnderscore(a) => (10, i64::from(a), 0),
            FromStorageLength { call_site, valid } => (11, i64::from(call_site), i64::from(valid)),
            RequestContext(a) => (12, i64::from(a), 0),
            IncludedRequestSameSite(a) => (13, i64::from(a), 0),
            IncludedResponseSameSite(a) => (14, i64::from(a), 0),
            RedirectRead(a) => (15, i64::from(a), 0),
            RedirectWrite(a) => (16, i64::from(a), 0),
            RedirectDowngradeRead(a) => (17, i64::from(a), 0),
            RedirectDowngradeWrite(a) => (18, i64::from(a), 0),
        };
        self.run(1109, id, a, &b.to_ne_bytes(), &[], util::Calendar::default());
    }
    fn debug_invalid_parse(&mut self) {
        self.number(1110, 0, 0);
    }
    fn debug_rejection(&mut self, reason: i32) {
        self.number(1111, i64::from(reason), 0);
    }
    fn check_creation_not_null(&mut self, a: bool) {
        self.number(1112, i64::from(a), 0);
    }
    fn check_test_call_site(&mut self) {
        self.number(1113, 0, 0);
    }
}
fn bytes(native: &mut Native, id: i64, field: i64) -> Vec<u8> {
    let out = native.run(1121, id, field, &[], &[], util::Calendar::default());
    // SAFETY: callback byte views are initialized and copied immediately.
    unsafe { out.bytes.copy() }
}
pub(crate) fn read_cookie(native: &mut Native, id: i64, value: bool) -> c::Cookie {
    let mut number = |field| native.number(1120, id, field);
    let creation = number(4);
    let expiry = number(5);
    let access = number(6);
    let update = number(7);
    let secure = number(8) != 0;
    let http_only = number(9) != 0;
    let same_site = number(10) as i32;
    let priority = number(11) as i32;
    let partition = number(12);
    let nonce = number(13) != 0;
    let source_scheme = number(14) as i32;
    let source_port = number(15) as i32;
    let source_type = number(16) as i32;
    let value_size = number(17) as usize;
    c::Cookie {
        name: bytes(native, id, 0),
        value: if value { bytes(native, id, 1) } else { Vec::new() },
        domain: bytes(native, id, 2),
        path: bytes(native, id, 3),
        creation,
        expiry,
        access,
        update,
        secure,
        http_only,
        same_site,
        priority,
        partition: (partition != 0).then_some(c::Partition { identity: partition as u64, nonce }),
        source_scheme,
        source_port,
        source_type,
        value_size,
    }
}
pub(crate) fn features(native: &mut Native) -> c::Features {
    let mut get = |id| native.number(1122, id, 0) != 0;
    c::Features {
        reject_non_ascii: get(0),
        reject_domain_non_ascii: get(1),
        reject_empty_ambiguous: get(2),
        time_limited_insecure: get(3),
        encrypt_only_important: get(4),
        encrypt_http_only: get(5),
        rigorous_lax_method: get(6),
        short_lax_threshold: get(7),
        port_bound: get(8),
        scheme_bound: get(9),
    }
}
fn emit_cookie(cookie: c::Cookie, native: &mut Native) {
    for (id, bytes) in
        [(0, &cookie.name), (1, &cookie.value), (2, &cookie.domain), (3, &cookie.path)]
    {
        native.run(1130, id, 0, bytes, &[], util::Calendar::default());
    }
    let values = [
        cookie.creation,
        cookie.expiry,
        cookie.access,
        cookie.update,
        i64::from(cookie.secure),
        i64::from(cookie.http_only),
        i64::from(cookie.same_site),
        i64::from(cookie.priority),
        cookie.partition.map_or(0, |p| p.identity as i64),
        i64::from(cookie.partition.is_some_and(|p| p.nonce)),
        i64::from(cookie.source_scheme),
        i64::from(cookie.source_port),
        i64::from(cookie.source_type),
        cookie.value_size as i64,
    ];
    for (index, value) in values.into_iter().enumerate() {
        native.number(1130, index as i64 + 4, value);
    }
}
pub(crate) fn dispatch(op: u32, request: &Request, url: Url<'_>, native: &mut Native) -> Output {
    let mut out = Output::default();
    let values = request.scalar;
    match op {
        1000 => {
            if values[0] == 0 {
                out.access.status = status::apply(status::State::default(), 0, 10, 0);
                return out;
            }
            let f = features(native);
            // SAFETY: request is initialized, input view copied before effects.
            let line = unsafe { request.first.copy() };
            let mut status = status::State::default();
            let partition = native.number(1120, 0, 12);
            let nonce = native.number(1120, 0, 13) != 0;
            let input = c::CreateInput {
                url,
                url_valid: values[0] != 0,
                line: &line,
                creation: values[1],
                server: (values[2] != 0).then_some(values[3]),
                partition: (partition != 0)
                    .then_some(c::Partition { identity: partition as u64, nonce }),
                source_type: values[4] as i32,
                features: f,
            };
            if let Some(cookie) = c::create(input, &mut status, native) {
                emit_cookie(cookie, native);
                out.scalar = 1;
            }
            out.access.status = status;
        }
        1001 => {
            let f = features(native);
            let cookie = read_cookie(native, 0, true);
            let mut status = status::State::default();
            if let Some(cookie) = c::sanitize(
                c::SanitizeInput { url, url_valid: values[0] != 0, cookie, features: f },
                &mut status,
                native,
            ) {
                emit_cookie(cookie, native);
                out.scalar = 1;
            }
            out.access.status = status;
        }
        1002 => {
            let f = features(native);
            let cookie = read_cookie(native, 0, true);
            if let Some(cookie) = c::from_storage(cookie, values[0] as i32, f, native) {
                emit_cookie(cookie, native);
                out.scalar = 1;
            }
        }
        1003 => {
            let mut pc = c::Parsed {
                name: Vec::new(),
                value: Vec::new(),
                attributes: std::array::from_fn(|_| None),
                valid: true,
                internal_htab: false,
                nameless: None,
            };
            for index in [2, 3] {
                if native.number(1123, index, 0) != 0 {
                    pc.attributes[index as usize] = Some(bytes(native, -1, index));
                }
            }
            out.scalar = c::parse_expiry(&pc, values[0], values[1], native);
        }
        1004 => {
            let f = features(native);
            out.scalar = c::adjust_expiry(values[0], values[1], values[2] as i32, f, native);
        }
        1005 | 1006 => {
            let f = features(native);
            let cookie = read_cookie(native, 0, true);
            let result = if op == 1005 {
                c::canonical_storage(&cookie, f, native)
            } else {
                c::canonical(&cookie, f, native)
            };
            out.scalar = result.err().map_or(-1, |failure| i64::from(failure as i32));
        }
        1007 => {
            let f = features(native);
            out.scalar = i64::from(c::should_encrypt(values[0] != 0, values[1] != 0, f));
        }
        1008 => out.scalar = i64::from(c::trustworthy_port(url, values[0] != 0)),
        1009..=1012 => {
            let a = read_cookie(native, 0, false);
            let b = read_cookie(native, 1, false);
            out.scalar = i64::from(match op {
                1009 => c::secure_matching(&a, &b, native),
                1010 => c::probably_equal(&a, &b, native),
                1011 => c::equivalent_data(&a, &b, (0, 1), native),
                _ => {
                    let f = features(native);
                    let equivalent = c::key_equal(&a, &b, 0, f, native);
                    let secure_a =
                        crate::base::is_secure(a.secure, f.scheme_bound, a.source_scheme);
                    let secure_b =
                        crate::base::is_secure(b.secure, f.scheme_bound, b.source_scheme);
                    c::web_equal(&a, &b, equivalent, (secure_a, secure_b), (0, 1), native)
                }
            });
        }
        1013 => {
            let count = native.number(1126, 0, 0);
            let mut line = Vec::new();
            for index in 0..count {
                let cookie = read_cookie(native, index, true);
                c::append_line(&cookie, &mut line);
            }
            out.buffer = Buffer::owned(Some(line));
        }
        1014 => {
            let cookie = read_cookie(native, 0, true);
            out.buffer = Buffer::owned(Some(c::attributes_line(&cookie, native)));
        }
        1015 => {
            let cookie = read_cookie(native, 0, true);
            let time = native.number(1127, 0, 0);
            out.buffer = Buffer::owned(Some(c::debug(&cookie, time)));
        }
        1016 => out.scalar = c::lax_threshold(features(native)),
        1017 => {
            let sampled = <Native as c::Native>::should_sample(native);
            c::post_request(
                sampled,
                status::query(request.access.status, 0, 0, 0),
                request.access.effective_same_site,
                values[0] as i32,
                values[1] as i32,
                status::query(request.access.status, 5, 4, 0),
                values[2] as i32,
                native,
            );
        }
        1018 => {
            let sampled = <Native as c::Native>::should_sample(native);
            c::post_set(
                sampled,
                status::query(request.access.status, 0, 0, 0),
                request.access.effective_same_site,
                values[0] as i32,
                status::query(request.access.status, 5, 4, 0),
                values[1] as i32,
                native,
            );
        }
        1019 => out.scalar = i64::from(values[0] != 0),
        1020 => out.scalar = i64::from(values[0] != 0 && values[0] <= values[1]),
        1021 | 1022 => {
            let a = read_cookie(native, 0, false);
            let b = read_cookie(native, 1, false);
            let f = features(native);
            out.scalar = if op == 1021 {
                i64::from(c::key_equal(&a, &b, 0, f, native))
            } else {
                i64::from(c::key_compare(&a, &b, 0, f, native))
            };
        }
        1023 => {
            let equal = <Native as c::Native>::value_equal(native, 0, 1);
            out.scalar = i64::from(equal);
        }
        1024 => {
            let size = native.number(1120, 0, 17) as usize;
            // SAFETY: initialized request bytes are copied before native reads.
            let other = unsafe { request.first.copy() };
            out.scalar = i64::from(size == other.len() && bytes(native, 0, 1) == other);
        }
        1025 => out.scalar = i64::from(c::effectively_none(values[0] as i32)),
        1026 => {
            let name = c::failure_name(values[0] as i32);
            if let Some(name) = name {
                out.buffer = Buffer::owned(Some(name.to_vec()));
            } else {
                native.number(1114, 0, 0);
            }
        }
        _ => {
            native.number(399, i64::from(op), 0);
        }
    }
    if native.failed.get() {
        // SAFETY: only this module's owned result buffer is consumed once.
        unsafe { crate::ffi::cookie_family_buffer_free(out.buffer) };
        Output::default()
    } else {
        out
    }
}
