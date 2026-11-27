// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_WHITESPACE_H_
#define NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_WHITESPACE_H_

namespace net::connection_baseline_original {

#define RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_ASCII_NO_CR_LF      \
  0x09,     /* CHARACTER TABULATION */ \
      0x0B, /* LINE TABULATION */      \
      0x0C, /* FORM FEED (FF) */       \
      0x20  /* SPACE */

#define RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_ASCII                                                  \
  RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_ASCII_NO_CR_LF, /* Comment to make clang-format linebreak */ \
      0x0A,                  /* LINE FEED (LF) */                         \
      0x0D                   /* CARRIAGE RETURN (CR) */

#define RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE_NON_ASCII          \
  0x0085,     /* NEXT LINE (NEL) */           \
      0x00A0, /* NO-BREAK SPACE */            \
      0x1680, /* OGHAM SPACE MARK */          \
      0x2000, /* EN QUAD */                   \
      0x2001, /* EM QUAD */                   \
      0x2002, /* EN SPACE */                  \
      0x2003, /* EM SPACE */                  \
      0x2004, /* THREE-PER-EM SPACE */        \
      0x2005, /* FOUR-PER-EM SPACE */         \
      0x2006, /* SIX-PER-EM SPACE */          \
      0x2007, /* FIGURE SPACE */              \
      0x2008, /* PUNCTUATION SPACE */         \
      0x2009, /* THIN SPACE */                \
      0x200A, /* HAIR SPACE */                \
      0x2028, /* LINE SEPARATOR */            \
      0x2029, /* PARAGRAPH SEPARATOR */       \
      0x202F, /* NARROW NO-BREAK SPACE */     \
      0x205F, /* MEDIUM MATHEMATICAL SPACE */ \
      0x3000  /* IDEOGRAPHIC SPACE */

#define RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE_NO_CR_LF \
  RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_ASCII_NO_CR_LF, RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE_NON_ASCII

#define RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_ASCII, RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE_NON_ASCII

// Contains the set of characters representing whitespace in the corresponding
// encoding. Null-terminated. The ASCII versions are the whitespaces as defined
// by HTML5, and don't include control characters.
inline constexpr wchar_t kWhitespaceWide[] = {RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE, 0};
inline constexpr char16_t kWhitespaceUTF16[] = {RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE, 0};
inline constexpr char16_t kWhitespaceNoCrLfUTF16[] = {
    RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_UNICODE_NO_CR_LF, 0};
inline constexpr char kWhitespaceASCII[] = {RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_ASCII, 0};
inline constexpr char16_t kWhitespaceASCIIAs16[] = {RUSTIUM_CONNECTION_ORIGINAL_WHITESPACE_ASCII, 0};

// Null-terminated string representing the UTF-8 byte order mark.
inline constexpr char kUtf8ByteOrderMark[] = "\xEF\xBB\xBF";

}  // namespace net::connection_baseline_original

#endif  // NET_SOCKET_CONNECTION_BASELINE_ORIGINAL_WHITESPACE_H_
