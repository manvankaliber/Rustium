// Copyright 2010 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/base/host_mapping_rules.h"

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/host_mapping_rust_ffi.rs.h"
#include "net/base/host_port_pair.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"
namespace net {
namespace {
rust::Slice<const uint8_t> Bytes(std::string_view input) {
  // SAFETY: CXX borrows initialized input only for the synchronous call. Empty
  // views may have null data; do not pass that pointer through the bridge.
  return input.empty() ? rust::Slice<const uint8_t>()
                       : UNSAFE_BUFFERS(rust::Slice<const uint8_t>(
                             reinterpret_cast<const uint8_t*>(input.data()),
                             input.size()));
}
std::string_view View(const rust::Vec<uint8_t>& input) {
  // SAFETY: The owned CXX vector stays live through the C++ string assignment.
  return input.empty()
             ? std::string_view()
             : UNSAFE_BUFFERS(std::string_view(
                   reinterpret_cast<const char*>(input.data()), input.size()));
}
}  // namespace
struct HostMappingRules::Impl {
  rust::Box<host_mapping_rust::Rules> rules = host_mapping_rust::new_rules();
  Impl() = default;
  explicit Impl(const Impl& other)
      : rules(host_mapping_rust::clone_rules(*other.rules)) {}
};
HostMappingRules::HostMappingRules() : impl_(std::make_unique<Impl>()) {}
HostMappingRules::HostMappingRules(const HostMappingRules& other)
    : impl_(std::make_unique<Impl>(*other.impl_)) {}
HostMappingRules::~HostMappingRules() = default;
HostMappingRules& HostMappingRules::operator=(const HostMappingRules& other) {
  if (this != &other) {
    impl_ = std::make_unique<Impl>(*other.impl_);
  }
  return *this;
}
bool HostMappingRules::RewriteHost(HostPortPair* pair) const {
  // Input borrows end before mutation. Rust returns independently owned bytes,
  // so a host view passed to AddRuleFromString may alias this same pair safely.
  auto result = host_mapping_rust::rewrite(*impl_->rules, Bytes(pair->host()),
                                           pair->port());
  if (!result.matched) {
    return false;
  }
  pair->set_host(View(result.host));
  pair->set_port(result.port);
  return true;
}
HostMappingRules::RewriteResult HostMappingRules::RewriteUrl(GURL& url) const {
  // Must be a valid and standard URL. Otherwise, Chrome might not know how to
  // find/replace the contained host or port.
  DCHECK(url.is_valid());
  DCHECK(url.IsStandard());
  DCHECK(url.has_host());

  HostPortPair host_port_pair(url.HostNoBracketsPiece(),
                              static_cast<uint16_t>(url.EffectiveIntPort()));
  if (!RewriteHost(&host_port_pair)) {
    return RewriteResult::kNoMatchingRule;
  }

  GURL::Replacements replacements;
  std::string port_str = base::NumberToString(host_port_pair.port());
  replacements.SetPortStr(port_str);
  const auto host =
      host_mapping_rust::format_url_host(Bytes(host_port_pair.host()));
  replacements.SetHostStr(View(host));
  GURL new_url = url.ReplaceComponents(replacements);

  if (!new_url.is_valid()) {
    return RewriteResult::kInvalidRewrite;
  }

  DCHECK(new_url.IsStandard());
  DCHECK(new_url.has_host());
  DCHECK_EQ(url.EffectiveIntPort() == url::PORT_UNSPECIFIED,
            new_url.EffectiveIntPort() == url::PORT_UNSPECIFIED);

  url = std::move(new_url);
  return RewriteResult::kRewritten;
}

bool HostMappingRules::AddRuleFromString(std::string_view input) {
  return host_mapping_rust::add(*impl_->rules, Bytes(input));
}
void HostMappingRules::SetRulesFromString(std::string_view input) {
  const auto invalid = host_mapping_rust::set(*impl_->rules, Bytes(input));
  for (const auto& rule : invalid) {
    LOG(ERROR) << "Failed parsing rule: " << input.substr(rule.begin, rule.len);
  }
}
}  // namespace net
