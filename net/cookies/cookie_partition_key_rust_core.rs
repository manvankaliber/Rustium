// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]

use std::cmp::Ordering;

// Persisted IDs. C++ static_asserts bind these to AncestorChainBit.
pub const SAME_SITE: i32 = 0;
pub const CROSS_SITE: i32 = 1;
const GENERAL_PARTITION: i32 = 0;

#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Primitive {
    SiteOpaque = 0,
    SiteIsFile = 1,
    SiteSerialize = 2,
    SiteSerializeFileHost = 3,
    SiteDeserialize = 4,
    SerializedEqualsInput = 5,
    NikPartition = 6,
    NikNoncePresent = 7,
    NikSelectTop = 8,
    NikSelectFrame = 9,
    SfcIsNull = 10,
    SfcIsFirstParty = 11,
    SiteEqual = 12,
    SiteCompare = 13,
    LeftNoncePresent = 14,
    RightNoncePresent = 15,
    NonceEqual = 16,
    NonceCompare = 17,
}

// Native services own SchemefulSite/URL/token objects and call-scoped output.
// Only this core selects which primitive to invoke and when. For predicates,
// native services return zero/one; order primitives return a signed ordering.
pub trait Primitives {
    fn run(&mut self, operation: Primitive) -> i32;
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Status {
    Present = 0,
    Absent = 1,
    Error = 2,
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    None = 0,
    NotSerializable = 1,
    UnexpectedEmpty = 2,
    Opaque = 3,
    Malformed = 4,
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SiteSource {
    None = 0,
    Top = 1,
    Frame = 2,
    Parsed = 3,
    Existing = 4,
}

// Call-scoped result only, never persisted or overlaid on the public object.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Result {
    pub status: i32,
    pub ancestor: i32,
    pub site_source: u32,
    pub error: u32,
    pub warning_count: u32,
}

impl Result {
    fn present(ancestor: i32, site_source: SiteSource) -> Self {
        Self {
            status: Status::Present as i32,
            ancestor,
            site_source: site_source as u32,
            error: Error::None as u32,
            warning_count: 0,
        }
    }

    fn absent() -> Self {
        Self {
            status: Status::Absent as i32,
            ancestor: CROSS_SITE,
            site_source: SiteSource::None as u32,
            error: Error::None as u32,
            warning_count: 0,
        }
    }

