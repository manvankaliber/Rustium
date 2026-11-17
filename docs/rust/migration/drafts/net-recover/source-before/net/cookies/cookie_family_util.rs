// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]
use crate::types::{Context, Metadata, Url};
use crate::status::{self, State};

pub fn domain_is_host_only(domain: &[u8]) -> bool {
    !domain.starts_with(b".")
}
pub fn cookie_domain_as_host(domain: &[u8]) -> &[u8] {
    if domain_is_host_only(domain) {
        domain
    } else {
        &domain[1..]
    }
}
pub fn is_domain_match(domain: &[u8], host: &[u8]) -> bool {
    host == domain
        || (domain.starts_with(b".")
            && (domain.get(1..) == Some(host)
                || (host.len() > domain.len() && host.ends_with(domain))))
}
pub fn is_on_path(cookie_path: &[u8], url_path: &[u8]) -> bool {
    !cookie_path.is_empty()
        && url_path.starts_with(cookie_path)
        && (cookie_path.len() == url_path.len()
            || cookie_path.ends_with(b"/")
            || url_path.get(cookie_path.len()) == Some(&b'/'))
}
pub fn canon_path(url_path: &[u8], attribute: &[u8]) -> Vec<u8> {
    if attribute.starts_with(b"/") {
        return attribute.to_vec();
    }
    match url_path.iter().rposition(|byte| *byte == b'/') {
        Some(index) if index != 0 => url_path[..index].to_vec(),
        _ => b"/".to_vec(),
    }
}
pub fn domain_path_url(domain: &[u8], path: &[u8], scheme: &[u8]) -> Option<Vec<u8>> {
    let host = cookie_domain_as_host(domain);
    if host.is_empty() || scheme.is_empty() {
        return None;
    }
    let mut output = scheme.to_vec();
    output.extend_from_slice(b"://");
    output.extend_from_slice(host);
    output.extend_from_slice(path);
    Some(output)
}
pub fn source_scheme(secure: bool) -> &'static [u8] {
    if secure {
        b"https"
    } else {
        b"http"
    }
}
pub fn provisional_access_scheme(url: Url<'_>) -> i32 {
    if url.cryptographic {
        1
    } else if url.localhost {
        2
    } else {
        0
    }
}
pub fn url_host_valid(url: Url<'_>) -> bool {
    !matches!(url.scheme, b"http" | b"https" | b"ws" | b"wss") || url.is_ip || url.host_compliant
}

pub trait DomainProvider {
    fn canonical_host(&mut self, host: &[u8]) -> Vec<u8>;
    fn registry(&mut self, host: &[u8]) -> Vec<u8>;
    fn check_host_only(&mut self, value: bool);
}
pub fn effective_domain(scheme: &[u8], host: &[u8], services: &mut impl DomainProvider) -> Vec<u8> {
    if matches!(scheme, b"http" | b"https" | b"ws" | b"wss") {
        services.registry(host)
    } else {
        cookie_domain_as_host(host).to_vec()
    }
}
pub fn cookie_domain(
    url: Url<'_>,
    domain: &[u8],
    reject_non_ascii: bool,
    status_out: &mut State,
    services: &mut impl DomainProvider,
) -> Option<Vec<u8>> {
    if !domain.is_ascii() {
        if reject_non_ascii {
            *status_out = status::apply(*status_out, 0, 19, 0);
            return None;
        }
        *status_out = status::apply(*status_out, 6, 7, 0);
    }
    if url.host.ends_with(b"..") {
        return None;
    }
    let matches_host = url.host.eq_ignore_ascii_case(domain)
        || (domain.starts_with(b".") && url.host.eq_ignore_ascii_case(&domain[1..]));
    if domain.is_empty() || (url.is_ip && matches_host) {
        let result = if url.standard {
            url.host.to_vec()
        } else {
            let result = services.canonical_host(url.host);
            if result.starts_with(b".") || (result.is_empty() && !url.host.is_empty()) {
                return None;
            }
            result
        };
        services.check_host_only(domain_is_host_only(&result));
        return Some(result);
    }
    if url.is_ip || domain.contains(&b'%') {
        return None;
    }
    let mut cookie_domain = services.canonical_host(domain);
    if cookie_domain.is_empty() {
        return None;
    }
    if !cookie_domain.starts_with(b".") {
        cookie_domain.insert(0, b'.');
    }
    let url_registry = effective_domain(url.scheme, url.host, services);
    if url_registry.is_empty() {
        let result = cookie_domain_as_host(domain).to_ascii_lowercase();
        if url.host == result {
            services.check_host_only(domain_is_host_only(&result));
            return Some(result);
        }
        return None;
    }
    if url_registry != effective_domain(url.scheme, &cookie_domain, services) {
        *status_out = status::apply(*status_out, 0, 3, 0);
        return None;
    }
    let not_suffix = if url.host.len() < cookie_domain.len() {
        !cookie_domain.starts_with(b".") || &cookie_domain[1..] != url.host
    } else {
        !url.host.ends_with(&cookie_domain)
    };
    if not_suffix {
        None
    } else {
        Some(cookie_domain)
    }
}

