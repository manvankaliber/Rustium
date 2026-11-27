// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Generated paired SDK/binding drift guards. Rust checks the same values.
#ifndef NET_BASE_NET_ERROR_RUST_WIN_SDK_CHECKS_H_
#define NET_BASE_NET_ERROR_RUST_WIN_SDK_CHECKS_H_
#include <cstdint>
static_assert(ERROR_SUCCESS == 0);
static_assert(static_cast<uint32_t>(WSAEWOULDBLOCK) == 10035u);
static_assert(static_cast<uint32_t>(WSA_IO_PENDING) == 997u);
static_assert(static_cast<uint32_t>(WSAEACCES) == 10013u);
static_assert(static_cast<uint32_t>(WSAENETDOWN) == 10050u);
static_assert(static_cast<uint32_t>(WSAETIMEDOUT) == 10060u);
static_assert(static_cast<uint32_t>(WSAECONNRESET) == 10054u);
static_assert(static_cast<uint32_t>(WSAENETRESET) == 10052u);
static_assert(static_cast<uint32_t>(WSAECONNABORTED) == 10053u);
static_assert(static_cast<uint32_t>(WSAECONNREFUSED) == 10061u);
static_assert(static_cast<uint32_t>(WSA_IO_INCOMPLETE) == 996u);
static_assert(static_cast<uint32_t>(WSAEDISCON) == 10101u);
static_assert(static_cast<uint32_t>(WSAEISCONN) == 10056u);
static_assert(static_cast<uint32_t>(WSAEHOSTUNREACH) == 10065u);
static_assert(static_cast<uint32_t>(WSAENETUNREACH) == 10051u);
static_assert(static_cast<uint32_t>(WSAEADDRNOTAVAIL) == 10049u);
static_assert(static_cast<uint32_t>(WSAEMSGSIZE) == 10040u);
static_assert(static_cast<uint32_t>(WSAENOTCONN) == 10057u);
static_assert(static_cast<uint32_t>(WSAEAFNOSUPPORT) == 10047u);
static_assert(static_cast<uint32_t>(WSAEINVAL) == 10022u);
static_assert(static_cast<uint32_t>(WSAEADDRINUSE) == 10048u);
static_assert(static_cast<uint32_t>(WSAENOBUFS) == 10055u);
static_assert(static_cast<uint32_t>(ERROR_FILE_NOT_FOUND) == 2u);
static_assert(static_cast<uint32_t>(ERROR_PATH_NOT_FOUND) == 3u);
static_assert(static_cast<uint32_t>(ERROR_TOO_MANY_OPEN_FILES) == 4u);
static_assert(static_cast<uint32_t>(ERROR_ACCESS_DENIED) == 5u);
static_assert(static_cast<uint32_t>(ERROR_INVALID_HANDLE) == 6u);
static_assert(static_cast<uint32_t>(ERROR_NOT_ENOUGH_MEMORY) == 8u);
static_assert(static_cast<uint32_t>(ERROR_OUTOFMEMORY) == 14u);
static_assert(static_cast<uint32_t>(ERROR_WRITE_PROTECT) == 19u);
static_assert(static_cast<uint32_t>(ERROR_SHARING_VIOLATION) == 32u);
static_assert(static_cast<uint32_t>(ERROR_LOCK_VIOLATION) == 33u);
static_assert(static_cast<uint32_t>(ERROR_HANDLE_EOF) == 38u);
static_assert(static_cast<uint32_t>(ERROR_HANDLE_DISK_FULL) == 39u);
static_assert(static_cast<uint32_t>(ERROR_FILE_EXISTS) == 80u);
static_assert(static_cast<uint32_t>(ERROR_INVALID_PARAMETER) == 87u);
static_assert(static_cast<uint32_t>(ERROR_BUFFER_OVERFLOW) == 111u);
static_assert(static_cast<uint32_t>(ERROR_DISK_FULL) == 112u);
static_assert(static_cast<uint32_t>(ERROR_CALL_NOT_IMPLEMENTED) == 120u);
static_assert(static_cast<uint32_t>(ERROR_INVALID_NAME) == 123u);
static_assert(static_cast<uint32_t>(ERROR_DIR_NOT_EMPTY) == 145u);
static_assert(static_cast<uint32_t>(ERROR_BUSY) == 170u);
static_assert(static_cast<uint32_t>(ERROR_ALREADY_EXISTS) == 183u);
static_assert(static_cast<uint32_t>(ERROR_FILENAME_EXCED_RANGE) == 206u);
static_assert(static_cast<uint32_t>(ERROR_FILE_TOO_LARGE) == 223u);
static_assert(static_cast<uint32_t>(ERROR_VIRUS_INFECTED) == 225u);
static_assert(static_cast<uint32_t>(ERROR_IO_DEVICE) == 1117u);
static_assert(static_cast<uint32_t>(ERROR_POSSIBLE_DEADLOCK) == 1131u);
static_assert(static_cast<uint32_t>(ERROR_BAD_DEVICE) == 1200u);
static_assert(static_cast<uint32_t>(ERROR_BROKEN_PIPE) == 109u);
#endif