    fn error(error: Error, warning_count: u32) -> Self {
        Self {
            status: Status::Error as i32,
            ancestor: CROSS_SITE,
            site_source: SiteSource::None as u32,
            error: error as u32,
            warning_count,
        }
    }
}

pub fn ancestor(cross_site: bool) -> i32 {
    if cross_site {
        CROSS_SITE
    } else {
        SAME_SITE
    }
}

// FromWire/testing/storage-component construction does not normalize the bit
// when a nonce is supplied, or reject opaque sites. NIK construction has its
// separate nonce rule below. Preserve all representable original enum values.
pub fn construct_ancestor(value: i32) -> i32 {
    value
}

pub fn query(operation: u32, present: bool, opaque: bool, nonce: bool, bit: i32) -> bool {
    match operation {
        0 => !opaque && !nonce, // IsSerializeable, on a present object.
        1 => nonce,             // ForbidsUnpartitionedCookieAccess.
        2 => present && nonce,  // HasNonce(optional_ref).
        3 => bit == CROSS_SITE,
        _ => false,
    }
}

fn serialize_site(services: &mut impl Primitives) {
    let operation = if services.run(Primitive::SiteIsFile) != 0 {
        Primitive::SiteSerializeFileHost
    } else {
        Primitive::SiteSerialize
    };
    services.run(operation);
}

pub fn serialize(present: bool, nonce: bool, bit: i32, services: &mut impl Primitives) -> Result {
    if !present {
        return Result::absent();
    }
    if !query(0, true, services.run(Primitive::SiteOpaque) != 0, nonce, bit) {
        return Result::error(Error::NotSerializable, 1);
    }
    serialize_site(services);
    // IsThirdParty tests equality to kCrossSite, rather than treating arbitrary
    // underlying values as truthy. Serialized bool is represented by this bit.
    Result::present(ancestor(query(3, true, false, nonce, bit)), SiteSource::Existing)
}

// 0/1 retain private DeserializeInternal strict/loose behavior. Public storage
// and untrusted modes add their own empty handling and second warning, exactly
// as the original wrapper did. Valid callers use only these four fixed IDs.
pub fn parse(mode: u32, input_empty: bool, bit: i32, services: &mut impl Primitives) -> Result {
    if mode == 2 && input_empty {
        return Result::absent();
    }
    if mode == 3 && input_empty {
        return Result::error(Error::UnexpectedEmpty, 1);
    }
    services.run(Primitive::SiteDeserialize);
    let warnings = if mode >= 2 { 2 } else { 1 };
    if services.run(Primitive::SiteOpaque) != 0 {
        return Result::error(Error::Opaque, warnings);
    }
    if mode == 0 || mode == 2 {
        serialize_site(services);
        if services.run(Primitive::SerializedEqualsInput) == 0 {
            return Result::error(Error::Malformed, warnings);
        }
    }
    Result::present(construct_ancestor(bit), SiteSource::Parsed)
}

pub fn from_nik(main_frame_navigation: bool, services: &mut impl Primitives) -> Result {
    if services.run(Primitive::NikPartition) != GENERAL_PARTITION {
        return Result::absent();
    }
    let nonce = services.run(Primitive::NikNoncePresent) != 0;
    let (selection, source) = if nonce {
        (Primitive::NikSelectFrame, SiteSource::Frame)
    } else {
        (Primitive::NikSelectTop, SiteSource::Top)
    };
    if services.run(selection) == 0 {
        return Result::absent();
    }
    let bit = if nonce {
        CROSS_SITE
    } else if main_frame_navigation {
        SAME_SITE
    } else if services.run(Primitive::SfcIsNull) != 0 {
        CROSS_SITE
    } else {
        ancestor(services.run(Primitive::SfcIsFirstParty) == 0)
    };
    Result::present(bit, source)
}

pub fn equals(left_bit: i32, right_bit: i32, services: &mut impl Primitives) -> bool {
    // Keep equality separate from ordering: token == is a distinct native
    // primitive, and opaque-site equality must retain its original behavior.
    if services.run(Primitive::SiteEqual) == 0 {
        return false;
    }
    let left_nonce = services.run(Primitive::LeftNoncePresent) != 0;
    let right_nonce = services.run(Primitive::RightNoncePresent) != 0;
    if left_nonce != right_nonce {
        return false;
    }
    if left_nonce && services.run(Primitive::NonceEqual) == 0 {
        return false;
    }
    left_bit == right_bit
}

pub fn compare(left_bit: i32, right_bit: i32, services: &mut impl Primitives) -> i32 {
    let site_order = services.run(Primitive::SiteCompare).signum();
    if site_order != 0 {
        return site_order;
    }
    let left_nonce = services.run(Primitive::LeftNoncePresent) != 0;
    let right_nonce = services.run(Primitive::RightNoncePresent) != 0;
    match left_nonce.cmp(&right_nonce) {
        Ordering::Less => return -1,
        Ordering::Greater => return 1,
        Ordering::Equal => {}
    }
    if left_nonce {
        let nonce_order = services.run(Primitive::NonceCompare).signum();
        if nonce_order != 0 {
            return nonce_order;
        }
    }
    match left_bit.cmp(&right_bit) {
        Ordering::Less => -1,
        Ordering::Equal => 0,
        Ordering::Greater => 1,
    }
}

pub fn message(error: u32) -> &'static str {
    match error {
        1 => "CookiePartitionKey is not serializeable",
        2 => "top_level_site is unexpectedly empty",
        3 => "Cannot deserialize opaque origin to CookiePartitionKey",
        4 => "Cannot deserialize malformed top_level_site to CookiePartitionKey",
        _ => "",
    }
}