// Prefix IDs are frozen to CookiePrefix's persisted native values.
pub fn cookie_prefix(name: &[u8]) -> i32 {
    for (prefix, id) in
        [(b"__Secure-".as_slice(), 1), (b"__Host-Http-", 4), (b"__Http-", 3), (b"__Host-", 2)]
    {
        if name.get(..prefix.len()).is_some_and(|part| part.eq_ignore_ascii_case(prefix)) {
            return id;
        }
    }
    0
}
pub fn hidden_prefix(value: &[u8]) -> bool {
    let skip = value.iter().take_while(|byte| **byte == b' ' || **byte == b'\t').count();
    cookie_prefix(&value[skip..]) != 0
}
pub fn prefix_valid(
    prefix: i32,
    url: Option<Url<'_>>,
    secure: bool,
    http_only: bool,
    domain: &[u8],
    path: &[u8],
) -> bool {
    let secure_valid = || secure && url.is_none_or(|url| provisional_access_scheme(url) != 0);
    let host_valid = || {
        secure_valid()
            && path == b"/"
            && match url {
                Some(url) => domain.is_empty() || (url.is_ip && url.host == domain),
                None => !domain.is_empty() && !domain.starts_with(b"."),
            }
    };
    match prefix {
        0 => true,
        1 => secure_valid(),
        2 => host_valid(),
        3 => secure_valid() && http_only,
        4 => secure_valid() && http_only && host_valid(),
        _ => false, // Invalid native enum violates the original switch contract.
    }
}
pub fn partitioned_valid(url: Option<Url<'_>>, secure: bool, present: bool, nonce: bool) -> bool {
    !present || nonce || (url.is_none_or(|url| provisional_access_scheme(url) != 0) && secure)
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(C)]
pub struct Calendar {
    pub year: i32,
    pub month: i32,
    pub day_of_week: i32,
    pub day: i32,
    pub hour: i32,
    pub minute: i32,
    pub second: i32,
    pub millisecond: i32,
}
pub trait CalendarProvider {
    fn implode(&mut self, calendar: Calendar) -> Option<i64>;
    fn valid_values(&mut self, calendar: Calendar) -> bool;
    fn year_bounds(&self) -> (i32, i32);
}
fn decimal(token: &[u8]) -> Option<u32> {
    if token.is_empty() || !token.iter().all(u8::is_ascii_digit) {
        return None;
    }
    token
        .iter()
        .try_fold(0_u32, |value, byte| value.checked_mul(10)?.checked_add(u32::from(byte - b'0')))
}
pub fn parse_expiration(input: &[u8], services: &mut impl CalendarProvider) -> i64 {
    const DELIMITERS: &[u8] = b"\t !\"#$%&'()*+,-./;<=>?@[\\]^_`{|}~";
    const MONTHS: [&[u8]; 12] = [
        b"jan", b"feb", b"mar", b"apr", b"may", b"jun", b"jul", b"aug", b"sep", b"oct", b"nov",
        b"dec",
    ];
    let mut out = Calendar::default();
    let (mut day, mut month, mut time, mut year) = (false, false, false, false);
    for token in input.split(|byte| DELIMITERS.contains(byte)).filter(|token| !token.is_empty()) {
        if !token[0].is_ascii_digit() {
            if !month {
                if let Some(index) = MONTHS
                    .iter()
                    .position(|name| token.get(..3).is_some_and(|p| p.eq_ignore_ascii_case(name)))
                {
                    out.month = index as i32 + 1;
                    month = true;
                }
            }
        } else if token.contains(&b':') {
            if !time {
                let pieces: Vec<_> = token.split(|byte| *byte == b':').collect();
                if pieces.len() == 3 {
                    if let (Some(h), Some(m), Some(s)) =
                        (decimal(pieces[0]), decimal(pieces[1]), decimal(pieces[2]))
                    {
                        // Original unsigned-to-int native conversion is modulo 2^32.
                        out.hour = h as i32;
                        out.minute = m as i32;
                        out.second = s as i32;
                        time = true;
                    }
                }
            }
        } else if !day && token.len() <= 2 {
            if let Some(value) = decimal(token) {
                out.day = value as i32;
                day = true;
            }
        } else if !year && token.len() <= 5 {
            if let Some(value) = decimal(token) {
                out.year = value as i32;
                year = true;
            }
        }
    }
    if !(day && month && time && year) {
        return 0;
    }
    if (70..=99).contains(&out.year) {
        out.year += 1900;
    }
    if (0..=69).contains(&out.year) {
        out.year += 2000;
    }
    if let Some(time) = services.implode(out) {
        return if time == 0 { 1 } else { time };
    }
    if !services.valid_values(out) {
        return 0;
    }
    let (min, max) = services.year_bounds();
    if out.year > max {
        i64::MAX
    } else if out.year < min {
        1
    } else {
        0
    }
}

