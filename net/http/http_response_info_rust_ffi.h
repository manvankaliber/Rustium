// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_RESPONSE_INFO_RUST_FFI_H_
#define NET_HTTP_HTTP_RESPONSE_INFO_RUST_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "base/memory/raw_ptr_exclusion.h"
// A callback performs one primitive operation. No callback retains a Rust
// reference. C++ owns all public values, Pickles and referenced services.
struct ResponseInfoRustReply {
  int64_t value;
  bool ok;
};
using ResponseInfoRustCallback = ResponseInfoRustReply (*)(void*,
                                                           uint32_t,
                                                           uint32_t,
                                                           int64_t);
struct ResponseInfoRustBridge {
  RAW_PTR_EXCLUSION void* context;
  ResponseInfoRustCallback call;
};
static_assert(sizeof(ResponseInfoRustReply) == 16);
static_assert(offsetof(ResponseInfoRustReply, ok) == 8);
static_assert(std::is_standard_layout_v<ResponseInfoRustBridge>);
namespace net::response_info_rust {
inline constexpr uint32_t kGet = 0;
inline constexpr uint32_t kSet = 1;
inline constexpr uint32_t kReadInt = 2;
inline constexpr uint32_t kReadInt64 = 3;
inline constexpr uint32_t kReadUInt32 = 4;
inline constexpr uint32_t kReadUInt16 = 5;
inline constexpr uint32_t kWriteInt = 6;
inline constexpr uint32_t kWriteInt64 = 7;
inline constexpr uint32_t kWriteUInt32 = 8;
inline constexpr uint32_t kWriteUInt16 = 9;
inline constexpr uint32_t kReadHeaders = 10;
inline constexpr uint32_t kReadCert = 11;
inline constexpr uint32_t kReadSct = 12;
inline constexpr uint32_t kReadVary = 13;
inline constexpr uint32_t kReadHost = 14;
inline constexpr uint32_t kSetEndpoint = 15;
inline constexpr uint32_t kReadAlpn = 16;
inline constexpr uint32_t kReadAlias = 17;
inline constexpr uint32_t kCommitAliases = 18;
inline constexpr uint32_t kReadProxy = 19;
inline constexpr uint32_t kWriteHeaders = 20;
inline constexpr uint32_t kWriteCert = 21;
inline constexpr uint32_t kWriteVary = 22;
inline constexpr uint32_t kWriteHost = 23;
inline constexpr uint32_t kWriteAlpn = 24;
inline constexpr uint32_t kWriteAlias = 25;
inline constexpr uint32_t kWriteProxy = 26;
inline constexpr uint32_t kCipherEcdhe = 27;
inline constexpr uint32_t kSetTruncated = 28;
inline constexpr uint32_t kCopyField = 29;
inline constexpr uint32_t kUnexpectedVersion = 30;
inline constexpr uint32_t kCached = 0;
inline constexpr uint32_t kSpdy = 1;
inline constexpr uint32_t kAlpn = 2;
inline constexpr uint32_t kAuth = 3;
inline constexpr uint32_t kUnused = 4;
inline constexpr uint32_t kRestricted = 5;
inline constexpr uint32_t kDictionary = 6;
inline constexpr uint32_t kPkp = 7;
inline constexpr uint32_t kEch = 8;
inline constexpr uint32_t kConnection = 9;
inline constexpr uint32_t kCertStatus = 10;
inline constexpr uint32_t kSslConnection = 11;
inline constexpr uint32_t kGroup = 12;
inline constexpr uint32_t kSignature = 13;
inline constexpr uint32_t kRequest = 14;
inline constexpr uint32_t kResponse = 15;
inline constexpr uint32_t kOriginal = 16;
inline constexpr uint32_t kStale = 17;
inline constexpr uint32_t kBody = 18;
inline constexpr uint32_t kZstd = 19;
inline constexpr uint32_t kCertValid = 20;
inline constexpr uint32_t kVaryValid = 21;
inline constexpr uint32_t kProxyValid = 22;
inline constexpr uint32_t kProxyDirect = 23;
inline constexpr uint32_t kAliasCount = 24;
inline constexpr uint32_t kBodyPresent = 25;
inline constexpr uint32_t kZstdPresent = 26;
}  // namespace net::response_info_rust
extern "C" bool rustium_response_info(ResponseInfoRustBridge,
                                      uint32_t operation,
                                      bool first,
                                      bool second);
#endif
