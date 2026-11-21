// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PROXY_MATCHING_RUST_REF_SCHEME_HOST_PORT_MATCHER_RULE_H_
#define PROXY_MATCHING_RUST_REF_SCHEME_HOST_PORT_MATCHER_RULE_H_

#include <memory>
#include <string>
#include <string_view>

#include "net/base/cronet_buildflags.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_export.h"
#include "net/base/scheme_host_port_matcher_result.h"

class GURL;

namespace net::proxy_matching_reference {

// Interface for an individual SchemeHostPortMatcher rule.
class SchemeHostPortMatcherRule {
 public:
  SchemeHostPortMatcherRule() = default;
  SchemeHostPortMatcherRule(const SchemeHostPortMatcherRule&) = delete;
  SchemeHostPortMatcherRule& operator=(const SchemeHostPortMatcherRule&) =
      delete;

  virtual ~SchemeHostPortMatcherRule() = default;

  // Creates a SchemeHostPortMatcherRule by best-effort parsing the string. If
  // it can't parse, returns a nullptr. It only parses all the rule types in
  // this header file. Types with other serializations will need to be handled
  // by the caller.
  static std::unique_ptr<SchemeHostPortMatcherRule> FromUntrimmedRawString(
      std::string_view raw_untrimmed);

  // Evaluates the rule against |url|.
  virtual SchemeHostPortMatcherResult Evaluate(const GURL& url) const = 0;
  // Returns a string representation of this rule. The returned string will not
  // match any distinguishable rule of any type.
  virtual std::string ToString() const = 0;
  // Returns true if |this| is an instance of
  // SchemeHostPortMatcherHostnamePatternRule.
  virtual bool IsHostnamePatternRule() const;

#if !BUILDFLAG(CRONET_BUILD)
  // Cronet disables tracing and doesn't provide an implementation of
  // base::trace_event::EstimateMemoryUsage. Having this conditional is
  // preferred over a fake implementation to avoid reporting fake metrics.

  // Estimates dynamic memory usage.
  // See base/trace_event/memory_usage_estimator.h for more info.
  virtual size_t EstimateMemoryUsage() const;
#endif  // !BUILDFLAG(CRONET_BUILD)
};

// Rule that matches URLs with wildcard hostname patterns, and
// scheme/port restrictions.
//
// For example:
//   *.google.com
//   https://*.google.com
//   google.com:443
class SchemeHostPortMatcherHostnamePatternRule
    : public SchemeHostPortMatcherRule {
 public:
  SchemeHostPortMatcherHostnamePatternRule(const std::string& optional_scheme,
                                           const std::string& hostname_pattern,
                                           int optional_port);
  SchemeHostPortMatcherHostnamePatternRule(
      const SchemeHostPortMatcherHostnamePatternRule&) = delete;
  SchemeHostPortMatcherHostnamePatternRule& operator=(
      const SchemeHostPortMatcherHostnamePatternRule&) = delete;

  // SchemeHostPortMatcherRule implementation:
  SchemeHostPortMatcherResult Evaluate(const GURL& url) const override;
  std::string ToString() const override;
  bool IsHostnamePatternRule() const override;

  // Generates a new SchemeHostPortMatcherHostnamePatternRule based on the
  // current rule. The new rule will do suffix matching if the current rule
  // doesn't. For example, "google.com" would become "*google.com" and match
  // "foogoogle.com".
  std::unique_ptr<SchemeHostPortMatcherHostnamePatternRule>
  GenerateSuffixMatchingRule() const;

#if !BUILDFLAG(CRONET_BUILD)
  // Cronet disables tracing and doesn't provide an implementation of
  // base::trace_event::EstimateMemoryUsage. Having this conditional is
  // preferred over a fake implementation to avoid reporting fake metrics.

  // Estimates dynamic memory usage.
  // See base/trace_event/memory_usage_estimator.h for more info.
  size_t EstimateMemoryUsage() const override;
#endif  // !BUILDFLAG(CRONET_BUILD)

 private:
  const std::string optional_scheme_;
  const std::string hostname_pattern_;
  const int optional_port_;
};

// Rule that matches URLs with IP address as hostname, and scheme/port
// restrictions. * only works in the host portion. i18n domain names must be
// input in punycode format.
//
// For example:
//   127.0.0.1,
//   http://127.0.0.1
//   [::1]
//   [0:0::1]
//   http://[::1]:99
class SchemeHostPortMatcherIPHostRule : public SchemeHostPortMatcherRule {
 public:
  SchemeHostPortMatcherIPHostRule(const std::string& optional_scheme,
                                  const IPEndPoint& ip_end_point);
  SchemeHostPortMatcherIPHostRule(const SchemeHostPortMatcherIPHostRule&) =
      delete;
  SchemeHostPortMatcherIPHostRule& operator=(
      const SchemeHostPortMatcherIPHostRule&) = delete;

