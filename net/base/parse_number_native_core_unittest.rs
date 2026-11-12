// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Contract tests for the decoded iterator interfaces used by HSTS and other
//! parsers. Public-API differential C++ tests validate the slice
//! implementations; these checks compare iterator consumption/results across
//! the same byte cases.
#![forbid(unsafe_code)]

use parse_number_core::{parse_i64, parse_i64_iter, parse_u32, parse_u32_iter};

fn compare(input: &[u8]) {
    // Force a decoded, non-contiguous-input iterator rather than a slice
    // iterator.
    let encoded: Vec<u8> = input.iter().map(|byte| byte ^ 0x5a).collect();
    for report in [false, true] {
        for format in [-1, 0, 1, 2, 3, 4, i32::MAX] {
            assert_eq!(
                parse_i64_iter(encoded.iter().map(|byte| byte ^ 0x5a), format, report),
                parse_i64(input, format, report),
                "signed format {format} report {report}: {input:?}"
            );
        }
        for strict in [false, true] {
            assert_eq!(
                parse_u32_iter(encoded.iter().map(|byte| byte ^ 0x5a), strict, report),
                parse_u32(input, if strict { 2 } else { 0 }, report),
                "unsigned strict {strict} report {report}: {input:?}"
            );
        }
    }
}

#[test]
fn all_short_bytes_and_formats() {
    compare(b"");
    for a in 0..=255 {
        compare(&[a]);
        for b in 0..=255 {
            compare(&[a, b]);
        }
    }
}

#[test]
fn decoded_overflow_and_underflow_precedence() {
    for input in [
        "0",
        "-0",
        "00",
        "-00",
        "0001",
        "-0001",
        "4294967295",
        "4294967296",
        "9223372036854775807",
        "9223372036854775808",
        "-9223372036854775808",
        "-9223372036854775809",
        "--1",
        "-+1",
        "-x",
    ] {
        compare(input.as_bytes());
    }
    for length in [20, 31, 32, 33, 47, 48, 49, 64, 65, 4096] {
        let mut input = vec![b'9'; length];
        compare(&input);
        input.insert(0, b'-');
        compare(&input);
        for index in [0, 1, 20, length] {
            for byte in [b'/', b':', 0, 0x80, 0xff] {
                let mut malformed = input.clone();
                malformed[index] = byte;
                compare(&malformed);
            }
        }
    }
}

#[test]
fn every_byte_in_decoded_numeric_fields() {
    for mut input in [b"18446744073709551616".to_vec(), b"-9223372036854775809".to_vec()] {
        for index in 0..input.len() {
            let previous = input[index];
            for byte in 0..=255 {
                input[index] = byte;
                compare(&input);
            }
            input[index] = previous;
        }
    }
}
