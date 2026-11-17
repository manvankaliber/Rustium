// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
fn format(error: i32, full: bool, quic: Option<&[u8]>, mut emit: impl FnMut(&[u8])) {
    for part in format_parts(error, full, quic) {
        emit(part);
    }
}
use super::*;
#[test]
fn defined_matches_canonical_names() {
    // Includes every persisted error and intervening holes, plus signed bounds.
    for error in (-1024..=1024).chain([i32::MIN, i32::MIN + 1, i32::MAX - 1, i32::MAX]) {
        assert_eq!(defined(error), names::short_name(error).is_some(), "{error}");
    }
}
#[test]
fn boundaries_and_format() {
    for error in [i32::MIN, -900, -813, -221, -220, -219, -201, -200, -199, -1, 0, 1, i32::MAX] {
        assert_eq!(
            certificate(error),
            (-219..=-200).contains(&error) || error == ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN
        );
        let mut output = Vec::new();
        format(error, false, None, |p| output.extend_from_slice(p));
        assert_eq!(output == b"ERR_<unknown>", !defined(error));
        let mut full = Vec::new();
        format(error, true, None, |p| full.extend_from_slice(p));
        assert_eq!(full, [b"net::".as_slice(), output.as_slice()].concat());
    }
    for error in -16..=0 {
        assert!(file(error).is_some());
    }
    for error in [i32::MIN, -18, FILE_ERROR_MAX, 1, i32::MAX] {
        assert_eq!(file(error), None);
    }
    assert!(extended(ERR_QUIC_PROTOCOL_ERROR, -1));
    assert!(!extended(ERR_QUIC_PROTOCOL_ERROR, 0));
    assert!(!extended(0, 1));
    for name in [b"QUIC_INTERNAL_ERROR".as_slice(), b"INVALID_ERROR_CODE", b"", b"a\0b"] {
        let mut output = Vec::new();
        format(ERR_QUIC_PROTOCOL_ERROR, true, Some(name), |p| output.extend_from_slice(p));
        assert_eq!(output, [b"net::ERR_QUIC_PROTOCOL_ERROR.".as_slice(), name].concat());
    }
}
#[test]
fn predicates() {
    for error in -1000..=1000 {
        assert_eq!(
            client_certificate(error),
            [
                ERR_BAD_SSL_CLIENT_AUTH_CERT,
                ERR_SSL_CLIENT_AUTH_PRIVATE_KEY_ACCESS_DENIED,
                ERR_SSL_CLIENT_AUTH_CERT_NO_PRIVATE_KEY,
                ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED,
                ERR_SSL_CLIENT_AUTH_NO_COMMON_ALGORITHMS
            ]
            .contains(&error)
        );
        assert_eq!(
            hostname(error),
            if error == ERR_NAME_RESOLUTION_FAILED {
                -1
            } else {
                i8::from(error == ERR_NAME_NOT_RESOLVED)
            }
        );
        assert_eq!(
            blocked(error),
            [
                ERR_BLOCKED_BY_CLIENT,
                ERR_BLOCKED_BY_ADMINISTRATOR,
                ERR_BLOCKED_BY_CSP,
                ERR_BLOCKED_IN_INCOGNITO_BY_ADMINISTRATOR
            ]
            .contains(&error)
        );
    }
}
