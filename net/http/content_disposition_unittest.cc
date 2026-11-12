// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "base/memory/ref_counted.h"
#include "net/http/content_disposition_reference.h"
#include "net/http/http_content_disposition.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
uint64_t comparison_count = 0;
void Compare(std::string_view input, const std::string& charset) {
  ++comparison_count;
  ContentDispositionReference old(input, charset);
  HttpContentDisposition now(input, charset);
  EXPECT_EQ(old.type(),
            static_cast<ContentDispositionReference::Type>(now.type()));
  EXPECT_EQ(old.filename(), now.filename());
  EXPECT_EQ(old.parse_result_flags(), now.parse_result_flags());
}
TEST(ContentDispositionRustTest, DifferentialEdgeCases) {
  comparison_count = 0;
  const std::string values[] = {"",
                                "x",
                                "\"",
                                "\"x",
                                "\"x\\\"",
                                "\"x\\\\\"",
                                "'x'",
                                "x  y\t\r\nz",
                                " a ",
                                "=?",
                                "=",
                                "=?UTF-8?=",
                                "=?UTF-8?Q?=",
                                "=?UTF-8?Q??=",
                                "=?UTF-8?Q?x?=",
                                "=?UTF-8?Q?x?",
                                "=?UTF-8?Q?x?=?",
                                "=?UTF-8?Q?=a?=",
                                "=?UTF-8?Q?=a0?=",
                                "=?UTF-8?Q?x?=foo",
                                "=?UTF-8?Q?x?==?UTF-8?Q?y?=",
                                "=?UTF-8?Q?x?= =?UTF-8?Q?y?=",
                                "=?UTF-8?Q?x=?=",
                                "=?UTF-8?Q?=x0?=",
                                "=?UTF-8?B?A!AA?=",
                                "=?UTF-8?B?AA!A?=",
                                "=?UTF-8?B?AAA!?=",
                                "=?UTF-8?B?AA=A?=",
                                "=?UTF-8?B?AA==AAAA?=",
                                "=?UTF-8?B?AAA=AAAA?=",
                                "=?UTF-8?B?YQ==?=",
                                "=?UTF-8?B?YR==?=",
                                "=?UTF-8?B?YWJ=?=",
                                "=?UTF-8?B?YWJj?=",
                                "=?UTF-8?B?YWJjZA==?=",
                                "=?UTF-8?B?YWJj=AAA?=",
                                "=?UTF-8?B?YQ?=",
                                "=?unknown?Q?x?=",
                                "=?ISO-8859-1?Q?=e9?=",
                                "=?Shift_JIS?B?g2WDWINn?=",
                                "=?UTF-8?Q?=ff?=",
                                "utf-8''abc",
                                "utf-8'en'%C3%A9",
                                "utf-8''foo%cc%88",
                                "iso-8859-1''%e9",
                                "unknown''x",
                                "''x",
                                "utf-8''",
                                "utf-8'''",
                                "utf-8'x'y'z",
                                "utf-8''\xc3\xa9",
                                "\"utf-8''foo",
                                "\"utf-8''foo\"",
                                "%00%ff",
                                "%ED%A0%80",
                                "%EF%B7%90",
                                "%F4%8F%BF%BF",
                                "%x0%0%",
                                "%41%20a+b",
                                std::string("ab\0c", 4),
                                "\xc3\xa9",
                                "\xff",
                                "\xef\xb7\x90",
                                "\xf4\x8f\xbf\xbf"};
  const std::string types[] = {
      "",         "inline;",     "attachment;",  "unknown;",
      "in line;", "\tINLINE\t;", ",attachment;", "\rattachment;"};
  const std::string charsets[] = {
      "",          "UTF-8",   "ISO-8859-1", "windows-1252",
      "Shift_JIS", "GB18030", "unknown",    std::string("utf-8\0unknown", 13)};
  for (const auto& charset : charsets) {
    for (const auto& type : types) {
      for (const auto& value : values) {
        SCOPED_TRACE(type + " / " + value + " / " + charset);
        Compare(type + " filename=" + value, charset);
        Compare(type + " filename*=" + value, charset);
        Compare(type + " filename=" + value +
                    ";filename=x;filename*=utf-8''extended",
                charset);
        Compare(type + " filename*=" + value +
                    ";filename*=utf-8''next;filename='fallback'",
                charset);
      }
    }
  }
  RecordProperty("differential_comparisons", std::to_string(comparison_count));
}
TEST(ContentDispositionRustTest, IteratorFailureAndByteBoundaries) {
  comparison_count = 0;
  const std::string headers[] = {"attachment; filename=; filename=x",
                                 "attachment; =x;filename=y",
                                 "attachment;bad;filename=y",
                                 "attachment;\"bad\"=x;filename=y",
                                 "attachment; =x",
                                 "attachment; ; ; filename=x",
                                 "filename=x,filename=y",
                                 ",filename=y",
                                 "filename=\"a,b;c\"",
                                 "filename=\"unterminated,a;b",
                                 "filename=\"x\"junk",
                                 "filename=\"x\\\"",
                                 "filename* = \"utf-8''x",
                                 "filename* = \"utf-8''x\""};
  for (const auto& h : headers) {
    Compare(h, "UTF-8");
  }
  for (unsigned c = 0; c < 256; ++c) {
    std::string byte(1, static_cast<char>(c));
    Compare("filename=ab" + byte + "cd;filename=z", "windows-1252");
    Compare("attachment;filename=\"ab" + byte + "cd\"", "");
    Compare("filename==?UTF-8?Q?" + byte + "?=", "UTF-8");
    Compare("filename==?UTF-8?B?" + byte + "AAA?=", "UTF-8");
    Compare(byte + ";filename=x", "UTF-8");
  }
  RecordProperty("differential_comparisons", std::to_string(comparison_count));
}
TEST(ContentDispositionRustTest, ResponseHeaderConstructorAndOwnership) {
  for (const char* input :
       {"inline; filename=abc", "filename*=utf-8''%65%cc%81",
        "filename=\"a,b\"", "filename=bad;filename*=unknown''x", ""}) {
    auto headers = base::MakeRefCounted<HttpResponseHeaders>(
        std::string("HTTP/1.1 200 OK\0", 16) + "Content-Disposition: " + input +
        std::string(2, '\0'));
    ContentDispositionReference old(*headers, "UTF-8");
    HttpContentDisposition now(*headers, "UTF-8");
    EXPECT_EQ(old.filename(), now.filename());
    EXPECT_EQ(old.parse_result_flags(), now.parse_result_flags());
  }
  std::string storage = "attachment; filename=owned";
  HttpContentDisposition saved(storage, "");
  storage.assign(4096, '!');
  EXPECT_EQ("owned", saved.filename());
  Compare(std::string_view(), "");
}
}  // namespace
}  // namespace net
