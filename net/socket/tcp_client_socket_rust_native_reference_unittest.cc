// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/tcp_client_socket_rust_native_reference.h"

#include <cstdint>
#include <limits>

#include "build/build_config.h"
#include "net/base/net_errors.h"
#include "net/socket/tcp_client_socket.h"
#include "net/socket/tcp_client_socket_rust_native_reference_error.h"
#include "net/socket/tcp_client_socket_rust_reference.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#include <winsock2.h>
#endif

namespace net {
namespace {
namespace frozen = tcp_client_socket_rust_native_reference;

#define NET_ERROR(label, value) \
  static_assert(static_cast<int>(frozen::ERR_##label) == ERR_##label);
#include "net/socket/tcp_client_socket_rust_native_reference_error_list.h"
#undef NET_ERROR
static_assert(static_cast<int>(frozen::OK) == OK);

TEST(TCPClientSocketRustNativeReferenceTest, NativePublicStorageShape) {
  EXPECT_EQ(sizeof(TCPSocket),
            sizeof(TCPClientSocketRustNativeReferenceSocket));
  EXPECT_EQ(alignof(TCPSocket),
            alignof(TCPClientSocketRustNativeReferenceSocket));
  EXPECT_EQ(sizeof(TCPClientSocket), sizeof(TCPClientSocketRustReference));
  EXPECT_EQ(alignof(TCPClientSocket), alignof(TCPClientSocketRustReference));
}

#if BUILDFLAG(IS_WIN)
TEST(TCPClientSocketRustNativeReferenceTest, NativeSystemErrorInputs) {
  const logging::SystemErrorCode inputs[] = {
      0,
#define NET_ERROR_RUST_CONSTANT(name) name,
#include "net/base/net_error_rust_win_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
      std::numeric_limits<uint32_t>::max(), 0x80000000u, 0x7fffffffu};
  for (auto input : inputs) {
    EXPECT_EQ(static_cast<int>(frozen::MapSystemError(input)),
              static_cast<int>(MapSystemError(input)))
        << input;
  }
}
#endif
}  // namespace
}  // namespace net
