// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Actual named bindings must preserve the generic arbitrary-POD mapping.
#[test]
fn native_mappings_and_boundaries() {
    use super::*;
    let c = super::windows::WindowsConstants {
        wsaewouldblock: ws::WSAEWOULDBLOCK as u32,
        wsa_io_pending: ws::WSA_IO_PENDING as u32,
        wsaeacces: ws::WSAEACCES as u32,
        wsaenetdown: ws::WSAENETDOWN as u32,
        wsaetimedout: ws::WSAETIMEDOUT as u32,
        wsaeconnreset: ws::WSAECONNRESET as u32,
        wsaenetreset: ws::WSAENETRESET as u32,
        wsaeconnaborted: ws::WSAECONNABORTED as u32,
        wsaeconnrefused: ws::WSAECONNREFUSED as u32,
        wsa_io_incomplete: ws::WSA_IO_INCOMPLETE as u32,
        wsaediscon: ws::WSAEDISCON as u32,
        wsaeisconn: ws::WSAEISCONN as u32,
        wsaehostunreach: ws::WSAEHOSTUNREACH as u32,
        wsaenetunreach: ws::WSAENETUNREACH as u32,
        wsaeaddrnotavail: ws::WSAEADDRNOTAVAIL as u32,
        wsaemsgsize: ws::WSAEMSGSIZE as u32,
        wsaenotconn: ws::WSAENOTCONN as u32,
        wsaeafnosupport: ws::WSAEAFNOSUPPORT as u32,
        wsaeinval: ws::WSAEINVAL as u32,
        wsaeaddrinuse: ws::WSAEADDRINUSE as u32,
        wsaenobufs: ws::WSAENOBUFS as u32,
        error_file_not_found: foundation::ERROR_FILE_NOT_FOUND,
        error_path_not_found: foundation::ERROR_PATH_NOT_FOUND,
        error_too_many_open_files: foundation::ERROR_TOO_MANY_OPEN_FILES,
        error_access_denied: foundation::ERROR_ACCESS_DENIED,
        error_invalid_handle: foundation::ERROR_INVALID_HANDLE,
        error_not_enough_memory: foundation::ERROR_NOT_ENOUGH_MEMORY,
        error_outofmemory: foundation::ERROR_OUTOFMEMORY,
        error_write_protect: foundation::ERROR_WRITE_PROTECT,
        error_sharing_violation: foundation::ERROR_SHARING_VIOLATION,
        error_lock_violation: foundation::ERROR_LOCK_VIOLATION,
        error_handle_eof: foundation::ERROR_HANDLE_EOF,
        error_handle_disk_full: foundation::ERROR_HANDLE_DISK_FULL,
        error_file_exists: foundation::ERROR_FILE_EXISTS,
        error_invalid_parameter: foundation::ERROR_INVALID_PARAMETER,
        error_buffer_overflow: foundation::ERROR_BUFFER_OVERFLOW,
        error_disk_full: foundation::ERROR_DISK_FULL,
        error_call_not_implemented: foundation::ERROR_CALL_NOT_IMPLEMENTED,
        error_invalid_name: foundation::ERROR_INVALID_NAME,
        error_dir_not_empty: foundation::ERROR_DIR_NOT_EMPTY,
        error_busy: foundation::ERROR_BUSY,
        error_already_exists: foundation::ERROR_ALREADY_EXISTS,
        error_filename_exced_range: foundation::ERROR_FILENAME_EXCED_RANGE,
        error_file_too_large: foundation::ERROR_FILE_TOO_LARGE,
        error_virus_infected: foundation::ERROR_VIRUS_INFECTED,
        error_io_device: foundation::ERROR_IO_DEVICE,
        error_possible_deadlock: foundation::ERROR_POSSIBLE_DEADLOCK,
        error_bad_device: foundation::ERROR_BAD_DEVICE,
        error_broken_pipe: foundation::ERROR_BROKEN_PIPE,
    };
    for code in (0..=65535).chain([0x7fffffff, 0x80000000, u32::MAX]) {
        assert_eq!(map(code), super::windows::map(code, &c), "{code}");
    }
    assert_eq!(map(ws::WSAEWOULDBLOCK as u32), Some(ERR_IO_PENDING));
    assert_eq!(map(ws::WSA_IO_PENDING as u32), Some(ERR_IO_PENDING));
    assert_eq!(map(ws::WSAEACCES as u32), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(ws::WSAENETDOWN as u32), Some(ERR_INTERNET_DISCONNECTED));
    assert_eq!(map(ws::WSAETIMEDOUT as u32), Some(ERR_TIMED_OUT));
    assert_eq!(map(ws::WSAECONNRESET as u32), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(ws::WSAENETRESET as u32), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(ws::WSAECONNABORTED as u32), Some(ERR_CONNECTION_ABORTED));
    assert_eq!(map(ws::WSAECONNREFUSED as u32), Some(ERR_CONNECTION_REFUSED));
    assert_eq!(map(ws::WSA_IO_INCOMPLETE as u32), Some(ERR_CONNECTION_CLOSED));
    assert_eq!(map(ws::WSAEDISCON as u32), Some(ERR_CONNECTION_CLOSED));
    assert_eq!(map(ws::WSAEISCONN as u32), Some(ERR_SOCKET_IS_CONNECTED));
    assert_eq!(map(ws::WSAEHOSTUNREACH as u32), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(ws::WSAENETUNREACH as u32), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(ws::WSAEADDRNOTAVAIL as u32), Some(ERR_ADDRESS_INVALID));
    assert_eq!(map(ws::WSAEMSGSIZE as u32), Some(ERR_MSG_TOO_BIG));
    assert_eq!(map(ws::WSAENOTCONN as u32), Some(ERR_SOCKET_NOT_CONNECTED));
    assert_eq!(map(ws::WSAEAFNOSUPPORT as u32), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(ws::WSAEINVAL as u32), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(ws::WSAEADDRINUSE as u32), Some(ERR_ADDRESS_IN_USE));
    assert_eq!(map(ws::WSAENOBUFS as u32), Some(ERR_NO_BUFFER_SPACE));
    assert_eq!(map(foundation::ERROR_FILE_NOT_FOUND), Some(ERR_FILE_NOT_FOUND));
    assert_eq!(map(foundation::ERROR_PATH_NOT_FOUND), Some(ERR_FILE_NOT_FOUND));
    assert_eq!(map(foundation::ERROR_TOO_MANY_OPEN_FILES), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(foundation::ERROR_ACCESS_DENIED), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(foundation::ERROR_INVALID_HANDLE), Some(ERR_INVALID_HANDLE));
    assert_eq!(map(foundation::ERROR_NOT_ENOUGH_MEMORY), Some(ERR_OUT_OF_MEMORY));
    assert_eq!(map(foundation::ERROR_OUTOFMEMORY), Some(ERR_OUT_OF_MEMORY));
    assert_eq!(map(foundation::ERROR_WRITE_PROTECT), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(foundation::ERROR_SHARING_VIOLATION), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(foundation::ERROR_LOCK_VIOLATION), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(foundation::ERROR_HANDLE_EOF), Some(ERR_FAILED));
    assert_eq!(map(foundation::ERROR_HANDLE_DISK_FULL), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(foundation::ERROR_FILE_EXISTS), Some(ERR_FILE_EXISTS));
    assert_eq!(map(foundation::ERROR_INVALID_PARAMETER), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(foundation::ERROR_BUFFER_OVERFLOW), Some(ERR_FILE_PATH_TOO_LONG));
    assert_eq!(map(foundation::ERROR_DISK_FULL), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(foundation::ERROR_CALL_NOT_IMPLEMENTED), Some(ERR_NOT_IMPLEMENTED));
    assert_eq!(map(foundation::ERROR_INVALID_NAME), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(foundation::ERROR_DIR_NOT_EMPTY), Some(ERR_FAILED));
    assert_eq!(map(foundation::ERROR_BUSY), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(foundation::ERROR_ALREADY_EXISTS), Some(ERR_FILE_EXISTS));
    assert_eq!(map(foundation::ERROR_FILENAME_EXCED_RANGE), Some(ERR_FILE_PATH_TOO_LONG));
    assert_eq!(map(foundation::ERROR_FILE_TOO_LARGE), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(foundation::ERROR_VIRUS_INFECTED), Some(ERR_FILE_VIRUS_INFECTED));
    assert_eq!(map(foundation::ERROR_IO_DEVICE), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(foundation::ERROR_POSSIBLE_DEADLOCK), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(foundation::ERROR_BAD_DEVICE), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(foundation::ERROR_BROKEN_PIPE), Some(ERR_CONNECTION_RESET));
}
