#ifndef NET_HTTP_CONNECTION_HEADER_ORIGINAL_HELPERS_H_
#define NET_HTTP_CONNECTION_HEADER_ORIGINAL_HELPERS_H_
// Frozen original helper closure; no production HttpUtil dependency.
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>
namespace net::connection_header_original_helpers {
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
}  // namespace net::connection_header_original_helpers

#endif
