// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Frozen range parsers plus their original same-TU trimming/tokenizer helpers.
// Helpers are local to avoid adding asymmetric imported-DLL calls in
// benchmarks.
#include "net/http/http_range_promote_reference.h"

#include <limits>
#include <string>

#include "base/check_op.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "net/base/parse_number.h"
#include "net/base/parse_number_native_reference.h"
namespace net::http_range_promote_internal::test {
namespace {
constexpr std::string_view kBytesUnit = "bytes";
bool IsLWS(char c) {
  return c == ' ' || c == '\t';
}
void TrimLWS(std::string_view string,
             size_t& begin_offset,
             size_t& end_offset) {
  // Leading whitespace
  while (begin_offset < end_offset && IsLWS(string[begin_offset])) {
    ++begin_offset;
  }

  // Trailing whitespace
  while (begin_offset < end_offset && IsLWS(string[end_offset - 1])) {
    --end_offset;
  }
}
std::string_view TrimLWS(std::string_view string) {
  size_t begin_offset = 0;
  size_t end_offset = string.size();
  TrimLWS(string, begin_offset, end_offset);
  return string.substr(begin_offset, end_offset - begin_offset);
}
class ValuesIterator {
 public:
  ValuesIterator(std::string_view values,
                 char delimiter,
                 bool ignore_empty_values = true);

  ValuesIterator(const ValuesIterator& other);
  ~ValuesIterator();

  // Advances the iterator to the next value, if any.  Returns true if there
  // is a next value.  Use value* methods to access the resultant value.
  bool GetNext();

  std::string_view value() const {
    return values_.substr(value_begin_, value_end_ - value_begin_);
  }

  // The begin/end offsets of the current value, relative to the start of
  // `values`.
  size_t value_begin() const { return value_begin_; }
  size_t value_end() const { return value_end_; }

 private:
  // The original input value.
  std::string_view values_;

  bool ignore_empty_values_;

  base::StringViewTokenizer tokenizer_;

