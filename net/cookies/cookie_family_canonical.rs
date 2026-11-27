// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
//! Complete CanonicalCookie policy, composed directly with parsed/status/util.
//! Native providers own URL canonicalization, calendar/clock arithmetic,
//! encryption and telemetry. No provider may decide cookie acceptance.
#![forbid(unsafe_code)]

pub fn effectively_none(effective: i32) -> bool {
    effective == 0
}
pub fn failure_name(failure: i32) -> Option<&'static [u8]> {
    const NAMES: [&[u8]; 16] = [
        b"kInvalidExpiryDate",
        b"kUnparseableName",
        b"kUnparseableValue",
        b"kInvalidName",
        b"kInvalidValue",
        b"kInconsistentCreationAndLastAccessDate",
        b"kNonAsciiCharactersDisallowed",
        b"kInvalidDomain",
        b"kInvalidPath",
        b"kInvalidHostPrefix",
        b"kInvalidSecurePrefix",
        b"kInvalidHttpPrefix",
        b"kInvalidHostHttpPrefix",
        b"kEmptyNameWithHiddenPrefix",
        b"kPartitionedInsecure",
        b"kEmptyNameWithAmbiguousValue",
    ];
    usize::try_from(failure).ok().and_then(|id| NAMES.get(id).copied())
}

use crate::{parsed, status, types::Url, util};
use status::State;

pub const NULL_TIME: i64 = 0;
pub const SECURE_SCHEME: i32 = 2;
pub const NONSECURE_SCHEME: i32 = 1;

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Partition {
    // Native immutable value identity, not its address or persisted encoding.
    pub identity: u64,
    pub nonce: bool,
}

#[derive(Clone, Debug)]
pub struct Cookie {
    pub name: Vec<u8>,
    pub value: Vec<u8>,
    pub domain: Vec<u8>,
    pub path: Vec<u8>,
    pub creation: i64,
    pub expiry: i64,
    pub access: i64,
    pub update: i64,
    pub secure: bool,
    pub http_only: bool,
    pub same_site: i32,
    pub priority: i32,
    pub partition: Option<Partition>,
    pub source_scheme: i32,
    pub source_port: i32,
    pub source_type: i32,
    pub value_size: usize,
}
impl Default for Cookie {
    fn default() -> Self {
        Self {
            name: Vec::new(),
            value: Vec::new(),
            domain: Vec::new(),
            path: Vec::new(),
            creation: 0,
            expiry: 0,
            access: 0,
            update: 0,
            secure: false,
            http_only: false,
            same_site: -1,
            priority: 1,
            partition: None,
            source_scheme: 0,
            source_port: -1,
            source_type: 3,
            value_size: 0,
        }
    }
}
impl Cookie {
    pub fn expired(&self, now: i64) -> bool {
        self.expiry != 0 && self.expiry <= now
    }
    pub fn persistent(&self) -> bool {
        self.expiry != 0
    }
    pub fn host_only(&self) -> bool {
        util::domain_is_host_only(&self.domain)
    }
    pub fn size(&self) -> usize {
        self.name.len().saturating_add(self.value_size)
    }
}

#[derive(Clone, Copy, Debug, Default)]
pub struct Features {
    pub reject_non_ascii: bool,
    pub reject_domain_non_ascii: bool,
    pub reject_empty_ambiguous: bool,
    pub time_limited_insecure: bool,
    pub encrypt_only_important: bool,
    pub encrypt_http_only: bool,
    pub rigorous_lax_method: bool,
    pub short_lax_threshold: bool,
    pub port_bound: bool,
    pub scheme_bound: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(i32)]
