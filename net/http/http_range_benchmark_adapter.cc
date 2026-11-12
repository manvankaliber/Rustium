// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Exact production function bodies, verified by the benchmark runner.
#include "net/http/http_range_ffi.h"
#include "net/http/http_util.h"
namespace net {
bool HttpUtil::ParseRangeHeader(std::string_view ranges_specifier,
                                std::vector<HttpByteRange>* ranges) {
  if (ranges_specifier.empty()) {
    return false;
  }
  http_range_internal::RangesBuffer buffer;
  const uint8_t state = http_range_internal::rustium_http_range_list(
      reinterpret_cast<const uint8_t*>(ranges_specifier.data()),
      ranges_specifier.size(), &buffer);
  if (state & http_range_internal::kListHasEntries) {
    const http_range_internal::ScopedRangeList owned(buffer);
    // Preserve valid prefixes even if a later entry is malformed.
    for (const auto& parsed : owned.values()) {
      HttpByteRange range;
      range.set_first_byte_position(parsed.first);
      range.set_last_byte_position(parsed.last);
      range.set_suffix_length(parsed.suffix);
      ranges->push_back(range);
    }
  }
  // Invalid input must not inspect caller storage when no prefix was appended.
  return (state & http_range_internal::kListSyntaxValid) && !ranges->empty();
}
std::optional<HttpByteRange> HttpUtil::ParseFetchSingleRange(
    std::string_view range_header_value,
    bool allow_whitespace) {
  http_range_internal::ParsedRange parsed;
  if (!http_range_internal::rustium_http_range_fetch(
          reinterpret_cast<const uint8_t*>(range_header_value.data()),
          range_header_value.size(), allow_whitespace, &parsed)) {
    return std::nullopt;
  }
  HttpByteRange range;
  range.set_first_byte_position(parsed.first);
  range.set_last_byte_position(parsed.last);
  range.set_suffix_length(parsed.suffix);
  return range;
}
bool HttpUtil::ParseContentRangeHeaderFor206(
    std::string_view content_range_spec,
    int64_t* first_byte_position,
    int64_t* last_byte_position,
    int64_t* instance_length) {
  // The adapter initializes outputs before borrowing input and scopes each
  // input borrow between sequential scalar writes, preserving aliases.
  return http_range_internal::rustium_http_range_content(
      reinterpret_cast<const uint8_t*>(content_range_spec.data()),
      content_range_spec.size(), first_byte_position, last_byte_position,
      instance_length);
}
}  // namespace net
