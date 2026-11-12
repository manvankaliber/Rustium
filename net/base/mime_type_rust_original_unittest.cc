// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Original parser tests copied verbatim for focused native validation.
#include "base/strings/utf_string_conversions.h"
#include "net/base/mime_util.h"
#include "net/http/http_util.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
TEST(MimeUtilTest, TestParseMimeType) {
  const struct {
    std::string type_str;
    std::string mime_type;
    base::StringPairs params;
  } tests[] = {
      // Simple tests.
      {"image/jpeg", "image/jpeg"},
      {"application/octet-stream;foo=bar;name=\"test.jpg\"",
       "application/octet-stream",
       {{"foo", "bar"}, {"name", "test.jpg"}}},
      // Quoted string parsing.
      {"t/s;name=\"t\\\\est\\\".jpg\"", "t/s", {{"name", "t\\est\".jpg"}}},
      {"t/s;name=\"test.jpg\"", "t/s", {{"name", "test.jpg"}}},
      {"t/s;name=\"test;jpg\"", "t/s", {{"name", "test;jpg"}}},
      // Lenient for no closing quote.
      {"t/s;name=\"test.jpg", "t/s", {{"name", "test.jpg"}}},
      {"t/s;name=\"ab\\\"", "t/s", {{"name", "ab\""}}},
      // Strip whitespace from start/end of mime_type.
      {" t/s", "t/s"},
      {"t/s ", "t/s"},
      {" t/s ", "t/s"},
      {"t/=", "t/="},
      // Generally ignore whitespace.
      {"t/s;a=1;b=2", "t/s", {{"a", "1"}, {"b", "2"}}},
      {"t/s ;a=1;b=2", "t/s", {{"a", "1"}, {"b", "2"}}},
      {"t/s; a=1;b=2", "t/s", {{"a", "1"}, {"b", "2"}}},
      // Special case, include whitespace after param name until equals.
      {"t/s;a =1;b=2", "t/s", {{"a ", "1"}, {"b", "2"}}},
      {"t/s;a= 1;b=2", "t/s", {{"a", "1"}, {"b", "2"}}},
      {"t/s;a=1 ;b=2", "t/s", {{"a", "1"}, {"b", "2"}}},
      {"t/s;a=1; b=2", "t/s", {{"a", "1"}, {"b", "2"}}},
      {"t/s; a = 1;b=2", "t/s", {{"a ", "1"}, {"b", "2"}}},
      // Do not trim whitespace from quoted-string param values.
      {"t/s;a=\" 1\";b=2", "t/s", {{"a", " 1"}, {"b", "2"}}},
      {"t/s;a=\"1 \";b=2", "t/s", {{"a", "1 "}, {"b", "2"}}},
      {"t/s;a=\" 1 \";b=2", "t/s", {{"a", " 1 "}, {"b", "2"}}},
      // Ignore incomplete params.
      {"t/s;a", "t/s", {}},
      {"t/s;a=", "t/s", {}},
      {"t/s;a=1;", "t/s", {{"a", "1"}}},
      {"t/s;a=1;b", "t/s", {{"a", "1"}}},
      {"t/s;a=1;b=", "t/s", {{"a", "1"}}},
      // Allow empty subtype.
      {"t/", "t/", {}},
      {"ts/", "ts/", {}},
      {"t/;", "t/", {}},
      {"t/ s", "t/", {}},
      // Questionable: allow anything as long as there is a slash somewhere.
      {"/ts", "/ts", {}},
      {"/s", "/s", {}},
      {"/", "/", {}},
  };
  for (const auto& test : tests) {
    std::string mime_type;
    base::StringPairs params;
    EXPECT_TRUE(ParseMimeType(test.type_str, &mime_type, &params));
    EXPECT_EQ(test.mime_type, mime_type);
    EXPECT_EQ(test.params, params);
  }
  for (auto* type_str : {
           // Must have slash in mime type.
           "",
           "ts",
           "t / s",
       }) {
    EXPECT_FALSE(ParseMimeType(type_str, nullptr, nullptr));
  }
}
TEST(MimeUtilTest, TestParseMimeTypeWithoutParameter) {
  std::string nonAscii("application/nonutf8");
  EXPECT_TRUE(ParseMimeTypeWithoutParameter(nonAscii, nullptr, nullptr));
#if BUILDFLAG(IS_WIN)
  nonAscii.append(base::WideToUTF8(L"\u2603"));
#else
  nonAscii.append("\u2603");  // unicode snowman
#endif
  EXPECT_FALSE(ParseMimeTypeWithoutParameter(nonAscii, nullptr, nullptr));

  std::string top_level_type;
  std::string subtype;
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("application/mime", &top_level_type,
                                            &subtype));
  EXPECT_EQ("application", top_level_type);
  EXPECT_EQ("mime", subtype);

  // Various allowed subtype forms.
  EXPECT_TRUE(
      ParseMimeTypeWithoutParameter("application/json", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("application/x-suggestions+json",
                                            nullptr, nullptr));
  EXPECT_TRUE(
      ParseMimeTypeWithoutParameter("application/+json", nullptr, nullptr));

  // Upper case letters are allowed.
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("text/mime", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("TEXT/mime", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("Text/mime", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("TeXt/mime", nullptr, nullptr));

  // Experimental types are also considered to be valid.
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("x-video/mime", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("X-Video/mime", nullptr, nullptr));

  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text/", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text/ ", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("te(xt/ ", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text/()plain", nullptr, nullptr));

  EXPECT_FALSE(ParseMimeTypeWithoutParameter("x-video", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("x-video/", nullptr, nullptr));

  EXPECT_FALSE(
      ParseMimeTypeWithoutParameter("application/a/b/c", nullptr, nullptr));

  // Test leading and trailing whitespace
  EXPECT_TRUE(ParseMimeTypeWithoutParameter(" text/plain", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("text/plain ", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text /plain", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text/ plain ", nullptr, nullptr));

  EXPECT_TRUE(ParseMimeTypeWithoutParameter("\ttext/plain", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("text/plain\t", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text\t/plain", nullptr, nullptr));
  EXPECT_FALSE(
      ParseMimeTypeWithoutParameter("text/\tplain ", nullptr, nullptr));

  EXPECT_TRUE(ParseMimeTypeWithoutParameter("\vtext/plain", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("text/plain\v", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text\v/plain", nullptr, nullptr));
  EXPECT_FALSE(
      ParseMimeTypeWithoutParameter("text/\vplain ", nullptr, nullptr));

  EXPECT_TRUE(ParseMimeTypeWithoutParameter("\rtext/plain", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("text/plain\r", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text\r/plain", nullptr, nullptr));
  EXPECT_FALSE(
      ParseMimeTypeWithoutParameter("text/\rplain ", nullptr, nullptr));

  EXPECT_TRUE(ParseMimeTypeWithoutParameter("\ntext/plain", nullptr, nullptr));
  EXPECT_TRUE(ParseMimeTypeWithoutParameter("text/plain\n", nullptr, nullptr));
  EXPECT_FALSE(ParseMimeTypeWithoutParameter("text\n/plain", nullptr, nullptr));
  EXPECT_FALSE(
      ParseMimeTypeWithoutParameter("text/\nplain ", nullptr, nullptr));
}
TEST(HttpUtilTest, ParseContentType) {
  // clang-format off
  const struct {
    const char* const content_type;
    const char* const expected_mime_type;
    const char* const expected_charset;
    const bool expected_had_charset;
    const char* const expected_boundary;
  } tests[] = {
    { "text/html",
      "text/html",
      "",
      false,
      ""
    },
    { "text/html;",
      "text/html",
      "",
      false,
      ""
    },
    { "text/html; charset=utf-8",
      "text/html",
      "utf-8",
      true,
      ""
    },
    // Parameter name is "charset ", not "charset".  See https://crbug.com/772834.
    { "text/html; charset =utf-8",
      "text/html",
      "",
      false,
      ""
    },
    { "text/html; charset= utf-8",
      "text/html",
      "utf-8",
      true,
      ""
    },
    { "text/html; charset=utf-8 ",
      "text/html",
      "utf-8",
      true,
      ""
    },

    { "text/html; boundary=\"WebKit-ada-df-dsf-adsfadsfs\"",
      "text/html",
      "",
      false,
      "WebKit-ada-df-dsf-adsfadsfs"
    },
    // Parameter name is "boundary ", not "boundary".
    // See https://crbug.com/772834.
    { "text/html; boundary =\"WebKit-ada-df-dsf-adsfadsfs\"",
      "text/html",
      "",
      false,
      ""
    },
    // Parameter value includes leading space.  See https://crbug.com/772834.
    { "text/html; boundary= \"WebKit-ada-df-dsf-adsfadsfs\"",
      "text/html",
      "",
      false,
      "WebKit-ada-df-dsf-adsfadsfs"
    },
    // Parameter value includes leading space.  See https://crbug.com/772834.
    { "text/html; boundary= \"WebKit-ada-df-dsf-adsfadsfs\"   ",
      "text/html",
      "",
      false,
      "WebKit-ada-df-dsf-adsfadsfs"
    },
    { "text/html; boundary=\"WebKit-ada-df-dsf-adsfadsfs  \"",
      "text/html",
      "",
      false,
      "WebKit-ada-df-dsf-adsfadsfs"
    },
    { "text/html; boundary=WebKit-ada-df-dsf-adsfadsfs",
      "text/html",
      "",
      false,
      "WebKit-ada-df-dsf-adsfadsfs"
    },
    { "text/html; charset",
      "text/html",
      "",
      false,
      ""
    },
    { "text/html; charset=",
      "text/html",
      "",
      false,
      ""
    },
    { "text/html; charset= ",
      "text/html",
      "",
      false,
      ""
    },
    { "text/html; charset= ;",
      "text/html",
      "",
      false,
      ""
    },
    // Empty quoted strings are allowed.
    { "text/html; charset=\"\"",
      "text/html",
      "",
      true,
      ""
    },

    // Leading and trailing whitespace in quotes is trimmed.
    { "text/html; charset=\" \"",
      "text/html",
      "",
      true,
      ""
    },
    { "text/html; charset=\" foo \"",
      "text/html",
      "foo",
      true,
      ""
    },

    // With multiple values, should use the first one.
    { "text/html; charset=foo; charset=utf-8",
      "text/html",
      "foo",
      true,
      ""
    },
    { "text/html; charset; charset=; charset=utf-8",
      "text/html",
      "utf-8",
      true,
      ""
    },
    { "text/html; charset=utf-8; charset=; charset",
      "text/html",
      "utf-8",
      true,
      ""
    },
    { "text/html; boundary=foo; boundary=bar",
      "text/html",
      "",
      false,
      "foo"
    },

    // Stray quotes ignored.
    { "text/html; \"; \"\"; charset=utf-8",
      "text/html",
      "utf-8",
      true,
      ""
    },
    // Non-leading quotes kept as-is.
    { "text/html; charset=u\"tf-8\"",
      "text/html",
      "u\"tf-8\"",
      true,
      ""
    },
    { "text/html; charset=\"utf-8\"",
      "text/html",
      "utf-8",
      true,
      ""
    },
    // No closing quote.
    { "text/html; charset=\"utf-8",
      "text/html",
      "utf-8",
      true,
      ""
    },
    // Check that \ is treated as an escape character.
    { "text/html; charset=\"\\utf\\-\\8\"",
      "text/html",
      "utf-8",
      true,
      ""
    },
    // More interseting escape character test - test escaped backslash, escaped
    // quote, and backslash at end of input in unterminated quoted string.
    { "text/html; charset=\"\\\\\\\"\\",
      "text/html",
      "\\\"\\",
      true,
      ""
    },
    // Check quoted semicolon.
    { "text/html; charset=\";charset=utf-8;\"",
      "text/html",
      ";charset=utf-8;",
      true,
      ""
    },
    // Unclear if this one should just return utf-8 or not.
    { "text/html; charset= \"utf-8\"",
      "text/html",
      "utf-8",
      true,
      ""
    },
    // Regression test for https://crbug.com/772350:
    // Single quotes are not delimiters but must be treated as part of charset.
    { "text/html; charset='utf-8'",
      "text/html",
      "'utf-8'",
      true,
      ""
    },
    // Empty subtype should be accepted.
    { "text/",
      "text/",
      "",
      false,
      ""
    },
    // "*/*" is ignored unless it has params, or is not an exact match.
    { "*/*", "", "", false, "" },
    { "*/*; charset=utf-8", "*/*", "utf-8", true, "" },
    { "*/* ", "*/*", "", false, "" },
    // Regression test for https://crbug.com/1326529
    { "teXT/html", "text/html", "", false, ""},
    // TODO(abarth): Add more interesting test cases.
  };
  // clang-format on
  for (const auto& test : tests) {
    std::string mime_type;
    std::string charset;
    bool had_charset = false;
    std::string boundary;
    HttpUtil::ParseContentType(test.content_type, &mime_type, &charset,
                               &had_charset, &boundary);
    EXPECT_EQ(test.expected_mime_type, mime_type)
        << "content_type=" << test.content_type;
    EXPECT_EQ(test.expected_charset, charset)
        << "content_type=" << test.content_type;
    EXPECT_EQ(test.expected_had_charset, had_charset)
        << "content_type=" << test.content_type;
    EXPECT_EQ(test.expected_boundary, boundary)
        << "content_type=" << test.content_type;
  }
}
TEST(HttpUtilTest, ParseContentResetCharset) {
  std::string mime_type;
  std::string charset;
  bool had_charset = false;
  std::string boundary;

  // Set mime (capitalization should be ignored), but not charset.
  HttpUtil::ParseContentType("Text/Html", &mime_type, &charset, &had_charset,
                             &boundary);
  EXPECT_EQ("text/html", mime_type);
  EXPECT_EQ("", charset);
  EXPECT_FALSE(had_charset);

  // The same mime, add charset.
  HttpUtil::ParseContentType("tExt/hTml;charset=utf-8", &mime_type, &charset,
                             &had_charset, &boundary);
  EXPECT_EQ("text/html", mime_type);
  EXPECT_EQ("utf-8", charset);
  EXPECT_TRUE(had_charset);

  // The same mime (different capitalization), but no charset - should not clear
  // charset.
  HttpUtil::ParseContentType("teXt/htMl", &mime_type, &charset, &had_charset,
                             &boundary);
  EXPECT_EQ("text/html", mime_type);
  EXPECT_EQ("utf-8", charset);
  EXPECT_TRUE(had_charset);

  // A different mime will clear charset.
  HttpUtil::ParseContentType("texT/plaiN", &mime_type, &charset, &had_charset,
                             &boundary);
  EXPECT_EQ("text/plain", mime_type);
  EXPECT_EQ("", charset);
  EXPECT_TRUE(had_charset);
}
}  // namespace net