pub enum Failure {
    InvalidExpiry = 0,
    UnparseableName = 1,
    UnparseableValue = 2,
    InvalidName = 3,
    InvalidValue = 4,
    InconsistentDates = 5,
    NonAscii = 6,
    InvalidDomain = 7,
    InvalidPath = 8,
    HostPrefix = 9,
    SecurePrefix = 10,
    HttpPrefix = 11,
    HostHttpPrefix = 12,
    HiddenPrefix = 13,
    PartitionedInsecure = 14,
    AmbiguousValue = 15,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Metric {
    NameOrValueHtab(bool),
    DomainNonAscii(bool),
    EmptyName(bool),
    EmptyNameAmbiguous(bool),
    Prefix(i32),
    PartitionedValid(bool),
    SameSiteAttribute(i32),
    NamelessParse { kind: usize, source_type: i32 },
    NameNonAscii(bool),
    ValueNonAscii(bool),
    DoubleUnderscore(bool),
    FromStorageLength { call_site: i32, valid: bool },
    RequestContext(i32),
    IncludedRequestSameSite(i32),
    IncludedResponseSameSite(i32),
    RedirectRead(i32),
    RedirectWrite(i32),
    RedirectDowngradeRead(i32),
    RedirectDowngradeWrite(i32),
}

pub trait Native: util::DomainProvider + util::CalendarProvider {
    fn should_sample(&mut self) -> bool;
    fn now(&mut self) -> i64;
    // base::Time/TimeDelta saturation and infinity CHECK semantics, not policy.
    fn add_time(&mut self, time: i64, delta: i64) -> i64;
    fn subtract_time(&mut self, left: i64, right: i64) -> i64;
    fn seconds(&mut self, seconds: i64) -> i64;
    fn encode_path(&mut self, path: &[u8]) -> Vec<u8>;
    fn http_date(&mut self, time: i64) -> Vec<u8>;
    fn partition_equal(&mut self, left: Partition, right: Partition) -> bool;
    fn partition_compare(&mut self, left: Partition, right: Partition) -> i32;
    fn value_equal(&mut self, left_identity: u64, right_identity: u64) -> bool;
    fn metric(&mut self, metric: Metric);
    fn debug_invalid_parse(&mut self);
    fn debug_rejection(&mut self, reason: i32);
    fn check_creation_not_null(&mut self, valid: bool);
    fn check_test_call_site(&mut self);
}

fn exclude(out: &mut State, reason: i32) {
    *out = status::apply(*out, 0, reason, 0);
}
fn warn(out: &mut State, reason: i32) {
    *out = status::apply(*out, 6, reason, 0);
}
fn pair_reason(out: &mut State, reason: usize) {
    match reason {
        0 => {}
        1 => exclude(out, 25),
        2 => exclude(out, 17),
        _ => exclude(out, 23),
    }
}

#[derive(Clone, Debug)]
pub struct Parsed {
    pub name: Vec<u8>,
    pub value: Vec<u8>,
    pub attributes: [Option<Vec<u8>>; 9],
    pub valid: bool,
    pub internal_htab: bool,
    pub nameless: Option<usize>,
}
impl Parsed {
    pub fn parse(input: &[u8], out: &mut State) -> Self {
        *out = State::default();
        let mut result = Self {
            name: Vec::new(),
            value: Vec::new(),
            attributes: std::array::from_fn(|_| None),
            valid: false,
            internal_htab: false,
            nameless: None,
        };
        let (mut at, mut first) = (0, true);
        loop {
            let pair = parsed::next_pair(input, at, first);
            if first {
                result.internal_htab = pair.internal_htab != 0;
                result.nameless = (pair.nameless != parsed::NO_NAMELESS).then_some(pair.nameless);
            }
            if pair.action == 0 {
                break;
            }
            if pair.action == 3 {
                pair_reason(out, pair.reason);
                result.valid = false;
                result.name.clear();
                result.value.clear();
                result.attributes = std::array::from_fn(|_| None);
                break;
            }
            if pair.action == 2 {
                warn(out, 6);
            } else if first {
                result.name = input[pair.name_begin..pair.name_begin + pair.name_len].to_vec();
                result.value = input[pair.value_begin..pair.value_begin + pair.value_len].to_vec();
                result.valid = true;
            } else if pair.attribute != 0 {
                result.attributes[pair.attribute - 1] =
                    Some(input[pair.value_begin..pair.value_begin + pair.value_len].to_vec());
            }
            at = pair.next;
            first = false;
        }
        result
    }
    pub fn attribute(&self, id: usize) -> &[u8] {
        self.attributes[id].as_deref().unwrap_or_default()
    }
    pub fn secure(&self) -> bool {
        self.attributes[4].is_some()
    }
    pub fn http_only(&self) -> bool {
        self.attributes[5].is_some()
    }
    pub fn partitioned(&self) -> bool {
        self.attributes[8].is_some()
    }
    pub fn same_site(&self) -> parsed::SameSite {
        parsed::same_site(self.attribute(6), self.attributes[6].is_some())
    }
}

// base::StringToInt64 compatibility: best-effort leading ASCII whitespace,
// optional sign, partial digits and saturation; only an exact parse is valid.
fn max_age(input: &[u8]) -> (bool, i64) {
    let leading = input.iter().take_while(|byte| matches!(**byte,b' '|b'\t'..=b'\r')).count();
    let input = &input[leading..];
    let (negative, digits) = match input.first() {
        Some(b'-') => (true, &input[1..]),
        Some(b'+') => (false, &input[1..]),
        _ => (false, input),
    };
    if digits.is_empty() {
        return (false, 0);
    }
    let limit = if negative { 1_u64 << 63 } else { i64::MAX as u64 };
    let (mut value, mut overflow) = (0_u64, false);
    for byte in digits {
        if !byte.is_ascii_digit() {
            return (
                false,
                if overflow {
                    if negative {
                        i64::MIN
                    } else {
                        i64::MAX
                    }
                } else if negative {
                    (0_i64).wrapping_sub(value as i64)
                } else {
                    value as i64
                },
            );
        }
        match value.checked_mul(10).and_then(|v| v.checked_add(u64::from(byte - b'0'))) {
            Some(next) if next <= limit && !overflow => value = next,
            _ => {
                value = limit;
                overflow = true;
            }
        }
    }
    let value = if negative { (0_i64).wrapping_sub(value as i64) } else { value as i64 };
    (!overflow && leading == 0, value)
}
pub fn parse_expiry(pc: &Parsed, current: i64, server: i64, native: &mut impl Native) -> i64 {
    if let Some(age) = &pc.attributes[3] {
        let (perfect, value) = max_age(age);
        if perfect {
            if value <= 0 {
                return i64::MIN;
            }
            let delta = native.seconds(value);
            return native.add_time(current, delta);
        }
        if value == i64::MIN || value == i64::MAX {
            return value;
        }
    }
    let expires = pc.attribute(2);
    if expires.is_empty() {
        return 0;
    }
    let parsed = util::parse_expiration(expires, native);
    if parsed == 0 {
        return 0;
    }
    let skew = native.subtract_time(current, server);
    native.add_time(parsed, skew)
}
pub fn adjust_expiry(
    expiry: i64,
    creation: i64,
    scheme: i32,
    features: Features,
    native: &mut impl Native,
) -> i64 {
    if expiry == 0 {
        return 0;
    }
    let creation = if creation == 0 { native.now() } else { creation };
    let seconds =
        if !(features.scheme_bound && features.time_limited_insecure) || scheme == SECURE_SCHEME {
            400 * 86400
        } else {
            3 * 3600
        };
    let delta = native.seconds(seconds);
    let maximum = native.add_time(creation, delta);
    expiry.min(maximum)
}
pub fn should_encrypt(secure: bool, http_only: bool, features: Features) -> bool {
    secure || !features.encrypt_only_important || (http_only && features.encrypt_http_only)
}
pub fn trustworthy_port(url: Url<'_>, trustworthy: bool) -> i32 {
    if trustworthy
        && !url.cryptographic
        && matches!(url.scheme,b"http"|b"ws")
        && url.effective_port == 80
    {
        443
    } else {
        url.effective_port
    }
}
pub fn source_scheme(url: Url<'_>, secure: bool) -> i32 {
    if secure || url.cryptographic {
        SECURE_SCHEME
    } else {
        NONSECURE_SCHEME
    }
}

pub fn canonical_storage(
    cookie: &Cookie,
    features: Features,
    native: &mut impl Native,
) -> Result<(), Failure> {
    let token = parsed::string_scan(&cookie.name, true);
    if cookie.name.get(token.begin..token.begin + token.len) != Some(cookie.name.as_slice()) {
        return Err(Failure::UnparseableName);
    }
    let value = parsed::string_scan(&cookie.value, false);
    if cookie.value.get(value.begin..value.begin + value.len) != Some(cookie.value.as_slice()) {
        return Err(Failure::UnparseableValue);
    }
    if !parsed::valid_chars(&cookie.name, true) {
        return Err(Failure::InvalidName);
    }
    if !parsed::valid_chars(&cookie.value, false) {
        return Err(Failure::InvalidValue);
    }
    if cookie.access != 0 && cookie.creation == 0 {
        return Err(Failure::InconsistentDates);
    }
    if features.reject_non_ascii && (!cookie.name.is_ascii() || !cookie.value.is_ascii()) {
        return Err(Failure::NonAscii);
    }
    if native.canonical_host(&cookie.domain) != cookie.domain {
        return Err(Failure::InvalidDomain);
    }
    if !cookie.path.starts_with(b"/") {
        return Err(Failure::InvalidPath);
    }
    let prefix = util::cookie_prefix(&cookie.name);
    if !util::prefix_valid(
        prefix,
        None,
        cookie.secure,
        cookie.http_only,
        &cookie.domain,
        &cookie.path,
    ) {
        match prefix {
            1 => return Err(Failure::SecurePrefix),
            2 => return Err(Failure::HostPrefix),
            3 => return Err(Failure::HttpPrefix),
            4 => return Err(Failure::HostHttpPrefix),
            _ => {}
        }
    }
    if cookie.name.is_empty() && util::hidden_prefix(&cookie.value) {
        return Err(Failure::HiddenPrefix);
    }
    if cookie.name.is_empty() && features.reject_empty_ambiguous && cookie.value.contains(&b'=') {
        return Err(Failure::AmbiguousValue);
    }
    if cookie.partition.is_some_and(|p| !p.nonce) && !cookie.secure {
        return Err(Failure::PartitionedInsecure);
    }
    Ok(())
}
pub fn canonical(
    cookie: &Cookie,
    features: Features,
    native: &mut impl Native,
) -> Result<(), Failure> {
    if adjust_expiry(cookie.expiry, cookie.creation, cookie.source_scheme, features, native)
        != cookie.expiry
    {
        return Err(Failure::InvalidExpiry);
    }
    canonical_storage(cookie, features, native)
}

pub struct CreateInput<'a> {
    pub url: Url<'a>,
    pub url_valid: bool,
    pub line: &'a [u8],
    pub creation: i64,
    pub server: Option<i64>,
    pub partition: Option<Partition>,
    pub source_type: i32,
    pub features: Features,
}
pub fn create(input: CreateInput<'_>, out: &mut State, native: &mut impl Native) -> Option<Cookie> {
    *out = State::default();
    if !input.url_valid {
        exclude(out, 10);
        return None;
    }
    let pc = Parsed::parse(input.line, out);
    let collect_metrics = native.should_sample();
    if collect_metrics {
        native.metric(Metric::NameOrValueHtab(pc.internal_htab));
    }
    if !pc.valid {
        native.debug_invalid_parse();
        return None;
    }
    if collect_metrics {
        native.metric(Metric::DomainNonAscii(!pc.attribute(1).is_ascii()));
    }
    native.metric(Metric::EmptyName(pc.name.is_empty()));
    if pc.name.is_empty() {
        let ambiguous = pc.value.contains(&b'=');
        native.metric(Metric::EmptyNameAmbiguous(ambiguous));
        if input.features.reject_empty_ambiguous && ambiguous {
            exclude(out, 28);
        }
    }
    let domain = util::cookie_domain(
        input.url,
        pc.attribute(1),
        input.features.reject_domain_non_ascii,
        out,
        native,
    );
    if domain.is_none() {
        native.debug_rejection(14);
        exclude(out, 14);
    }
    let path = util::canon_path(input.url.path, pc.attribute(0));
    let server = input.server.filter(|time| *time != 0).unwrap_or(input.creation);
    native.check_creation_not_null(input.creation != 0);
    let prefix = util::cookie_prefix(&pc.name);
    let mut prefix_valid = util::prefix_valid(
        prefix,
        Some(input.url),
        pc.secure(),
        pc.http_only(),
        pc.attribute(1),
        pc.attribute(0),
    );
    if collect_metrics {
        native.metric(Metric::Prefix(prefix));
    }
    if pc.name.is_empty() {
        prefix_valid = !util::hidden_prefix(&pc.value);
    }
    if !prefix_valid {
        native.debug_rejection(15);
        exclude(out, 15);
    }
    let selected = if pc.partitioned() { input.partition } else { None };
    let partition_valid = util::partitioned_valid(
        Some(input.url),
        pc.secure(),
        selected.is_some(),
        selected.is_some_and(|p| p.nonce),
    );
    if !partition_valid {
        exclude(out, 16);
    }
    let nonced = input.partition.is_some_and(|p| p.nonce);
    let partition = if pc.partitioned() {
        if !nonced && collect_metrics {
            native.metric(Metric::PartitionedValid(partition_valid));
        }
        input.partition
    } else if nonced {
        input.partition
    } else {
        None
    };
    if !status::query(*out, 0, 0, 0) {
        return None;
    }
    let same_site = pc.same_site();
    let scheme = source_scheme(input.url, pc.secure());
    if scheme == SECURE_SCHEME && !input.url.cryptographic {
        warn(out, 10);
    }
    let port = trustworthy_port(input.url, pc.secure());
    let expiry = parse_expiry(&pc, input.creation, server, native);
    let expiry = adjust_expiry(expiry, input.creation, scheme, input.features, native);
    if input.features.reject_non_ascii && (!pc.name.is_ascii() || !pc.value.is_ascii()) {
        exclude(out, 23);
    }
    let mut cookie = Cookie {
        name: pc.name.clone(),
        value: pc.value.clone(),
        domain: domain.unwrap_or_default(),
        path,
        creation: input.creation,
        expiry,
        access: input.creation,
        update: 0,
        secure: pc.secure(),
        http_only: pc.http_only(),
        same_site: same_site.value,
        priority: parsed::priority(pc.attribute(7)),
        partition,
        source_scheme: scheme,
        source_port: port,
        source_type: input.source_type,
        value_size: pc.value.len(),
    };
    if canonical(&cookie, input.features, native).is_err() {
        exclude(out, 10);
        return None;
    }
    if native.should_sample() {
        native.metric(Metric::SameSiteAttribute(same_site.metric));
    }
    if let Some(kind) = pc.nameless {
        native.metric(Metric::NamelessParse { kind, source_type: input.source_type });
    }
    if collect_metrics {
        native.metric(Metric::NameNonAscii(!pc.name.is_ascii()));
        native.metric(Metric::ValueNonAscii(!pc.value.is_ascii()));
        native.metric(Metric::DoubleUnderscore(prefix == 0 && pc.name.starts_with(b"__")));
    }
    cookie.update = native.now();
    Some(cookie)
}

