// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_RANGE_PROMOTE_REFERENCE_H_
#define NET_HTTP_HTTP_RANGE_PROMOTE_REFERENCE_H_
#include <optional>
#include <string_view>
#include <vector>

#include "net/http/http_byte_range.h"

namespace net::http_range_promote_internal::test {
bool ParseRangeHeader(std::string_view, std::vector<HttpByteRange>*);
std::optional<HttpByteRange> ParseFetchSingleRange(std::string_view, bool);
bool ParseContentRangeHeaderFor206(std::string_view,
                                   int64_t*,
                                   int64_t*,
                                   int64_t*);
}  // namespace net::http_range_promote_internal::test
#endif  // NET_HTTP_HTTP_RANGE_PROMOTE_REFERENCE_H_
