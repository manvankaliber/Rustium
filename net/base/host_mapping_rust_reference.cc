// Copyright 2010 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/host_mapping_rust_reference.h"

#include <string>

#include "base/logging.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/base/host_port_pair.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"
#include "url/url_canon_ip.h"

namespace net {

namespace host_mapping_original {
bool MatchPattern(std::string_view eval, std::string_view pattern);
}
bool HostMappingRustOriginalMatchPattern(std::string_view input,
                                         std::string_view pattern) {
  return host_mapping_original::MatchPattern(input, pattern);
}
namespace {
bool OriginalParseHostAndPort(std::string_view input,
                              std::string* host,
                              int* port) {
  if (input.empty()) {
    return false;
  }

  url::Component auth_component(0, input.size());
  url::Component username_component;
  url::Component password_component;
  url::Component hostname_component;
  url::Component port_component;

  url::ParseAuthority(input, auth_component, url::ParserMode::kSpecialURL,
                      &username_component, &password_component,
                      &hostname_component, &port_component);

  // There shouldn't be a username/password.
  if (username_component.is_valid() || password_component.is_valid()) {
    return false;
  }

  if (hostname_component.is_empty()) {
    return false;  // Failed parsing.
  }

  int parsed_port_number = -1;
  if (port_component.is_nonempty()) {
    parsed_port_number = url::ParsePort(input, port_component);

    // If parsing failed, port_number will be either PORT_INVALID or
    // PORT_UNSPECIFIED, both of which are negative.
    if (parsed_port_number < 0) {
      return false;  // Failed parsing the port number.
    }
  }

  if (port_component.len == 0) {
    return false;  // Reject inputs like "foo:"
  }

  unsigned char tmp_ipv6_addr[16];

  // If the hostname starts with a bracket, it is either an IPv6 literal or
  // invalid. If it is an IPv6 literal then strip the brackets.
  if (hostname_component.len > 0 && input[hostname_component.begin] == '[') {
    if (input[hostname_component.end() - 1] == ']' &&
        url::IPv6AddressToNumber(hostname_component.AsViewOn(input),
                                 tmp_ipv6_addr)) {
      // Strip the brackets.
      hostname_component.begin++;
      hostname_component.len -= 2;
    } else {
      return false;
    }
  }

  // Pass results back to caller.
  *host = std::string(
      input.substr(hostname_component.begin, hostname_component.len));
  *port = parsed_port_number;

  return true;  // Success.
}

std::string OriginalHostForURL(const std::string& host) {
  if (host.find('\0') != std::string::npos) {
    std::string host_for_log(host);
    size_t nullpos;
    while ((nullpos = host_for_log.find('\0')) != std::string::npos) {
      host_for_log.replace(nullpos, 1, "%00");
    }
    LOG(DFATAL) << "Host has a null char: " << host_for_log;
  }
  if (host.find(':') != std::string::npos) {
    DCHECK_NE(host[0], '[');
    return base::StringPrintf("[%s]", host.c_str());
  }
  return std::string(host);
}
std::string OriginalPairString(const std::string& host, uint16_t port) {
  std::string ret(OriginalHostForURL(host));
  ret += ':';
  ret += base::NumberToString(port);
  return ret;
}
}  // namespace

struct HostMappingRustReference::MapRule {
  MapRule() = default;

  std::string hostname_pattern;
  std::string replacement_hostname;
  int replacement_port = -1;
};

struct HostMappingRustReference::ExclusionRule {
  std::string hostname_pattern;
};

HostMappingRustReference::HostMappingRustReference() = default;

HostMappingRustReference::HostMappingRustReference(
    const HostMappingRustReference& host_mapping_rules) = default;

HostMappingRustReference::~HostMappingRustReference() = default;

HostMappingRustReference& HostMappingRustReference::operator=(
    const HostMappingRustReference& host_mapping_rules) = default;

bool HostMappingRustReference::RewriteHost(HostPortPair* host_port) const {
  // Check if the hostname was remapped.
  for (const auto& map_rule : map_rules_) {
    // The rule's hostname_pattern will be something like:
    //     www.foo.com
    //     *.foo.com
    //     www.foo.com:1234
    //     *.foo.com:1234
    // First, we'll check for a match just on hostname.
    // If that fails, we'll check for a match with both hostname and port.
    if (!host_mapping_original::MatchPattern(host_port->host(),
                                             map_rule.hostname_pattern)) {
      std::string host_port_string =
          OriginalPairString(host_port->host(), host_port->port());
      if (!host_mapping_original::MatchPattern(host_port_string,
                                               map_rule.hostname_pattern)) {
        continue;  // This rule doesn't apply.
      }
    }

    // Check if the hostname was excluded.
    for (const auto& exclusion_rule : exclusion_rules_) {
      if (host_mapping_original::MatchPattern(
              host_port->host(), exclusion_rule.hostname_pattern)) {
        return false;
      }
    }

    host_port->set_host(map_rule.replacement_hostname);
    if (map_rule.replacement_port != -1) {
      host_port->set_port(static_cast<uint16_t>(map_rule.replacement_port));
    }
    return true;
  }

  return false;
}

HostMappingRustReference::RewriteResult HostMappingRustReference::RewriteUrl(
    GURL& url) const {
  // Must be a valid and standard URL. Otherwise, Chrome might not know how to
  // find/replace the contained host or port.
  DCHECK(url.is_valid());
  DCHECK(url.IsStandard());
  DCHECK(url.has_host());

  HostPortPair host_port_pair = HostPortPair(
      url.HostNoBracketsPiece(), static_cast<uint16_t>(url.EffectiveIntPort()));
  if (!RewriteHost(&host_port_pair)) {
    return RewriteResult::kNoMatchingRule;
  }

  GURL::Replacements replacements;
  std::string port_str = base::NumberToString(host_port_pair.port());
  replacements.SetPortStr(port_str);
  std::string host_str = OriginalHostForURL(host_port_pair.host());
  replacements.SetHostStr(host_str);
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

bool HostMappingRustReference::AddRuleFromString(std::string_view rule_string) {
  std::vector<std::string_view> parts = base::SplitStringPiece(
      base::TrimWhitespaceASCII(rule_string, base::TRIM_ALL), " ",
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  // Test for EXCLUSION rule.
  if (parts.size() == 2 &&
      base::EqualsCaseInsensitiveASCII(parts[0], "exclude")) {
    ExclusionRule rule;
    rule.hostname_pattern = base::ToLowerASCII(parts[1]);
    exclusion_rules_.push_back(rule);
    return true;
  }

  // Test for MAP rule.
  if (parts.size() == 3 && base::EqualsCaseInsensitiveASCII(parts[0], "map")) {
    MapRule rule;
    rule.hostname_pattern = base::ToLowerASCII(parts[1]);

    if (!OriginalParseHostAndPort(parts[2], &rule.replacement_hostname,
                                  &rule.replacement_port)) {
      return false;  // Failed parsing the hostname/port.
    }

    map_rules_.push_back(rule);
    return true;
  }

  return false;
}

void HostMappingRustReference::SetRulesFromString(
    std::string_view rules_string) {
  exclusion_rules_.clear();
  map_rules_.clear();

  std::vector<std::string_view> rules = base::SplitStringPiece(
      rules_string, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (std::string_view rule : rules) {
    bool ok = AddRuleFromString(rule);
    LOG_IF(ERROR, !ok) << "Failed parsing rule: " << rule;
  }
}

}  // namespace net