pub struct SanitizeInput<'a> {
    pub url: Url<'a>,
    pub url_valid: bool,
    pub cookie: Cookie,
    pub features: Features,
}
fn exactly_parsed(value: &[u8], token: bool) -> bool {
    let parsed = parsed::string_scan(value, token);
    parsed.begin == 0 && parsed.len == value.len()
}
pub fn sanitize(
    input: SanitizeInput<'_>,
    out: &mut State,
    native: &mut impl Native,
) -> Option<Cookie> {
    *out = State::default();
    let mut cookie = input.cookie;
    if !exactly_parsed(&cookie.name, true)
        || !exactly_parsed(&cookie.value, false)
        || !exactly_parsed(&cookie.path, false)
    {
        exclude(out, 23);
    }
    if input.features.reject_non_ascii && (!cookie.name.is_ascii() || !cookie.value.is_ascii()) {
        exclude(out, 23);
    }
    pair_reason(out, parsed::valid_pair(&cookie.name, &cookie.value));
    let mut domain_valid = true;
    if !exactly_parsed(&cookie.domain, false) {
        exclude(out, 14);
        domain_valid = false;
    }
    if !parsed::valid_chars(&cookie.domain, false) {
        exclude(out, 14);
        domain_valid = false;
    }
    if cookie.domain.len() > 1024 {
        exclude(out, 18);
        domain_valid = false;
    }
    let domain_attribute = if domain_valid { cookie.domain.as_slice() } else { b"" };
    let domain = if !util::domain_is_host_only(input.url.host) {
        exclude(out, 14);
        None
    } else {
        let result = util::cookie_domain(
            input.url,
            domain_attribute,
            input.features.reject_domain_non_ascii,
            out,
            native,
        );
        if result.is_none() {
            exclude(out, 14);
        }
        result
    };
    let mut scheme = NONSECURE_SCHEME;
    if !input.url_valid {
        exclude(out, 14);
    } else {
        scheme = source_scheme(input.url, cookie.secure);
        if scheme == SECURE_SCHEME && !input.url.cryptographic {
            warn(out, 10);
        }
    }
    let port = trustworthy_port(input.url, cookie.secure);
    let path = util::canon_path(input.url.path, &cookie.path);
    let encoded = native.encode_path(&path);
    if !cookie.path.is_empty() {
        if path != cookie.path {
            exclude(out, 27);
        } else if encoded.len() > 1024 {
            exclude(out, 18);
        }
    }
    let prefix = util::cookie_prefix(&cookie.name);
    if !util::prefix_valid(
        prefix,
        Some(input.url),
        cookie.secure,
        cookie.http_only,
        domain_attribute,
        &path,
    ) {
        exclude(out, 15);
    }
    if cookie.name.is_empty() && util::hidden_prefix(&cookie.value) {
        exclude(out, 15);
    }
    if cookie.name.is_empty()
        && input.features.reject_empty_ambiguous
        && cookie.value.contains(&b'=')
    {
        exclude(out, 28);
    }
    if !util::partitioned_valid(
        Some(input.url),
        cookie.secure,
        cookie.partition.is_some(),
        cookie.partition.is_some_and(|p| p.nonce),
    ) {
        exclude(out, 16);
    }
    if cookie.access != 0 && cookie.creation == 0 {
        exclude(out, 10);
    }
    cookie.expiry = adjust_expiry(cookie.expiry, cookie.creation, scheme, input.features, native);
    if !status::query(*out, 0, 0, 0) {
        return None;
    }
    cookie.domain = domain.unwrap_or_default();
    cookie.path = encoded;
    cookie.source_scheme = scheme;
    cookie.source_port = port;
    cookie.source_type = 3;
    cookie.update = native.now();
    Some(cookie)
}
pub fn from_storage(
    mut cookie: Cookie,
    call_site: i32,
    features: Features,
    native: &mut impl Native,
) -> Option<Cookie> {
    canonical_storage(&cookie, features, native).ok()?;
    let valid = parsed::valid_pair(&cookie.name, &cookie.value) == 0;
    if call_site == 3 {
        if !valid {
            return None;
        }
    } else if native.should_sample() {
        if call_site == 8 {
            native.check_test_call_site();
        }
        native.metric(Metric::FromStorageLength { call_site, valid });
    }
    cookie.source_port =
        if (-1..=65535).contains(&cookie.source_port) { cookie.source_port } else { -2 };
    Some(cookie)
}

