// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Prepared ordinary tests. These sources have not been compiled or executed.
#![forbid(unsafe_code)]
use crate::{base, util, status, types};
use types::{Context, Options, AccessParams, Url};

fn url() -> Url<'static> {
    Url {
        identity: 0,
        scheme: b"https",
        host: b"example.com",
        path: b"/a/b",
        standard: true,
        is_ip: false,
        cryptographic: true,
        localhost: false,
        host_compliant: true,
        effective_port: 443,
    }
}
#[test]
fn domain_and_path_boundaries_match_cookie_host_flag_representation() {
    assert!(util::is_domain_match(b".example.com",b"example.com"));
    assert!(util::is_domain_match(b".example.com",b"sub.example.com"));
    assert!(!util::is_domain_match(b"example.com",b"sub.example.com"));
    assert!(!util::is_domain_match(b".example.com",b"otherexample.com"));
    assert!(util::is_domain_match(b"",b""));
    assert!(util::is_on_path(b"/a",b"/a/b"));
    assert!(!util::is_on_path(b"/a",b"/ab"));
    assert!(!util::is_on_path(b"",b"/"));
    assert_eq!(util::canon_path(b"/a/b",b""),b"/a");
    assert_eq!(util::canon_path(b"no-slash",b"relative"),b"/");
    assert_eq!(util::canon_path(b"/ignored",b"/explicit\0bytes"),b"/explicit\0bytes");
}
#[test]
fn prefix_specificity_case_and_storage_host_constraints() {
    assert_eq!(util::cookie_prefix(b"__host-http-Foo"),4);
    assert_eq!(util::cookie_prefix(b"__HOST-Foo"),2);
    assert!(util::hidden_prefix(b" \t__SeCuRe-value"));
    assert!(!util::hidden_prefix(b"\r__Secure-value"));
    assert!(util::prefix_valid(4,Some(url()),true,true,b"",b"/"));
    assert!(!util::prefix_valid(4,Some(url()),true,false,b"",b"/"));
    assert!(!util::prefix_valid(2,None,true,false,b".example.com",b"/"));
    assert!(util::prefix_valid(2,None,true,false,b"example.com",b"/"));
    assert!(!util::prefix_valid(2,None,true,false,b"",b"/"));
}
#[test]
fn partitioned_validation_distinguishes_nonce_and_optional_url() {
    let insecure = Url { cryptographic: false, localhost: false, ..url() };
    assert!(util::partitioned_valid(Some(insecure),false,false,false));
    assert!(util::partitioned_valid(Some(insecure),false,true,true));
    assert!(!util::partitioned_valid(Some(insecure),true,true,false));
    assert!(util::partitioned_valid(None,true,true,false));
    assert!(!util::partitioned_valid(None,false,true,false));
}
#[test]
fn request_parser_keeps_completed_pairs_before_bad_quote_and_raw_bytes() {
    let mut pairs = vec![(b"existing".to_vec(),b"keep".to_vec())];
    util::parse_request_line(b" first=one; second=\"unterminated", |a, b| {
        pairs.push((a.to_vec(), b.to_vec()))
    });
    assert_eq!(pairs,vec![(b"existing".to_vec(),b"keep".to_vec()),(b"first".to_vec(),b"one".to_vec())]);
    let mut pairs = Vec::new();
    util::parse_request_line(b"a=\"quoted\"; b=\xff\0; lonely", |a, b| {
        pairs.push((a.to_vec(), b.to_vec()))
    });
    assert_eq!(pairs,vec![(b"a".to_vec(),b"\"quoted\"".to_vec()),(b"b".to_vec(),b"\xff\0".to_vec()),(b"lonely".to_vec(),Vec::new())]);
    let serialized =
        util::serialize_request_line(pairs.iter().map(|(a, b)| (a.as_slice(), b.as_slice())));
    assert_eq!(serialized,b"a=\"quoted\"; b=\xff\0; lonely=");
}
struct Domain {
    calls: Vec<Vec<u8>>,
}
impl util::DomainProvider for Domain {
    fn canonical_host(&mut self, host: &[u8]) -> Vec<u8> {
        self.calls.push(host.to_vec());
        host.to_ascii_lowercase()
    }
    fn registry(&mut self, host: &[u8]) -> Vec<u8> {
        if host.ends_with(b"example.com") {
            b"example.com".to_vec()
        } else {
            Vec::new()
        }
    }
    fn check_host_only(&mut self, value: bool) {
        assert!(value);
    }
}
#[test]
fn domain_rejection_preserves_existing_status_and_correct_non_ascii_reason() {
    let mut provider = Domain { calls: Vec::new() };
    let mut state = status::apply(status::State::default(), 6, 6, 0);
    assert!(util::cookie_domain(url(),b"\xff",true,&mut state,&mut provider).is_none());
    assert!(status::query(state,1,19,0));
    assert!(status::query(state,5,6,0));
    assert!(!status::query(state,5,9,0));
    assert!(provider.calls.is_empty());
    let mut state = status::State::default();
    assert_eq!(util::cookie_domain(url(),b".EXAMPLE.COM",true,&mut state,&mut provider),Some(b".example.com".to_vec()));
}
struct Calendar {
    last: Option<util::Calendar>,
    implode: Option<i64>,
    valid: bool,
    bounds: (i32, i32),
}
impl util::CalendarProvider for Calendar {
    fn implode(&mut self, value: util::Calendar) -> Option<i64> {
        self.last = Some(value);
        self.implode
    }
    fn valid_values(&mut self, _: util::Calendar) -> bool {
        self.valid
    }
    fn year_bounds(&self) -> (i32, i32) {
        self.bounds
    }
}
#[test]
fn expiration_uses_first_fields_century_rules_and_nonnull_success() {
    let mut service = Calendar { last: None, implode: Some(0), valid: true, bounds: (1601, 30827) };
    assert_eq!(util::parse_expiration(b"Wed, 09 Jun 21 10:18:14 GMT",&mut service),1);
    let got = service.last.unwrap();
    assert_eq!((got.year,got.month,got.day,got.hour,got.minute,got.second),(2021,6,9,10,18,14));
    assert_eq!(util::parse_expiration(b"incomplete 09 Jun 2021",&mut service),0);
}
#[test]
fn expiration_saturation_only_after_native_failure_and_valid_values() {
    let mut service = Calendar { last: None, implode: None, valid: true, bounds: (1601, 30827) };
    assert_eq!(util::parse_expiration(b"01 Jan 1600 00:00:00",&mut service),1);
    assert_eq!(util::parse_expiration(b"01 Jan 99999 00:00:00",&mut service),i64::MAX);
    service.valid = false;
    assert_eq!(util::parse_expiration(b"01 Jan 99999 00:00:00",&mut service),0);
}
struct Sites {
    chain: Vec<(bool, bool)>,
    initiator: Option<(bool, bool)>,
    null: bool,
    redirects: bool,
}
impl util::SameSiteProvider for Sites {
    fn chain_len(&self) -> usize {
        self.chain.len()
    }
    fn has_initiator(&self) -> bool {
        self.initiator.is_some()
    }
    fn first_party(&mut self, index: usize, schemeful: bool) -> bool {
        if schemeful {
            self.chain[index].1
        } else {
            self.chain[index].0
        }
    }
    fn initiator_first_party(&mut self, _: usize, schemeful: bool) -> bool {
        let pair = self.initiator.unwrap();
        if schemeful {
            pair.1
        } else {
            pair.0
        }
    }
    fn site_for_cookies_null(&mut self) -> bool {
        self.null
    }
    fn check_main_navigation(&mut self, first: bool, _: usize) {
        assert!(first||self.null);
    }
    fn check_response_navigation(&mut self, index: usize) {
        assert!(self.chain[index].1);
    }
    fn considers_redirect_chain(&mut self) -> bool {
        self.redirects
    }
}
#[test]
fn request_context_records_redirect_downgrade_and_unsafe_method() {
    let mut sites = Sites {
        chain: vec![(true,true),(false,false),(true,true)],
        initiator: Some((true, true)),
        null: false,
        redirects: true,
    };
    let get = util::same_site_context(0, b"GET", true, false, false, &mut sites);
    assert_eq!((get.context,get.schemeful_context,get.metadata.downgrade,get.metadata.redirect),(2,2,1,3));
    let post = util::same_site_context(0, b"POST", true, false, false, &mut sites);
    assert_eq!(post.context,1);
    let response = util::same_site_context(2, b"", true, false, false, &mut sites);
    assert_eq!(response.context,2);
    assert_eq!(response.metadata.downgrade,0);
}
#[test]
fn force_context_does_not_touch_empty_chain_and_subresource_is_schemeful() {
    let mut sites = Sites { chain: Vec::new(), initiator: None, null: true, redirects: true };
    assert_eq!(util::same_site_context(0,b"POST",false,true,false,&mut sites),Context::inclusive());
    sites.chain = vec![(true,false)];
    let sub = util::same_site_context(4, b"", false, false, false, &mut sites);
    assert_eq!((sub.context,sub.schemeful_context),(3,0));
}
struct Base {
    secure: bool,
    httponly: bool,
    same_site: i32,
    scheme: i32,
    port: i32,
    options: Options,
    params: AccessParams,
    threshold: i64,
    age: i64,
    reenter: bool,
    threshold_calls: usize,
    logs: Vec<u32>,
    flags: [bool; 3],
}
impl Default for Base {
    fn default() -> Self {
        Self {
            secure: true,
            httponly: false,
            same_site: -1,
            scheme: 2,
            port: 443,
            options: Options { context: Context::inclusive(), ..Options::original_default() },
            params: AccessParams {
                access_semantics: 0,
                scope_semantics: 0,
                delegate_trustworthy: false,
            },
            threshold: 120,
            age: 100,
            reenter: false,
            threshold_calls: 0,
            logs: Vec::new(),
            flags: [true, true, true],
        }
    }
}
impl base::BaseProvider for Base {
    fn secure(&mut self) -> bool {
        self.secure
    }
    fn httponly(&mut self) -> bool {
        self.httponly
    }
    fn same_site(&mut self) -> i32 {
        self.same_site
    }
    fn source_scheme(&mut self) -> i32 {
        self.scheme
    }
    fn source_port(&mut self) -> i32 {
        self.port
    }
    fn domain(&mut self) -> Vec<u8> {
        b"example.com".to_vec()
    }
    fn path(&mut self) -> Vec<u8> {
        b"/a".to_vec()
    }
    fn options(&mut self) -> Options {
        self.options
    }
    fn params(&mut self) -> AccessParams {
        self.params
    }
    fn feature(&mut self, id: u32) -> bool {
        self.flags[id as usize]
    }
    fn age_threshold(&mut self) -> i64 {
        self.threshold_calls += 1;
        if self.reenter {
            self.secure = false;
            self.same_site = 0;
            self.options.context = Context::default();
            self.params.access_semantics = 1;
            self.params.scope_semantics = 1;
        }
        self.threshold
    }
    fn now_minus_creation(&mut self) -> i64 {
        self.age
    }
    fn check(&mut self, _: u32, value: bool) {
        assert!(value);
    }
    fn log(&mut self, id: u32) {
        self.logs.push(id);
    }
}
#[test]
fn whole_inclusion_uses_lax_allow_unsafe_and_specific_warning() {
    let mut service = Base::default();
    service.options.context = Context { context: 1, schemeful_context: 1, ..Context::default() };
    let (result, context) = base::include(url(), &mut service);
    assert_eq!((result.status.exclusions,result.effective_same_site,context),(0,3,1));
    assert!(status::query(result.status,5,2,0));
    assert!(!status::query(result.status,5,0,0));
    assert_eq!(service.threshold_calls,1);
}
#[test]
fn unrelated_exclusion_clears_samesite_warnings_after_full_pipeline() {
    let mut service = Base::default();
    service.httponly = true;
    service.options.context = Context::default();
    let (result, _) = base::include(url(), &mut service);
    assert!(status::query(result.status,1,1,0));
    assert!(status::query(result.status,1,7,0));
    assert!(!status::query(result.status,5,0,0));
}
#[test]
fn virtual_threshold_reentry_refreshes_later_fields_without_recomputing_earlier_context() {
    let mut service = Base::default();
    service.reenter = true;
    let (result, context) = base::include(url(), &mut service);
    assert_eq!(context,3);
    assert_eq!(result.effective_same_site,0);
    assert_eq!((result.access_semantics,result.scope_semantics),(1,1));
    assert!(status::query(result.status,5,1,0));
    assert_eq!(result.status.exclusions,0);
}
#[test]
fn set_preserves_chained_scope_and_exclusions_and_calls_effective_once() {
    let mut service = Base::default();
    let mut initial = base::AccessResult::default();
    initial.scope_semantics = 1;
    initial.status = status::apply(initial.status, 0, 9, 0);
    let result = base::set_permitted(url(), false, Some(initial), &mut service);
    assert_eq!(result.scope_semantics,1);
    assert_eq!(result.access_semantics,0);
    assert!(status::query(result.status,1,9,0));
    assert!(status::query(result.status,1,11,0));
    assert_eq!(service.threshold_calls,1);
}
#[test]
fn unique_key_policy_respects_host_domain_and_feature_axes() {
    assert_eq!(base::key_policy(0,true,true,true),base::KeyPolicy {kind:2,scheme:true,port:false});
    assert_eq!(base::key_policy(0,false,false,true),base::KeyPolicy {kind:1,scheme:false,port:true});
    assert_eq!(base::key_policy(1,true,false,false),base::KeyPolicy {kind:0,scheme:true,port:true});
    assert_eq!(base::key_policy(2,false,true,true),base::KeyPolicy {kind:3,scheme:false,port:false});
    assert_eq!(base::source_port(-1),-1);
    assert_eq!(base::source_port(65535),65535);
    assert_eq!(base::source_port(65536),-2);
}
