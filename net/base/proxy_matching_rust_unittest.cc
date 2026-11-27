// Copyright 2026 The Chromium Authors
#include "base/memory/raw_ref.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/proxy_matching_rust_differential.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
TEST(ProxyMatchingRust, GrammarCrossProductAndEveryByte) {
  uint64_t n = 0;
  for (auto r : proxy_matching_rust_test::kRules) {
    for (auto u : proxy_matching_rust_test::kUrls) {
      proxy_matching_rust_test::Differential(std::string(r), std::string(u),
                                             n++);
    }
  }
  for (unsigned c = 0; c < 256; ++c) {
    for (auto r : {std::string("a?*b"), std::string("http://a:80"),
                   std::string("127.0.0.1"), std::string("x://a")}) {
      for (size_t i = 0; i <= r.size(); ++i) {
        auto s = r;
        s.insert(i, 1, static_cast<char>(c));
        proxy_matching_rust_test::Differential(s, "http://a.example.com", n++);
      }
    }
  }
}
TEST(ProxyMatchingRust, FeatureAndMappedAddressConditions) {
  for (bool enabled : {true, false}) {
    base::test::ScopedFeatureList features;
    features.InitFromCommandLine(
        enabled ? "BypassProxyForUnspecifiedIpAddresses" : "",
        enabled ? "" : "BypassProxyForUnspecifiedIpAddresses");
    for (auto u : proxy_matching_rust_test::kUrls) {
      for (auto r : proxy_matching_rust_test::kRules) {
        proxy_matching_rust_test::Differential(std::string(r), std::string(u),
                                               3);
      }
    }
  }
}
class MutableRule : public SchemeHostPortMatcherHostnamePatternRule {
 public:
  MutableRule(int& calls, int& deaths)
      : SchemeHostPortMatcherHostnamePatternRule("", "never", -1),
        calls_(calls),
        deaths_(deaths) {}
  ~MutableRule() override { ++*deaths_; }
  SchemeHostPortMatcherResult Evaluate(const GURL&) const override {
    ++*calls_;
    return result;
  }
  std::string ToString() const override { return text; }
  SchemeHostPortMatcherResult result = SchemeHostPortMatcherResult::kExclude;
  std::string text = "mutable";