// Emits completed pairs immediately in original append order. An unterminated
// quoted value returns while retaining pairs appended before it.
pub fn parse_request_line(input: &[u8], mut append: impl FnMut(&[u8], &[u8])) {
    let mut at = 0;
    while at < input.len() {
        while input.get(at) == Some(&b' ') {
            at += 1;
        }
        if at == input.len() {
            return;
        }
        let name_start = at;
        while at < input.len() && input[at] != b'=' {
            at += 1;
        }
        let name = &input[name_start..at];
        let mut value = &input[0..0];
        if at < input.len() && at + 1 < input.len() {
            at += 1;
            let value_start = at;
            if input[at] == b'"' {
                at += 1;
                while at < input.len() && input[at] != b'"' {
                    at += 1;
                }
                if at == input.len() {
                    return;
                }
                at += 1;
                value = &input[value_start..at];
            } else {
                while at < input.len() && input[at] != b';' {
                    at += 1;
                }
                value = &input[value_start..at];
            }
        }
        append(name, value);
        if at < input.len() {
            at += 1;
        }
    }
}
pub fn serialize_request_line<'a>(
    pairs: impl IntoIterator<Item = (&'a [u8], &'a [u8])>,
) -> Vec<u8> {
    let mut output = Vec::new();
    for (name, value) in pairs {
        if !output.is_empty() {
            output.extend_from_slice(b"; ");
        }
        output.extend_from_slice(name);
        output.push(b'=');
        output.extend_from_slice(value);
    }
    output
}
pub trait RequestPairs {
    fn pair_count(&mut self) -> usize;
    fn pair(&mut self, index: usize) -> (Vec<u8>, Vec<u8>);
}
pub fn serialize_request_pairs(services: &mut impl RequestPairs) -> Vec<u8> {
    let mut result = Vec::new();
    for index in 0..services.pair_count() {
        let (name, value) = services.pair(index);
        if !result.is_empty() {
            result.extend_from_slice(b"; ");
        }
        result.extend_from_slice(&name);
        result.push(b'=');
        result.extend_from_slice(&value);
    }
    result
}

