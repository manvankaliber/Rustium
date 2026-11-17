// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Generated from frozen mapping policy and named Microsoft windows-sys
// bindings.
use super::ids::*;
use windows_sys::Win32::{Foundation as foundation, Networking::WinSock as ws};
const _: () = assert!(ws::WSAEWOULDBLOCK as u32 == 10035_u32);
const _: () = assert!(ws::WSA_IO_PENDING as u32 == 997_u32);
const _: () = assert!(ws::WSAEACCES as u32 == 10013_u32);
const _: () = assert!(ws::WSAENETDOWN as u32 == 10050_u32);
const _: () = assert!(ws::WSAETIMEDOUT as u32 == 10060_u32);
const _: () = assert!(ws::WSAECONNRESET as u32 == 10054_u32);
const _: () = assert!(ws::WSAENETRESET as u32 == 10052_u32);
const _: () = assert!(ws::WSAECONNABORTED as u32 == 10053_u32);
const _: () = assert!(ws::WSAECONNREFUSED as u32 == 10061_u32);
const _: () = assert!(ws::WSA_IO_INCOMPLETE as u32 == 996_u32);
const _: () = assert!(ws::WSAEDISCON as u32 == 10101_u32);
const _: () = assert!(ws::WSAEISCONN as u32 == 10056_u32);
const _: () = assert!(ws::WSAEHOSTUNREACH as u32 == 10065_u32);
const _: () = assert!(ws::WSAENETUNREACH as u32 == 10051_u32);
const _: () = assert!(ws::WSAEADDRNOTAVAIL as u32 == 10049_u32);
const _: () = assert!(ws::WSAEMSGSIZE as u32 == 10040_u32);
const _: () = assert!(ws::WSAENOTCONN as u32 == 10057_u32);
const _: () = assert!(ws::WSAEAFNOSUPPORT as u32 == 10047_u32);
const _: () = assert!(ws::WSAEINVAL as u32 == 10022_u32);
const _: () = assert!(ws::WSAEADDRINUSE as u32 == 10048_u32);
const _: () = assert!(ws::WSAENOBUFS as u32 == 10055_u32);
const _: () = assert!(foundation::ERROR_FILE_NOT_FOUND == 2_u32);
const _: () = assert!(foundation::ERROR_PATH_NOT_FOUND == 3_u32);
const _: () = assert!(foundation::ERROR_TOO_MANY_OPEN_FILES == 4_u32);
const _: () = assert!(foundation::ERROR_ACCESS_DENIED == 5_u32);
const _: () = assert!(foundation::ERROR_INVALID_HANDLE == 6_u32);
const _: () = assert!(foundation::ERROR_NOT_ENOUGH_MEMORY == 8_u32);
const _: () = assert!(foundation::ERROR_OUTOFMEMORY == 14_u32);
const _: () = assert!(foundation::ERROR_WRITE_PROTECT == 19_u32);
const _: () = assert!(foundation::ERROR_SHARING_VIOLATION == 32_u32);
const _: () = assert!(foundation::ERROR_LOCK_VIOLATION == 33_u32);
const _: () = assert!(foundation::ERROR_HANDLE_EOF == 38_u32);
const _: () = assert!(foundation::ERROR_HANDLE_DISK_FULL == 39_u32);
const _: () = assert!(foundation::ERROR_FILE_EXISTS == 80_u32);
const _: () = assert!(foundation::ERROR_INVALID_PARAMETER == 87_u32);
const _: () = assert!(foundation::ERROR_BUFFER_OVERFLOW == 111_u32);
const _: () = assert!(foundation::ERROR_DISK_FULL == 112_u32);
const _: () = assert!(foundation::ERROR_CALL_NOT_IMPLEMENTED == 120_u32);
const _: () = assert!(foundation::ERROR_INVALID_NAME == 123_u32);
const _: () = assert!(foundation::ERROR_DIR_NOT_EMPTY == 145_u32);
const _: () = assert!(foundation::ERROR_BUSY == 170_u32);
const _: () = assert!(foundation::ERROR_ALREADY_EXISTS == 183_u32);
const _: () = assert!(foundation::ERROR_FILENAME_EXCED_RANGE == 206_u32);
const _: () = assert!(foundation::ERROR_FILE_TOO_LARGE == 223_u32);
const _: () = assert!(foundation::ERROR_VIRUS_INFECTED == 225_u32);
const _: () = assert!(foundation::ERROR_IO_DEVICE == 1117_u32);
const _: () = assert!(foundation::ERROR_POSSIBLE_DEADLOCK == 1131_u32);
const _: () = assert!(foundation::ERROR_BAD_DEVICE == 1200_u32);
const _: () = assert!(foundation::ERROR_BROKEN_PIPE == 109_u32);
pub fn map(error: u32) -> Option<i32> {
    if error == 0 {
        return Some(0);
    }
    if error == ws::WSAEWOULDBLOCK as u32 {
        return Some(ERR_IO_PENDING);
    }
    if error == ws::WSA_IO_PENDING as u32 {
        return Some(ERR_IO_PENDING);
    }
    if error == ws::WSAEACCES as u32 {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == ws::WSAENETDOWN as u32 {
        return Some(ERR_INTERNET_DISCONNECTED);
    }
    if error == ws::WSAETIMEDOUT as u32 {
        return Some(ERR_TIMED_OUT);
    }
    if error == ws::WSAECONNRESET as u32 {
        return Some(ERR_CONNECTION_RESET);
    }
    if error == ws::WSAENETRESET as u32 {
        return Some(ERR_CONNECTION_RESET);
    }
    if error == ws::WSAECONNABORTED as u32 {
        return Some(ERR_CONNECTION_ABORTED);
    }
    if error == ws::WSAECONNREFUSED as u32 {
        return Some(ERR_CONNECTION_REFUSED);
    }
    if error == ws::WSA_IO_INCOMPLETE as u32 {
        return Some(ERR_CONNECTION_CLOSED);
    }
    if error == ws::WSAEDISCON as u32 {
        return Some(ERR_CONNECTION_CLOSED);
    }
    if error == ws::WSAEISCONN as u32 {
        return Some(ERR_SOCKET_IS_CONNECTED);
    }
    if error == ws::WSAEHOSTUNREACH as u32 {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == ws::WSAENETUNREACH as u32 {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == ws::WSAEADDRNOTAVAIL as u32 {
        return Some(ERR_ADDRESS_INVALID);
    }
    if error == ws::WSAEMSGSIZE as u32 {
        return Some(ERR_MSG_TOO_BIG);
    }
    if error == ws::WSAENOTCONN as u32 {
        return Some(ERR_SOCKET_NOT_CONNECTED);
    }
    if error == ws::WSAEAFNOSUPPORT as u32 {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == ws::WSAEINVAL as u32 {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == ws::WSAEADDRINUSE as u32 {
        return Some(ERR_ADDRESS_IN_USE);
    }
    if error == ws::WSAENOBUFS as u32 {
        return Some(ERR_NO_BUFFER_SPACE);
    }
    if error == foundation::ERROR_FILE_NOT_FOUND {
        return Some(ERR_FILE_NOT_FOUND);
    }
    if error == foundation::ERROR_PATH_NOT_FOUND {
        return Some(ERR_FILE_NOT_FOUND);
    }
    if error == foundation::ERROR_TOO_MANY_OPEN_FILES {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == foundation::ERROR_ACCESS_DENIED {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == foundation::ERROR_INVALID_HANDLE {
        return Some(ERR_INVALID_HANDLE);
    }
    if error == foundation::ERROR_NOT_ENOUGH_MEMORY {
        return Some(ERR_OUT_OF_MEMORY);
    }
    if error == foundation::ERROR_OUTOFMEMORY {
        return Some(ERR_OUT_OF_MEMORY);
    }
    if error == foundation::ERROR_WRITE_PROTECT {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == foundation::ERROR_SHARING_VIOLATION {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == foundation::ERROR_LOCK_VIOLATION {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == foundation::ERROR_HANDLE_EOF {
        return Some(ERR_FAILED);
    }
    if error == foundation::ERROR_HANDLE_DISK_FULL {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == foundation::ERROR_FILE_EXISTS {
        return Some(ERR_FILE_EXISTS);
    }
    if error == foundation::ERROR_INVALID_PARAMETER {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == foundation::ERROR_BUFFER_OVERFLOW {
        return Some(ERR_FILE_PATH_TOO_LONG);
    }
    if error == foundation::ERROR_DISK_FULL {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == foundation::ERROR_CALL_NOT_IMPLEMENTED {
        return Some(ERR_NOT_IMPLEMENTED);
    }
    if error == foundation::ERROR_INVALID_NAME {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == foundation::ERROR_DIR_NOT_EMPTY {
        return Some(ERR_FAILED);
    }
    if error == foundation::ERROR_BUSY {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == foundation::ERROR_ALREADY_EXISTS {
        return Some(ERR_FILE_EXISTS);
    }
    if error == foundation::ERROR_FILENAME_EXCED_RANGE {
        return Some(ERR_FILE_PATH_TOO_LONG);
    }
    if error == foundation::ERROR_FILE_TOO_LARGE {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == foundation::ERROR_VIRUS_INFECTED {
        return Some(ERR_FILE_VIRUS_INFECTED);
    }
    if error == foundation::ERROR_IO_DEVICE {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == foundation::ERROR_POSSIBLE_DEADLOCK {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == foundation::ERROR_BAD_DEVICE {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == foundation::ERROR_BROKEN_PIPE {
        return Some(ERR_CONNECTION_RESET);
    }
    None
}
#[cfg(test)]
include!("net_error_rust_win_native_tests.rs");
