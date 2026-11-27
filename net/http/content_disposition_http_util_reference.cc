// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/content_disposition_http_util_reference.h"

#include <algorithm>
#include <array>

#include "base/check.h"
namespace net::content_disposition_reference {
std::string_view LegacyHttpUtil::TrimLWS(std::string_view string) {
  size_t begin_offset = 0;
  size_t end_offset = string.size();
  TrimLWS(string, begin_offset, end_offset);
  return string.substr(begin_offset, end_offset - begin_offset);
}

// static
void LegacyHttpUtil::TrimLWS(std::string_view string,
                             size_t& begin_offset,
                             size_t& end_offset) {
  // Leading whitespace
  while (begin_offset < end_offset &&
         LegacyHttpUtil::IsLWS(string[begin_offset])) {
    ++begin_offset;
  }

  // Trailing whitespace
  while (begin_offset < end_offset &&
         LegacyHttpUtil::IsLWS(string[end_offset - 1])) {
    --end_offset;
  }
}

bool LegacyHttpUtil::IsTokenChar(char c) {
  // See RFC 7230 Sec 3.2.6.
  static constexpr std::array<uint32_t, 8> kIsTokenCharMask = [] {
    std::array<uint32_t, 8> mask = {};
    for (int i = 0; i < 256; ++i) {
      if (!(i >= 0x7F || i <= 0x20 || i == '(' || i == ')' || i == '<' ||
            i == '>' || i == '@' || i == ',' || i == ';' || i == ':' ||
            i == '\\' || i == '"' || i == '/' || i == '[' || i == ']' ||
            i == '?' || i == '=' || i == '{' || i == '}')) {
        mask[i >> 5] |= (uint32_t{1} << (i & 31));
      }
    }
    return mask;
  }();
  uint8_t index = static_cast<uint8_t>(c);
  return (kIsTokenCharMask[index >> 5] >> (index & 31)) & 1;
}

// See RFC 7230 Sec 3.2.6 for the definition of |token|.
bool LegacyHttpUtil::IsToken(std::string_view string) {
  if (string.empty()) {
    return false;
  }
  for (char c : string) {
    if (!IsTokenChar(c)) {
      return false;
    }
  }
  return true;
}

namespace {

bool IsQuote(char c) {
  return c == '"';
}

bool UnquoteImpl(std::string_view str, bool strict_quotes, std::string* out) {
  if (str.empty()) {
    return false;
  }

  // Nothing to unquote.
  if (!IsQuote(str[0])) {
    return false;
  }

  // No terminal quote mark.
  if (str.size() < 2 || str.front() != str.back()) {
    return false;
  }

  // Strip quotemarks
  str.remove_prefix(1);
  str.remove_suffix(1);

  // Unescape quoted-pair (defined in RFC 2616 section 2.2)
  bool prev_escape = false;
  std::string unescaped;
  for (char c : str) {
    if (c == '\\' && !prev_escape) {
      prev_escape = true;
      continue;
    }
    if (strict_quotes && !prev_escape && IsQuote(c)) {
      return false;
    }
    prev_escape = false;
    unescaped.push_back(c);
  }

  // Terminal quote is escaped.
  if (strict_quotes && prev_escape) {
    return false;
  }

  *out = std::move(unescaped);
  return true;
}

}  // anonymous namespace

// static
std::string LegacyHttpUtil::Unquote(std::string_view str) {
  std::string result;
  if (!UnquoteImpl(str, false, &result)) {
    return std::string(str);
  }

  return result;
}

// static
bool LegacyHttpUtil::StrictUnquote(std::string_view str, std::string* out) {
  return UnquoteImpl(str, true, out);
}

LegacyHttpUtil::ValuesIterator::ValuesIterator(std::string_view values,
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
    tokenizer_.set_options(StringTokenizer::RETURN_EMPTY_TOKENS);
  }
}

LegacyHttpUtil::ValuesIterator::ValuesIterator(const ValuesIterator& other) =
    default;

LegacyHttpUtil::ValuesIterator::~ValuesIterator() = default;

bool LegacyHttpUtil::ValuesIterator::GetNext() {
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

LegacyHttpUtil::NameValuePairsIterator::NameValuePairsIterator(
    std::string_view value,
    char delimiter,
    Values optional_values,
    Quotes strict_quotes)
    : props_(value, delimiter),
      values_optional_(optional_values == Values::NOT_REQUIRED),
      strict_quotes_(strict_quotes == Quotes::STRICT_QUOTES) {}

LegacyHttpUtil::NameValuePairsIterator::NameValuePairsIterator(
    const NameValuePairsIterator& other) = default;

LegacyHttpUtil::NameValuePairsIterator::~NameValuePairsIterator() = default;

// We expect properties to be formatted as one of:
//   name="value"
//   name='value'
//   name='\'value\''
//   name=value
//   name = value
//   name (if values_optional_ is true)
// Due to buggy implementations found in some embedded devices, we also
// accept values with missing close quotemark (http://crbug.com/39836):
//   name="value
bool LegacyHttpUtil::NameValuePairsIterator::GetNext() {
  CHECK(valid_);
  // Not an error, but nothing left to do.
  if (props_.GetNext()) {
    // State only becomes invalid if there's another element, but parsing it
    // fails.
    valid_ = ParseNameValuePair(props_.value());
    if (valid_) {
      return true;
    }
  }

  // Clear all fields when returning false, regardless of whether `valid` is
  // true or not, since any populated data is no longer valid.
  name_ = std::string_view();
  value_ = std::string_view();
  unquoted_value_.clear();
  value_is_quoted_ = false;
  return false;
}

bool LegacyHttpUtil::NameValuePairsIterator::ParseNameValuePair(
    std::string_view name_value_pair) {
  // Scan for the equals sign.
  const size_t equals = name_value_pair.find('=');
  if (equals == 0) {
    return false;  // Malformed, no name
  }
  const bool has_value = (equals != std::string_view::npos);
  if (!has_value && !values_optional_) {
    return false;  // Malformed, no equals sign and values are required
  }

  // Make `name_` everything up until the equals sign.
  name_ = TrimLWS(name_value_pair.substr(0, equals));
  // Clear rest of state.
  value_ = std::string_view();
  value_is_quoted_ = false;
  unquoted_value_.clear();

  // If there is a value, do additional checking and calculate the value.
  if (has_value) {
    // Check that no quote appears before the equals sign.
    if (std::ranges::any_of(name_, IsQuote)) {
      return false;
    }

    // Value consists of everything after the equals sign, with whitespace
    // trimmed.
    value_ = TrimLWS(name_value_pair.substr(equals + 1));
    if (value_.empty()) {
      // Malformed; value is empty
      return false;
    }
  }

  if (has_value && IsQuote(value_.front())) {
    value_is_quoted_ = true;

    if (strict_quotes_) {
      return LegacyHttpUtil::StrictUnquote(value_, &unquoted_value_);
    }

    // Trim surrounding quotemarks off the value
    if (value_.front() != value_.back() || value_.size() == 1) {
      // NOTE: This is not as graceful as it sounds:
      // * quoted-pairs will no longer be unquoted
      //   (["\"hello] should give ["hello]).
      // * Does not detect when the final quote is escaped
      //   (["value\"] should give [value"])
      value_is_quoted_ = false;
      value_ = value_.substr(1);  // Gracefully recover from mismatching quotes.
    } else {
      // Do not store iterators into this. See declaration of `unquoted_value_`.
      unquoted_value_ = LegacyHttpUtil::Unquote(value_);
    }
  }

  return true;
}

}  // namespace net::content_disposition_reference