 private:
  raw_ref<int> calls_;
  raw_ref<int> deaths_;
};
TEST(ProxyMatchingRust, ExternalVirtualDispatchLifetimeAndMutableState) {
  int calls = 0, deaths = 0;
  GURL url("http://example.com");
  SchemeHostPortMatcher m;
  auto custom = std::make_unique<MutableRule>(calls, deaths);
  auto* p = custom.get();
  m.AddAsLastRule(std::move(custom));
  EXPECT_EQ("mutable;", m.ToString());
  EXPECT_EQ(SchemeHostPortMatcherResult::kExclude, m.Evaluate(url));
  EXPECT_EQ(1, calls);
  p->text = "changed";
  p->result = SchemeHostPortMatcherResult::kInclude;
  EXPECT_EQ("changed;", m.ToString());
  EXPECT_TRUE(m.Includes(url));
  m.AddAsLastRule(SchemeHostPortMatcherRule::FromUntrimmedRawString("*"));
  EXPECT_TRUE(m.Includes(url));
  EXPECT_EQ(2, calls);
  SchemeHostPortMatcher moved(std::move(m));
  EXPECT_EQ(p, moved.rules()[0].get());
  moved.ReplaceRule(0, SchemeHostPortMatcherRule::FromUntrimmedRawString("x"));
  EXPECT_EQ(1, deaths);
  m.Clear();
  m.AddAsFirstRule(SchemeHostPortMatcherRule::FromUntrimmedRawString("*"));
  EXPECT_TRUE(m.Includes(url));
}
TEST(ProxyMatchingRust, EmptySpanAndSerializationEmbeddedNull) {
  EXPECT_FALSE(
      SchemeHostPortMatcherRule::FromUntrimmedRawString(std::string_view()));
  proxy_matching_rust_test::Differential(std::string("sc\0heme://a", 11),
                                         "sc://a", 11);
  EXPECT_EQ("<-loopback>;",
            ProxyHostMatchingRules::GetRulesToSubtractImplicit());
}
}  // namespace net
// Additional direct-constructor paths keep the public classes usable without
// parsing, and include all three built-in ownership/memory estimators.
namespace net {
TEST(ProxyMatchingRust, PublicConstructorsAndCustomReplacementEquality) {
  const GURL url("http://[::1]:81");
  IPAddress ip;
  ASSERT_TRUE(ip.AssignFromIPLiteral("::1"));
  for (uint16_t port : {uint16_t(0), uint16_t(81), uint16_t(65535)}) {
    SchemeHostPortMatcherIPHostRule a("HTTP", IPEndPoint(ip, port));
    proxy_matching_reference::SchemeHostPortMatcherIPHostRule b(
        "HTTP", IPEndPoint(ip, port));
    EXPECT_EQ(a.ToString(), b.ToString());
    EXPECT_EQ(a.Evaluate(url), b.Evaluate(url));
#if !BUILDFLAG(CRONET_BUILD)
    EXPECT_GT(a.EstimateMemoryUsage(), 0u);
#endif
  }
  SchemeHostPortMatcherIPBlockRule block("arbitrary-description", "HTTP", ip,
                                         0);
  proxy_matching_reference::SchemeHostPortMatcherIPBlockRule original(
      "arbitrary-description", "HTTP", ip, 0);
  EXPECT_EQ(block.ToString(), original.ToString());
  EXPECT_EQ(block.Evaluate(url), original.Evaluate(url));
  SchemeHostPortMatcherHostnamePatternRule name("HTTP", "*host", -1);
#if !BUILDFLAG(CRONET_BUILD)
  EXPECT_GT(block.EstimateMemoryUsage(), 0u);
  EXPECT_GT(name.EstimateMemoryUsage(), 0u);
  auto matcher = SchemeHostPortMatcher::FromRawString("host;[::1];fe80::/10");
  EXPECT_GT(matcher.EstimateMemoryUsage(), 0u);
#endif
  int calls = 0, deaths = 0;
  ProxyHostMatchingRules a, b;
  a.ParseFromString("*");
  b.ParseFromString("*");
  auto custom = std::make_unique<MutableRule>(calls, deaths);
  auto* p = custom.get();
  a.ReplaceRule(0, std::move(custom));
  b.ReplaceRule(0, std::make_unique<MutableRule>(calls, deaths));
  EXPECT_TRUE(a == b);
  p->text = "changed";
  EXPECT_FALSE(a == b);
  EXPECT_FALSE(a.Matches(url));
}
}  // namespace net
#include <functional>
namespace net {
class ReentrantRule : public SchemeHostPortMatcherRule {
 public:
  std::function<SchemeHostPortMatcherResult()> evaluate;
  std::function<std::string()> serialize;
  SchemeHostPortMatcherResult Evaluate(const GURL&) const override {
    return evaluate();
  }
  std::string ToString() const override { return serialize(); }
};
TEST(ProxyMatchingRust, CustomCallbacksReleaseAllRustAndUrlBorrows) {
  GURL url("http://old.example");
  SchemeHostPortMatcher matcher;
  matcher.AddAsLastRule(
      SchemeHostPortMatcherRule::FromUntrimmedRawString("new.example"));
  auto custom = std::make_unique<ReentrantRule>();
  custom->evaluate = [&] {
    url = GURL("http://new.example");
    return SchemeHostPortMatcherResult::kNoMatch;
  };
  custom->serialize = [] { return "custom"; };
  matcher.AddAsLastRule(std::move(custom));
  EXPECT_TRUE(matcher.Includes(url));
  auto replace = std::make_unique<ReentrantRule>();
  replace->evaluate = [&] {
    matcher.ReplaceRule(0, SchemeHostPortMatcherRule::FromUntrimmedRawString(
                               "replacement.example"));
    url = GURL("http://replacement.example");
    return SchemeHostPortMatcherResult::kNoMatch;
  };
  replace->serialize = [] { return "replace"; };
  matcher.ReplaceRule(1, std::move(replace));
  EXPECT_TRUE(matcher.Includes(url));
  ProxyHostMatchingRules a, b;
  a.ParseFromString("placeholder;old.example");
  b.ParseFromString("custom;new.example");
  auto text = std::make_unique<ReentrantRule>();
  text->evaluate = [] { return SchemeHostPortMatcherResult::kNoMatch; };
  text->serialize = [&] {
    a.ReplaceRule(
        1, SchemeHostPortMatcherRule::FromUntrimmedRawString("new.example"));
    return "custom";
  };
  a.ReplaceRule(0, std::move(text));
  EXPECT_TRUE(a == b);
  a.ReplaceRule(
      1, SchemeHostPortMatcherRule::FromUntrimmedRawString("old.example"));
  EXPECT_EQ("custom;new.example;", a.ToString());
}
}  // namespace net

namespace net {
class DestructionRule : public SchemeHostPortMatcherRule {
 public:
  explicit DestructionRule(std::string& raw) : raw_(raw) {}
  ~DestructionRule() override { *raw_ = "after.example"; }
  SchemeHostPortMatcherResult Evaluate(const GURL&) const override {
    return SchemeHostPortMatcherResult::kNoMatch;
  }
  std::string ToString() const override { return "destructor"; }

 private:
  raw_ref<std::string> raw_;
};
TEST(ProxyMatchingRust, ParseDestroysOldRulesBeforeReadingExternalInput) {
  std::string raw = "before.example";
  ProxyHostMatchingRules rules;
  rules.ParseFromString("*");
  rules.ReplaceRule(0, std::make_unique<DestructionRule>(raw));
  rules.ParseFromString(raw);
  EXPECT_EQ("after.example;", rules.ToString());
}
}  // namespace net