  // SchemeHostPortMatcherRule implementation:
  SchemeHostPortMatcherResult Evaluate(const GURL& url) const override;
  std::string ToString() const override;

#if !BUILDFLAG(CRONET_BUILD)
  // Cronet disables tracing and doesn't provide an implementation of
  // base::trace_event::EstimateMemoryUsage. Having this conditional is
  // preferred over a fake implementation to avoid reporting fake metrics.

  // Estimates dynamic memory usage.
  // See base/trace_event/memory_usage_estimator.h for more info.
  size_t EstimateMemoryUsage() const override;
#endif  // !BUILDFLAG(CRONET_BUILD)

 private:
  const std::string optional_scheme_;
  const std::string ip_host_;
  const int optional_port_;
};

// Rule for matching a URL that is an IP address, if that IP address falls
// within a certain numeric range.
//
// For example:
//   127.0.0.1/8.
//   FE80::/10
//   but not http://127.0.0.1:7/8 or http://[FE80::]/10 (IPv6 with brackets).
class SchemeHostPortMatcherIPBlockRule : public SchemeHostPortMatcherRule {
 public:
  // |ip_prefix| + |prefix_length| define the IP block to match.
  SchemeHostPortMatcherIPBlockRule(const std::string& description,
                                   const std::string& optional_scheme,
                                   const IPAddress& ip_prefix,
                                   size_t prefix_length_in_bits);
  SchemeHostPortMatcherIPBlockRule(const SchemeHostPortMatcherIPBlockRule&) =
      delete;
  SchemeHostPortMatcherIPBlockRule& operator=(
      const SchemeHostPortMatcherIPBlockRule&) = delete;

  // SchemeHostPortMatcherRule implementation:
  SchemeHostPortMatcherResult Evaluate(const GURL& url) const override;
  std::string ToString() const override;

#if !BUILDFLAG(CRONET_BUILD)
  // Cronet disables tracing and doesn't provide an implementation of
  // base::trace_event::EstimateMemoryUsage. Having this conditional is
  // preferred over a fake implementation to avoid reporting fake metrics.

  // Estimates dynamic memory usage.
  // See base/trace_event/memory_usage_estimator.h for more info.
  size_t EstimateMemoryUsage() const override;
#endif  // !BUILDFLAG(CRONET_BUILD)

 private:
  const std::string description_;
  const std::string optional_scheme_;
  const IPAddress ip_prefix_;
  const size_t prefix_length_in_bits_;
};

}  // namespace net::proxy_matching_reference

#endif  // PROXY_MATCHING_RUST_REF_SCHEME_HOST_PORT_MATCHER_RULE_H_

// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PROXY_MATCHING_RUST_REF_SCHEME_HOST_PORT_MATCHER_H_
#define PROXY_MATCHING_RUST_REF_SCHEME_HOST_PORT_MATCHER_H_

#include <string>
#include <vector>

#include "net/base/net_export.h"

