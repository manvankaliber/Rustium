// Copyright 2026 The Rustium Authors
// Independent original baseline binding; no migrated Rust policy call.
#ifndef NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_STRING_SPLIT_H_
#define NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_STRING_SPLIT_H_
#include "base/strings/string_split.h"
#include "net/socket/connection_baseline_original_split_internal.h"
namespace net::connection_baseline_original {
inline constexpr std::vector<std::string> SplitString(
    std::string_view input, std::string_view separators,
    base::WhitespaceHandling whitespace, base::SplitResult results) {
  return SplitStringT<std::string>(input, separators, whitespace, results);
}
}  // namespace net::connection_baseline_original
#endif