pub trait SameSiteProvider {
    fn chain_len(&self) -> usize;
    fn has_initiator(&self) -> bool;
    fn first_party(&mut self, index: usize, schemeful: bool) -> bool;
    fn initiator_first_party(&mut self, index: usize, schemeful: bool) -> bool;
    fn site_for_cookies_null(&mut self) -> bool;
    fn check_main_navigation(&mut self, first_party: bool, request_index: usize);
    fn check_response_navigation(&mut self, request_index: usize);
    fn considers_redirect_chain(&mut self) -> bool;
}
pub fn redirect_type(one: bool, initiator: bool, site: bool, chain: bool) -> i32 {
    if one {
        1
    } else if !initiator || !site {
        2
    } else if !chain {
        3
    } else {
        4
    }
}
fn all_redirects(services: &mut impl SameSiteProvider, schemeful: bool) -> bool {
    services.chain_len() == 1
        || (0..services.chain_len()).all(|index| services.first_party(index, schemeful))
}
fn compute_context(
    services: &mut impl SameSiteProvider,
    http: bool,
    main: bool,
    schemeful: bool,
) -> (i32, Metadata) {
    let request = services.chain_len().saturating_sub(1);
    let site = services.first_party(request, schemeful);
    if main {
        services.check_main_navigation(site, request);
    }
    let initiator = !services.has_initiator() || services.initiator_first_party(request, schemeful);
    let chain = all_redirects(services, schemeful);
    let mut metadata = Metadata {
        null_initiator: !services.has_initiator(),
        redirect: redirect_type(services.chain_len() == 1, initiator, site, chain),
        ..Metadata::default()
    };
    if !site {
        return (0, metadata);
    }
    let mut use_strict = false;
    if initiator {
        if chain {
            return (3, metadata);
        }
        use_strict = !services.considers_redirect_chain();
    }
    if !http || main {
        if initiator {
            metadata.downgrade = 1;
        }
        (if use_strict { 3 } else { 2 }, metadata)
    } else {
        if initiator {
            metadata.downgrade = 2;
        }
        (if use_strict { 3 } else { 0 }, metadata)
    }
}
fn normalize_for_set(context: &mut i32, metadata: &mut Metadata) {
    if *context == 3 {
        *context = 2;
    }
    metadata.downgrade = match metadata.downgrade {
        1 => 0,
        2 => 3,
        other => other,
    };
}
// kind 0=request,1=script-get,2=response,3=script-set,4=subresource.
pub fn same_site_context(
    kind: u32,
    method: &[u8],
    main: bool,
    force: bool,
    ignore_unsafe: bool,
    services: &mut impl SameSiteProvider,
) -> Context {
    let setting = kind == 2 || kind == 3;
    if force {
        return if setting { Context::inclusive_for_set() } else { Context::inclusive() };
    }
    if kind == 4 {
        if !services.first_party(0, false) {
            return Context::default();
        }
        return if services.first_party(0, true) {
            Context::inclusive()
        } else {
            Context { context: 3, ..Context::default() }
        };
    }
    if kind == 2 {
        let _ = services.chain_len();
    }
    if kind == 2 && main && !services.site_for_cookies_null() {
        services.check_response_navigation(services.chain_len().saturating_sub(1));
        let mut result = Context::inclusive_for_set();
        for schemeful in [false, true] {
            let request = services.chain_len().saturating_sub(1);
            let initiator =
                !services.has_initiator() || services.initiator_first_party(request, schemeful);
            let chain = all_redirects(services, schemeful);
            let metadata = Metadata {
                null_initiator: !services.has_initiator(),
                redirect: redirect_type(services.chain_len() == 1, initiator, true, chain),
                ..Metadata::default()
            };
            if schemeful {
                result.schemeful_metadata = metadata;
            } else {
                result.metadata = metadata;
            }
        }
        return result;
    }
    let http = kind == 0 || kind == 2;
    let (mut context, mut metadata) = compute_context(services, http, main, false);
    let (mut schemeful_context, mut schemeful_metadata) =
        compute_context(services, http, main, true);
    if setting {
        normalize_for_set(&mut context, &mut metadata);
        normalize_for_set(&mut schemeful_context, &mut schemeful_metadata);
    } else if kind == 0 && !ignore_unsafe && !method_safe(method) {
        if context == 2 {
            context = 1;
        }
        if schemeful_context == 2 {
            schemeful_context = 1;
        }
    }
    Context { context, schemeful_context, metadata, schemeful_metadata }
}
pub fn method_safe(method: &[u8]) -> bool {
    matches!(method,b"GET"|b"HEAD"|b"OPTIONS"|b"TRACE")
}
pub fn http_method(method: &[u8]) -> i32 {
    match method {
        b"GET" => 1,
        b"HEAD" => 2,
        b"POST" => 3,
        b"PUT" => 4,
        b"DELETE" => 5,
        b"CONNECT" => 6,
        b"OPTIONS" => 7,
        b"TRACE" => 8,
        b"PATCH" => 9,
        _ => 0,
    }
}
pub fn storage_override(api_status: i32, has_initiator: bool, same_origin: bool) -> bool {
    storage_override_lazy(api_status, has_initiator, || same_origin)
}
pub fn storage_override_lazy(
    api_status: i32,
    present: bool,
    same_origin: impl FnOnce() -> bool,
) -> bool {
    api_status == 1 && present && same_origin()
}
pub fn feature(kind: u32, mut read: impl FnMut(u32) -> bool) -> bool {
    match kind {
        0 | 1 => read(kind),
        2 => read(0) || read(1),
        3 => read(1) && read(2),
        4 => read(3) && read(4),
        _ => false,
    }
}
pub trait Lists {
    fn len(&mut self, excluded: bool) -> usize;
    fn included(&mut self, index: usize, excluded: bool) -> bool;
    fn order_facts(&mut self, left: usize, right: usize) -> (usize, usize, i64, i64);
    fn check(&mut self, id: u32, valid: bool);
    fn copy_cookie(&mut self, index: usize);
}
pub fn strip_access_results(services: &mut impl Lists) {
    for index in 0..services.len(false) {
        services.copy_cookie(index);
    }
}
pub fn check_lists(services: &mut impl Lists) {
    let included = services.len(false);
    let excluded = services.len(true);
    let all = (0..included).all(|index| services.included(index, false));
    services.check(0, all);
    let none = (0..excluded).all(|index| !services.included(index, true));
    services.check(1, none);
    let sorted = (1..included).all(|index| {
        let (a, b, creation_a, creation_b) = services.order_facts(index, index - 1);
        !(a > b || (a == b && creation_a < creation_b))
    });
    services.check(2, sorted);
}
pub fn name_value_size(name: usize, value: usize) -> Option<usize> {
    name.checked_add(value)
}
pub fn cookie_port_histogram(port: i32, localhost: bool) -> Option<&'static str> {
    if port == -1 {
        None
    } else if localhost {
        Some("Cookie.Port.OmniboxURLNavigation.Localhost")
    } else {
        Some("Cookie.Port.OmniboxURLNavigation.RemoteHost")
    }
}