pub fn serialized_suffix(cross_site: bool) -> &'static str {
    if cross_site {
        ", cross-site"
    } else {
        ", same-site"
    }
}

pub fn stream_suffix(nonce: bool, bit: i32) -> (&'static str, &'static str) {
    let nonce = if nonce { ",nonced" } else { "" };
    let ancestor = if bit == CROSS_SITE { ",cross_site" } else { ",same_site" };
    (nonce, ancestor)
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Script {
        remaining: std::collections::VecDeque<(Primitive, i32)>,
    }

    impl Script {
        fn new(steps: &[(Primitive, i32)]) -> Self {
            Self { remaining: steps.iter().copied().collect() }
        }
        fn finish(&self) {
            assert!(self.remaining.is_empty(), "unused primitive steps: {:?}", self.remaining);
        }
    }

    impl Primitives for Script {
        fn run(&mut self, operation: Primitive) -> i32 {
            let (expected, result) = self.remaining.pop_front().expect("unexpected native call");
            assert_eq!(operation, expected);
            result
        }
    }

    #[test]
    fn persisted_values_and_direct_constructor_do_not_normalize() {
        assert_eq!((ancestor(false), ancestor(true)), (0, 1));
        for bit in [i32::MIN, -1, SAME_SITE, CROSS_SITE, 2, i32::MAX] {
            assert_eq!(construct_ancestor(bit), bit);
            assert_eq!(query(3, true, false, true, bit), bit == 1);
        }
    }

    #[test]
    fn predicates_preserve_optional_presence_and_nonce_rules() {
        for present in [false, true] {
            for opaque in [false, true] {
                for nonce in [false, true] {
                    assert_eq!(query(0, present, opaque, nonce, 0), !opaque && !nonce);
                    assert_eq!(query(1, present, opaque, nonce, 0), nonce);
                    assert_eq!(query(2, present, opaque, nonce, 0), present && nonce);
                }
            }
        }
    }

    #[test]
    fn absent_serialize_never_calls_native() {
        let mut native = Script::new(&[]);
        assert_eq!(serialize(false, false, 0, &mut native), Result::absent());
        native.finish();
    }

    #[test]
    fn serialize_rejects_opaque_or_nonce_before_serialization() {
        for (opaque, nonce) in [(1, false), (0, true), (1, true)] {
            let mut native = Script::new(&[(Primitive::SiteOpaque, opaque)]);
            assert_eq!(serialize(true, nonce, 0, &mut native), Result::error(Error::NotSerializable, 1));
            native.finish();
        }
    }

    #[test]
    fn file_serialize_retains_host_service_and_exact_cross_site_test() {
        for bit in [-1, 0, 1, 2] {
            let mut native = Script::new(&[
                (Primitive::SiteOpaque, 0),
                (Primitive::SiteIsFile, 1),
                (Primitive::SiteSerializeFileHost, 0),
            ]);
            assert_eq!(serialize(true, false, bit, &mut native).ancestor, ancestor(bit == 1));
            native.finish();
        }
    }

    #[test]
    fn parse_empty_public_modes_do_not_call_native() {
        let mut native = Script::new(&[]);
        assert_eq!(parse(2, true, 0, &mut native), Result::absent());
        assert_eq!(parse(3, true, 1, &mut native), Result::error(Error::UnexpectedEmpty, 1));
        native.finish();
    }

    #[test]
    fn internal_parse_preserves_supplied_ancestor() {
        for bit in [-1, 0, 1, 2] {
            let mut native =
                Script::new(&[(Primitive::SiteDeserialize, 0), (Primitive::SiteOpaque, 0)]);
            assert_eq!(parse(1, false, bit, &mut native), Result::present(bit, SiteSource::Parsed));
            native.finish();
        }
    }

    #[test]
    fn parse_opaque_precedes_canonicality_and_keeps_warning_count() {
        for mode in 0..4 {
            let mut native =
                Script::new(&[(Primitive::SiteDeserialize, 0), (Primitive::SiteOpaque, 1)]);
            let expected_warnings = if mode >= 2 { 2 } else { 1 };
            assert_eq!(parse(mode, false, 1, &mut native), Result::error(Error::Opaque, expected_warnings));
            native.finish();
        }
    }

    #[test]
    fn strict_parse_checks_roundtrip_and_loose_parse_does_not() {
        for mode in [0, 2] {
            for canonical in [0, 1] {
                let mut native = Script::new(&[
                    (Primitive::SiteDeserialize, 0),
                    (Primitive::SiteOpaque, 0),
                    (Primitive::SiteIsFile, 0),
                    (Primitive::SiteSerialize, 0),
                    (Primitive::SerializedEqualsInput, canonical),
                ]);
                let got = parse(mode, false, 0, &mut native);
                assert_eq!(got.status, if canonical == 0 { 2 } else { 0 });
                assert_eq!(got.ancestor, if canonical == 0 { 1 } else { 0 });
                native.finish();
            }
        }
        for mode in [1, 3] {
            let mut native =
                Script::new(&[(Primitive::SiteDeserialize, 0), (Primitive::SiteOpaque, 0)]);
            assert_eq!(parse(mode, false, 1, &mut native), Result::present(1, SiteSource::Parsed));
            native.finish();
        }
    }

    #[test]
    fn non_general_nik_never_reads_nonce_or_sites() {
        for partition in [-1, 1, 2, 3, 4, i32::MAX] {
            let mut native = Script::new(&[(Primitive::NikPartition, partition)]);
            assert_eq!(from_nik(false, &mut native), Result::absent());
            native.finish();
        }
    }

    #[test]
    fn nik_missing_selected_site_short_circuits() {
        for nonce in [0, 1] {
            let select =
                if nonce == 1 { Primitive::NikSelectFrame } else { Primitive::NikSelectTop };
            let mut native = Script::new(&[
                (Primitive::NikPartition, 0),
                (Primitive::NikNoncePresent, nonce),
                (select, 0),
            ]);
            assert_eq!(from_nik(true, &mut native), Result::absent());
            native.finish();
        }
    }

    #[test]
    fn nik_nonce_precedes_main_navigation_and_does_not_query_sfc() {
        for main_navigation in [false, true] {
            let mut native = Script::new(&[
                (Primitive::NikPartition, 0),
                (Primitive::NikNoncePresent, 1),
                (Primitive::NikSelectFrame, 1),
            ]);
            assert_eq!(from_nik(main_navigation, &mut native), Result::present(1, SiteSource::Frame));
            native.finish();
        }
    }

    #[test]
    fn nik_main_navigation_precedes_null_sfc() {
        let mut native = Script::new(&[
            (Primitive::NikPartition, 0),
            (Primitive::NikNoncePresent, 0),
            (Primitive::NikSelectTop, 1),
        ]);
        assert_eq!(from_nik(true, &mut native), Result::present(0, SiteSource::Top));
        native.finish();
    }

    #[test]
    fn nik_sfc_null_and_first_party_branches() {
        let mut native = Script::new(&[
            (Primitive::NikPartition, 0),
            (Primitive::NikNoncePresent, 0),
            (Primitive::NikSelectTop, 1),
            (Primitive::SfcIsNull, 1),
        ]);
        assert_eq!(from_nik(false, &mut native), Result::present(1, SiteSource::Top));
        native.finish();
        for first_party in [0, 1] {
            let mut native = Script::new(&[
                (Primitive::NikPartition, 0),
                (Primitive::NikNoncePresent, 0),
                (Primitive::NikSelectTop, 1),
                (Primitive::SfcIsNull, 0),
                (Primitive::SfcIsFirstParty, first_party),
            ]);
            assert_eq!(from_nik(false, &mut native), Result::present(ancestor(first_party == 0), SiteSource::Top));
            native.finish();
        }
    }

    #[test]
    fn equality_site_short_circuit_and_nonce_presence() {
        let mut native = Script::new(&[(Primitive::SiteEqual, 0)]);
        assert!(!equals(0, 0, &mut native));
        native.finish();
        for (left, right) in [(0, 1), (1, 0)] {
            let mut native = Script::new(&[
                (Primitive::SiteEqual, 1),
                (Primitive::LeftNoncePresent, left),
                (Primitive::RightNoncePresent, right),
            ]);
            assert!(!equals(0, 0, &mut native));
            native.finish();
        }
    }

    #[test]
    fn equality_uses_nonce_equal_primitive_and_last_ancestor() {
        for nonce in [0, 1] {
            for nonce_equal in [0, 1] {
                let mut steps = vec![
                    (Primitive::SiteEqual, 1), (Primitive::LeftNoncePresent, nonce), (Primitive::RightNoncePresent, nonce),
                ];
                if nonce == 1 {
                    steps.push((Primitive::NonceEqual, nonce_equal));
                }
                let mut native = Script::new(&steps);
                assert_eq!(equals(1, 1, &mut native), nonce == 0 || nonce_equal == 1);
                native.finish();
            }
        }
    }

    #[test]
    fn comparison_is_site_then_optional_nonce_then_raw_ancestor() {
        for site_order in [-2, 1] {
            let mut native = Script::new(&[(Primitive::SiteCompare, site_order)]);
            assert_eq!(compare(1, 0, &mut native), site_order.signum());
            native.finish();
        }
        for (left, right, expected) in [(0, 1, -1), (1, 0, 1)] {
            let mut native = Script::new(&[
                (Primitive::SiteCompare, 0),
                (Primitive::LeftNoncePresent, left),
                (Primitive::RightNoncePresent, right),
            ]);
            assert_eq!(compare(0, 1, &mut native), expected);
            native.finish();
        }
        for nonce_order in [-1, 0, 1] {
            let mut native = Script::new(&[
                (Primitive::SiteCompare, 0),
                (Primitive::LeftNoncePresent, 1),
                (Primitive::RightNoncePresent, 1),
                (Primitive::NonceCompare, nonce_order),
            ]);
            assert_eq!(compare(2, 1, &mut native), if nonce_order == 0 { 1 } else { nonce_order });
            native.finish();
        }
        let mut native = Script::new(&[
            (Primitive::SiteCompare, 0),
            (Primitive::LeftNoncePresent, 0),
            (Primitive::RightNoncePresent, 0),
        ]);
        assert_eq!(compare(i32::MIN, i32::MAX, &mut native), -1);
        native.finish();
    }

    #[test]
    fn exact_diagnostics_and_suffixes() {
        assert_eq!(message(1), "CookiePartitionKey is not serializeable");
        assert_eq!(message(2), "top_level_site is unexpectedly empty");
        assert_eq!(message(3), "Cannot deserialize opaque origin to CookiePartitionKey");
        assert_eq!(message(4), "Cannot deserialize malformed top_level_site to CookiePartitionKey");
        assert_eq!(serialized_suffix(false), ", same-site");
        assert_eq!(serialized_suffix(true), ", cross-site");
        assert_eq!(stream_suffix(false, 0), ("", ",same_site"));
        assert_eq!(stream_suffix(false, 1), ("", ",cross_site"));
        assert_eq!(stream_suffix(true, 0), (",nonced", ",same_site"));
        assert_eq!(stream_suffix(true, 1), (",nonced", ",cross_site"));
    }
}
