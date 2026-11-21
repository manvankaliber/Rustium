// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]
use crate::status::{self, State};
use crate::types::{AccessParams, Context, Options, Url, PORT_INVALID, PORT_UNSPECIFIED};
use crate::util;

#[derive(Clone, Copy, Debug)]
pub struct Cookie<'a> {
    pub name: &'a [u8],
    pub domain: &'a [u8],
    pub path: &'a [u8],
    pub creation: i64,
    pub secure: bool,
    pub httponly: bool,
    pub same_site: i32,
    pub partitioned: bool,
    pub partition_nonce: bool,
    pub source_scheme: i32,
    pub source_port: i32,
}
#[derive(Clone, Copy, Debug)]
#[repr(C)]
pub struct AccessResult {
    pub status: State,
    pub effective_same_site: i32,
    pub access_semantics: i32,
    pub scope_semantics: i32,
    pub allowed_secure: bool,
}
impl PartialEq for AccessResult {
    fn eq(&self, other: &Self) -> bool {
        status::same(self.status, other.status)
            && self.effective_same_site == other.effective_same_site
            && self.access_semantics == other.access_semantics
            && self.scope_semantics == other.scope_semantics
            && self.allowed_secure == other.allowed_secure
    }
}
impl Eq for AccessResult {}
impl Default for AccessResult {
    fn default() -> Self {
        Self {
            status: State::default(),
            effective_same_site: 4,
            access_semantics: -1,
            scope_semantics: -1,
            allowed_secure: false,
        }
    }
}

// Getter methods deliberately return owned/scalar values. A native virtual
// threshold/log hook can reenter and mutate an originally nonconst object via
// another alias; no native borrowed byte references are kept across effects.
// Safe Rust callers implement this directly over their model and shared util.
pub trait BaseProvider {
    fn secure(&mut self) -> bool;
    fn httponly(&mut self) -> bool;
    fn same_site(&mut self) -> i32;
    fn source_scheme(&mut self) -> i32;
    fn source_port(&mut self) -> i32;
    fn domain(&mut self) -> Vec<u8>;
    fn path(&mut self) -> Vec<u8>;
    fn options(&mut self) -> Options;
    fn params(&mut self) -> AccessParams;
    fn feature(&mut self, id: u32) -> bool; // 0 scheme,1 port,2 compliant-host.
    fn age_threshold(&mut self) -> i64; // Original virtual effect and order.
    fn now_minus_creation(&mut self) -> i64; // Saturating native Time arithmetic.
    fn check(&mut self, id: u32, condition: bool);
    fn log(&mut self, id: u32);
}
fn exclude(status_out: &mut State, reason: i32) {
    *status_out = status::apply(*status_out, 0, reason, 0);
}
fn warn(status_out: &mut State, reason: i32) {
    *status_out = status::apply(*status_out, 6, reason, 0);
}
fn has_exclusion(state: State, reason: i32) -> bool {
    status::query(state, 1, reason, 0)
}

