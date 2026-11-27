// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/strings/string_view_util.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_sniffer_reference.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer provides a live buffer of exactly size bytes.
  const auto bytes = UNSAFE_BUFFERS(base::span(data, size));
  const uint8_t control = bytes.empty() ? 0 : bytes[0];
  // Keep GURL/header construction outside the content fuzzing loop while
  // varying every metadata category used by the sniffer.
  static constexpr std::array<std::string_view, 10> urls = {
      "",
      "https://example.test/a",
      "file:///a",
      "data:text/plain,hi",
      "https://example.test/a.crx",
      "https://example.test/a.doc",
      "https://example.test/a.xls",
      "https://example.test/a.ppt",
      "https://example.test/a.docx",
      "https://example.test/a.xlsx"};
  static constexpr std::array<std::string_view, 12> hints = {
      "",
      "text/plain",
      "application/octet-stream",
      "text/xml",
      "application/xml",
      "application/msword",
      "image/png",
      "unknown/unknown",
      "application/unknown",
      "*/*",
      "application/"
      "vnd.openxmlformats-officedocument.presentationml.presentation",
      "noslash"};
  const GURL url(urls[control % urls.size()]);
  const auto force = control & 0x80 ? net::ForceSniffFileUrlsForHtml::kEnabled
                                    : net::ForceSniffFileUrlsForHtml::kDisabled;
  const auto content = bytes.empty() ? bytes : bytes.subspan(size_t{1});
  const std::string_view input = base::as_string_view(
      content.first(std::min(content.size(), size_t{65536})));
  const std::string arbitrary_hint(
      input.substr(0, std::min(input.size(), size_t{64})));
  for (const auto& hint :
       {std::string(hints[(control / 10) % hints.size()]), arbitrary_hint}) {
    std::string actual = "unchanged sentinel";
    std::string expected = actual;
    CHECK_EQ(net::SniffMimeType(input, url, hint, force, &actual),
             net::mime_sniffer_test::SniffMimeType(input, url, hint, force,
                                                   &expected));
    CHECK_EQ(actual, expected);
    CHECK_EQ(net::ShouldSniffMimeType(url, nullptr, hint),
             net::mime_sniffer_test::ShouldSniffMimeType(url, nullptr, hint));
  }
  std::string actual = "unchanged sentinel";
  std::string expected = actual;
  CHECK_EQ(
      net::SniffMimeTypeFromLocalData(input, &actual),
      net::mime_sniffer_test::SniffMimeTypeFromLocalData(input, &expected));
  CHECK_EQ(actual, expected);
  if (control & 0x20) {
    actual.assign(input);
    expected.assign(input);
    actual.reserve(input.size() + 128);
    expected.reserve(input.size() + 128);
    const size_t offset =
        control & 0x10 ? std::min(input.size(), size_t{3}) : 0;
    const std::string_view a(actual.data() + offset, actual.size() - offset);
    const std::string_view b(expected.data() + offset,
                             expected.size() - offset);
    const auto& hint = hints[(control / 10) % hints.size()];
    const std::string hint_copy(hint);
    const auto& actual_hint = control & 0x40 ? actual : hint_copy;
    const auto& expected_hint = control & 0x40 ? expected : hint_copy;
    CHECK_EQ(net::SniffMimeType(a, url, actual_hint, force, &actual),
             net::mime_sniffer_test::SniffMimeType(b, url, expected_hint, force,
                                                   &expected));
    CHECK_EQ(actual, expected);
  }
  CHECK_EQ(net::LooksLikeBinary(input),
           net::mime_sniffer_test::LooksLikeBinary(input));
  return 0;
}