  // These internally track the range of the current value withint `values_`,
  // to can provide begin/end indices for the current value for
  // HttpResponseHeaders, the only consumer that needs them.
  size_t value_begin_ = 0u;
  size_t value_end_ = 0u;
};
ValuesIterator::ValuesIterator(std::string_view values,
                               char delimiter,
                               bool ignore_empty_values)
    : values_(values),
      ignore_empty_values_(ignore_empty_values),
      // It's important to use `values_` here rather than `value`, since
      // subtracting two string_view::iterators from each other is not
      // guaranteed to work, if they're from different string_views, even if one
      // is a copy of the other. Note that StringViewTokenizer uses iterators to
      // the passed in string_view, rather than a copying it.
      tokenizer_(values_, std::string(1, delimiter)) {
  tokenizer_.set_quote_chars("\"");
  // Could set this unconditionally, since code below has to check for empty
  // values after trimming, anyways, but may provide a minor performance
  // improvement.
  if (!ignore_empty_values_) {
    tokenizer_.set_options(base::StringTokenizer::RETURN_EMPTY_TOKENS);
  }
}

ValuesIterator::ValuesIterator(const ValuesIterator& other) = default;

ValuesIterator::~ValuesIterator() = default;

bool ValuesIterator::GetNext() {
  while (tokenizer_.GetNext()) {
    // Since `tokenizer_` was constructed using `values_`, this is subtracting
    // iterators for the exact same string_view, rather than to two copies of
    // the same string_view, so is safe.
    value_begin_ = tokenizer_.token_begin() - values_.begin();
    value_end_ = tokenizer_.token_end() - values_.begin();
    TrimLWS(values_, value_begin_, value_end_);

    if (!ignore_empty_values_ || value_begin_ != value_end_) {
      return true;
    }
  }
  return false;
}

}  // namespace
bool ParseRangeHeader(std::string_view ranges_specifier,
                      std::vector<HttpByteRange>* ranges) {
  size_t equal_char_offset = ranges_specifier.find('=');
  if (equal_char_offset == std::string::npos) {
    return false;
  }

  // Try to extract bytes-unit part.
  std::string_view bytes_unit = ranges_specifier.substr(0, equal_char_offset);

  // "bytes" unit identifier is not found.
  bytes_unit = TrimLWS(bytes_unit);
  if (!base::EqualsCaseInsensitiveASCII(bytes_unit, kBytesUnit)) {
    return false;
  }

  ValuesIterator byte_range_set_iterator(
      ranges_specifier.substr(equal_char_offset + 1),
      /*delimiter=*/',');
  while (byte_range_set_iterator.GetNext()) {
    std::string_view value = byte_range_set_iterator.value();
    size_t minus_char_offset = value.find('-');
    // If '-' character is not found, reports failure.
    if (minus_char_offset == std::string::npos) {
      return false;
    }

    std::string_view first_byte_pos = value.substr(0, minus_char_offset);
    first_byte_pos = TrimLWS(first_byte_pos);

    HttpByteRange range;
    // Try to obtain first-byte-pos.
    if (!first_byte_pos.empty()) {
      int64_t first_byte_position = -1;
      if (!base::StringToInt64(first_byte_pos, &first_byte_position)) {
        return false;
      }
      range.set_first_byte_position(first_byte_position);
    }

    std::string_view last_byte_pos = value.substr(minus_char_offset + 1);
    last_byte_pos = TrimLWS(last_byte_pos);

    // We have last-byte-pos or suffix-byte-range-spec in this case.
    if (!last_byte_pos.empty()) {
      int64_t last_byte_position;
      if (!base::StringToInt64(last_byte_pos, &last_byte_position)) {
        return false;
      }
      if (range.HasFirstBytePosition()) {
        range.set_last_byte_position(last_byte_position);
      } else {
        range.set_suffix_length(last_byte_position);
      }
    } else if (!range.HasFirstBytePosition()) {
      return false;
    }

    // Do a final check on the HttpByteRange object.
    if (!range.IsValid()) {
      return false;
    }
    ranges->push_back(range);
  }
  return !ranges->empty();
}

std::optional<HttpByteRange> ParseFetchSingleRange(
    std::string_view range_header_value,
    bool allow_whitespace) {
  // Fetch requires the range unit to be exactly "bytes".
  if (!base::StartsWith(range_header_value, kBytesUnit)) {
    return std::nullopt;
  }
  size_t pos = kBytesUnit.size();

  // Returns whether there is still input left to read.
  auto in_bounds = [&]() { return pos < range_header_value.size(); };

  // Consume spec-allowed whitespace.
  auto skip_optional_whitespace = [&]() {
    if (!allow_whitespace) {
      return;
    }
    while (in_bounds() && (range_header_value[pos] == ' ' ||
                           range_header_value[pos] == '\t')) {
      ++pos;
    }
  };

  // Skip optional whitespace, consume `delimiter`, then skip optional
  // whitespace. Used for the grammar delimiters '=' and '-'.
  auto consume_delimiter = [&](char delimiter) {
    skip_optional_whitespace();
    if (!in_bounds() || range_header_value[pos] != delimiter) {
      return false;
    }
    ++pos;
    skip_optional_whitespace();
    return true;
  };

  // Values too large for int64_t are saturated to int64_t max.
  auto parse_decimal_number = [&]() -> std::optional<int64_t> {
    size_t start = pos;
    while (in_bounds() && base::IsAsciiDigit(range_header_value[pos])) {
      ++pos;
    }
    if (pos == start) {
      return std::nullopt;
    }
    int64_t parsed = 0;
    if (!parse_number_native_internal::test::ParseInt64(
            range_header_value.substr(start, pos - start),
            ParseIntFormat::NON_NEGATIVE, &parsed, nullptr)) {
      // If parsing fails, the value is larger than int64_t can represent.
      // Treat it as int64_t max instead of rejecting it.
      return std::numeric_limits<int64_t>::max();
    }
    return parsed;
  };

  if (!consume_delimiter('=')) {
    return std::nullopt;
  }
  // Parse the range separator.
  std::optional<int64_t> range_start = parse_decimal_number();
  if (!consume_delimiter('-')) {
    return std::nullopt;
  }
  std::optional<int64_t> range_end = parse_decimal_number();
  // Reject trailing input and empty ranges.
  if (in_bounds() || (!range_start && !range_end)) {
    return std::nullopt;
  }

  if (!range_start) {
    return HttpByteRange::Suffix(*range_end);
  }
  if (!range_end) {
    return HttpByteRange::RightUnbounded(*range_start);
  }
  return HttpByteRange::Bounded(*range_start, *range_end);
}

bool ParseContentRangeHeaderFor206(std::string_view content_range_spec,
                                   int64_t* first_byte_position,
                                   int64_t* last_byte_position,
                                   int64_t* instance_length) {
  *first_byte_position = *last_byte_position = *instance_length = -1;
  content_range_spec = TrimLWS(content_range_spec);

  size_t space_position = content_range_spec.find(' ');
  if (space_position == std::string_view::npos) {
    return false;
  }

  // Invalid header if it doesn't contain "bytes-unit".
  if (!base::EqualsCaseInsensitiveASCII(
          TrimLWS(content_range_spec.substr(0, space_position)), kBytesUnit)) {
    return false;
  }

  size_t minus_position = content_range_spec.find('-', space_position + 1);
  if (minus_position == std::string_view::npos) {
    return false;
  }
  size_t slash_position = content_range_spec.find('/', minus_position + 1);
  if (slash_position == std::string_view::npos) {
    return false;
  }

  if (base::StringToInt64(
          TrimLWS(content_range_spec.substr(
              space_position + 1, minus_position - (space_position + 1))),
          first_byte_position) &&
      *first_byte_position >= 0 &&
      base::StringToInt64(
          TrimLWS(content_range_spec.substr(
              minus_position + 1, slash_position - (minus_position + 1))),
          last_byte_position) &&
      *last_byte_position >= *first_byte_position &&
      base::StringToInt64(
          TrimLWS(content_range_spec.substr(slash_position + 1)),
          instance_length) &&
      *instance_length > *last_byte_position) {
    return true;
  }
  *first_byte_position = *last_byte_position = *instance_length = -1;
  return false;
}
}  // namespace net::http_range_promote_internal::test
