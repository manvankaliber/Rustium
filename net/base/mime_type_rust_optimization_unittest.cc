// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/mime_type_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
TEST(MimeTypeRustOptimizationTest, TransformedScratchGrowthAndEmptySpans) {
  const std::string escaped = std::string(2048, 'A') + "\\\";\\ ";
  mime_type_test::Check("TEXT/PLAIN; x=\"" + escaped +
                        "\"; charset=\"\\ \\U\\T\\F-8\\ \"; boundary=\"" +
                        escaped + "\"; ignored=\"" + std::string(4096, 'z'));
  for (std::string_view suffix :
       {"charset=\"\\ \\t\"; boundary=\"\\ \"",
        "charset=\"\\ \"; boundary=\"\\ \\t\"",
        "charset=\"\\\"\\\\\"; boundary=\"\\ \\ \"",
        "charset=first; boundary=second; ignored=\"unterminated\\",
        "boundary=first; charset=second; charset=\"ignored\\"}) {
    mime_type_test::Check(std::string("A/B; ") + std::string(suffix));
  }
  std::string raw = "A/B; charset=\"";
  raw.append({'\0', static_cast<char>(0xff), '\\', '\0', 'Z'});
  raw += "\"; boundary=\"";
  raw.append({'\0', static_cast<char>(0x80), '\\', '\0', 'Q'});
  raw += '"';
  mime_type_test::Check(raw);
}

TEST(MimeTypeRustOptimizationTest, InputAliasesEveryMutableOutput) {
  for (const auto ids :
       {std::array{0, 1, 2}, std::array{0, 0, 1}, std::array{0, 1, 0},
        std::array{0, 1, 1}, std::array{0, 0, 0}, std::array{0, 1, -1}}) {
    for (size_t input_slot = 0; input_slot < 3; ++input_slot) {
      for (bool initial_had : {false, true}) {
        for (const std::string& header :
             {std::string(
                  "TEXT/HTML; charset=\" UTF-8 \"; boundary=\" TEXT/HTML \""),
              "A/B; charset=\"\\ \\U\\T\\F-8\\ \"; boundary=\"" +
                  std::string(512, 'b') + "\\\"\\ ",
              std::string("TEXT/HTML; boundary=\"TEXT/HTML\"")}) {
          std::array<std::string, 3> a = {"Text/Html", "old", "keep"};
          a[input_slot] = header;
          auto b = a;
          bool ha = initial_had, hb = ha;
          mime_type_test::LegacyContentType(a[input_slot], &a[ids[0]],
                                            &a[ids[1]], &ha,
                                            ids[2] < 0 ? nullptr : &a[ids[2]]);
          HttpUtil::ParseContentType(b[input_slot], &b[ids[0]], &b[ids[1]], &hb,
                                     ids[2] < 0 ? nullptr : &b[ids[2]]);
          EXPECT_EQ(a, b);
          EXPECT_EQ(ha, hb);
        }
      }
    }
  }
}

TEST(MimeTypeRustOptimizationTest, HeapBackedInputSurvivesOutputCommits) {
  const std::string type(512, 'a');
  const std::string sub(1024, 'b');
  std::string input = type + "/" + sub;
  std::string top;
  EXPECT_TRUE(ParseMimeTypeWithoutParameter(input, &top, &input));
  EXPECT_EQ(type, top);
  EXPECT_EQ(sub, input);
  input = type + "/" + sub;
  std::string subtype;
  EXPECT_TRUE(ParseMimeTypeWithoutParameter(input, &input, &subtype));
  EXPECT_EQ(type, input);
  EXPECT_EQ(sub, subtype);

  base::StringPairs params = {{type + "/" + sub + "; p=\"x\\y\"", "old"}};
  EXPECT_TRUE(ParseMimeType(params[0].first, &params[0].second, &params));
  EXPECT_EQ((base::StringPairs{{"p", "xy"}}), params);
}
}  // namespace net
