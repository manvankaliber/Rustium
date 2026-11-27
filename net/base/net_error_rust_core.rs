// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! All local net-error policy. Native code provides diagnostics, owned strings,
//! QUICHE names, and named platform constants; it makes no mapping decisions.
#![forbid(unsafe_code)]
#[allow(dead_code)]
#[path = "net_error_rust_ids.rs"]
pub mod ids;
use ids::*;
#[path = "net_error_rust_names.rs"]
mod names;
#[path = "net_error_rust_posix.rs"]
pub mod posix;
#[path = "net_error_rust_win.rs"]
pub mod windows;
#[cfg(target_os = "windows")]
#[path = "net_error_rust_win_native.rs"]
pub mod windows_native;
pub fn defined(error: i32) -> bool {
    names::defined(error)
}
pub fn certificate(error: i32) -> bool {
    (error <= ERR_CERT_BEGIN && error > ERR_CERT_END)
        || error == ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN
}
pub fn client_certificate(error: i32) -> bool {
    matches!(
        error,
        ERR_BAD_SSL_CLIENT_AUTH_CERT
            | ERR_SSL_CLIENT_AUTH_PRIVATE_KEY_ACCESS_DENIED
            | ERR_SSL_CLIENT_AUTH_CERT_NO_PRIVATE_KEY
            | ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED
            | ERR_SSL_CLIENT_AUTH_NO_COMMON_ALGORITHMS
    )
}
// Negative marks the forbidden debug-only diagnostic, without panicking.
pub fn hostname(error: i32) -> i8 {
    if error == ERR_NAME_RESOLUTION_FAILED {
        -1
    } else {
        i8::from(error == ERR_NAME_NOT_RESOLVED)
    }
}
pub fn blocked(error: i32) -> bool {
    matches!(
        error,
        ERR_BLOCKED_BY_CLIENT
            | ERR_BLOCKED_BY_ADMINISTRATOR
            | ERR_BLOCKED_BY_CSP
            | ERR_BLOCKED_IN_INCOGNITO_BY_ADMINISTRATOR
    )
}
pub fn extended(error: i32, extended_error: i32) -> bool {
    error == ERR_QUIC_PROTOCOL_ERROR && extended_error != 0
}
/// Two immutable fragments describe all local formatting, without allocation.
pub fn format_parts(error: i32, full: bool, quic: Option<&[u8]>) -> [&[u8]; 2] {
    if let Some(name) = quic {
        [b"net::ERR_QUIC_PROTOCOL_ERROR.", name]
    } else {
        [if full { b"net::" } else { b"" }, names::short_name(error).unwrap_or(b"ERR_<unknown>")]
    }
}
pub fn file(error: i32) -> Option<i32> {
    match error {
        FILE_OK => Some(0),
        FILE_ERROR_EXISTS => Some(ERR_FILE_EXISTS),
        FILE_ERROR_NOT_FOUND => Some(ERR_FILE_NOT_FOUND),
        FILE_ERROR_ACCESS_DENIED | FILE_ERROR_SECURITY => Some(ERR_ACCESS_DENIED),
        FILE_ERROR_NO_MEMORY => Some(ERR_OUT_OF_MEMORY),
        FILE_ERROR_NO_SPACE => Some(ERR_FILE_NO_SPACE),
        FILE_ERROR_INVALID_OPERATION => Some(ERR_INVALID_ARGUMENT),
        FILE_ERROR_ABORT => Some(ERR_ABORTED),
        FILE_ERROR_INVALID_URL => Some(ERR_INVALID_URL),
        FILE_ERROR_TOO_MANY_OPENED => Some(ERR_INSUFFICIENT_RESOURCES),
        FILE_ERROR_NOT_A_DIRECTORY
        | FILE_ERROR_NOT_A_FILE
        | FILE_ERROR_NOT_EMPTY
        | FILE_ERROR_IO
        | FILE_ERROR_IN_USE
        | FILE_ERROR_FAILED => Some(ERR_FAILED),
        _ => None,
    }
}
#[cfg(test)]
#[path = "net_error_rust_core_tests.rs"]
mod tests;