namespace net::proxy_matching_reference {

// SchemeHostPortMatcher holds an ordered list of rules for matching URLs, that
// is serializable to a string.
//
// Rules are evaluated in reverse, and each can be either an "include this URL"
// or an "exclude this URL".
//
// In a simple configuration, all rules are "include this URL" so evaluation
// order doesn't matter. When combining include and exclude rules,
// later rules will have precedence over earlier rules.
class SchemeHostPortMatcher {
 public:
  using RuleList = std::vector<std::unique_ptr<SchemeHostPortMatcherRule>>;

  // Note: This class is movable but not copiable.
  SchemeHostPortMatcher();
  SchemeHostPortMatcher(SchemeHostPortMatcher&& rhs);
  SchemeHostPortMatcher& operator=(SchemeHostPortMatcher&& rhs);
  ~SchemeHostPortMatcher();

  // The delimiter used by |ToString()|.
  constexpr static char kPrintRuleListDelimiter = ';';

  // The delimiters to use when parsing rules.
  constexpr static char kParseRuleListDelimiterList[] = ",;";

  // Creates a SchemeHostPortMatcher by best-effort parsing each of the
  // |kParseRuleListDelimiterList| separated rules. Any rules that could not be
  // parsed are silently rejected. It only parses all the rule types that appear
  // in scheme_host_port_rules.h, types with other serializations will need to
  // be handled by the caller.
  static SchemeHostPortMatcher FromRawString(const std::string& raw);

  // Returns the current list of rules.
  const RuleList& rules() const { return rules_; }

  // Add rule to the matcher as the first one in the rule list.
  void AddAsFirstRule(std::unique_ptr<SchemeHostPortMatcherRule> rule);

  // Add rule to the matcher as the last one in the rule list.
  void AddAsLastRule(std::unique_ptr<SchemeHostPortMatcherRule> rule);

  // Replace rule on |index| in the internal RuleList.
  void ReplaceRule(size_t index,
                   std::unique_ptr<SchemeHostPortMatcherRule> rule);

  // Returns true if |url| was positively matched by the rules.
  bool Includes(const GURL& url) const;

  // Returns the result of evaluating the rule list. Prefer using Includes()
  // over this function. Evaluate() can be used when the caller needs to
  // distinguish when matches were due to negative rules.
  SchemeHostPortMatcherResult Evaluate(const GURL& url) const;

  // Serializes the rules to a string representation. The serialized
  // representation is a |kPrintRuleListDelimiter| delimited list of rules,
  // where each rule defines its own serialization.
  std::string ToString() const;

  // Removes all the rules.
  void Clear();

#if !BUILDFLAG(CRONET_BUILD)
  // Cronet disables tracing and doesn't provide an implementation of
  // base::trace_event::EstimateMemoryUsage. Having this conditional is
  // preferred over a fake implementation to avoid reporting fake metrics.

  // Estimates dynamic memory usage.
  // See base/trace_event/memory_usage_estimator.h for more info.
  size_t EstimateMemoryUsage() const;
#endif  // !BUILDFLAG(CRONET_BUILD)

 private:
  RuleList rules_;
};

}  // namespace net::proxy_matching_reference

#endif  // PROXY_MATCHING_RUST_REF_SCHEME_HOST_PORT_MATCHER_H_

// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PROXY_MATCHING_RUST_REF_PROXY_HOST_MATCHING_RULES_H_
#define PROXY_MATCHING_RUST_REF_PROXY_HOST_MATCHING_RULES_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "net/base/net_export.h"

class GURL;

