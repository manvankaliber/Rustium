// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Generated named constant layout; Rust owns comparisons and mapping.
use super::ids::*;
#[repr(C)]
#[derive(Clone, Copy)]
pub struct WindowsConstants {
    pub wsaewouldblock: u32,
    pub wsa_io_pending: u32,
    pub wsaeacces: u32,
    pub wsaenetdown: u32,
    pub wsaetimedout: u32,
    pub wsaeconnreset: u32,
    pub wsaenetreset: u32,
    pub wsaeconnaborted: u32,
    pub wsaeconnrefused: u32,
    pub wsa_io_incomplete: u32,
    pub wsaediscon: u32,
    pub wsaeisconn: u32,
    pub wsaehostunreach: u32,
    pub wsaenetunreach: u32,
    pub wsaeaddrnotavail: u32,
    pub wsaemsgsize: u32,
    pub wsaenotconn: u32,
    pub wsaeafnosupport: u32,
    pub wsaeinval: u32,
    pub wsaeaddrinuse: u32,
    pub wsaenobufs: u32,
    pub error_file_not_found: u32,
    pub error_path_not_found: u32,
    pub error_too_many_open_files: u32,
    pub error_access_denied: u32,
    pub error_invalid_handle: u32,
    pub error_not_enough_memory: u32,
    pub error_outofmemory: u32,
    pub error_write_protect: u32,
    pub error_sharing_violation: u32,
    pub error_lock_violation: u32,
    pub error_handle_eof: u32,
    pub error_handle_disk_full: u32,
    pub error_file_exists: u32,
    pub error_invalid_parameter: u32,
    pub error_buffer_overflow: u32,
    pub error_disk_full: u32,
    pub error_call_not_implemented: u32,
    pub error_invalid_name: u32,
    pub error_dir_not_empty: u32,
    pub error_busy: u32,
    pub error_already_exists: u32,
    pub error_filename_exced_range: u32,
    pub error_file_too_large: u32,
    pub error_virus_infected: u32,
    pub error_io_device: u32,
    pub error_possible_deadlock: u32,
    pub error_bad_device: u32,
    pub error_broken_pipe: u32,
}
pub fn map(error: u32, c: &WindowsConstants) -> Option<i32> {
    if error == 0 {
        return Some(0);
    }
    if error == c.wsaewouldblock {
        return Some(ERR_IO_PENDING);
    }
    if error == c.wsa_io_pending {
        return Some(ERR_IO_PENDING);
    }
    if error == c.wsaeacces {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.wsaenetdown {
        return Some(ERR_INTERNET_DISCONNECTED);
    }
    if error == c.wsaetimedout {
        return Some(ERR_TIMED_OUT);
    }
    if error == c.wsaeconnreset {
        return Some(ERR_CONNECTION_RESET);
    }
    if error == c.wsaenetreset {
        return Some(ERR_CONNECTION_RESET);
    }
    if error == c.wsaeconnaborted {
        return Some(ERR_CONNECTION_ABORTED);
    }
    if error == c.wsaeconnrefused {
        return Some(ERR_CONNECTION_REFUSED);
    }
    if error == c.wsa_io_incomplete {
        return Some(ERR_CONNECTION_CLOSED);
    }
    if error == c.wsaediscon {
        return Some(ERR_CONNECTION_CLOSED);
    }
    if error == c.wsaeisconn {
        return Some(ERR_SOCKET_IS_CONNECTED);
    }
    if error == c.wsaehostunreach {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == c.wsaenetunreach {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == c.wsaeaddrnotavail {
        return Some(ERR_ADDRESS_INVALID);
    }
    if error == c.wsaemsgsize {
        return Some(ERR_MSG_TOO_BIG);
    }
    if error == c.wsaenotconn {
        return Some(ERR_SOCKET_NOT_CONNECTED);
    }
    if error == c.wsaeafnosupport {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == c.wsaeinval {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.wsaeaddrinuse {
        return Some(ERR_ADDRESS_IN_USE);
    }
    if error == c.wsaenobufs {
        return Some(ERR_NO_BUFFER_SPACE);
    }
    if error == c.error_file_not_found {
        return Some(ERR_FILE_NOT_FOUND);
    }
    if error == c.error_path_not_found {
        return Some(ERR_FILE_NOT_FOUND);
    }
    if error == c.error_too_many_open_files {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == c.error_access_denied {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.error_invalid_handle {
        return Some(ERR_INVALID_HANDLE);
    }
    if error == c.error_not_enough_memory {
        return Some(ERR_OUT_OF_MEMORY);
    }
    if error == c.error_outofmemory {
        return Some(ERR_OUT_OF_MEMORY);
    }
    if error == c.error_write_protect {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.error_sharing_violation {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.error_lock_violation {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.error_handle_eof {
        return Some(ERR_FAILED);
    }
    if error == c.error_handle_disk_full {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == c.error_file_exists {
        return Some(ERR_FILE_EXISTS);
    }
    if error == c.error_invalid_parameter {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.error_buffer_overflow {
        return Some(ERR_FILE_PATH_TOO_LONG);
    }
    if error == c.error_disk_full {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == c.error_call_not_implemented {
        return Some(ERR_NOT_IMPLEMENTED);
    }
    if error == c.error_invalid_name {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.error_dir_not_empty {
        return Some(ERR_FAILED);
    }
    if error == c.error_busy {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.error_already_exists {
        return Some(ERR_FILE_EXISTS);
    }
    if error == c.error_filename_exced_range {
        return Some(ERR_FILE_PATH_TOO_LONG);
    }
    if error == c.error_file_too_large {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == c.error_virus_infected {
        return Some(ERR_FILE_VIRUS_INFECTED);
    }
    if error == c.error_io_device {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.error_possible_deadlock {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.error_bad_device {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.error_broken_pipe {
        return Some(ERR_CONNECTION_RESET);
    }
    None
}
#[cfg(test)]
include!("net_error_rust_win_tests.rs");
