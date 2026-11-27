// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Generated named constant layout; Rust owns comparisons and mapping.
use super::ids::*;
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PosixConstants {
    pub eagain: i32,
    pub ewouldblock: i32,
    pub eacces: i32,
    pub enetdown: i32,
    pub etimedout: i32,
    pub econnreset: i32,
    pub enetreset: i32,
    pub epipe: i32,
    pub econnaborted: i32,
    pub econnrefused: i32,
    pub ehostunreach: i32,
    pub ehostdown: i32,
    pub enetunreach: i32,
    pub eafnosupport: i32,
    pub eaddrnotavail: i32,
    pub emsgsize: i32,
    pub enotconn: i32,
    pub eisconn: i32,
    pub einval: i32,
    pub eaddrinuse: i32,
    pub e2big: i32,
    pub ebadf: i32,
    pub ebusy: i32,
    pub ecanceled: i32,
    pub edeadlk: i32,
    pub edquot: i32,
    pub eexist: i32,
    pub efault: i32,
    pub efbig: i32,
    pub eisdir: i32,
    pub enametoolong: i32,
    pub enfile: i32,
    pub enobufs: i32,
    pub enodev: i32,
    pub enoent: i32,
    pub enolck: i32,
    pub enomem: i32,
    pub enospc: i32,
    pub enosys: i32,
    pub enotdir: i32,
    pub enotsup: i32,
    pub eperm: i32,
    pub erofs: i32,
    pub etxtbsy: i32,
    pub eusers: i32,
    pub emfile: i32,
    pub enoprotoopt: i32,
}
pub fn map(error: i32, c: &PosixConstants) -> Option<i32> {
    if error == 0 {
        return Some(0);
    }
    if error == c.eagain {
        return Some(ERR_IO_PENDING);
    }
    if error == c.ewouldblock {
        return Some(ERR_IO_PENDING);
    }
    if error == c.eacces {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.enetdown {
        return Some(ERR_INTERNET_DISCONNECTED);
    }
    if error == c.etimedout {
        return Some(ERR_TIMED_OUT);
    }
    if error == c.econnreset {
        return Some(ERR_CONNECTION_RESET);
    }
    if error == c.enetreset {
        return Some(ERR_CONNECTION_RESET);
    }
    if error == c.epipe {
        return Some(ERR_CONNECTION_RESET);
    }
    if error == c.econnaborted {
        return Some(ERR_CONNECTION_ABORTED);
    }
    if error == c.econnrefused {
        return Some(ERR_CONNECTION_REFUSED);
    }
    if error == c.ehostunreach {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == c.ehostdown {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == c.enetunreach {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == c.eafnosupport {
        return Some(ERR_ADDRESS_UNREACHABLE);
    }
    if error == c.eaddrnotavail {
        return Some(ERR_ADDRESS_INVALID);
    }
    if error == c.emsgsize {
        return Some(ERR_MSG_TOO_BIG);
    }
    if error == c.enotconn {
        return Some(ERR_SOCKET_NOT_CONNECTED);
    }
    if error == c.eisconn {
        return Some(ERR_SOCKET_IS_CONNECTED);
    }
    if error == c.einval {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.eaddrinuse {
        return Some(ERR_ADDRESS_IN_USE);
    }
    if error == c.e2big {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.ebadf {
        return Some(ERR_INVALID_HANDLE);
    }
    if error == c.ebusy {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == c.ecanceled {
        return Some(ERR_ABORTED);
    }
    if error == c.edeadlk {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == c.edquot {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == c.eexist {
        return Some(ERR_FILE_EXISTS);
    }
    if error == c.efault {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.efbig {
        return Some(ERR_FILE_TOO_BIG);
    }
    if error == c.eisdir {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.enametoolong {
        return Some(ERR_FILE_PATH_TOO_LONG);
    }
    if error == c.enfile {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == c.enobufs {
        return Some(ERR_NO_BUFFER_SPACE);
    }
    if error == c.enodev {
        return Some(ERR_INVALID_ARGUMENT);
    }
    if error == c.enoent {
        return Some(ERR_FILE_NOT_FOUND);
    }
    if error == c.enolck {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == c.enomem {
        return Some(ERR_OUT_OF_MEMORY);
    }
    if error == c.enospc {
        return Some(ERR_FILE_NO_SPACE);
    }
    if error == c.enosys {
        return Some(ERR_NOT_IMPLEMENTED);
    }
    if error == c.enotdir {
        return Some(ERR_FILE_NOT_FOUND);
    }
    if error == c.enotsup {
        return Some(ERR_NOT_IMPLEMENTED);
    }
    if error == c.eperm {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.erofs {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.etxtbsy {
        return Some(ERR_ACCESS_DENIED);
    }
    if error == c.eusers {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == c.emfile {
        return Some(ERR_INSUFFICIENT_RESOURCES);
    }
    if error == c.enoprotoopt {
        return Some(ERR_NOT_IMPLEMENTED);
    }
    None
}
#[cfg(test)]
include!("net_error_rust_posix_tests.rs");
