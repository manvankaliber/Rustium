// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![deny(unsafe_op_in_unsafe_fn)]
use std::ffi::c_void;
use crate::{base, collection, options, partition, status, util};
use crate::types::{AccessParams, Context, Options, Url};
#[repr(C)]
#[derive(Clone, Copy)]
pub struct View {
    pub(crate) data: *const u8,
    pub(crate) size: usize,
}
impl Default for View {
    fn default() -> Self {
        Self { data: std::ptr::null(), size: 0 }
    }
}
impl View {
    pub(crate) fn from_bytes(bytes: &[u8]) -> Self {
        Self { data: bytes.as_ptr(), size: bytes.len() }
    }
    pub(crate) unsafe fn copy(self) -> Vec<u8> {
        if self.size == 0 {
            Vec::new()
        } else {
            // SAFETY: input contract supplies initialized, readable live bytes.
            unsafe { std::slice::from_raw_parts(self.data, self.size) }.to_vec()
        }
    }
}
#[repr(C)]
pub struct Buffer {
    pub(crate) data: *mut u8,
    pub(crate) size: usize,
    pub(crate) capacity: usize,
    pub(crate) present: u32,
}
impl Default for Buffer {
    fn default() -> Self {
        Self { data: std::ptr::null_mut(), size: 0, capacity: 0, present: 0 }
    }
}
impl Buffer {
    pub(crate) fn owned(bytes: Option<Vec<u8>>) -> Self {
        match bytes {
            None => Self::default(),
            Some(mut bytes) => {
                let out = Self {
                    data: bytes.as_mut_ptr(),
                    size: bytes.len(),
                    capacity: bytes.capacity(),
                    present: 1,
                };
                std::mem::forget(bytes);
                out
            }
        }
    }
}
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct UrlView {
    pub(crate) identity: usize,
    pub(crate) scheme: View,
    pub(crate) host: View,
    pub(crate) path: View,
    pub(crate) standard: bool,
    pub(crate) is_ip: bool,
    pub(crate) cryptographic: bool,
    pub(crate) localhost: bool,
    pub(crate) host_compliant: bool,
    pub(crate) effective_port: i32,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Request {
    pub(crate) first: View,
    pub(crate) second: View,
    pub(crate) third: View,
    pub(crate) url: UrlView,
    pub(crate) scalar: [i64; 8],
    pub(crate) context: Context,
    pub(crate) access: base::AccessResult,
    pub(crate) other_access: base::AccessResult,
}
#[repr(C)]
pub struct Output {
    pub(crate) buffer: Buffer,
    pub(crate) scalar: i64,
    pub(crate) context: Context,
    pub(crate) access: base::AccessResult,
    pub(crate) options: Options,
    pub(crate) key: base::KeyPolicy,
}
impl Default for Output {
    fn default() -> Self {
        Self {
            buffer: Buffer::default(),
            scalar: 0,
            context: Context::default(),
            access: base::AccessResult::default(),
            options: Options::default(),
            key: base::KeyPolicy { kind: 0, scheme: false, port: false },
        }
    }
}
#[repr(C)]
#[derive(Default)]
pub struct Call {
    pub(crate) operation: u32,
    pub(crate) first: i64,
    pub(crate) second: i64,
    pub(crate) bytes: View,
    pub(crate) bytes2: View,
    pub(crate) calendar: util::Calendar,
}
#[repr(C)]
#[derive(Default)]
pub struct NativeResult {
    pub(crate) number: i64,
    pub(crate) number2: i64,
    pub(crate) number3: i64,
    pub(crate) number4: i64,
    pub(crate) bytes: View,
    pub(crate) bytes2: View,
    pub(crate) context: Context,
    pub(crate) options: Options,
    pub(crate) params: AccessParams,
    pub(crate) failed: u32,
}
const _: () = assert!(std::mem::size_of::<Context>()==32);
const _: () = assert!(std::mem::size_of::<Options>()==40);
const _: () = assert!(std::mem::size_of::<AccessParams>()==12);
const _: () = assert!(std::mem::size_of::<base::AccessResult>()==40);
const _: () = assert!(std::mem::size_of::<NativeResult>()==120+4*std::mem::size_of::<usize>());
pub(crate) type Callback = unsafe extern "C" fn(*mut c_void, *const Call, *mut NativeResult);
pub(crate) struct Native {
    pub(crate) context: *mut c_void,
    pub(crate) callback: Callback,
    pub(crate) failed: std::cell::Cell<bool>,
}
impl Native {
    pub(crate) fn run(
        &mut self,
        operation: u32,
        first: i64,
        second: i64,
        bytes: &[u8],
        bytes2: &[u8],
        calendar: util::Calendar,
    ) -> NativeResult {
        let call = Call {
            operation,
            first,
            second,
            bytes: View::from_bytes(bytes),
            bytes2: View::from_bytes(bytes2),
            calendar,
        };
        self.invoke(&call)
    }
    fn invoke(&self, call: &Call) -> NativeResult {
        if self.failed.get() {
            return NativeResult::default();
        }
        let mut result = NativeResult::default();
        // SAFETY: synchronous callback uses valid call/context and exclusively
        // initialized result. It retains no addresses; reentry uses a new context.
        unsafe { (self.callback)(self.context, call, &mut result) };
        if result.failed != 0 {
            self.failed.set(true);
            return NativeResult::default();
        }
        result
    }
    pub(crate) fn number(&mut self, operation: u32, first: i64, second: i64) -> i64 {
        self.run(operation, first, second, &[], &[], util::Calendar::default()).number
    }
    pub(crate) fn bytes(&mut self, operation: u32, input: &[u8]) -> Vec<u8> {
        let result = self.run(operation, 0, 0, input, &[], util::Calendar::default());
        // SAFETY: native result bytes live until the next callback; copy now.
        unsafe { result.bytes.copy() }
    }
}
impl util::DomainProvider for Native {
    fn canonical_host(&mut self, host: &[u8]) -> Vec<u8> {
        self.bytes(1, host)
    }
    fn registry(&mut self, host: &[u8]) -> Vec<u8> {
        self.bytes(2, host)
    }
    fn check_host_only(&mut self, value: bool) {
        self.number(3, i64::from(value), 0);
    }
}
impl util::CalendarProvider for Native {
    fn implode(&mut self, calendar: util::Calendar) -> Option<i64> {
        let out = self.run(10, 0, 0, &[], &[], calendar);
        (out.number2 != 0).then_some(out.number)
    }
    fn valid_values(&mut self, calendar: util::Calendar) -> bool {
        self.run(11, 0, 0, &[], &[], calendar).number != 0
    }
    fn year_bounds(&self) -> (i32, i32) {
        // Primitive callback is synchronous and returns only platform constants.
        let call = Call { operation: 12, ..Call::default() };
        let out = self.invoke(&call);
        (out.number as i32, out.number2 as i32)
    }
}
impl util::SameSiteProvider for Native {
    fn chain_len(&self) -> usize {
        let call = Call { operation: 20, ..Call::default() };
        self.invoke(&call).number as usize
    }
    fn has_initiator(&self) -> bool {
        let call = Call { operation: 21, ..Call::default() };
        self.invoke(&call).number != 0
    }
    fn first_party(&mut self, index: usize, schemeful: bool) -> bool {
        self.number(22, index as i64, i64::from(schemeful)) != 0
    }
    fn initiator_first_party(&mut self, index: usize, schemeful: bool) -> bool {
        self.number(23, index as i64, i64::from(schemeful)) != 0
    }
    fn site_for_cookies_null(&mut self) -> bool {
        self.number(24, 0, 0) != 0
    }
    fn check_main_navigation(&mut self, first_party: bool, index: usize) {
        self.number(25, i64::from(first_party), index as i64);
    }
    fn check_response_navigation(&mut self, index: usize) {
        self.number(27, index as i64, 0);
    }
    fn considers_redirect_chain(&mut self) -> bool {
        self.number(26, 0, 0) != 0
    }
}
impl base::BaseProvider for Native {
    fn secure(&mut self) -> bool {
        self.number(100, 0, 0) != 0
    }
    fn httponly(&mut self) -> bool {
        self.number(101, 0, 0) != 0
    }
    fn same_site(&mut self) -> i32 {
        self.number(102, 0, 0) as i32
    }
    fn source_scheme(&mut self) -> i32 {
        self.number(103, 0, 0) as i32
    }
    fn source_port(&mut self) -> i32 {
        self.number(104, 0, 0) as i32
    }
    fn domain(&mut self) -> Vec<u8> {
        self.bytes(105, &[])
    }
    fn path(&mut self) -> Vec<u8> {
        self.bytes(106, &[])
    }
    fn options(&mut self) -> Options {
        self.run(107, 0, 0, &[], &[], util::Calendar::default()).options
    }
    fn params(&mut self) -> AccessParams {
        self.run(108, 0, 0, &[], &[], util::Calendar::default()).params
    }
    fn feature(&mut self, id: u32) -> bool {
        self.number(109, i64::from(id), 0) != 0
    }
    fn age_threshold(&mut self) -> i64 {
        self.number(110, 0, 0)
    }
    fn now_minus_creation(&mut self) -> i64 {
        self.number(111, 0, 0)
    }
    fn check(&mut self, id: u32, condition: bool) {
        self.number(112, i64::from(id), i64::from(condition));
    }
    fn log(&mut self, id: u32) {
        self.number(113, i64::from(id), 0);
    }
}
impl base::SchemeProvider for Native {
    fn scheme_count(&mut self) -> usize {
        self.number(320, 0, 0) as usize
    }
    fn scheme(&mut self, index: usize) -> Vec<u8> {
        let item = self.run(321, index as i64, 0, &[], &[], util::Calendar::default());
        unsafe { item.bytes.copy() }
    }
}
impl base::PartitionProvider for Native {
    fn partition_domain(&mut self) -> Vec<u8> {
        self.bytes(105, &[])
    }
    fn partition_same_site(&mut self, url: &[u8]) -> bool {
        self.run(114, 0, 0, url, &[], util::Calendar::default()).number != 0
    }
}
impl util::RequestPairs for Native {
    fn pair_count(&mut self) -> usize {
        self.number(301, 0, 0) as usize
    }
    fn pair(&mut self, index: usize) -> (Vec<u8>, Vec<u8>) {
        let pair = self.run(302, index as i64, 0, &[], &[], util::Calendar::default());
        (unsafe { pair.bytes.copy() }, unsafe { pair.bytes2.copy() })
    }
}
impl util::Lists for Native {
    fn len(&mut self, excluded: bool) -> usize {
        self.number(307, i64::from(excluded), 0) as usize
    }
    fn included(&mut self, index: usize, excluded: bool) -> bool {
        self.number(308, index as i64, i64::from(excluded)) != 0
    }
    fn order_facts(&mut self, left: usize, right: usize) -> (usize, usize, i64, i64) {
        let pair = self.run(309, left as i64, right as i64, &[], &[], util::Calendar::default());
        (pair.number as usize, pair.number2 as usize, pair.number3, pair.number4)
    }
    fn check(&mut self, id: u32, valid: bool) {
        self.number(310, i64::from(id), i64::from(valid));
    }
    fn copy_cookie(&mut self, index: usize) {
        self.number(305, index as i64, 0);
    }
}
impl collection::Stream for Native {
    fn contains_all(&mut self) -> bool {
        self.number(200, 0, 0) != 0
    }
    fn begin(&mut self) {
        self.number(201, 0, 0);
    }
    fn next(&mut self) -> bool {
        self.number(202, 0, 0) != 0
    }
    fn key(&mut self) {
        self.number(203, 0, 0);
    }
    fn text(&mut self, text: &'static str) {
        self.run(204, 0, 0, text.as_bytes(), &[], util::Calendar::default());
    }
}
struct PartitionServices<'a>(&'a mut Native);
impl partition::Primitives for PartitionServices<'_> {
    fn run(&mut self, operation: partition::Primitive) -> i32 {
        self.0.number(214, operation as i64, 0) as i32
    }
}
impl collection::Keys for Native {
    fn len(&mut self, right: bool) -> usize {
        self.number(210, i64::from(right), 0) as usize
    }
    fn compare_query(&mut self, index: usize) -> i32 {
        self.number(212, index as i64, -1);
        let left = self.number(213, 0, 0) as i32;
        let right = self.number(213, 1, 0) as i32;
        partition::compare(left, right, &mut PartitionServices(self))
    }
    fn equal_at(&mut self, index: usize) -> bool {
        self.number(212, index as i64, index as i64);
        let left = self.number(213, 0, 0) as i32;
        let right = self.number(213, 1, 0) as i32;
        partition::equals(left, right, &mut PartitionServices(self))
    }
}
/// # Safety
/// Request/result are valid initialized disjoint objects. Every view is readable
/// for its length. Native callbacks obey cookie_family_ffi.h; no pointer escapes.
/// Rust copies all incoming bytes before effects can mutate their native owner.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_family_call(
    operation: u32,
    input: *const Request,
    output: *mut Output,
    context: *mut c_void,
    callback: Callback,
) {
    // SAFETY: documented initialized request; copied before any output write.
    let request = unsafe { input.read() };
    let scheme = unsafe { request.url.scheme.copy() };
    let host = unsafe { request.url.host.copy() };
    let path = unsafe { request.url.path.copy() };
    let url = Url {
        identity: request.url.identity,
        scheme: &scheme,
        host: &host,
        path: &path,
        standard: request.url.standard,
        is_ip: request.url.is_ip,
        cryptographic: request.url.cryptographic,
        localhost: request.url.localhost,
        host_compliant: request.url.host_compliant,
        effective_port: request.url.effective_port,
    };
    let values = request.scalar;
    let mut native = Native { context, callback, failed: std::cell::Cell::new(false) };
    // Owned dispatchers preserve their own validity/alias/failure short circuits
    // and decode their request strings only at the original semantic point.
    if (1000..=2999).contains(&operation) {
        let result = if operation < 2000 {
            crate::canonical_dispatch::dispatch(operation, &request, url, &mut native)
        } else {
            crate::monster_dispatch::dispatch(operation, &request, url, &mut native)
        };
        unsafe { output.write(result) };
        return;
    }
    let first = unsafe { request.first.copy() };
    let second = unsafe { request.second.copy() };
    let third = unsafe { request.third.copy() };
    let mut result = Output::default();
    match operation {
        0 => result.scalar = i64::from(util::domain_is_host_only(&first)),
        1 => result.buffer = Buffer::owned(Some(util::cookie_domain_as_host(&first).to_vec())),
        2 => {
            result.buffer =
                Buffer::owned(Some(util::effective_domain(&first, &second, &mut native)))
        }
        3 => {
            let mut status = request.access.status;
            result.buffer = Buffer::owned(util::cookie_domain(
                url,
                &first,
                values[0] != 0,
                &mut status,
                &mut native,
            ));
            result.access.status = status;
        }
        4 => result.scalar = util::parse_expiration(&first, &mut native),
        5 => result.buffer = Buffer::owned(Some(util::canon_path(&first, &second))),
        6 => result.buffer = Buffer::owned(util::domain_path_url(&first, &second, &third)),
        7 => result.scalar = i64::from(util::provisional_access_scheme(url)),
        8 => result.scalar = i64::from(util::is_domain_match(&first, &second)),
        9 => result.scalar = i64::from(util::is_on_path(&first, &second)),
        10 => result.scalar = i64::from(util::cookie_prefix(&first)),
        11 => result.scalar = i64::from(util::hidden_prefix(&first)),
        12 => {
            if !(0..=4).contains(&values[0]) {
                native.number(399, values[0], 0);
            }
            result.scalar = i64::from(util::prefix_valid(
                values[0] as i32,
                (values[1] != 0).then_some(url),
                values[2] != 0,
                values[3] != 0,
                &first,
                &second,
            ));
        }
        13 => {
            result.scalar = i64::from(util::partitioned_valid(
                (values[0] != 0).then_some(url),
                values[1] != 0,
                values[2] != 0,
                values[3] != 0,
            ))
        }
        14 => util::parse_request_line(&first, |name, value| {
            native.run(300, 0, 0, name, value, util::Calendar::default());
        }),
        15 => result.buffer = Buffer::owned(Some(util::serialize_request_pairs(&mut native))),
        16 => {
            result.context = util::same_site_context(
                values[0] as u32,
                &first,
                values[1] != 0,
                values[2] != 0,
                values[3] != 0,
                &mut native,
            )
        }
        17 => {
            result.scalar = i64::from(util::feature(values[0] as u32, |flag| {
                native.number(303, i64::from(flag), 0) != 0
            }))
        }
        18 => result.scalar = i64::from(util::http_method(&first)),
        19 => result.scalar = i64::from(status::query(request.access.status, 0, 0, 0)),
        20 => util::strip_access_results(&mut native),
        21 => {
            if let Some(histogram) = util::cookie_port_histogram(values[0] as i32, values[1] != 0) {
                native.run(306, values[0], 0, histogram.as_bytes(), &[], util::Calendar::default());
            }
        }
        22 => util::check_lists(&mut native),
        23 => {
            result.scalar =
                i64::from(util::feature(4, |flag| native.number(303, i64::from(flag), 0) != 0))
        }
        24 => {
            result.scalar =
                i64::from(util::storage_override_lazy(values[0] as i32, values[1] != 0, || {
                    native.number(313, 0, 0) != 0
                }))
        }
        25 => match util::name_value_size(values[0] as usize, values[1] as usize) {
            Some(size) => result.scalar = size as i64,
            None => {
                native.number(311, 0, 0);
            }
        },
        26 => {
            result.buffer = Buffer::owned(util::domain_path_url(
                &first,
                &second,
                util::source_scheme(values[0] != 0),
            ))
        }
        27 => {
            result.buffer = Buffer::owned(util::domain_path_url(
                &first,
                &second,
                util::source_scheme(values[0] == 2),
            ))
        }
        28 => result.scalar = i64::from(util::url_host_valid(url)),
        29 => result.scalar = i64::from(!util::domain_is_host_only(&first)),
        30 => {
            let (access, context) = base::include(url, &mut native);
            result.access = access;
            result.scalar = i64::from(context);
        }
        31 => {
            let cookieable = base::cookieable_scheme(url.scheme, &mut native);
            result.access = base::set_permitted(
                url,
                cookieable,
                (values[1] != 0).then_some(request.access),
                &mut native,
            );
        }
        32 => result.scalar = i64::from(base::effective_same_site(values[0] as i32, &mut native)),
        33 => {
            result.scalar =
                i64::from(base::is_secure_with_feature(values[0] != 0, values[2] as i32, || {
                    native.number(303, 1, 0) != 0
                }))
        }
        34 | 35 => {
            result.scalar = i64::from(base::partition_metric(
                operation == 35,
                values[0] != 0,
                values[1] != 0,
                &mut native,
            ));
        }
        36 => {
            result.key = base::key_policy_with_features(values[0] as u32, &first, |flag| {
                native.number(303, i64::from(flag), 0) != 0
            });
        }
        37 => result.scalar = i64::from(base::source_port(values[0] as i32)),
        38 => result.scalar = i64::from(native.number(111, 0, 0) <= values[0]),
        40 => result.context = options::inclusive(values[0] != 0),
        41 => {
            let (value, consistent) = options::context_for_inclusion(request.context);
            native.number(112, 1, i64::from(consistent));
            result.scalar = i64::from(value);
        }
        42 => {
            result.scalar = i64::from(options::equivalent(
                request.context,
                native.run(312, 0, 0, &[], &[], util::Calendar::default()).context,
                values[0] != 0,
            ))
        }
        43 => result.options = options::all_inclusive(),
        44 => result.scalar = i64::from(request.access == request.other_access),
        50 => {
            result.scalar = i64::from(collection::is_empty(
                values[0] != 0,
                values[0] != 0 && native.number(208, 0, 0) != 0,
            ))
        }
        51 => result.scalar = i64::from(collection::contains_all(values[0] != 0)),
        52 => result.scalar = i64::from(collection::contains(values[0] != 0, &mut native)),
        53 => collection::matches_site(|bit| {
            native.number(206, i64::from(bit), 0);
        }),
        54 => collection::stream(&mut native),
        55 => {
            result.scalar =
                i64::from(collection::equals(values[0] != 0, values[1] != 0, &mut native))
        }
        _ => {
            native.number(399, i64::from(operation), 0);
        }
    }
    // SAFETY: caller supplies disjoint writable result storage; no references
    // are created over uninitialized caller memory and no input pointer escapes.
    unsafe { output.write(result) };
}
/// # Safety
/// Buffer is an unmodified unique result of this module, released exactly once.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn cookie_family_buffer_free(buffer: Buffer) {
    if buffer.capacity != 0 {
        unsafe { drop(Vec::from_raw_parts(buffer.data, buffer.size, buffer.capacity)) };
    }
}
