// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Synthetic named constants exercise every policy case on any host.
#[test]
fn mappings_win() {
    use super::*;
    let c = WindowsConstants {
        wsaewouldblock: 100,
        wsa_io_pending: 101,
        wsaeacces: 102,
        wsaenetdown: 103,
        wsaetimedout: 104,
        wsaeconnreset: 105,
        wsaenetreset: 106,
        wsaeconnaborted: 107,
        wsaeconnrefused: 108,
        wsa_io_incomplete: 109,
        wsaediscon: 110,
        wsaeisconn: 111,
        wsaehostunreach: 112,
        wsaenetunreach: 113,
        wsaeaddrnotavail: 114,
        wsaemsgsize: 115,
        wsaenotconn: 116,
        wsaeafnosupport: 117,
        wsaeinval: 118,
        wsaeaddrinuse: 119,
        wsaenobufs: 120,
        error_file_not_found: 121,
        error_path_not_found: 122,
        error_too_many_open_files: 123,
        error_access_denied: 124,
        error_invalid_handle: 125,
        error_not_enough_memory: 126,
        error_outofmemory: 127,
        error_write_protect: 128,
        error_sharing_violation: 129,
        error_lock_violation: 130,
        error_handle_eof: 131,
        error_handle_disk_full: 132,
        error_file_exists: 133,
        error_invalid_parameter: 134,
        error_buffer_overflow: 135,
        error_disk_full: 136,
        error_call_not_implemented: 137,
        error_invalid_name: 138,
        error_dir_not_empty: 139,
        error_busy: 140,
        error_already_exists: 141,
        error_filename_exced_range: 142,
        error_file_too_large: 143,
        error_virus_infected: 144,
        error_io_device: 145,
        error_possible_deadlock: 146,
        error_bad_device: 147,
        error_broken_pipe: 148,
    };
    assert_eq!(map(0, &c), Some(0));
    assert_eq!(map(c.wsaewouldblock, &c), Some(ERR_IO_PENDING));
    assert_eq!(map(c.wsa_io_pending, &c), Some(ERR_IO_PENDING));
    assert_eq!(map(c.wsaeacces, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.wsaenetdown, &c), Some(ERR_INTERNET_DISCONNECTED));
    assert_eq!(map(c.wsaetimedout, &c), Some(ERR_TIMED_OUT));
    assert_eq!(map(c.wsaeconnreset, &c), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(c.wsaenetreset, &c), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(c.wsaeconnaborted, &c), Some(ERR_CONNECTION_ABORTED));
    assert_eq!(map(c.wsaeconnrefused, &c), Some(ERR_CONNECTION_REFUSED));
    assert_eq!(map(c.wsa_io_incomplete, &c), Some(ERR_CONNECTION_CLOSED));
    assert_eq!(map(c.wsaediscon, &c), Some(ERR_CONNECTION_CLOSED));
    assert_eq!(map(c.wsaeisconn, &c), Some(ERR_SOCKET_IS_CONNECTED));
    assert_eq!(map(c.wsaehostunreach, &c), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(c.wsaenetunreach, &c), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(c.wsaeaddrnotavail, &c), Some(ERR_ADDRESS_INVALID));
    assert_eq!(map(c.wsaemsgsize, &c), Some(ERR_MSG_TOO_BIG));
    assert_eq!(map(c.wsaenotconn, &c), Some(ERR_SOCKET_NOT_CONNECTED));
    assert_eq!(map(c.wsaeafnosupport, &c), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(c.wsaeinval, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.wsaeaddrinuse, &c), Some(ERR_ADDRESS_IN_USE));
    assert_eq!(map(c.wsaenobufs, &c), Some(ERR_NO_BUFFER_SPACE));
    assert_eq!(map(c.error_file_not_found, &c), Some(ERR_FILE_NOT_FOUND));
    assert_eq!(map(c.error_path_not_found, &c), Some(ERR_FILE_NOT_FOUND));
    assert_eq!(map(c.error_too_many_open_files, &c), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(c.error_access_denied, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.error_invalid_handle, &c), Some(ERR_INVALID_HANDLE));
    assert_eq!(map(c.error_not_enough_memory, &c), Some(ERR_OUT_OF_MEMORY));
    assert_eq!(map(c.error_outofmemory, &c), Some(ERR_OUT_OF_MEMORY));
    assert_eq!(map(c.error_write_protect, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.error_sharing_violation, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.error_lock_violation, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.error_handle_eof, &c), Some(ERR_FAILED));
    assert_eq!(map(c.error_handle_disk_full, &c), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(c.error_file_exists, &c), Some(ERR_FILE_EXISTS));
    assert_eq!(map(c.error_invalid_parameter, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.error_buffer_overflow, &c), Some(ERR_FILE_PATH_TOO_LONG));
    assert_eq!(map(c.error_disk_full, &c), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(c.error_call_not_implemented, &c), Some(ERR_NOT_IMPLEMENTED));
    assert_eq!(map(c.error_invalid_name, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.error_dir_not_empty, &c), Some(ERR_FAILED));
    assert_eq!(map(c.error_busy, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.error_already_exists, &c), Some(ERR_FILE_EXISTS));
    assert_eq!(map(c.error_filename_exced_range, &c), Some(ERR_FILE_PATH_TOO_LONG));
    assert_eq!(map(c.error_file_too_large, &c), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(c.error_virus_infected, &c), Some(ERR_FILE_VIRUS_INFECTED));
    assert_eq!(map(c.error_io_device, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.error_possible_deadlock, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.error_bad_device, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.error_broken_pipe, &c), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(1, &c), None);
    assert_eq!(map(u32::MAX, &c), None);
}
