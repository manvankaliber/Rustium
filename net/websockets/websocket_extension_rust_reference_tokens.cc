#include <array>
#include <cstdint>

#include "net/websockets/websocket_extension_rust_reference.h"
namespace net::websocket_extension_rust_reference {
bool IsTokenChar(char c) {
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
bool IsToken(std::string_view string) {
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

}  // namespace net::websocket_extension_rust_reference
