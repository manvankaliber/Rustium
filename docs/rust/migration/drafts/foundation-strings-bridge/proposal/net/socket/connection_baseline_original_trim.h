// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_TRIM_H_
#define NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_TRIM_H_

#include <algorithm>
#include <string_view>

#include "net/socket/connection_baseline_original_whitespace.h"

namespace net::connection_baseline_original {

template <typename T, typename CharT = typename T::value_type>
constexpr T TrimStringPieceT(T input,
                             T trim_chars,
                             bool trim_leading,
                             bool trim_trailing) {
  size_t begin = trim_leading ? input.find_first_not_of(trim_chars) : 0;
  size_t end =
      trim_trailing ? input.find_last_not_of(trim_chars) + 1 : input.size();
  return input.substr(std::min(begin, input.size()), end - begin);
}

}  // namespace net::connection_baseline_original

#endif  // NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_TRIM_H_
