// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "net/proxy_resolution/proxy_host_matching_rules.h"

#include "build/build_config.h"
#include "net/base/proxy_matching_rust_ffi.h"
#include "net/base/proxy_matching_rust_ffi.rs.h"
#include "url/gurl.h"
namespace net {
using namespace proxy_matching_rust;
constexpr char ProxyHostMatchingRules::kBypassListDelimeter[];
ProxyHostMatchingRules::ProxyHostMatchingRules() = default;
ProxyHostMatchingRules::ProxyHostMatchingRules(
    const ProxyHostMatchingRules& rhs) {
  *this = rhs;
}
ProxyHostMatchingRules::ProxyHostMatchingRules(ProxyHostMatchingRules&& rhs) =
    default;
ProxyHostMatchingRules::~ProxyHostMatchingRules() = default;
ProxyHostMatchingRules& ProxyHostMatchingRules::operator=(
    const ProxyHostMatchingRules& rhs) {
  ParseFromString(rhs.ToString());
  return *this;
}
ProxyHostMatchingRules& ProxyHostMatchingRules::operator=(
    ProxyHostMatchingRules&& rhs) = default;
void ProxyHostMatchingRules::ReplaceRule(
    size_t i,
    std::unique_ptr<SchemeHostPortMatcherRule> r) {
  matcher_.ReplaceRule(i, std::move(r));
}
bool ProxyHostMatchingRules::Matches(const GURL& url, bool reverse) const {
  return matches_proxy(static_cast<uint8_t>(matcher_.Evaluate(url)), reverse,
                       Bytes(url.host()), BUILDFLAG(IS_WIN),
                       UnspecifiedEnabled());
}
bool ProxyHostMatchingRules::operator==(
    const ProxyHostMatchingRules& other) const {
  // Moved-from matchers have the same observable empty list as cleared ones.
  if (!matcher_.rust_ || !other.matcher_.rust_) {
    return rules().empty() && other.rules().empty();
  }
  size_t cursor = 0;
  bool has_previous = false;
  std::string a, b;
  while (true) {
    auto step = matcher_compare_step(*matcher_.rust_->matcher,
                                     *other.matcher_.rust_->matcher, cursor,
                                     Bytes(a), Bytes(b), has_previous);
    if (!step.custom) {
      return step.result == 1;
    }
    a = rules()[step.index]->ToString();
    b = other.rules()[step.index]->ToString();
    cursor = step.index + 1;
    has_previous = true;
  }
}
void ProxyHostMatchingRules::ParseFromString(const std::string& raw) {
  // Preserve destruction-before-parse behavior, including user rule
  // destructors.
  Clear();
  auto parsed = parse_list(Bytes(raw), true);
  if (!matcher_.rust_) {
    matcher_.rust_ = std::make_unique<MatcherStorage>();
  }
  matcher_.rust_->matcher = std::move(parsed);
  for (size_t i = 0; i < matcher_size(*matcher_.rust_->matcher); ++i) {
    matcher_.rules_.push_back(Wrap(matcher_rule(*matcher_.rust_->matcher, i)));
  }
}
void ProxyHostMatchingRules::PrependRuleToBypassSimpleHostnames() {
  matcher_.AddAsFirstRule(
      Wrap(new_rule(4, Bytes({}), Bytes({}), 0, Bytes({}), 0)));
}
bool ProxyHostMatchingRules::AddRuleFromString(std::string_view raw) {
  auto r = Wrap(parse_rule(Bytes(raw), true));
  if (!r) {
    return false;
  }
  matcher_.AddAsLastRule(std::move(r));
  return true;
}
void ProxyHostMatchingRules::AddRulesToSubtractImplicit() {
  matcher_.AddAsLastRule(
      Wrap(new_rule(5, Bytes({}), Bytes({}), 0, Bytes({}), 0)));
}
std::string ProxyHostMatchingRules::GetRulesToSubtractImplicit() {
  ProxyHostMatchingRules r;
  r.AddRulesToSubtractImplicit();
  return r.ToString();
}
std::string ProxyHostMatchingRules::ToString() const {
  return matcher_.ToString();
}
void ProxyHostMatchingRules::Clear() {
  matcher_.Clear();
}
bool ProxyHostMatchingRules::MatchesImplicitRules(const GURL& url) {
  return matches_implicit(Bytes(url.host()), BUILDFLAG(IS_WIN),
                          UnspecifiedEnabled());
}
}  // namespace net