fn partition_equal(a: Option<Partition>, b: Option<Partition>, native: &mut impl Native) -> bool {
    match (a, b) {
        (None, None) => true,
        (Some(a), Some(b)) => native.partition_equal(a, b),
        _ => false,
    }
}
pub fn key_compare(
    a: &Cookie,
    b: &Cookie,
    mode: u32,
    features: Features,
    native: &mut impl Native,
) -> i32 {
    use std::cmp::Ordering;
    let left =
        crate::base::key_policy(mode, !a.host_only(), features.scheme_bound, features.port_bound);
    let right =
        crate::base::key_policy(mode, !b.host_only(), features.scheme_bound, features.port_bound);
    let sign = |order: Ordering| match order {
        Ordering::Less => -1,
        Ordering::Equal => 0,
        Ordering::Greater => 1,
    };
    let kind = left.kind.cmp(&right.kind);
    if kind != Ordering::Equal {
        return sign(kind);
    }
    match (a.partition, b.partition) {
        (None, Some(_)) => return -1,
        (Some(_), None) => return 1,
        (Some(a), Some(b)) => {
            let order = native.partition_compare(a, b);
            if order != 0 {
                return order;
            }
        }
        _ => {}
    }
    for (a, b) in [(&a.name, &b.name), (&a.domain, &b.domain), (&a.path, &b.path)] {
        let order = a.cmp(b);
        if order != Ordering::Equal {
            return sign(order);
        }
    }
    let scheme =
        left.scheme.then_some(a.source_scheme).cmp(&right.scheme.then_some(b.source_scheme));
    if scheme != Ordering::Equal {
        return sign(scheme);
    }
    sign(left.port.then_some(a.source_port).cmp(&right.port.then_some(b.source_port)))
}
pub fn key_equal(
    a: &Cookie,
    b: &Cookie,
    mode: u32,
    features: Features,
    native: &mut impl Native,
) -> bool {
    let left =
        crate::base::key_policy(mode, !a.host_only(), features.scheme_bound, features.port_bound);
    let right =
        crate::base::key_policy(mode, !b.host_only(), features.scheme_bound, features.port_bound);
    // Keep native token equality's distinct primitive rather than defining
    // equality as ordering==0.
    left.kind == right.kind
        && partition_equal(a.partition, b.partition, native)
        && a.name == b.name
        && a.domain == b.domain
        && a.path == b.path
        && left.scheme.then_some(a.source_scheme) == right.scheme.then_some(b.source_scheme)
        && left.port.then_some(a.source_port) == right.port.then_some(b.source_port)
}
fn subdomain(a: &[u8], b: &[u8]) -> bool {
    a == b || (a.len() > b.len() && a.ends_with(b) && a.get(a.len() - b.len() - 1) == Some(&b'.'))
}
pub fn secure_matching(cookie: &Cookie, secure: &Cookie, native: &mut impl Native) -> bool {
    let same_partition = partition_equal(cookie.partition, secure.partition, native);
    let same_name = cookie.name == secure.name;
    let (a, b) =
        (util::cookie_domain_as_host(&cookie.domain), util::cookie_domain_as_host(&secure.domain));
    let domain = subdomain(a, b) || subdomain(b, a);
    let path = util::is_on_path(&secure.path, &cookie.path);
    same_partition && same_name && domain && path
}
pub fn probably_equal(a: &Cookie, b: &Cookie, native: &mut impl Native) -> bool {
    a.update == b.update
        && a.access == b.access
        && a.expiry == b.expiry
        && a.creation == b.creation
        && a.secure == b.secure
        && a.http_only == b.http_only
        && a.same_site == b.same_site
        && a.priority == b.priority
        && partition_equal(a.partition, b.partition, native)
        && a.name == b.name
        && a.domain == b.domain
        && a.path == b.path
        && a.source_scheme == b.source_scheme
        && a.source_port == b.source_port
        && a.source_type == b.source_type
}
pub fn equivalent_data(a: &Cookie, b: &Cookie, ids: (u64, u64), native: &mut impl Native) -> bool {
    probably_equal(a, b, native) && native.value_equal(ids.0, ids.1)
}
pub fn web_equal(
    a: &Cookie,
    b: &Cookie,
    equivalent: bool,
    is_secure: (bool, bool),
    ids: (u64, u64),
    native: &mut impl Native,
) -> bool {
    equivalent
        && is_secure.0 == is_secure.1
        && a.same_site == b.same_site
        && a.http_only == b.http_only
        && a.expiry == b.expiry
        && native.value_equal(ids.0, ids.1)
}
pub fn append_line(cookie: &Cookie, out: &mut Vec<u8>) {
    if !out.is_empty() {
        out.extend_from_slice(b"; ");
    }
    if !cookie.name.is_empty() {
        out.extend_from_slice(&cookie.name);
        out.push(b'=');
    }
    out.extend_from_slice(&cookie.value);
}
pub fn attributes_line(cookie: &Cookie, native: &mut impl Native) -> Vec<u8> {
    let mut out = Vec::new();
    append_line(cookie, &mut out);
    if !cookie.domain.is_empty() {
        out.extend_from_slice(b"; domain=");
        out.extend_from_slice(&cookie.domain);
    }
    if !cookie.path.is_empty() {
        out.extend_from_slice(b"; path=");
        out.extend_from_slice(&cookie.path);
    }
    if cookie.expiry != 0 {
        out.extend_from_slice(b"; expires=");
        out.extend_from_slice(&native.http_date(cookie.expiry));
    }
    if cookie.secure {
        out.extend_from_slice(b"; secure");
    }
    if cookie.http_only {
        out.extend_from_slice(b"; httponly");
    }
    if cookie.partition.is_some_and(|p| !p.nonce) {
        out.extend_from_slice(b"; partitioned");
    }
    out.extend_from_slice(match cookie.same_site {
        0 => b"; samesite=none",
        1 => b"; samesite=lax",
        2 => b"; samesite=strict",
        _ => b"",
    });
    out
}
pub fn lax_threshold(features: Features) -> i64 {
    if features.rigorous_lax_method {
        i64::MIN
    } else if features.short_lax_threshold {
        10_000_000
    } else {
        120_000_000
    }
}
pub fn debug(cookie: &Cookie, creation_time_t: i64) -> Vec<u8> {
    // StringPrintf's %s truncates at embedded NUL; preserve that behavior.
    let trunc = |value: &Vec<u8>| {
        value[..value.iter().position(|byte| *byte == 0).unwrap_or(value.len())].to_vec()
    };
    let mut out = b"name: ".to_vec();
    out.extend(trunc(&cookie.name));
    out.extend_from_slice(b" value: ");
    out.extend(trunc(&cookie.value));
    out.extend_from_slice(b" domain: ");
    out.extend(trunc(&cookie.domain));
    out.extend_from_slice(b" path: ");
    out.extend(trunc(&cookie.path));
    out.extend_from_slice(b" creation: ");
    out.extend_from_slice(creation_time_t.to_string().as_bytes());
    out
}

