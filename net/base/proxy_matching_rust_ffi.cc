// Copyright 2026 The Chromium Authors
#include "net/base/proxy_matching_rust_ffi.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "net/base/proxy_matching_rust_ffi.rs.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"
namespace net::proxy_matching_rust {
namespace {
BASE_FEATURE(kBypassProxyForUnspecifiedIpAddresses,
             base::FEATURE_ENABLED_BY_DEFAULT);
// Only these final, factory-created classes opt into direct Rust evaluation.
// A subclass of a public built-in may override Evaluate/ToString; it must keep
// normal virtual dispatch, rather than accidentally using inherited Rust data.
class ParsedHostname final : public SchemeHostPortMatcherHostnamePatternRule {
 public:
  explicit ParsedHostname(rust::Box<Rule> r)
      : SchemeHostPortMatcherHostnamePatternRule(
            std::make_unique<RuleStorage>(std::move(r))) {}
  const RuleStorage* RustRuleForMatcher() const override {
    return storage_.get();
  }
};
class ParsedIPHost final : public SchemeHostPortMatcherIPHostRule {
 public:
  explicit ParsedIPHost(rust::Box<Rule> r)
      : SchemeHostPortMatcherIPHostRule(
            std::make_unique<RuleStorage>(std::move(r))) {}
  const RuleStorage* RustRuleForMatcher() const override {
    return storage_.get();
  }
};
class ParsedIPBlock final : public SchemeHostPortMatcherIPBlockRule {
 public:
  explicit ParsedIPBlock(rust::Box<Rule> r)
      : SchemeHostPortMatcherIPBlockRule(
            std::make_unique<RuleStorage>(std::move(r))) {}
  const RuleStorage* RustRuleForMatcher() const override {
    return storage_.get();
  }
};
class ProxySpecial final : public SchemeHostPortMatcherRule {
 public:
  explicit ProxySpecial(rust::Box<Rule> r) : storage_(std::move(r)) {}
  SchemeHostPortMatcherResult Evaluate(const GURL& url) const override {
    return static_cast<SchemeHostPortMatcherResult>(
        proxy_matching_rust::Evaluate(*storage_.rule, url));
  }
  std::string ToString() const override {
    return String(rule_serialize(*storage_.rule));
  }
  const RuleStorage* RustRuleForMatcher() const override { return &storage_; }
#if !BUILDFLAG(CRONET_BUILD)
  size_t EstimateMemoryUsage() const override {
    return sizeof(RuleStorage) + rule_memory(*storage_.rule);
  }
#endif

 private:
  RuleStorage storage_;
};
}  // namespace
RuleStorage::RuleStorage(rust::Box<Rule> r) : rule(std::move(r)) {}
RuleStorage::~RuleStorage() = default;
MatcherStorage::MatcherStorage() : matcher(new_matcher()) {}
MatcherStorage::~MatcherStorage() = default;
rust::Slice<const uint8_t> Bytes(std::string_view v) {
  // CXX slices require a non-null aligned pointer even for length zero.
  static constexpr uint8_t kEmpty = 0;
  return {v.empty() ? &kEmpty : reinterpret_cast<const uint8_t*>(v.data()),
          v.size()};
}
std::string String(rust::Vec<uint8_t> v) {
  return {reinterpret_cast<const char*>(v.data()), v.size()};
}
rust::Vec<uint8_t> ip_service(rust::Slice<const uint8_t> raw) {
  rust::Vec<uint8_t> result;
  IPAddress ip;
  if (!ip.AssignFromIPLiteral(
          {reinterpret_cast<const char*>(raw.data()), raw.size()})) {
    return result;
  }
  for (uint8_t b : ip.bytes()) {
    result.push_back(b);
  }
  return result;
}
rust::Vec<uint8_t> canonical_service(rust::Slice<const uint8_t> bytes) {
  IPAddress ip{base::span<const uint8_t>(bytes)};
  rust::Vec<uint8_t> result;
  for (uint8_t b : ip.ToString()) {
    result.push_back(b);
  }
  return result;
}
Authority authority_service(rust::Slice<const uint8_t> raw) {
  Authority result;
  std::string_view v(reinterpret_cast<const char*>(raw.data()), raw.size());
  url::Component user, password, host, port;
  url::ParseAuthority(v, url::Component(0, v.size()),
                      url::ParserMode::kSpecialURL, &user, &password, &host,
                      &port);
  result.user = user.is_valid() || password.is_valid();
  result.has_port = port.is_valid();
  if (port.is_nonempty()) {
    for (uint8_t b : port.AsViewOn(v)) {
      result.port.push_back(b);
    }
  }
  if (host.is_nonempty()) {
    for (uint8_t b : host.AsViewOn(v)) {
      result.host.push_back(b);
    }
  }
  return result;
}
std::unique_ptr<SchemeHostPortMatcherRule> Wrap(rust::Box<Rule> r) {
  switch (rule_kind(*r)) {
    case 1:
      return std::make_unique<ParsedHostname>(std::move(r));
    case 2:
      return std::make_unique<ParsedIPHost>(std::move(r));
    case 3:
      return std::make_unique<ParsedIPBlock>(std::move(r));
    case 4:
    case 5:
      return std::make_unique<ProxySpecial>(std::move(r));
    default:
      return nullptr;
  }
}
bool UnspecifiedEnabled() {
  return base::FeatureList::IsEnabled(kBypassProxyForUnspecifiedIpAddresses);
}
uint8_t Evaluate(const Rule& r, const GURL& url) {
  return rule_evaluate(r, Bytes(url.host()), Bytes(url.scheme()),
                       url.EffectiveIntPort(), url.HostIsIPAddress(),
                       BUILDFLAG(IS_WIN), UnspecifiedEnabled());
}
}  // namespace net::proxy_matching_rust
