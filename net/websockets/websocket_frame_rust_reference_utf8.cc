// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Frozen original UTF-8 helper closure, mechanically namespace/macro renamed.
#include "net/websockets/websocket_frame_rust_reference_utf8.h"

#include "base/compiler_specific.h"
#include "net/websockets/websocket_frame_rust_reference_icu_utf.h"
namespace net::websocket_frame_rust_reference {
namespace {
inline bool IsValidCodepoint(
    websocket_frame_rust_reference_icu::UChar32 code_point) {
  // Excludes code points that are not Unicode scalar values, i.e.
  // surrogate code points ([0xD800, 0xDFFF]). Additionally, excludes
  // code points larger than 0x10FFFF (the highest codepoint allowed).
  // Non-characters and unassigned code points are allowed.
  // https://unicode.org/glossary/#unicode_scalar_value
  return (code_point >= 0 && code_point < 0xD800) ||
         (code_point >= 0xE000 && code_point <= 0x10FFFF);
}
template <bool (*Validator)(websocket_frame_rust_reference_icu::UChar32)>
inline bool DoIsStringUTF8(std::string_view str) {
  const uint8_t* src = reinterpret_cast<const uint8_t*>(str.data());
  size_t src_len = str.length();
  size_t char_index = 0;

  while (char_index < src_len) {
    websocket_frame_rust_reference_icu::UChar32 code_point;
    UNSAFE_TODO(WFREF_CBU8_NEXT(src, char_index, src_len, code_point));
    if (!Validator(code_point)) {
      return false;
    }
  }
  return true;
}

}  // namespace
bool IsStringUTF8AllowingNoncharacters(std::string_view str) {
  return DoIsStringUTF8<IsValidCodepoint>(str);
}
}  // namespace net::websocket_frame_rust_reference