pub fn post_request(
    collect: bool,
    included: bool,
    effective: i32,
    context: i32,
    redirect: i32,
    downgrade: bool,
    same_site: i32,
    native: &mut impl Native,
) {
    if !collect {
        return;
    }
    native.metric(Metric::RequestContext(context));
    if included {
        native.metric(Metric::IncludedRequestSameSite(effective));
    }
    if redirect != 0 {
        native.metric(Metric::RedirectRead(redirect));
    }
    if downgrade {
        native.metric(Metric::RedirectDowngradeRead(same_site + 1));
    }
}
pub fn post_set(
    collect: bool,
    included: bool,
    effective: i32,
    redirect: i32,
    downgrade: bool,
    same_site: i32,
    native: &mut impl Native,
) {
    if !collect {
        return;
    }
    if included {
        native.metric(Metric::IncludedResponseSameSite(effective));
    }
    if redirect != 0 {
        native.metric(Metric::RedirectWrite(redirect));
    }
    if downgrade {
        native.metric(Metric::RedirectDowngradeWrite(same_site + 1));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[derive(Default)]
    struct Services {
        calls: Vec<&'static str>,
        metrics: Vec<Metric>,
    }
    impl util::DomainProvider for Services {
        fn canonical_host(&mut self, v: &[u8]) -> Vec<u8> {
            v.to_ascii_lowercase()
        }
        fn registry(&mut self, v: &[u8]) -> Vec<u8> {
            v.to_vec()
        }
        fn check_host_only(&mut self, v: bool) {
            assert!(v);
        }
    }
    impl util::CalendarProvider for Services {
        fn implode(&mut self, _: util::Calendar) -> Option<i64> {
            self.calls.push("calendar");
            Some(123)
        }
        fn valid_values(&mut self, _: util::Calendar) -> bool {
            true
        }
        fn year_bounds(&self) -> (i32, i32) {
            (1, 9999)
        }
    }
    impl Native for Services {
        fn should_sample(&mut self) -> bool {
            self.calls.push("sample");
            false
        }
        fn now(&mut self) -> i64 {
            self.calls.push("now");
            999
        }
        fn add_time(&mut self, t: i64, d: i64) -> i64 {
            self.calls.push("add");
            t.saturating_add(d)
        }
        fn subtract_time(&mut self, a: i64, b: i64) -> i64 {
            a.saturating_sub(b)
        }
        fn seconds(&mut self, v: i64) -> i64 {
            self.calls.push("seconds");
            v.saturating_mul(1_000_000)
        }
        fn encode_path(&mut self, v: &[u8]) -> Vec<u8> {
            v.to_vec()
        }
        fn http_date(&mut self, _: i64) -> Vec<u8> {
            b"date".to_vec()
        }
        fn partition_equal(&mut self, a: Partition, b: Partition) -> bool {
            a.identity == b.identity
        }
        fn partition_compare(&mut self, a: Partition, b: Partition) -> i32 {
            match a.identity.cmp(&b.identity) {
                std::cmp::Ordering::Less => -1,
                std::cmp::Ordering::Equal => 0,
                std::cmp::Ordering::Greater => 1,
            }
        }
        fn value_equal(&mut self, a: u64, b: u64) -> bool {
            a == b
        }
        fn metric(&mut self, m: Metric) {
            self.metrics.push(m);
        }
        fn debug_invalid_parse(&mut self) {
            self.calls.push("parse-failed");
        }
        fn debug_rejection(&mut self, _: i32) {
            self.calls.push("reject");
        }
        fn check_creation_not_null(&mut self, v: bool) {
            assert!(v);
        }
        fn check_test_call_site(&mut self) {
            self.calls.push("test-check");
        }
    }
    fn url() -> Url<'static> {
        Url {
            identity: 0,
            scheme: b"https",
            host: b"example.test",
            path: b"/a/b",
            standard: true,
            is_ip: false,
            cryptographic: true,
            localhost: false,
            host_compliant: true,
            effective_port: 443,
        }
    }
    fn cookie() -> Cookie {
        Cookie {
            name: b"n".to_vec(),
            value: b"v".to_vec(),
            value_size: 1,
            domain: b"example.test".to_vec(),
            path: b"/".to_vec(),
            creation: 10,
            access: 10,
            source_scheme: 2,
            source_port: 443,
            ..Cookie::default()
        }
    }
    #[test]
    fn max_age_exact_and_best_effort_overflow() {
        for (text, expected) in [
            (b"+7".as_slice(), (true, 7)),
            (b"-0", (true, 0)),
            (b"7junk", (false, 7)),
            (b"9223372036854775808", (false, i64::MAX)),
            (b"-9223372036854775809", (false, i64::MIN)),
            (b" \t9223372036854775808x", (false, i64::MAX)),
            (b"\n-9223372036854775808", (false, i64::MIN)),
        ] {
            assert_eq!(max_age(text),expected);
        }
    }
    #[test]
    fn null_expiry_does_not_read_clock() {
        let mut s = Services::default();
        assert_eq!(adjust_expiry(0,0,1,Features::default(),&mut s),0);
        assert!(s.calls.is_empty());
    }
    #[test]
    fn insecure_expiry_requires_both_features() {
        let mut s = Services::default();
        let mut f = Features { time_limited_insecure: true, ..Features::default() };
        let full = 10 + 400 * 86400 * 1_000_000;
        assert_eq!(adjust_expiry(i64::MAX,10,1,f,&mut s),full);
        f.scheme_bound = true;
        assert_eq!(adjust_expiry(i64::MAX,10,1,f,&mut s),10+3*3600*1_000_000);
        assert_eq!(adjust_expiry(i64::MAX,10,2,f,&mut s),full);
    }
    #[test]
    fn creation_clock_only_for_null_creation() {
        let mut s = Services::default();
        adjust_expiry(1, 0, 2, Features::default(), &mut s);
        assert_eq!(s.calls,["now","seconds","add"]);
    }
    #[test]
    fn invalid_url_precedes_parse_and_sample() {
        let mut s = Services::default();
        let mut status = status::apply(State::default(), 6, 7, 0);
        assert!(create(CreateInput{url:url(),url_valid:false,line:b"bad\0line",creation:10,server:None,partition:None,source_type:3,features:Features::default()},&mut status,&mut s).is_none());
        assert!(s.calls.is_empty());
        assert!(status::query(status,1,10,0));
        assert!(!status::query(status,5,7,0));
    }
    #[test]
    fn parsed_failure_samples_before_rejection() {
        let mut s = Services::default();
        let mut status = State::default();
        assert!(create(CreateInput{url:url(),url_valid:true,line:b"n=\x01",creation:10,server:None,partition:None,source_type:3,features:Features::default()},&mut status,&mut s).is_none());
        assert_eq!(s.calls,["sample","parse-failed"]);
    }
    #[test]
    fn valid_create_composes_parser_path_and_source() {
        let mut s = Services::default();
        let mut status = State::default();
        let c = create(
            CreateInput {
                url: url(),
                url_valid: true,
                line: b"n=v; Secure; Path=/a; SameSite=Lax",
                creation: 10,
                server: None,
                partition: None,
                source_type: 1,
                features: Features::default(),
            },
            &mut status,
            &mut s,
        )
        .unwrap();
        assert_eq!((&c.name[..],&c.value[..],&c.path[..]),(b"n".as_slice(),b"v".as_slice(),b"/a".as_slice()));
        assert_eq!((c.source_scheme,c.source_port,c.source_type,c.update),(2,443,1,999));
    }
    #[test]
    fn storage_source_port_invalid_sentinel() {
        let mut c = cookie();
        c.source_port = 65536;
        let mut s = Services::default();
        assert_eq!(from_storage(c,3,Features::default(),&mut s).unwrap().source_port,-2);
        assert!(s.calls.is_empty());
    }
    #[test]
    fn nonced_attribute_output_omits_partitioned_flag() {
        let mut c = cookie();
        c.partition = Some(Partition { identity: 1, nonce: true });
        let mut s = Services::default();
        assert_eq!(attributes_line(&c,&mut s),b"n=v; domain=example.test; path=/");
        c.partition.as_mut().unwrap().nonce = false;
        assert_eq!(attributes_line(&c,&mut s),b"n=v; domain=example.test; path=/; partitioned");
    }
    #[test]
    fn nameless_header_and_embedded_nul_debug() {
        let mut c = cookie();
        c.name.clear();
        c.value = b"bare".to_vec();
        let mut line = Vec::new();
        append_line(&c, &mut line);
        append_line(&cookie(), &mut line);
        assert_eq!(line,b"bare; n=v");
        c.name = b"n\0ignored".to_vec();
        assert!(debug(&c,7).starts_with(b"name: n value: bare"));
    }
    #[test]
    fn secure_match_uses_domain_and_path_boundaries() {
        let mut a = cookie();
        let mut b = cookie();
        a.domain = b"a.example.test".to_vec();
        b.path = b"/a".to_vec();
        let mut s = Services::default();
        assert!(!secure_matching(&a,&b,&mut s));
        a.path = b"/a/b".to_vec();
        assert!(secure_matching(&a,&b,&mut s));
        a.domain = b"badexample.test".to_vec();
        assert!(!secure_matching(&a,&b,&mut s));
    }
    #[test]
    fn failure_names_cover_persisted_ordinals() {
        assert_eq!(failure_name(5),Some(b"kInconsistentCreationAndLastAccessDate".as_slice()));
        assert_eq!(failure_name(15),Some(b"kEmptyNameWithAmbiguousValue".as_slice()));
        assert!(failure_name(-1).is_none());
        assert!(failure_name(16).is_none());
    }
}
