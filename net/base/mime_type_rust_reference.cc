// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Frozen before Rust migration. No dependency on public migrated parsers.
#include "net/base/mime_type_rust_reference.h"

#include <algorithm>
#include <array>

#include "base/check_op.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#define HTTP_LWS " \t"
namespace net::mime_type_test {
namespace {
class FrozenHttpUtil {
 public:
  static bool IsLWS(char c) { return c == ' ' || c == '\t'; }
  static std::string_view TrimLWS(std::string_view s) {
    size_t begin = 0, end = s.size();
    while (begin < end && IsLWS(s[begin])) {
      ++begin;
    }
    while (begin < end && IsLWS(s[end - 1])) {
      --end;
    }
    return s.substr(begin, end - begin);
  }
  static bool IsTokenChar(char c);
  static bool IsToken(std::string_view string);
};
bool FrozenHttpUtil::IsTokenChar(char c) {
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
bool FrozenHttpUtil::IsToken(std::string_view string) {
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
}  // namespace

bool LegacyParseMimeType(std::string_view type_str,
                         std::string* mime_type,
                         base::StringPairs* params) {
  // Trim leading and trailing whitespace from type.  We include '(' in
  // the trailing trim set to catch media-type comments, which are not at all
  // standard, but may occur in rare cases.
  size_t type_val = type_str.find_first_not_of(HTTP_LWS);
  type_val = std::min(type_val, type_str.length());
  size_t type_end = type_str.find_first_of(HTTP_LWS ";(", type_val);
  if (type_end == std::string::npos) {
    type_end = type_str.length();
  }

  // Reject a mime-type if it does not include a slash.
  size_t slash_pos = type_str.find_first_of('/');
  if (slash_pos == std::string::npos || slash_pos > type_end) {
    return false;
  }
  if (mime_type) {
    *mime_type = type_str.substr(type_val, type_end - type_val);
  }

  // Iterate over parameters. Can't split the string around semicolons
  // preemptively because quoted strings may include semicolons. Mostly matches
  // logic in https://mimesniff.spec.whatwg.org/. Main differences: Does not
  // validate characters are HTTP token code points / HTTP quoted-string token
  // code points, and ignores spaces after "=" in parameters.
  if (params) {
    params->clear();
  }
  std::string::size_type offset = type_str.find_first_of(';', type_end);
  while (offset < type_str.size()) {
    DCHECK_EQ(';', type_str[offset]);
    // Trim off the semicolon.
    ++offset;

    // Trim off any following spaces.
    offset = type_str.find_first_not_of(HTTP_LWS, offset);
    std::string::size_type param_name_start = offset;

    // Extend parameter name until run into a semicolon or equals sign.  Per
    // spec, trailing spaces are not removed.
    offset = type_str.find_first_of(";=", offset);

    // Nothing more to do if at end of string, or if there's no parameter
    // value, since names without values aren't allowed.
    if (offset == std::string::npos || type_str[offset] == ';') {
      continue;
    }

    auto param_name =
        type_str.substr(param_name_start, offset - param_name_start);

    // Now parse the value.
    DCHECK_EQ('=', type_str[offset]);
    // Trim off the '='.
    offset++;

    // Remove leading spaces. This violates the spec, though it matches
    // pre-existing behavior.
    //
    // TODO(mmenke): Consider doing this (only?) after parsing quotes, which
    // seems to align more with the spec - not the content-type spec, but the
    // GET spec's way of getting an encoding, and the spec for handling
    // boundary values as well.
    // See https://encoding.spec.whatwg.org/#names-and-labels.
    offset = type_str.find_first_not_of(HTTP_LWS, offset);

    std::string param_value;
    if (offset == std::string::npos || type_str[offset] == ';') {
      // Nothing to do here - an unquoted string of only whitespace should be
      // skipped.
      continue;
    } else if (type_str[offset] != '"') {
      // If the first character is not a quotation mark, copy data directly.
      std::string::size_type value_start = offset;
      offset = type_str.find_first_of(';', offset);
      std::string::size_type value_end = offset;

      // Remove terminal whitespace. If ran off the end of the string, have to
      // update |value_end| first.
      if (value_end == std::string::npos) {
        value_end = type_str.size();
      }
      while (value_end > value_start &&
             FrozenHttpUtil::IsLWS(type_str[value_end - 1])) {
        --value_end;
      }

      param_value = type_str.substr(value_start, value_end - value_start);
    } else {
      // Otherwise, append data, with special handling for backslashes, until
      // a close quote.  Do not trim whitespace for quoted-string.

      // Skip open quote.
      DCHECK_EQ('"', type_str[offset]);
      ++offset;

      while (offset < type_str.size() && type_str[offset] != '"') {
        // Skip over backslash and append the next character, when not at
        // the end of the string. Otherwise, copy the next character (Which may
        // be a backslash).
        if (type_str[offset] == '\\' && offset + 1 < type_str.size()) {
          ++offset;
        }
        param_value += type_str[offset];
        ++offset;
      }

      offset = type_str.find_first_of(';', offset);
    }
    if (params) {
      params->emplace_back(param_name, param_value);
    }
  }
  return true;
}
bool LegacyWithoutParameter(std::string_view type_string,
                            std::string* top_level_type,
                            std::string* subtype) {
  std::vector<std::string_view> components = base::SplitStringPiece(
      type_string, "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (components.size() != 2) {
    return false;
  }
  components[0] = TrimWhitespaceASCII(components[0], base::TRIM_LEADING);
  components[1] = TrimWhitespaceASCII(components[1], base::TRIM_TRAILING);
  if (!FrozenHttpUtil::IsToken(components[0]) ||
      !FrozenHttpUtil::IsToken(components[1])) {
    return false;
  }

  if (top_level_type) {
    top_level_type->assign(std::string(components[0]));
  }

  if (subtype) {
    subtype->assign(std::string(components[1]));
  }

  return true;
}
void LegacyContentType(std::string_view content_type_str,
                       std::string* mime_type,
                       std::string* charset,
                       bool* had_charset,
                       std::string* boundary) {
  std::string mime_type_value;
  base::StringPairs params;
  bool result =
      LegacyParseMimeType(content_type_str, &mime_type_value, &params);
  // If the server sent "*/*", it is meaningless, so do not store it.
  // Also, reject a mime-type if it does not include a slash.
  // Some servers give junk after the charset parameter, which may
  // include a comma, so this check makes us a bit more tolerant.
  if (!result || content_type_str == "*/*") {
    return;
  }

  std::string charset_value;
  bool type_has_charset = false;
  bool type_has_boundary = false;
  for (const auto& param : params) {
    // Trim LWS from param value, ParseMimeType() leaves WS for quoted-string.
    // TODO(mmenke): Check that name has only valid characters.
    if (!type_has_charset &&
        base::EqualsCaseInsensitiveASCII(param.first, "charset")) {
      type_has_charset = true;
      charset_value = std::string(FrozenHttpUtil::TrimLWS(param.second));
      continue;
    }

    if (boundary && !type_has_boundary &&
        base::EqualsCaseInsensitiveASCII(param.first, "boundary")) {
      type_has_boundary = true;
      *boundary = std::string(FrozenHttpUtil::TrimLWS(param.second));
      continue;
    }
  }

  // If `mime_type_value` is the same as `mime_type`, then just update
  // `charset`. However, if `charset` is empty and `mime_type` hasn't changed,
  // then don't wipe-out an existing `charset`.
  bool eq = base::EqualsCaseInsensitiveASCII(mime_type_value, *mime_type);
  if (!eq) {
    *mime_type = base::ToLowerASCII(mime_type_value);
  }
  if ((!eq && *had_charset) || type_has_charset) {
    *had_charset = true;
    *charset = base::ToLowerASCII(charset_value);
  }
}
}  // namespace net::mime_type_test
