// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/cookies/parsed_cookie_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
namespace {
using namespace parsed_cookie_rust_test;
TEST(ParsedCookieRust, IndependentStatusResetRetentionAndDiagnostics) {
  const std::array<std::string, 8> inputs = {
      "n=v",
      "",
      "=",
      "n=bad\tvalue",
      "n=v; bad\tname=value",
      "n=v; path=bad\tvalue",
      std::string(4097, 'x') + "=v",
      "n=v; path=" + std::string(1025, 'x')};
  for (const auto& input : inputs) {
    for (bool excluded : {false, true}) {
      OriginalStatus sa;
      CookieInclusionStatus sb;
      sa.AddWarningReason(OriginalStatus::WarningReason::WARN_DOMAIN_NON_ASCII);
      sb.AddWarningReason(
          CookieInclusionStatus::WarningReason::WARN_DOMAIN_NON_ASCII);
      sa.MaybeSetExemptionReason(OriginalStatus::ExemptionReason::kUserSetting);
      sb.MaybeSetExemptionReason(
          CookieInclusionStatus::ExemptionReason::kUserSetting);
      if (excluded) {
        SeedStatus(sa);
        SeedStatus(sb);
      }
      CompareStatus(sa, sb);
      const auto old_sa = sa;
      const auto old_sb = sb;
      CHECK_EQ(Original::IsValidCookieNameValuePair(input, "v", &sa),
               ParsedCookie::IsValidCookieNameValuePair(input, "v", &sb));
      CompareStatus(sa, sb);
      CHECK_EQ(Original::IsValidCookieNameValuePair(input, "v"),
               ParsedCookie::IsValidCookieNameValuePair(input, "v"));
      // Successful validation preserves preexisting status, unlike construction
      // which clears it before parsing even for valid input.
      if (Original::IsValidCookieNameValuePair(input, "v")) {
        CHECK(sa == old_sa);
        CHECK(sb == old_sb);
      }
      Original a(input, &sa);
      ParsedCookie b(input, &sb);
      CompareStatus(sa, sb);
      Compare(a, b);
      Original null_a(input);
      ParsedCookie null_b(input);
      Compare(null_a, null_b);
      CHECK_EQ(sa.HasExclusionReason(
                   OriginalStatus::ExclusionReason::EXCLUDE_FAILURE_TO_STORE),
               false);
      CHECK_EQ(static_cast<int>(sa.exemption_reason()),
               static_cast<int>(OriginalStatus::ExemptionReason::kNone));
    }
  }
}
TEST(ParsedCookieRust, AllBytesAndNullEmptyHelpers) {
  Statics({});
  for (unsigned c = 0; c < 256; ++c) {
    std::string value(1, static_cast<char>(c));
    for (const auto& prefix : {"", " ", "\t", "a=", "a=b; PATH="}) {
      const std::string input = std::string(prefix) + value + " \t;next=x";
      for (size_t offset = 0; offset <= input.size(); ++offset) {
        Statics(std::string_view(input).substr(offset));
      }
      Exercise(input, value, c * 10371);
    }
  }
}
TEST(ParsedCookieRust, RawScannerIteratorObjectAliases) {
  for (unsigned byte = 0; byte < 256; ++byte) {
    std::string input = " ";
    input += static_cast<char>(byte);
    input += " name \t=value \t; tail";
    for (size_t end_slot = 0; end_slot < 4; ++end_slot) {
      for (size_t start_slot = 0; start_slot < 4; ++start_slot) {
        for (size_t finish_slot = 0; finish_slot < 4; ++finish_slot) {
          for (bool token : {false, true}) {
            std::array<std::string_view::iterator, 4> old = {
                input.begin(), input.end(), input.end(), input.end()};
            auto current = old;
            if (token) {
              CHECK_EQ(
                  Original::ParseToken(&old[0], old[end_slot], &old[start_slot],
                                       &old[finish_slot]),
                  ParsedCookie::ParseToken(&current[0], current[end_slot],
                                           &current[start_slot],
                                           &current[finish_slot]));
            } else {
              Original::ParseValue(&old[0], old[end_slot], &old[start_slot],
                                   &old[finish_slot]);
              ParsedCookie::ParseValue(&current[0], current[end_slot],
                                       &current[start_slot],
                                       &current[finish_slot]);
            }
            CHECK(old == current);
          }
        }
      }
    }
  }
}
TEST(ParsedCookieRust, EverySetterFailureSuccessAndSelfAliasing) {
  const std::array<std::string, 18> inputs = {"",
                                              " ",
                                              "\t",
                                              " trimmed ",
                                              "\ttrimmed\t",
                                              "bad;rest",
                                              "bad=rest",
                                              std::string("nul\0rest", 8),
                                              "cr\rrest",
                                              "lf\nrest",
                                              "internal\tbyte",
                                              std::string(1, '\x7f'),
                                              std::string(1, '\xff'),
                                              std::string(1024, 'x'),
                                              std::string(1025, 'x'),
                                              std::string(4096, 'x'),
                                              std::string(4097, 'x'),
                                              "low"};
  for (auto initial : {"", "a=b", "=value", "a=",
                       "a=b; path=first; path=last; domain=example; "
                       "expires=tomorrow; max-age=4; secure=x; httponly=x; "
                       "samesite=LaX; priority=high; partitioned=x; other=z"}) {
    for (size_t op = 0; op < 11; ++op) {
      for (const auto& input : inputs) {
        Original a(initial);
        ParsedCookie b(initial);
        CHECK_EQ(Set(a, op, input, true), Set(b, op, input, true));
        Compare(a, b);
        CHECK_EQ(Set(a, op, {}, false), Set(b, op, {}, false));
        Compare(a, b);
      }
      for (size_t selector = 0; selector < 26; ++selector) {
        Original a(initial);
        ParsedCookie b(initial);
        auto av = Alias(a, selector);
        auto bv = Alias(b, selector);
        av = av.substr(av.size() / 2);
        bv = bv.substr(bv.size() / 2);
        CHECK_EQ(Set(a, op, av, true), Set(b, op, bv, true));
        Compare(a, b);
      }
    }
  }
}
TEST(ParsedCookieRust, DuplicateClearDoesNotReselectEarlierAttribute) {
  Original a(
      "a=b; path=first; secure=no; PATH=last; secure=yes; domain=x; "
      "partitioned");
  ParsedCookie b(
      "a=b; path=first; secure=no; PATH=last; secure=yes; domain=x; "
      "partitioned");
  CHECK(a.SetPath({}));
  CHECK(b.SetPath({}));
  Compare(a, b);
  EXPECT_FALSE(b.Path());
  EXPECT_NE(b.ToCookieLine().find("path=first"), std::string::npos);
  CHECK(a.SetIsSecure(false));
  CHECK(b.SetIsSecure(false));
  Compare(a, b);
  EXPECT_FALSE(b.IsSecure());
  CHECK(a.SetPath("new"));
  CHECK(b.SetPath("new"));
  Compare(a, b);
  CHECK(a.SetIsSecure(true));
  CHECK(b.SetIsSecure(true));
  Compare(a, b);
}
TEST(ParsedCookieRust, PartialFailureMetricsAndWarnings) {
  for (const auto& prefix :
       {"name=value", "=ambiguous=value", "secure", "a\tb=value"}) {
    for (const auto& ending :
         {"; bad\tname=value", "; path=bad\tvalue", "; trailing=ok"}) {
      Exercise(
          std::string(prefix) + "; path=" + std::string(1025, 'x') + ending,
          "repair", 345613521);
    }
  }
}
TEST(ParsedCookieRust, InvalidConstructorRepairClearsAllAttributeIndexes) {
  for (auto attr : {"path", "domain", "expires", "max-age", "secure",
                    "httponly", "samesite", "priority", "partitioned"}) {
    const std::string line =
        std::string("n=v; ") + attr + "=first; invalid=control\tbyte";
    Original a(line);
    ParsedCookie b(line);
    CHECK(!a.IsValid());
    CHECK(!b.IsValid());
    CHECK(a.SetName("repaired"));
    CHECK(b.SetName("repaired"));
    Compare(a, b);
    for (size_t op = 2; op < 11; ++op) {
      CHECK_EQ(Set(a, op, "valid", true), Set(b, op, "valid", true));
      Compare(a, b);
    }
  }
}
TEST(ParsedCookieRust, CanonicalEnumSelectionsAndBooleanSerialization) {
  for (const auto value :
       {"", "none", "NoNe", "lax", "LAX", "strict", "Strict", "extended",
        "EXTENDED", "unknown", "low", "LOW", "medium", "HIGH"}) {
    Exercise(std::string("n=v; secure=ignored; httponly=ignored; "
                         "partitioned=ignored; samesite=") +
                 value + "; priority=" + value,
             value, 0);
  }
}
TEST(ParsedCookieRust, SizeLimitsAndRetainedPublicStringReferences) {
  for (size_t len : {1023u, 1024u, 1025u, 4095u, 4096u, 4097u, 8192u}) {
    Exercise("n=v; path=" + std::string(len, 'x') + "; path=last",
             " " + std::string(len, 'x') + " ", 731256);
    Exercise("=" + std::string(len, 'x'), std::string(len, 'x'), 871231);
  }
  ParsedCookie c("name=value; path=/");
  const auto* name = &c.Name();
  const auto* value = &c.Value();
  EXPECT_TRUE(c.SetName(std::string_view(c.Name()).substr(1)));
  EXPECT_EQ(name, &c.Name());
  EXPECT_EQ("ame", *name);
  EXPECT_TRUE(c.SetValue(c.Name()));
  EXPECT_EQ(value, &c.Value());
  EXPECT_EQ("ame", *value);
  EXPECT_TRUE(c.SetPath(*c.Path()));
  EXPECT_EQ("/", c.Path());
}
}  // namespace
}  // namespace net
