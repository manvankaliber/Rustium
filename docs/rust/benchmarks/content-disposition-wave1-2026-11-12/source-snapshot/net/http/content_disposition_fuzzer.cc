// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/i18n/icu_util.h"
#include "net/http/content_disposition_reference.h"
#include "net/http/http_content_disposition.h"

namespace {
void Compare(std::string_view header, const std::string& charset) {
  net::ContentDispositionReference old(header, charset);
  net::HttpContentDisposition now(header, charset);
  CHECK_EQ(static_cast<int>(old.type()), static_cast<int>(now.type()));
  CHECK_EQ(old.filename(), now.filename());
  CHECK_EQ(old.parse_result_flags(), now.parse_result_flags());
}
}  // namespace
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static const bool initialized = base::i18n::InitializeICU();
  CHECK(initialized);
  if (!size) {
    Compare({}, "");
    return 0;
  }
  const std::string charsets[] = {
      "",          "UTF-8",   "ISO-8859-1", "windows-1252",
      "Shift_JIS", "GB18030", "unknown",    std::string("UTF-8\0x", 7)};
  const unsigned selector = data[0];
  const std::string_view bytes(reinterpret_cast<const char*>(data + 1),
                               size - 1);
  const auto& charset = charsets[selector % 8];
  Compare(bytes, charset);
  switch ((selector / 8) % 6) {
    case 0:
      Compare("attachment;filename=" + std::string(bytes) +
                  ";filename=next;filename*=utf-8''extended",
              charset);
      break;
    case 1:
      Compare("filename=\"" + std::string(bytes) + "\";filename=x", charset);
      break;
    case 2:
      Compare("filename==?UTF-8?Q?" + std::string(bytes) + "?=", charset);
      break;
    case 3:
      Compare("filename==?UTF-8?B?" + std::string(bytes) + "?=", charset);
      break;
    case 4:
      Compare("filename*=UTF-8''" + std::string(bytes) + ";filename=backup",
              charset);
      break;
    case 5:
      Compare("filename==?" + charset + "?Q?" + std::string(bytes) +
                  "?= =?UTF-8?Q?tail?=",
              charset);
      break;
  }
  return 0;
}
