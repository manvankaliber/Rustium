// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "net/base/scheme_host_port_matcher.h"

#include "base/trace_event/memory_usage_estimator.h"
#include "build/build_config.h"
#include "net/base/proxy_matching_rust_ffi.h"
#include "net/base/proxy_matching_rust_ffi.rs.h"
#include "url/gurl.h"
namespace net {
using namespace proxy_matching_rust;
SchemeHostPortMatcher::SchemeHostPortMatcher()
    : rust_(std::make_unique<MatcherStorage>()) {}
SchemeHostPortMatcher::SchemeHostPortMatcher(SchemeHostPortMatcher&& rhs) =
    default;
SchemeHostPortMatcher& SchemeHostPortMatcher::operator=(
    SchemeHostPortMatcher&& rhs) = default;
SchemeHostPortMatcher::~SchemeHostPortMatcher() = default;
constexpr char SchemeHostPortMatcher::kParseRuleListDelimiterList[];
constexpr char SchemeHostPortMatcher::kPrintRuleListDelimiter;
SchemeHostPortMatcher SchemeHostPortMatcher::FromRawString(
    const std::string& raw) {
  SchemeHostPortMatcher result;
  result.rust_->matcher = parse_list(Bytes(raw), false);
  for (size_t i = 0; i < matcher_size(*result.rust_->matcher); ++i) {
    result.rules_.push_back(Wrap(matcher_rule(*result.rust_->matcher, i)));
  }
  return result;
}
void SchemeHostPortMatcher::AddAsFirstRule(
    std::unique_ptr<SchemeHostPortMatcherRule> r) {
  DCHECK(r);
  if (!rust_) {
    rust_ = std::make_unique<MatcherStorage>();
  }
  if (const auto* s = r->RustRuleForMatcher()) {
    matcher_insert(*rust_->matcher, 0, *s->rule);
  } else {
    matcher_insert_custom(*rust_->matcher, 0);
  }
  rules_.insert(rules_.begin(), std::move(r));
}
void SchemeHostPortMatcher::AddAsLastRule(
    std::unique_ptr<SchemeHostPortMatcherRule> r) {
  DCHECK(r);
  if (!rust_) {
    rust_ = std::make_unique<MatcherStorage>();
  }
  if (const auto* s = r->RustRuleForMatcher()) {
    matcher_insert(*rust_->matcher, rules_.size(), *s->rule);
  } else {
    matcher_insert_custom(*rust_->matcher, rules_.size());
  }
  rules_.push_back(std::move(r));
}
void SchemeHostPortMatcher::ReplaceRule(
    size_t i,
    std::unique_ptr<SchemeHostPortMatcherRule> r) {
  DCHECK_LT(i, rules_.size());
  // Null replacement is permitted by the historical ownership operation; later
  // evaluation/serialization of such a list was undefined and remains invalid.
  if (r && r->RustRuleForMatcher()) {
    matcher_replace(*rust_->matcher, i, *r->RustRuleForMatcher()->rule);
  } else {
    matcher_replace_custom(*rust_->matcher, i);
  }
  rules_[i] = std::move(r);
}
bool SchemeHostPortMatcher::Includes(const GURL& url) const {
  return Evaluate(url) == SchemeHostPortMatcherResult::kInclude;
}
SchemeHostPortMatcherResult SchemeHostPortMatcher::Evaluate(
    const GURL& url) const {
  if (!rust_) {
    return SchemeHostPortMatcherResult::kNoMatch;
  }
  size_t cursor = rules_.size();
  while (true) {
    auto step = matcher_step(*rust_->matcher, cursor, Bytes(url.host()),
                             Bytes(url.scheme()), url.EffectiveIntPort(),
                             url.HostIsIPAddress(), BUILDFLAG(IS_WIN),
                             UnspecifiedEnabled());
    if (!step.custom) {
      return static_cast<SchemeHostPortMatcherResult>(step.result);
    }
    // Arbitrary virtual code runs only after matcher_step released every Rust
    // borrow. It may update another rule or the URL via an external alias.
    auto result = rules_[step.index]->Evaluate(url);
    if (result != SchemeHostPortMatcherResult::kNoMatch) {
      return result;
    }
    cursor = step.index;
  }
}
std::string SchemeHostPortMatcher::ToString() const {
  if (!rust_) {
    return {};
  }
  std::string result, previous;
  size_t cursor = 0;
  bool has_previous = false;
  while (true) {
    auto step = matcher_text_step(*rust_->matcher, cursor, Bytes(previous),
                                  has_previous);
    result += String(std::move(step.bytes));
    if (!step.custom) {
      return result;
    }
    previous = rules_[step.index]->ToString();
    cursor = step.index + 1;
    has_previous = true;
  }
}

void SchemeHostPortMatcher::Clear() {
  rules_.clear();
  if (rust_) {
    matcher_clear(*rust_->matcher);
  }
}
#if !BUILDFLAG(CRONET_BUILD)
size_t SchemeHostPortMatcher::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(rules_) +
         (rust_ ? matcher_memory(*rust_->matcher) : 0);
}
#endif
}  // namespace net
