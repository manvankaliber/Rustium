// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]

// Persisted IDs are pinned against the public C++ enums by compile-time
// asserts.
pub const EXCLUSION_COUNT: i32 = 30;
pub const WARNING_COUNT: i32 = 14;
pub const EXCLUSION_MASK: u64 = (1 << EXCLUSION_COUNT) - 1;
pub const WARNING_MASK: u64 = (1 << WARNING_COUNT) - 1;
const THIRD_PARTY: u64 = 1 << 24;
const THIRD_PARTY_WARNING: u64 = 1 << 12;
const SAME_SITE_WARNINGS: u64 = 7;
const SAME_SITE_WARN_EXCLUSIONS: u64 = (1 << 7) | (1 << 8);
const DOWNGRADE_EXCLUSIONS: u64 = (1 << 5) | (1 << 6) | (1 << 7);

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct State {
    pub exclusions: u64,
    pub warnings: u64,
    pub exemption: i32,
    // Result metadata only, never persisted in the public status object.
    pub invalid: u32,
}

fn bit(reason: i32, count: i32) -> u64 {
    if (0..count).contains(&reason) {
        1_u64 << reason
    } else {
        0
    }
}

pub fn same(a: State, b: State) -> bool {
    a.exclusions == b.exclusions && a.warnings == b.warnings && a.exemption == b.exemption
}

pub fn query(s: State, op: u32, reason: i32, mask: u64) -> bool {
    match op {
        0 => s.exclusions == 0,
        1 => s.exclusions & bit(reason, EXCLUSION_COUNT) != 0,
        2 => s.exclusions != 0 && s.exclusions == bit(reason, EXCLUSION_COUNT),
        3 => s.exclusions & !DOWNGRADE_EXCLUSIONS == 0,
        4 => s.warnings != 0,
        5 => s.warnings & bit(reason, WARNING_COUNT) != 0,
        6 => s.exclusions == (1 << 9) || s.exclusions == THIRD_PARTY,
        7 => s.exclusions == mask,
        8 => s.warnings == mask,
        _ => false,
    }
}

fn clear_same_site(s: &mut State) {
    if s.exclusions & !SAME_SITE_WARN_EXCLUSIONS != 0 {
        s.warnings &= !SAME_SITE_WARNINGS;
    }
    if !query(*s, 3, 0, 0) {
        s.warnings &= !(1 << 4);
    }
}

fn clear_third_party(s: &mut State) {
    if s.exclusions != 0 {
        s.warnings &= !THIRD_PARTY_WARNING;
    }
    if s.exclusions & !THIRD_PARTY != 0 {
        s.exclusions &= !THIRD_PARTY;
    }
}

pub fn apply(mut s: State, op: u32, reason: i32, mask: u64) -> State {
    s.invalid = 0;
    match op {
        0 => {
            let b = bit(reason, EXCLUSION_COUNT);
            if b == 0 {
                s.invalid = 1;
                return s;
            }
            s.exclusions |= b;
            // Order matters: downgrade warnings are cleared BEFORE 3PCD
            // removal.
            clear_same_site(&mut s);
            clear_third_party(&mut s);
            s.exemption = 0;
        }
        1 => s.exclusions &= !bit(reason, EXCLUSION_COUNT),
        2 => s.exclusions &= !(mask & EXCLUSION_MASK),
        3 => {
            // The original API accepts arbitrary underlying exemption integers.
            if s.exclusions == 0 && s.exemption == 0 {
                s.exemption = reason;
            }
        }
        4 => clear_same_site(&mut s),
        5 => clear_third_party(&mut s),
        6 => {
            let b = bit(reason, WARNING_COUNT);
            if b == 0 {
                s.invalid = 1;
                return s;
            }
            s.warnings |= b;
        }
        7 => s.warnings &= !bit(reason, WARNING_COUNT),
        8 => return State::default(),
        9 => s.exclusions &= !(mask & EXCLUSION_MASK),
        _ => s.invalid = 1,
    }
    s
}

pub fn components(exclusions: u64, warnings: u64, exemption: i32) -> State {
    // EnumSet::FromEnumBitmask discards unknown bits, including deserialized
    // ones.
    let exclusions = exclusions & EXCLUSION_MASK;
    let warnings = warnings & WARNING_MASK;
    let mut s = State::default();
    for reason in 0..EXCLUSION_COUNT {
        if exclusions & (1 << reason) != 0 {
            s = apply(s, 0, reason, 0);
        }
    }
    for reason in 0..WARNING_COUNT {
        if warnings & (1 << reason) != 0 {
            s = apply(s, 6, reason, 0);
        }
    }
    s = apply(s, 3, exemption, 0);
    s.invalid = u32::from(!same(s, State { exclusions, warnings, exemption, invalid: 0 }));
    s
}