pub fn source_port(port: i32) -> i32 {
    if (0..=65535).contains(&port) || port == PORT_UNSPECIFIED {
        port
    } else {
        PORT_INVALID
    }
}
pub fn is_secure(secure: bool, scheme_bound: bool, source_scheme: i32) -> bool {
    secure || (scheme_bound && source_scheme == 2)
}
pub fn is_secure_with_feature(
    secure: bool,
    source_scheme: i32,
    feature: impl FnOnce() -> bool,
) -> bool {
    secure || (feature() && source_scheme == 2)
}
pub trait SchemeProvider {
    fn scheme_count(&mut self) -> usize;
    fn scheme(&mut self, index: usize) -> Vec<u8>;
}
pub fn cookieable_scheme(scheme: &[u8], services: &mut impl SchemeProvider) -> bool {
    (0..services.scheme_count()).any(|index| services.scheme(index) == scheme)
}
pub fn effective_from_facts(same_site: i32, semantics: i32, recently_created: bool) -> i32 {
    match same_site {
        -1 => {
            if semantics == 1 {
                0
            } else if recently_created {
                3
            } else {
                1
            }
        }
        0 => 0,
        1 => 1,
        2 => 2,
        _ => 4,
    }
}
pub fn effective_same_site(semantics: i32, services: &mut impl BaseProvider) -> i32 {
    let threshold = services.age_threshold();
    let same_site = services.same_site();
    let recent = same_site == -1 && semantics != 1 && services.now_minus_creation() <= threshold;
    effective_from_facts(same_site, semantics, recent)
}
pub fn apply_same_site_warnings(
    same_site: i32,
    effective: i32,
    secure: bool,
    context: Context,
    status_out: &mut State,
    setting: bool,
) {
    if same_site == -1 && context.schemeful_context < 2 {
        warn(status_out, 0);
    }
    if effective == 3 && context.schemeful_context == 1 {
        *status_out = status::apply(*status_out, 7, 0, 0);
        warn(status_out, 2);
    }
    if same_site == 0 && !secure {
        warn(status_out, 1);
    }
    let downgrade = context.schemeful_metadata.downgrade;
    let changed = match effective {
        2 => {
            if setting {
                downgrade == 3
            } else {
                downgrade == 1 || downgrade == 2
            }
        }
        1 | 3 => downgrade == if setting { 3 } else { 2 },
        _ => false,
    };
    if changed {
        warn(status_out, 4);
        if context.schemeful_metadata.null_initiator {
            warn(status_out, 5);
        }
    }
    *status_out = status::apply(*status_out, 4, 0, 0);
}
fn inclusion_context(services: &mut impl BaseProvider) -> i32 {
    let legacy = services.params().access_semantics == 1;
    let context = services.options().context;
    if !legacy {
        services.check(1, context.schemeful_context <= context.context);
    }
    context.inclusion(if legacy { 1 } else { 0 })
}
fn effective_and_check(services: &mut impl BaseProvider) -> i32 {
    let semantics = services.params().access_semantics;
    let effective = effective_same_site(semantics, services);
    services.check(2, effective != 4);
    effective
}
fn check_warning_context(
    same_site: i32,
    effective: i32,
    context: Context,
    services: &mut impl BaseProvider,
) {
    // The original warning helper requests schemeful inclusion context even
    // under LEGACY semantics, and requests it again for lax-allow-unsafe.
    if same_site == -1 {
        services.check(1, context.schemeful_context <= context.context);
    }
    if effective == 3 {
        services.check(1, context.schemeful_context <= context.context);
    }
}
pub fn include(url: Url<'_>, services: &mut impl BaseProvider) -> (AccessResult, i32) {
    let mut out = State::default();
    if services.options().exclude_httponly && services.httponly() {
        exclude(&mut out, 1);
    }
    let mut scheme = util::provisional_access_scheme(url);
    if scheme == 0 && services.params().delegate_trustworthy {
        scheme = 2;
    }
    let allowed = scheme != 0;
    match scheme {
        0 => {
            if services.secure() {
                exclude(&mut out, 2);
            }
        }
        2 => {
            if services.secure()
                || (services.feature(0)
                    && services.source_scheme() == 2
                    && services.params().scope_semantics != 1)
            {
                warn(&mut out, 3);
            }
        }
        _ => {}
    }
    if services.source_scheme() == 2 && scheme == 0 && !has_exclusion(out, 2) {
        if services.feature(0) && services.params().scope_semantics != 1 {
            exclude(&mut out, 21);
        } else {
            warn(&mut out, 9);
        }
    } else if services.source_scheme() == 1 && scheme == 1 {
        if services.feature(0) && services.params().scope_semantics != 1 {
            exclude(&mut out, 21);
        } else {
            warn(&mut out, 9);
        }
    }
    services.check(0, url.effective_port != PORT_INVALID);
    let port_matches = url.effective_port == services.source_port()
        || services.source_port() == PORT_UNSPECIFIED
        || !util::domain_is_host_only(&services.domain());
    let trustworthy_443 = scheme == 2 && services.source_port() == 443;
    if !port_matches && !trustworthy_443 {
        if services.feature(1) && services.params().scope_semantics != 1 {
            exclude(&mut out, 20);
        } else {
            warn(&mut out, 8);
        }
    }
    if !util::url_host_valid(url) {
        if services.feature(2) {
            exclude(&mut out, 29);
        } else {
            warn(&mut out, 13);
        }
    }
    if !util::is_domain_match(&services.domain(), url.host) {
        exclude(&mut out, 3);
    }
    if !util::is_on_path(&services.path(), url.path) {
        exclude(&mut out, 4);
    }
    let context = inclusion_context(services);
    let effective = effective_and_check(services);
    match effective {
        2 => {
            if context < 3 {
                exclude(&mut out, 5);
            }
        }
        1 => {
            if context < 2 {
                exclude(&mut out, if services.same_site() == -1 { 7 } else { 6 });
            }
        }
        3 => {
            let unspecified = services.same_site() == -1;
            services.check(3, unspecified);
            if context < 1 {
                exclude(&mut out, 7);
            }
        }
        _ => {}
    }
    if services.params().access_semantics != 1 && services.same_site() == 0 && !services.secure() {
        exclude(&mut out, 8);
    }
    let same_site = services.same_site();
    let secure = services.secure();
    let current_context = services.options().context;
    check_warning_context(same_site, effective, current_context, services);
    apply_same_site_warnings(same_site, effective, secure, current_context, &mut out, false);
    let access = services.params().access_semantics;
    let scope = services.params().scope_semantics;
    (
        AccessResult {
            status: out,
            effective_same_site: effective,
            access_semantics: access,
            scope_semantics: scope,
            allowed_secure: allowed,
        },
        context,
    )
}
pub fn set_permitted(
    url: Url<'_>,
    cookieable: bool,
    initial: Option<AccessResult>,
    services: &mut impl BaseProvider,
) -> AccessResult {
    let mut out = initial.unwrap_or_default();
    if !cookieable {
        exclude(&mut out.status, 11);
    }
    if !util::url_host_valid(url) {
        if services.feature(2) {
            exclude(&mut out.status, 29);
        } else {
            warn(&mut out.status, 13);
        }
    }
    if !util::is_domain_match(&services.domain(), url.host) {
        exclude(&mut out.status, 3);
    }
    let mut scheme = util::provisional_access_scheme(url);
    if scheme == 0 && services.params().delegate_trustworthy {
        scheme = 2;
    }
    out.allowed_secure = scheme != 0;
    match scheme {
        0 => {
            if services.secure() {
                exclude(&mut out.status, 2);
            }
        }
        2 => {
            if services.secure() {
                warn(&mut out.status, 3);
            }
        }
        _ => {}
    }
    out.access_semantics = services.params().access_semantics;
    if services.options().exclude_httponly && services.httponly() {
        services.log(0);
        exclude(&mut out.status, 1);
    }
    if services.params().access_semantics != 1 && services.same_site() == 0 && !services.secure() {
        services.log(1);
        exclude(&mut out.status, 8);
    }
    let context = inclusion_context(services);
    out.effective_same_site = effective_and_check(services);
    match out.effective_same_site {
        2 => {
            if context < 2 {
                services.log(2);
                exclude(&mut out.status, 5);
            }
        }
        1 | 3 => {
            if context < 2 {
                if services.same_site() == -1 {
                    services.log(3);
                    exclude(&mut out.status, 7);
                } else {
                    services.log(4);
                    exclude(&mut out.status, 6);
                }
            }
        }
        _ => {}
    }
    let same_site = services.same_site();
    let secure = services.secure();
    let current_context = services.options().context;
    check_warning_context(same_site, out.effective_same_site, current_context, services);
    apply_same_site_warnings(
        same_site,
        out.effective_same_site,
        secure,
        current_context,
        &mut out.status,
        true,
    );
    out
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(C)]
pub struct KeyPolicy {
    pub kind: i32,
    pub scheme: bool,
    pub port: bool,
}
// mode0=current unique/ref,1=strict,2=legacy. Native key constructors are only
// ownership/materialization services; the Rust result selects their policy.
pub fn key_policy(
    mode: u32,
    domain_cookie: bool,
    scheme_bound: bool,
    port_bound: bool,
) -> KeyPolicy {
    if mode == 1 {
        KeyPolicy { kind: 0, scheme: true, port: true }
    } else if mode == 2 {
        KeyPolicy { kind: 3, scheme: false, port: false }
    } else if domain_cookie {
        KeyPolicy { kind: 2, scheme: scheme_bound, port: false }
    } else {
        KeyPolicy { kind: 1, scheme: scheme_bound, port: port_bound }
    }
}
pub fn key_policy_with_features(
    mode: u32,
    domain: &[u8],
    mut feature: impl FnMut(u32) -> bool,
) -> KeyPolicy {
    let domain_cookie = !util::domain_is_host_only(domain);
    let scheme = mode == 0 && feature(1);
    let port = mode == 0 && !domain_cookie && feature(0);
    key_policy(mode, domain_cookie, scheme, port)
}
pub fn first_party_partitioned(
    partitioned: bool,
    nonce: bool,
    same_site: impl FnOnce() -> bool,
) -> bool {
    partitioned && !nonce && same_site()
}
pub trait PartitionProvider {
    fn partition_domain(&mut self) -> Vec<u8>;
    fn partition_same_site(&mut self, url: &[u8]) -> bool;
}
pub fn partition_metric(
    third_party: bool,
    present: bool,
    nonce: bool,
    services: &mut impl PartitionProvider,
) -> bool {
    let first = if present && !nonce {
        let domain = services.partition_domain();
        let mut url = b"https://".to_vec();
        url.extend_from_slice(util::cookie_domain_as_host(&domain));
        services.partition_same_site(&url)
    } else {
        false
    };
    if third_party {
        present && !first
    } else {
        first
    }
}
