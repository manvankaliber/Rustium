// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "net/base/scheme_host_port_matcher_rule.h"

#include "base/trace_event/memory_usage_estimator.h"
#include "net/base/proxy_matching_rust_ffi.h"
#include "net/base/proxy_matching_rust_ffi.rs.h"
#include "url/url_util.h"
namespace net {
using namespace proxy_matching_rust;
std::unique_ptr<SchemeHostPortMatcherRule>
SchemeHostPortMatcherRule::FromUntrimmedRawString(std::string_view raw) {
  return Wrap(parse_rule(Bytes(raw), false));
}
bool SchemeHostPortMatcherRule::IsHostnamePatternRule() const {
  return false;
}
const RuleStorage* SchemeHostPortMatcherRule::RustRuleForMatcher() const {
  return nullptr;
}
#if !BUILDFLAG(CRONET_BUILD)
size_t SchemeHostPortMatcherRule::EstimateMemoryUsage() const {
  return 0;
}
#endif
SchemeHostPortMatcherHostnamePatternRule::
    SchemeHostPortMatcherHostnamePatternRule(const std::string& scheme,
                                             const std::string& host,
                                             int port)
    : storage_(std::make_unique<RuleStorage>(
          new_rule(1, Bytes(scheme), Bytes(host), port, Bytes({}), 0))) {
  DCHECK(!url::HostIsIPAddress(host));
}
SchemeHostPortMatcherHostnamePatternRule::
    SchemeHostPortMatcherHostnamePatternRule(std::unique_ptr<RuleStorage> s)
    : storage_(std::move(s)) {}
SchemeHostPortMatcherHostnamePatternRule::
    ~SchemeHostPortMatcherHostnamePatternRule() = default;
SchemeHostPortMatcherResult SchemeHostPortMatcherHostnamePatternRule::Evaluate(
    const GURL& url) const {
  return static_cast<SchemeHostPortMatcherResult>(
      proxy_matching_rust::Evaluate(*storage_->rule, url));
}
std::string SchemeHostPortMatcherHostnamePatternRule::ToString() const {
  return String(rule_serialize(*storage_->rule));
}
bool SchemeHostPortMatcherHostnamePatternRule::IsHostnamePatternRule() const {
  return true;
}
std::unique_ptr<SchemeHostPortMatcherHostnamePatternRule>
SchemeHostPortMatcherHostnamePatternRule::GenerateSuffixMatchingRule() const {
  // ParsedHostname derives from the public hostname rule, preserving downcasts.
  auto r = Wrap(rule_suffix(*storage_->rule));
  return std::unique_ptr<SchemeHostPortMatcherHostnamePatternRule>(
      static_cast<SchemeHostPortMatcherHostnamePatternRule*>(r.release()));
}
SchemeHostPortMatcherIPHostRule::SchemeHostPortMatcherIPHostRule(
    const std::string& scheme,
    const IPEndPoint& endpoint)
    : storage_([&] {
        std::string host = endpoint.address().ToString();
        return std::make_unique<RuleStorage>(new_rule(
            2, Bytes(scheme), Bytes(host), endpoint.port(), Bytes({}), 0));
      }()) {}
SchemeHostPortMatcherIPHostRule::SchemeHostPortMatcherIPHostRule(
    std::unique_ptr<RuleStorage> s)
    : storage_(std::move(s)) {}
SchemeHostPortMatcherIPHostRule::~SchemeHostPortMatcherIPHostRule() = default;
SchemeHostPortMatcherResult SchemeHostPortMatcherIPHostRule::Evaluate(
    const GURL& url) const {
  return static_cast<SchemeHostPortMatcherResult>(
      proxy_matching_rust::Evaluate(*storage_->rule, url));
}
std::string SchemeHostPortMatcherIPHostRule::ToString() const {
  return String(rule_serialize(*storage_->rule));
}
SchemeHostPortMatcherIPBlockRule::SchemeHostPortMatcherIPBlockRule(
    const std::string& description,
    const std::string& scheme,
    const IPAddress& prefix,
    size_t bits)
    : storage_(std::make_unique<RuleStorage>(
          new_rule(3,
                   Bytes(scheme),
                   Bytes(description),
                   0,
                   {prefix.bytes().data(), prefix.size()},
                   bits))) {}
SchemeHostPortMatcherIPBlockRule::SchemeHostPortMatcherIPBlockRule(
    std::unique_ptr<RuleStorage> s)
    : storage_(std::move(s)) {}
SchemeHostPortMatcherIPBlockRule::~SchemeHostPortMatcherIPBlockRule() = default;
SchemeHostPortMatcherResult SchemeHostPortMatcherIPBlockRule::Evaluate(
    const GURL& url) const {
  return static_cast<SchemeHostPortMatcherResult>(
      proxy_matching_rust::Evaluate(*storage_->rule, url));
}
std::string SchemeHostPortMatcherIPBlockRule::ToString() const {
  return String(rule_serialize(*storage_->rule));
}
#if !BUILDFLAG(CRONET_BUILD)
// Account for Rust-owned buffers as well as their backing storage.
size_t SchemeHostPortMatcherHostnamePatternRule::EstimateMemoryUsage() const {
  return rule_memory(*storage_->rule);
}
size_t SchemeHostPortMatcherIPHostRule::EstimateMemoryUsage() const {
  return rule_memory(*storage_->rule);
}
size_t SchemeHostPortMatcherIPBlockRule::EstimateMemoryUsage() const {
  return rule_memory(*storage_->rule);
}
#endif
}  // namespace net