const EXCLUSION_NAMES: [&str; EXCLUSION_COUNT as usize] = [
    "EXCLUDE_UNKNOWN_ERROR",
    "EXCLUDE_HTTP_ONLY",
    "EXCLUDE_SECURE_ONLY",
    "EXCLUDE_DOMAIN_MISMATCH",
    "EXCLUDE_NOT_ON_PATH",
    "EXCLUDE_SAMESITE_STRICT",
    "EXCLUDE_SAMESITE_LAX",
    "EXCLUDE_SAMESITE_UNSPECIFIED_TREATED_AS_LAX",
    "EXCLUDE_SAMESITE_NONE_INSECURE",
    "EXCLUDE_USER_PREFERENCES",
    "EXCLUDE_FAILURE_TO_STORE",
    "EXCLUDE_NONCOOKIEABLE_SCHEME",
    "EXCLUDE_OVERWRITE_SECURE",
    "EXCLUDE_OVERWRITE_HTTP_ONLY",
    "EXCLUDE_INVALID_DOMAIN",
    "EXCLUDE_INVALID_PREFIX",
    "EXCLUDE_INVALID_PARTITIONED",
    "EXCLUDE_NAME_VALUE_PAIR_EXCEEDS_MAX_SIZE",
    "EXCLUDE_ATTRIBUTE_VALUE_EXCEEDS_MAX_SIZE",
    "EXCLUDE_DOMAIN_NON_ASCII",
    "EXCLUDE_PORT_MISMATCH",
    "EXCLUDE_SCHEME_MISMATCH",
    "EXCLUDE_SHADOWING_DOMAIN",
    "EXCLUDE_DISALLOWED_CHARACTER",
    "EXCLUDE_THIRD_PARTY_PHASEOUT",
    "EXCLUDE_NO_COOKIE_CONTENT",
    "EXCLUDE_ANONYMOUS_CONTEXT",
    "EXCLUDE_INVALID_PATH",
    "EXCLUDE_AMBIGUOUS_SERIALIZATION",
    "EXCLUDE_INVALID_HOST",
];
const WARNING_NAMES: [&str; WARNING_COUNT as usize] = [
    "WARN_SAMESITE_UNSPECIFIED_CROSS_SITE_CONTEXT",
    "WARN_SAMESITE_NONE_INSECURE",
    "WARN_SAMESITE_UNSPECIFIED_LAX_ALLOW_UNSAFE",
    "WARN_SECURE_ACCESS_GRANTED_NON_CRYPTOGRAPHIC",
    "WARN_CROSS_SITE_REDIRECT_DOWNGRADE_CHANGES_INCLUSION",
    "WARN_CROSS_SITE_REDIRECT_DOWNGRADE_CHANGES_INCLUSION_NO_INITIATOR",
    "WARN_ATTRIBUTE_VALUE_EXCEEDS_MAX_SIZE",
    "WARN_DOMAIN_NON_ASCII",
    "WARN_PORT_MISMATCH",
    "WARN_SCHEME_MISMATCH",
    "WARN_TENTATIVELY_ALLOWING_SECURE_SOURCE_SCHEME",
    "WARN_SHADOWING_DOMAIN",
    "WARN_THIRD_PARTY_PHASEOUT",
    "WARN_INVALID_HOST",
];
const EXEMPTION_NAMES: [&str; 6] = [
    "NO_EXEMPTION",
    "ExemptionUserSetting",
    "ExemptionEnterprisePolicy",
    "ExemptionStorageAccess",
    "ExemptionScheme",
    "ExemptionSameSiteNoneCookiesInSandbox",
];

// Returns the required length, writing only the initialized prefix that fits.
// No allocation and no C++ callbacks; formatting order is the persisted ID
// order.
pub fn debug_string(s: State, output: &mut [u8]) -> usize {
    let mut length = 0;
    let mut append = |part: &str| {
        for byte in part.bytes() {
            if let Some(slot) = output.get_mut(length) {
                *slot = byte;
            }
            length += 1;
        }
    };
    if s.exclusions == 0 {
        append("INCLUDE, ");
    }
    for (i, name) in EXCLUSION_NAMES.iter().enumerate() {
        if s.exclusions & (1 << i) != 0 {
            append(name);
            append(", ");
        }
    }
    if s.warnings == 0 {
        append("DO_NOT_WARN, ");
    }
    for (i, name) in WARNING_NAMES.iter().enumerate() {
        if s.warnings & (1 << i) != 0 {
            append(name);
            append(", ");
        }
    }
    // No default diagnostic existed in the original exemption switch: an
    // invalid underlying enum formats as an empty suffix and remains stored
    // unchanged.
    if let Ok(index) = usize::try_from(s.exemption)
        && let Some(name) = EXEMPTION_NAMES.get(index)
    {
        append(name);
    }
    length
}
