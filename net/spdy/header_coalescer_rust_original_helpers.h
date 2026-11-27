// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Frozen original helper closure; no production HttpUtil dependency.
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "base/compiler_specific.h"

namespace net::header_coalescer_rust_original_helpers {
class HttpUtil {
 public:
  static bool IsValidHeaderName(std::string_view);
  static bool IsValidHeaderValue(std::string_view);
  static bool IsToken(std::string_view);
  static bool IsTokenChar(char);
  static bool IsLWS(char c) { return c == ' ' || c == '\t'; }
  static std::string_view TrimLWS(std::string_view);
  static void TrimLWS(std::string_view, size_t&, size_t&);
};
bool HttpUtil::IsValidHeaderName(std::string_view name) {
  // Check whether the header name is RFC 2616-compliant.
  return HttpUtil::IsToken(name);
}
bool HttpUtil::IsValidHeaderValue(std::string_view value) {
  // Just a sanity check: disallow NUL, CR and LF.
  for (char c : value) {
    if (c == '\0' || c == '\r' || c == '\n') {
      return false;
    }
  }
  return true;
}
bool HttpUtil::IsTokenChar(char c) {
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
bool HttpUtil::IsToken(std::string_view string) {
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
std::string_view HttpUtil::TrimLWS(std::string_view string) {
  size_t begin_offset = 0;
  size_t end_offset = string.size();
  TrimLWS(string, begin_offset, end_offset);
  return string.substr(begin_offset, end_offset - begin_offset);
}
void HttpUtil::TrimLWS(std::string_view string,
                       size_t& begin_offset,
                       size_t& end_offset) {
  // Leading whitespace
  while (begin_offset < end_offset && HttpUtil::IsLWS(string[begin_offset])) {
    ++begin_offset;
  }

  // Trailing whitespace
  while (begin_offset < end_offset && HttpUtil::IsLWS(string[end_offset - 1])) {
    --end_offset;
  }
}

// Frozen base ASCII equality and split/trim specializations. Library primitives
// (ranges::equal and string_view find/substr) remain C++ services, not Rust.
inline char ToLowerASCII(char c) {
  return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}
inline bool EqualsCaseInsensitiveASCII(std::string_view a, std::string_view b) {
  return std::ranges::equal(a, b, [](auto lhs, auto rhs) {
    return ToLowerASCII(lhs) == ToLowerASCII(rhs);
  });
}
inline std::vector<std::string_view> Split(std::string_view input) {
  std::vector<std::string_view> result;
  const std::string_view delimiter = "\r\n";
  // Specialized from SplitStringUsingSubstrT, TRIM_WHITESPACE/NONEMPTY.
  for (size_t begin_index = 0, end_index = 0;
       end_index != std::string_view::npos;
       begin_index = end_index + delimiter.size()) {
    end_index = input.find(delimiter, begin_index);
    std::string_view term =
        end_index == std::string_view::npos
            ? input.substr(begin_index)
            : input.substr(begin_index, end_index - begin_index);
    const size_t first = term.find_first_not_of("\t\n\v\f\r ");
    const size_t last = term.find_last_not_of("\t\n\v\f\r ");
    term = first == std::string_view::npos
               ? std::string_view()
               : term.substr(first, last - first + 1);
    if (!term.empty()) {
      result.emplace_back(term);
    }
  }
  return result;
}
}  // namespace net::header_coalescer_rust_original_helpers

namespace net::header_coalescer_rust_original_helpers {
// Specializations of the frozen ASCII and escaping primitives. No production
// NetLog value/redaction/HttpUtil or auth-tokenizer policy is imported.
inline bool IsAsciiUpper(char c) {
  return c >= 'A' && c <= 'Z';
}
inline std::string ToLowerASCII(std::string_view input) {
  std::string out(input);
  for (char& c : out) {
    c = ToLowerASCII(c);
  }
  return out;
}
inline base::Value NetLogStringValue(std::string_view raw) {
  if (std::ranges::all_of(raw, [](unsigned char c) { return c < 128; })) {
    return base::Value(raw);
  }
  std::string out = "%ESCAPED:\xE2\x80\x8B ";
  constexpr char digits[] = "0123456789ABCDEF";
  for (unsigned char c : raw) {
    if (c >= 128 || c == '%') {
      out += '%';
      // SAFETY: c is an unsigned byte, so its high nibble is at most 15;
      // digits contains all 16 hexadecimal characters plus a terminator.
      out += UNSAFE_BUFFERS(digits[c >> 4]);
      out += digits[c & 15];
    } else {
      out += c;
    }
  }
  return base::Value(std::move(out));
}
class HttpAuthChallengeTokenizer {
 public:
  explicit HttpAuthChallengeTokenizer(std::string_view challenge)
      : challenge_(challenge) {
    Init(challenge);
  }
  std::string_view challenge_text() const { return challenge_; }
  const std::string& auth_scheme() const { return lower_case_scheme_; }
  std::string_view params() const { return params_; }

 private:
  void Init(std::string_view challenge) {
    base::StringViewTokenizer tok(challenge, " \t");
    if (!tok.GetNext()) {
      return;
    }
    lower_case_scheme_ = ToLowerASCII(tok.token_piece());
    params_ =
        HttpUtil::TrimLWS(std::string_view(tok.token_end(), challenge.end()));
  }
  std::string_view challenge_, params_;
  std::string lower_case_scheme_;
};
inline bool NetLogCaptureIncludesSensitive(NetLogCaptureMode mode) {
  return mode >= NetLogCaptureMode::kIncludeSensitive;
}
bool ShouldRedactChallenge(HttpAuthChallengeTokenizer* challenge) {
  // Ignore lines with commas, as they may contain lists of schemes, and
  // the information we want to hide is Base64 encoded, so has no commas.
  if (challenge->challenge_text().find(',') != std::string::npos) {
    return false;
  }

  const std::string& scheme = challenge->auth_scheme();
  // Invalid input.
  if (scheme.empty()) {
    return false;
  }

  // Ignore Basic and Digest authentication challenges, as they contain
  // public information.
  if (scheme == "basic" || scheme == "digest") {
    return false;
  }

  return true;
}

std::string ElideHeaderValueForNetLog(NetLogCaptureMode capture_mode,
                                      std::string_view header,
                                      std::string_view value) {
  std::string_view redact;

  if (!header_coalescer_rust_original_helpers::NetLogCaptureIncludesSensitive(
          capture_mode)) {
    if (EqualsCaseInsensitiveASCII(header, "set-cookie") ||
        EqualsCaseInsensitiveASCII(header, "set-cookie2") ||
        EqualsCaseInsensitiveASCII(header, "cookie") ||
        EqualsCaseInsensitiveASCII(header, "authorization") ||
        EqualsCaseInsensitiveASCII(header, "proxy-authorization")) {
      redact = value;
    } else if (EqualsCaseInsensitiveASCII(header, "www-authenticate") ||
               EqualsCaseInsensitiveASCII(header, "proxy-authenticate")) {
      // Look for authentication information from data received from the server
      // in multi-round Negotiate authentication.
      HttpAuthChallengeTokenizer challenge(value);
      if (ShouldRedactChallenge(&challenge)) {
        redact = challenge.params();
      }
    }
  }

  if (redact.empty()) {
    return std::string(value);
  }

  // Create string_views that contain the part of `value` before the `redact`
  // substring, and the value after it. Need to use the data() field of the two
  // string_views to figure out where `redact` appears within `value`.
  size_t redact_offset = redact.data() - value.data();
  std::string_view value_before_redact = value.substr(0, redact_offset);
  std::string_view value_after_redact =
      value.substr(redact_offset + redact.length());

  return base::StrCat({value_before_redact,
                       base::StringPrintf("[%ld bytes were stripped]",
                                          static_cast<long>(redact.length())),
                       value_after_redact});
}

}  // namespace net::header_coalescer_rust_original_helpers
