// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Synthetic named constants exercise every policy case on any host.
#[test]
fn mappings_posix() {
    use super::*;
    let c = PosixConstants {
        eagain: 100,
        ewouldblock: 101,
        eacces: 102,
        enetdown: 103,
        etimedout: 104,
        econnreset: 105,
        enetreset: 106,
        epipe: 107,
        econnaborted: 108,
        econnrefused: 109,
        ehostunreach: 110,
        ehostdown: 111,
        enetunreach: 112,
        eafnosupport: 113,
        eaddrnotavail: 114,
        emsgsize: 115,
        enotconn: 116,
        eisconn: 117,
        einval: 118,
        eaddrinuse: 119,
        e2big: 120,
        ebadf: 121,
        ebusy: 122,
        ecanceled: 123,
        edeadlk: 124,
        edquot: 125,
        eexist: 126,
        efault: 127,
        efbig: 128,
        eisdir: 129,
        enametoolong: 130,
        enfile: 131,
        enobufs: 132,
        enodev: 133,
        enoent: 134,
        enolck: 135,
        enomem: 136,
        enospc: 137,
        enosys: 138,
        enotdir: 139,
        enotsup: 140,
        eperm: 141,
        erofs: 142,
        etxtbsy: 143,
        eusers: 144,
        emfile: 145,
        enoprotoopt: 146,
    };
    assert_eq!(map(0, &c), Some(0));
    assert_eq!(map(c.eagain, &c), Some(ERR_IO_PENDING));
    assert_eq!(map(c.ewouldblock, &c), Some(ERR_IO_PENDING));
    assert_eq!(map(c.eacces, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.enetdown, &c), Some(ERR_INTERNET_DISCONNECTED));
    assert_eq!(map(c.etimedout, &c), Some(ERR_TIMED_OUT));
    assert_eq!(map(c.econnreset, &c), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(c.enetreset, &c), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(c.epipe, &c), Some(ERR_CONNECTION_RESET));
    assert_eq!(map(c.econnaborted, &c), Some(ERR_CONNECTION_ABORTED));
    assert_eq!(map(c.econnrefused, &c), Some(ERR_CONNECTION_REFUSED));
    assert_eq!(map(c.ehostunreach, &c), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(c.ehostdown, &c), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(c.enetunreach, &c), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(c.eafnosupport, &c), Some(ERR_ADDRESS_UNREACHABLE));
    assert_eq!(map(c.eaddrnotavail, &c), Some(ERR_ADDRESS_INVALID));
    assert_eq!(map(c.emsgsize, &c), Some(ERR_MSG_TOO_BIG));
    assert_eq!(map(c.enotconn, &c), Some(ERR_SOCKET_NOT_CONNECTED));
    assert_eq!(map(c.eisconn, &c), Some(ERR_SOCKET_IS_CONNECTED));
    assert_eq!(map(c.einval, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.eaddrinuse, &c), Some(ERR_ADDRESS_IN_USE));
    assert_eq!(map(c.e2big, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.ebadf, &c), Some(ERR_INVALID_HANDLE));
    assert_eq!(map(c.ebusy, &c), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(c.ecanceled, &c), Some(ERR_ABORTED));
    assert_eq!(map(c.edeadlk, &c), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(c.edquot, &c), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(c.eexist, &c), Some(ERR_FILE_EXISTS));
    assert_eq!(map(c.efault, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.efbig, &c), Some(ERR_FILE_TOO_BIG));
    assert_eq!(map(c.eisdir, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.enametoolong, &c), Some(ERR_FILE_PATH_TOO_LONG));
    assert_eq!(map(c.enfile, &c), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(c.enobufs, &c), Some(ERR_NO_BUFFER_SPACE));
    assert_eq!(map(c.enodev, &c), Some(ERR_INVALID_ARGUMENT));
    assert_eq!(map(c.enoent, &c), Some(ERR_FILE_NOT_FOUND));
    assert_eq!(map(c.enolck, &c), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(c.enomem, &c), Some(ERR_OUT_OF_MEMORY));
    assert_eq!(map(c.enospc, &c), Some(ERR_FILE_NO_SPACE));
    assert_eq!(map(c.enosys, &c), Some(ERR_NOT_IMPLEMENTED));
    assert_eq!(map(c.enotdir, &c), Some(ERR_FILE_NOT_FOUND));
    assert_eq!(map(c.enotsup, &c), Some(ERR_NOT_IMPLEMENTED));
    assert_eq!(map(c.eperm, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.erofs, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.etxtbsy, &c), Some(ERR_ACCESS_DENIED));
    assert_eq!(map(c.eusers, &c), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(c.emfile, &c), Some(ERR_INSUFFICIENT_RESOURCES));
    assert_eq!(map(c.enoprotoopt, &c), Some(ERR_NOT_IMPLEMENTED));
    let mut c = c;
    c.ewouldblock = c.eagain;
    assert_eq!(map(c.eagain, &c), Some(ERR_IO_PENDING));
    assert_eq!(map(1, &c), None);
    assert_eq!(map(i32::MIN, &c), None);
    assert_eq!(map(-1, &c), None);
}
