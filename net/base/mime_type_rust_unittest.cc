// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/mime_type_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
TEST(MimeTypeRustTest, FailureAndEqualTypeDoNotInspectUnusedOutputs) {
  for (std::string_view input : {"", "no-slash", "*/*"}) {
    mime_type_test::LegacyContentType(input, nullptr, nullptr, nullptr,
                                      nullptr);
    HttpUtil::ParseContentType(input, nullptr, nullptr, nullptr, nullptr);
  }
  std::string a = "Text/Html", b = a;
  mime_type_test::LegacyContentType("text/html", &a, nullptr, nullptr, nullptr);
  HttpUtil::ParseContentType("text/html", &b, nullptr, nullptr, nullptr);
  EXPECT_EQ(a, b);
}

TEST(MimeTypeRustTest, DifferentialGrammarAndBoundaryCases) {
  for (std::string_view input :
       {"",
        " ",
        "\t",
        "no-slash",
        "/",
        "a/",
        "/b",
        "a/b/c",
        " a/b ",
        "a /b",
        "a\r/\nb",
        "a/b(comment); x=1",
        "*/*",
        "*/* ",
        "a/b; charset=",
        "a/b; charset= ; charset=UTF-8",
        "a/b; x=",
        "a/b; x; y=2; x=3",
        "a/b; x=\"\"",
        "a/b; x=\"\\",
        "a/b; x=\"a\\",
        "a/b; x=\"a\\\"b\"junk; charset=\" c \"; boundary=\" b \"",
        "a/b; CHARSET=\"\"; charset=second; BOUNDARY=\"\"; boundary=second",
        "a/b; charset =x; boundary =y; charset='UTF-8'",
        "\vA/B\f"}) {
    mime_type_test::Check(input);
  }
}

TEST(MimeTypeRustTest, EveryByteAndEmbeddedNull) {
  for (unsigned byte = 0; byte < 256; ++byte) {
    for (std::string input : {"a/b", "a/b; charset=\"Q\"; boundary=\"B\"",
                              "a/b; p=unquoted", "a/b; p=\"\\x\""}) {
      for (size_t i = 0; i <= input.size(); ++i) {
        std::string changed = input;
        changed.insert(i, 1, static_cast<char>(byte));
        mime_type_test::Check(changed);
      }
    }
  }
}

TEST(MimeTypeRustTest, InputOutputAliasingSnapshots) {
  // The old MIME API could invalidate input views while assigning outputs.
  // Owned Rust results define safe snapshot behavior for these combinations.
  for (size_t padding : {0u, 512u}) {
    std::string text = "a/b; x=\"value\"; charset=UTF-8";
    text.append(padding, ' ');
    base::StringPairs params;
    EXPECT_TRUE(ParseMimeType(text, &text, &params));
    EXPECT_EQ("a/b", text);
    EXPECT_EQ((base::StringPairs{{"x", "value"}, {"charset", "UTF-8"}}),
              params);
    text = "application/subtype";
    EXPECT_TRUE(ParseMimeTypeWithoutParameter(text, &text, &text));
    EXPECT_EQ("subtype", text);
    params = {{"a/b; p=v", "old"}};
    EXPECT_TRUE(ParseMimeType(params[0].first, nullptr, &params));
    EXPECT_EQ((base::StringPairs{{"p", "v"}}), params);
  }
}

TEST(MimeTypeRustTest, ContentTypeAliasedInputAndBoundaryEquality) {
  std::string a = "TeXT/HTML; boundary=\"TEXT/HTML\"", b = a;
  std::string ca = "keep", cb = ca;
  bool ha = true, hb = true;
  mime_type_test::LegacyContentType(a, &a, &ca, &ha, &a);
  HttpUtil::ParseContentType(b, &b, &cb, &hb, &b);
  EXPECT_EQ(a, b);
  EXPECT_EQ("TEXT/HTML", b);
  EXPECT_EQ(ca, cb);
  EXPECT_EQ("keep", cb);
  EXPECT_EQ(ha, hb);
}

TEST(MimeTypeRustTest, NullEmptyViewsAndLongQuotedValues) {
  mime_type_test::Check(std::string_view());
  mime_type_test::Check("a/b; x=\"" + std::string(65536, 'x') + "\\");
  std::string many = "a/b";
  for (unsigned i = 0; i < 1024; ++i) {
    many += "; charset=\"\"; boundary=foo; p=value";
  }
  mime_type_test::Check(many);
}
}  // namespace net
