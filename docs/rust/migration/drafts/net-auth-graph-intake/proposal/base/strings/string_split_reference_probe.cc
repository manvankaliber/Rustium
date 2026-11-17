// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_split.h"

#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_rust.h"
#include "base/logging.h"
#include "base/strings/string_split_rust_reference_probe.rs.h"

namespace base {
namespace {
namespace wire = internal::rust_string_split;

rust::Slice<const uint8_t> Bytes(std::string_view input) {
  // CXX performs its documented null/zero -> nonnull empty slice conversion.
  return SpanToRustSlice(as_byte_span(input));
}

std::string_view View(std::string_view input, const wire::Range& range) {
  if (range.default_empty) {
    CHECK_EQ(range.length, 0u);
    return std::string_view();
  }
  CHECK_LE(range.offset, input.size());
  CHECK_LE(range.length, input.size() - range.offset);
  // Preserve a null native empty input without pointer arithmetic on null.
  if (range.offset == 0 && range.length == input.size())
    return input;
  return input.substr(range.offset, range.length);
}

std::optional<std::pair<std::string_view, std::string_view>> Once(
    std::string_view input, std::string_view separators, bool reverse) {
  auto result = wire::split_once(Bytes(input), Bytes(separators), reverse);
  if (!result.found)
    return std::nullopt;
  return std::pair(View(input, result.first), View(input, result.second));
}
}  // namespace

std::optional<std::pair<std::string_view, std::string_view>> SplitStringOnce(
    std::string_view input, char separator) {
  return Once(input, std::string_view(&separator, 1), false);
}
std::optional<std::pair<std::string_view, std::string_view>> SplitStringOnce(
    std::string_view input, std::string_view separators) {
  return Once(input, separators, false);
}
std::optional<std::pair<std::string_view, std::string_view>> RSplitStringOnce(
    std::string_view input, char separator) {
  return Once(input, std::string_view(&separator, 1), true);
}
std::optional<std::pair<std::string_view, std::string_view>> RSplitStringOnce(
    std::string_view input, std::string_view separators) {
  return Once(input, separators, true);
}

bool SplitStringIntoKeyValuePairs(std::string_view input, char delimiter,
                                 char pairs_delimiter, StringPairs* output) {
  return SplitStringIntoKeyValuePairsUsingSubstr(
      input, delimiter, std::string_view(&pairs_delimiter, 1), output);
}
bool SplitStringIntoKeyValueViewPairs(std::string_view input, char delimiter,
                                     char pairs_delimiter,
                                     StringViewPairs* output) {
  return SplitStringIntoKeyValueViewPairsUsingSubstr(
      input, delimiter, std::string_view(&pairs_delimiter, 1), output);
}
bool SplitStringIntoKeyValuePairsUsingSubstr(std::string_view input,
                                            char delimiter,
                                            std::string_view pairs_delimiter,
                                            StringPairs* output) {
  StringViewPairs views;
  const bool success = SplitStringIntoKeyValueViewPairsUsingSubstr(
      input, delimiter, pairs_delimiter, &views);
  // Preserve the original parse-before-clearing order for owned outputs.
  output->clear();
  output->reserve(views.size());
  for (const auto& [key, value] : views)
    output->emplace_back(std::string(key), std::string(value));
  return success;
}
bool SplitStringIntoKeyValueViewPairsUsingSubstr(
    std::string_view input, char delimiter, std::string_view pairs_delimiter,
    StringViewPairs* output) {
  output->clear();
  const auto segments = wire::split_bytes(Bytes(input), Bytes(pairs_delimiter),
                                          true, true, true);
  output->reserve(segments.size());
  bool success = true;
  for (const auto& segment_range : segments) {
    const std::string_view segment = View(input, segment_range);
    // Preserve original callback order: parsing a later segment must observe
    // mutations made by the diagnostic callback for an earlier segment.
    auto pair = wire::key_value_segment(Bytes(segment), static_cast<uint8_t>(delimiter));
    output->emplace_back(View(segment, pair.key), View(segment, pair.value));
    if (pair.diagnostic != 0) {
      success = false;
      if (pair.diagnostic == 1) {
        DVLOG(1) << "cannot find delimiter in: " << segment;
      } else {
        CHECK_EQ(pair.diagnostic, 2u);
        DVLOG(1) << "cannot parse value from input: " << segment;
      }
    }
  }
  return success;
}
}  // namespace base