namespace net::proxy_matching_reference {

// ProxyHostMatchingRules describes a set of rules to match against URLs for
// general purposes, for example to determine if such URLs should bypass the use
// of a proxy.
//
// The rules are expressed as an ordered list of rules, which can be thought of
// as being evaluated left-to-right. Order only matters when mixing "negative
// rules" with "positive rules". For more details see the comments in
// ProxyHostMatchingRules::Matches().
//
// This rule list is serializable to a string (either comma or semi-colon
// separated), which has similar semantics across platforms.
//
// When evaluating ProxyHostMatchingRules there are some implicitly applied
// rules when the URL does not match any of the explicit rules. See
// MatchesImplicitRules() for details.
class ProxyHostMatchingRules {
 public:
  // Note: This class supports copy constructor and assignment.
  ProxyHostMatchingRules();
  ProxyHostMatchingRules(const ProxyHostMatchingRules& rhs);
  ProxyHostMatchingRules(ProxyHostMatchingRules&& rhs);
  ~ProxyHostMatchingRules();
  ProxyHostMatchingRules& operator=(const ProxyHostMatchingRules& rhs);
  ProxyHostMatchingRules& operator=(ProxyHostMatchingRules&& rhs);

  // Returns the current list of rules. The rules list contains pointers
  // which are owned by this class, callers should NOT keep references
  // or delete them.
  const SchemeHostPortMatcher::RuleList& rules() const {
    return matcher_.rules();
  }

  // Replace rule on |index| in the internal RuleList.
  void ReplaceRule(size_t index,
                   std::unique_ptr<SchemeHostPortMatcherRule> rule);

  // Returns true if the bypass rules indicate that |url| should bypass the
  // proxy. Matching is done using both the explicit rules, as well as a
  // set of global implicit rules.
  //
  // If |reverse| is set to true then the bypass
  // rule list is inverted (this is almost equivalent to negating the result of
  // Matches(), except for implicit matches).
  bool Matches(const GURL& url, bool reverse = false) const;

  // Returns true if |*this| has the same serialized list of rules as |other|.
  bool operator==(const ProxyHostMatchingRules& other) const;

  // Initializes the list of rules by parsing the string |raw|. |raw| is a
  // comma separated or semi-colon separated list of rules. See
  // AddRuleFromString() to see the specific rule grammar.
  void ParseFromString(const std::string& raw);

  // Adds a rule to the front of thelist that bypasses hostnames without a dot
  // in them (and is not an IP literal), which can be indicative of intranet
  // websites.
  //
  // On Windows this corresponds to the "Bypass proxy server for local
  // addresses" settings checkbox, and on macOS the "Exclude simple hostnames"
  // checkbox.
  void PrependRuleToBypassSimpleHostnames();

  // Adds a rule given by the string |raw|. The format of |raw| can be any of
  // the following:
  //
  // Returns true if the rule was successfully added.
  //
  // For the supported format of bypass rules see //net/docs/proxy.md.
  bool AddRuleFromString(std::string_view raw);

  // Appends rules that "cancels out" the implicit bypass rules. See
  // GetRulesToSubtractImplicit() for details.
  void AddRulesToSubtractImplicit();

  // Returns a list of bypass rules that "cancels out" the implicit bypass
  // rules.
  //
  // The current set of implicit bypass rules are localhost, link-local, and
  // unspecified IP addresses, and are subtracted using <-loopback> (an idiom
  // from Windows), however this could change.
  //
  // If using this for tests, see https://crbug.com/901896.
  static std::string GetRulesToSubtractImplicit();

  // Converts the rules to a string representation (ParseFormat::kDefault).
  std::string ToString() const;

  // Removes all the rules.
  void Clear();

  // Returns true if |url| matches one of the implicit proxy bypass rules
  // (localhost, link-local, or unspecified IP).
  static bool MatchesImplicitRules(const GURL& url);

  // The delimiter used by |ToString()| for the string representation of the
  // proxy bypass rules.
  constexpr static char kBypassListDelimeter[] = ";";

 private:
  SchemeHostPortMatcher matcher_;
};

}  // namespace net::proxy_matching_reference

#endif  // PROXY_MATCHING_RUST_REF_PROXY_HOST_MATCHING_RULES_H_
