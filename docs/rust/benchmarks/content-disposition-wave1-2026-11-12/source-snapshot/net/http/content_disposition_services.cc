// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/content_disposition_services.h"

#include <string>
#include <string_view>

#include "base/check.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/net_string_util.h"
#include "net/http/content_disposition_ffi.rs.h"
namespace net::content_disposition_internal {
Conversion convert(rust::Slice<const uint8_t> input,
                   rust::Slice<const uint8_t> charset,
                   uint8_t mode) {
  const std::string_view text(reinterpret_cast<const char*>(input.data()),
                              input.size());
  // c_str() intentionally preserves the legacy embedded-NUL charset behavior.
  const std::string encoding(reinterpret_cast<const char*>(charset.data()),
                             charset.size());
  std::string output;
  bool ok = false;
  switch (mode) {
    case 0:
      ok = ConvertToUtf8(text, encoding.c_str(), &output);
      break;
    case 1: {
      std::u16string wide;
      ok = ConvertToUTF16(text, encoding.c_str(), &wide);
      if (ok) {
        output = base::UTF16ToUTF8(wide);
      }
      break;
    }
    case 2:
      output = base::WideToUTF8(base::SysNativeMBToWide(text));
      ok = true;
      break;
    case 3:
      ok = ConvertToUtf8AndNormalize(text, encoding.c_str(), &output);
      break;
    default:
      CHECK(false);
  }
  Conversion result;
  result.ok = ok;
  result.bytes.reserve(output.size());
  for (unsigned char c : output) {
    result.bytes.push_back(c);
  }
  return result;
}
}  // namespace net::content_disposition_internal
