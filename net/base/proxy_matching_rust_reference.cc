namespace net::proxy_matching_rust {
bool UnspecifiedEnabled();
}
#include "net/base/proxy_matching_rust_reference.h"

#include "base/strings/string_split.h"
#include "net/base/parse_number.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon_ip.h"
namespace net::proxy_matching_frozen_pattern {
bool MatchPattern(std::string_view, std::string_view);
}
namespace net::proxy_matching_reference {
bool ParseHostAndPort(std::string_view, std::string*, int*);
bool IsLocalhost(const GURL&);
bool ParseCIDRBlock(std::string_view, IPAddress*, size_t*);
bool ParseUint32(std::string_view,
                 ParseIntFormat,
                 uint32_t*,
                 ParseIntError* = nullptr);
bool ParseInt32(std::string_view,
                ParseIntFormat,
                int32_t*,
                ParseIntError* = nullptr);
}  // namespace net::proxy_matching_reference
// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "net/base/parse_number.h"

namespace net::proxy_matching_reference {

namespace {

// The string to number conversion functions in //base include the type in the
// name (like StringToInt64()). The following wrapper methods create a
// consistent interface to StringToXXX() that calls the appropriate //base
// version. This simplifies writing generic code with a template.

bool StringToNumber(std::string_view input, int32_t* output) {
  // This assumes ints are 32-bits (will fail compile if that ever changes).
  return base::StringToInt(input, output);
}

bool StringToNumber(std::string_view input, uint32_t* output) {
  // This assumes ints are 32-bits (will fail compile if that ever changes).
  return base::StringToUint(input, output);
}

bool StringToNumber(std::string_view input, int64_t* output) {
  return base::StringToInt64(input, output);
}

bool StringToNumber(std::string_view input, uint64_t* output) {
  return base::StringToUint64(input, output);
}

bool SetError(ParseIntError error, ParseIntError* optional_error) {
  if (optional_error) {
    *optional_error = error;
  }
  return false;
}

template <typename T>
bool ParseIntHelper(std::string_view input,
                    ParseIntFormat format,
                    T* output,
                    ParseIntError* optional_error) {
  // Check that the input matches the format before calling StringToNumber().
  // Numbers must start with either a digit or a negative sign.
  if (input.empty()) {
    return SetError(ParseIntError::FAILED_PARSE, optional_error);
  }

  bool is_non_negative = (format == ParseIntFormat::NON_NEGATIVE ||
                          format == ParseIntFormat::STRICT_NON_NEGATIVE);
  bool is_strict = (format == ParseIntFormat::STRICT_NON_NEGATIVE ||
                    format == ParseIntFormat::STRICT_OPTIONALLY_NEGATIVE);

  bool starts_with_negative = input[0] == '-';
  bool starts_with_digit = base::IsAsciiDigit(input[0]);

  if (!starts_with_digit) {
    // The length() < 2 check catches "-". It's needed here to prevent reading
    // beyond the end of the array on line 70.
    if (is_non_negative || !starts_with_negative || input.length() < 2) {
      return SetError(ParseIntError::FAILED_PARSE, optional_error);
    }
    // If the first digit after the negative is a 0, then either the number is
    // -0 or it has an unnecessary leading 0. Either way, it violates the
    // requirements of being "strict", so fail if strict.
    if (is_strict && input[1] == '0') {
      return SetError(ParseIntError::FAILED_PARSE, optional_error);
    }
  } else {
    // Fail if the first character is a zero and the string has more than 1
    // digit.
    if (is_strict && input[0] == '0' && input.length() > 1) {
      return SetError(ParseIntError::FAILED_PARSE, optional_error);
    }
  }

  // Dispatch to the appropriate flavor of base::StringToXXX() by calling one of
  // the type-specific overloads.
  T result;
  if (StringToNumber(input, &result)) {
    *output = result;
    return true;
  }

  // Optimization: If the error is not going to be inspected, don't bother
  // calculating it.
  if (!optional_error) {
    return false;
  }

  // Set an error that distinguishes between parsing/underflow/overflow errors.
  //
  // Note that the output set by base::StringToXXX() on failure cannot be used
  // as it has ambiguity with parse errors.

  // Strip any leading negative sign off the number.
  std::string_view numeric_portion =
      starts_with_negative ? input.substr(1) : input;

  // Test if |numeric_portion| is a valid non-negative integer.
  if (!numeric_portion.empty() &&
      numeric_portion.find_first_not_of("0123456789") == std::string::npos) {
    // If it was, the failure must have been due to underflow/overflow.
    return SetError(starts_with_negative ? ParseIntError::FAILED_UNDERFLOW
                                         : ParseIntError::FAILED_OVERFLOW,
                    optional_error);
  }

  // Otherwise it was a mundane parsing error.
  return SetError(ParseIntError::FAILED_PARSE, optional_error);
}

}  // namespace

bool ParseInt32(std::string_view input,
                ParseIntFormat format,
                int32_t* output,
                ParseIntError* optional_error) {
  return ParseIntHelper(input, format, output, optional_error);
}

bool ParseInt64(std::string_view input,
                ParseIntFormat format,
                int64_t* output,
                ParseIntError* optional_error) {
  return ParseIntHelper(input, format, output, optional_error);
}

bool ParseUint32(std::string_view input,
                 ParseIntFormat format,
                 uint32_t* output,
                 ParseIntError* optional_error) {
  CHECK(format == ParseIntFormat::NON_NEGATIVE ||
        format == ParseIntFormat::STRICT_NON_NEGATIVE);
  return ParseIntHelper(input, format, output, optional_error);
}

bool ParseUint64(std::string_view input,
                 ParseIntFormat format,
                 uint64_t* output,
                 ParseIntError* optional_error) {
  CHECK(format == ParseIntFormat::NON_NEGATIVE ||
        format == ParseIntFormat::STRICT_NON_NEGATIVE);
  return ParseIntHelper(input, format, output, optional_error);
}

}  // namespace net::proxy_matching_reference

namespace net::proxy_matching_reference {
bool ParseHostAndPort(std::string_view input, std::string* host, int* port) {
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
bool IsNormalizedLocalhostTLD(std::string_view host) {
  return base::EndsWith(host, ".localhost",
                        base::CompareCase::INSENSITIVE_ASCII);
}
bool IsLocalHostname(std::string_view host) {
  // Remove any trailing '.'.
  if (!host.empty() && *host.rbegin() == '.') {
    host.remove_suffix(1);
  }

  return base::EqualsCaseInsensitiveASCII(host, "localhost") ||
         IsNormalizedLocalhostTLD(host);
}
bool HostStringIsLocalhost(std::string_view host) {
  IPAddress ip_address;
  if (ip_address.AssignFromIPLiteral(host)) {
    return ip_address.IsLoopback();
  }
  return IsLocalHostname(host);
}
bool IsLocalhost(const GURL& url) {
  return HostStringIsLocalhost(url.HostNoBracketsPiece());
}
bool ParseCIDRBlock(std::string_view cidr_literal,
                    IPAddress* ip_address,
                    size_t* prefix_length_in_bits) {
  // We expect CIDR notation to match one of these two templates:
  //   <IPv4-literal> "/" <number of bits>
  //   <IPv6-literal> "/" <number of bits>
  std::vector<std::string_view> parts = base::SplitStringPiece(
      cidr_literal, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() != 2) {
    return false;
  }

  // Parse the IP address.
  if (!ip_address->AssignFromIPLiteral(parts[0])) {
    return false;
  }

  uint32_t bits;
  if (!proxy_matching_reference::ParseUint32(
          parts[1], ParseIntFormat::NON_NEGATIVE, &bits, nullptr) ||
      bits > ip_address->size() * 8) {
    return false;
  }
  *prefix_length_in_bits = bits;
  return true;
}

}  // namespace net::proxy_matching_reference
// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "net/base/host_port_pair.h"
#include "net/base/parse_number.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace net::proxy_matching_reference {

namespace {

std::string AddBracketsIfIPv6(const IPAddress& ip_address) {
  std::string ip_host = ip_address.ToString();
  if (ip_address.IsIPv6()) {
    return base::StringPrintf("[%s]", ip_host.c_str());
  }
  return ip_host;
}

}  // namespace

// static
std::unique_ptr<SchemeHostPortMatcherRule>
SchemeHostPortMatcherRule::FromUntrimmedRawString(
    std::string_view raw_untrimmed) {
  std::string_view raw =
      base::TrimWhitespaceASCII(raw_untrimmed, base::TRIM_ALL);

  // Extract any scheme-restriction.
  std::string::size_type scheme_pos = raw.find("://");
  std::string scheme;
  if (scheme_pos != std::string::npos) {
    scheme = std::string(raw.substr(0, scheme_pos));
    raw = raw.substr(scheme_pos + 3);
    if (scheme.empty()) {
      return nullptr;
    }
  }

  if (raw.empty()) {
    return nullptr;
  }

  // If there is a forward slash in the input, it is probably a CIDR style
  // mask.
  if (raw.find('/') != std::string::npos) {
    IPAddress ip_prefix;
    size_t prefix_length_in_bits;

    if (!proxy_matching_reference::ParseCIDRBlock(raw, &ip_prefix,
                                                  &prefix_length_in_bits)) {
      return nullptr;
    }

    return std::make_unique<SchemeHostPortMatcherIPBlockRule>(
        std::string(raw), scheme, ip_prefix, prefix_length_in_bits);
  }

  // Check if we have an <ip-address>[:port] input. We need to treat this
  // separately since the IP literal may not be in a canonical form.
  std::string host;
  int port;
  if (ParseHostAndPort(raw, &host, &port)) {
    IPAddress ip_address;
    if (ip_address.AssignFromIPLiteral(host)) {
      // Instead of -1, 0 is invalid for IPEndPoint.
      int adjusted_port = port == -1 ? 0 : port;
      return std::make_unique<SchemeHostPortMatcherIPHostRule>(
          scheme, IPEndPoint(ip_address, adjusted_port));
    }
  }

  // Otherwise assume we have <hostname-pattern>[:port].
  std::string::size_type pos_colon = raw.rfind(':');
  port = -1;
  if (pos_colon != std::string::npos) {
    if (!proxy_matching_reference::ParseInt32(
            raw.substr(pos_colon + 1), ParseIntFormat::NON_NEGATIVE, &port) ||
        port > 0xFFFF) {
      return nullptr;  // Port was invalid.
    }
    raw = raw.substr(0, pos_colon);
  }

  // Special-case hostnames that begin with a period.
  // For example, we remap ".google.com" --> "*.google.com".
  std::string hostname_pattern;
  if (raw.starts_with(".")) {
    hostname_pattern = base::StrCat({"*", raw});
  } else {
    hostname_pattern = std::string(raw);
  }

  return std::make_unique<SchemeHostPortMatcherHostnamePatternRule>(
      scheme, hostname_pattern, port);
}

bool SchemeHostPortMatcherRule::IsHostnamePatternRule() const {
  return false;
}

#if !BUILDFLAG(CRONET_BUILD)
size_t SchemeHostPortMatcherRule::EstimateMemoryUsage() const {
  return 0;
}
#endif  // !BUILDFLAG(CRONET_BUILD)

SchemeHostPortMatcherHostnamePatternRule::
    SchemeHostPortMatcherHostnamePatternRule(
        const std::string& optional_scheme,
        const std::string& hostname_pattern,
        int optional_port)
    : optional_scheme_(base::ToLowerASCII(optional_scheme)),
      hostname_pattern_(base::ToLowerASCII(hostname_pattern)),
      optional_port_(optional_port) {
  // |hostname_pattern| shouldn't be an IP address.
  DCHECK(!url::HostIsIPAddress(hostname_pattern));
}

SchemeHostPortMatcherResult SchemeHostPortMatcherHostnamePatternRule::Evaluate(
    const GURL& url) const {
  if (optional_port_ != -1 && url.EffectiveIntPort() != optional_port_) {
    // Didn't match port expectation.
    return SchemeHostPortMatcherResult::kNoMatch;
  }

  if (!optional_scheme_.empty() && url.GetScheme() != optional_scheme_) {
    // Didn't match scheme expectation.
    return SchemeHostPortMatcherResult::kNoMatch;
  }

  // Note it is necessary to lower-case the host, since GURL uses capital
  // letters for percent-escaped characters.
  return proxy_matching_frozen_pattern::MatchPattern(url.GetHost(),
                                                     hostname_pattern_)
             ? SchemeHostPortMatcherResult::kInclude
             : SchemeHostPortMatcherResult::kNoMatch;
}

std::string SchemeHostPortMatcherHostnamePatternRule::ToString() const {
  std::string str;
  if (!optional_scheme_.empty()) {
    base::StringAppendF(&str, "%s://", optional_scheme_.c_str());
  }
  str += hostname_pattern_;
  if (optional_port_ != -1) {
    base::StringAppendF(&str, ":%d", optional_port_);
  }
  return str;
}

bool SchemeHostPortMatcherHostnamePatternRule::IsHostnamePatternRule() const {
  return true;
}

std::unique_ptr<SchemeHostPortMatcherHostnamePatternRule>
SchemeHostPortMatcherHostnamePatternRule::GenerateSuffixMatchingRule() const {
  if (!hostname_pattern_.starts_with("*")) {
    return std::make_unique<SchemeHostPortMatcherHostnamePatternRule>(
        optional_scheme_, "*" + hostname_pattern_, optional_port_);
  }
  // return a new SchemeHostPortMatcherHostNamePatternRule with the same data.
  return std::make_unique<SchemeHostPortMatcherHostnamePatternRule>(
      optional_scheme_, hostname_pattern_, optional_port_);
}

#if !BUILDFLAG(CRONET_BUILD)
size_t SchemeHostPortMatcherHostnamePatternRule::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(optional_scheme_) +
         base::trace_event::EstimateMemoryUsage(hostname_pattern_);
}
#endif  // !BUILDFLAG(CRONET_BUILD)

SchemeHostPortMatcherIPHostRule::SchemeHostPortMatcherIPHostRule(
    const std::string& optional_scheme,
    const IPEndPoint& ip_end_point)
    : optional_scheme_(base::ToLowerASCII(optional_scheme)),
      ip_host_(AddBracketsIfIPv6(ip_end_point.address())),
      optional_port_(ip_end_point.port()) {}

SchemeHostPortMatcherResult SchemeHostPortMatcherIPHostRule::Evaluate(
    const GURL& url) const {
  if (optional_port_ != 0 && url.EffectiveIntPort() != optional_port_) {
    // Didn't match port expectation.
    return SchemeHostPortMatcherResult::kNoMatch;
  }

  if (!optional_scheme_.empty() && url.GetScheme() != optional_scheme_) {
    // Didn't match scheme expectation.
    return SchemeHostPortMatcherResult::kNoMatch;
  }

  // Note it is necessary to lower-case the host, since GURL uses capital
  // letters for percent-escaped characters.
  return proxy_matching_frozen_pattern::MatchPattern(url.GetHost(), ip_host_)
             ? SchemeHostPortMatcherResult::kInclude
             : SchemeHostPortMatcherResult::kNoMatch;
}

std::string SchemeHostPortMatcherIPHostRule::ToString() const {
  std::string str;
  if (!optional_scheme_.empty()) {
    base::StringAppendF(&str, "%s://", optional_scheme_.c_str());
  }
  str += ip_host_;
  if (optional_port_ != 0) {
    base::StringAppendF(&str, ":%d", optional_port_);
  }
  return str;
}

#if !BUILDFLAG(CRONET_BUILD)
size_t SchemeHostPortMatcherIPHostRule::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(optional_scheme_) +
         base::trace_event::EstimateMemoryUsage(ip_host_);
}
#endif  // !BUILDFLAG(CRONET_BUILD)

SchemeHostPortMatcherIPBlockRule::SchemeHostPortMatcherIPBlockRule(
    const std::string& description,
    const std::string& optional_scheme,
    const IPAddress& ip_prefix,
    size_t prefix_length_in_bits)
    : description_(description),
      optional_scheme_(optional_scheme),
      ip_prefix_(ip_prefix),
      prefix_length_in_bits_(prefix_length_in_bits) {}

SchemeHostPortMatcherResult SchemeHostPortMatcherIPBlockRule::Evaluate(
    const GURL& url) const {
  if (!url.HostIsIPAddress()) {
    return SchemeHostPortMatcherResult::kNoMatch;
  }

  if (!optional_scheme_.empty() && url.GetScheme() != optional_scheme_) {
    // Didn't match scheme expectation.
    return SchemeHostPortMatcherResult::kNoMatch;
  }

  // Parse the input IP literal to a number.
  IPAddress ip_address;
  if (!ip_address.AssignFromIPLiteral(url.HostNoBracketsPiece())) {
    return SchemeHostPortMatcherResult::kNoMatch;
  }

  // Test if it has the expected prefix.
  return IPAddressMatchesPrefix(ip_address, ip_prefix_, prefix_length_in_bits_)
             ? SchemeHostPortMatcherResult::kInclude
             : SchemeHostPortMatcherResult::kNoMatch;
}

std::string SchemeHostPortMatcherIPBlockRule::ToString() const {
  return description_;
}

#if !BUILDFLAG(CRONET_BUILD)
size_t SchemeHostPortMatcherIPBlockRule::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(description_) +
         base::trace_event::EstimateMemoryUsage(optional_scheme_) +
         base::trace_event::EstimateMemoryUsage(ip_prefix_);
}
#endif  // !BUILDFLAG(CRONET_BUILD)

}  // namespace net::proxy_matching_reference

// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ranges>

#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/trace_event/memory_usage_estimator.h"

namespace net::proxy_matching_reference {

SchemeHostPortMatcher::SchemeHostPortMatcher() = default;
SchemeHostPortMatcher::SchemeHostPortMatcher(SchemeHostPortMatcher&& rhs) =
    default;
SchemeHostPortMatcher& SchemeHostPortMatcher::operator=(
    SchemeHostPortMatcher&& rhs) = default;
SchemeHostPortMatcher::~SchemeHostPortMatcher() = default;

// Declares SchemeHostPortMatcher::kParseRuleListDelimiterList[], not a
// redefinition. This is needed for link.
// static
constexpr char SchemeHostPortMatcher::kParseRuleListDelimiterList[];

// Declares SchemeHostPortMatcher::kPrintRuleListDelimiter, not a
// redefinition. This is needed for link.
// static
constexpr char SchemeHostPortMatcher::kPrintRuleListDelimiter;

// static
SchemeHostPortMatcher SchemeHostPortMatcher::FromRawString(
    const std::string& raw) {
  SchemeHostPortMatcher result;

  base::StringTokenizer entries(raw, kParseRuleListDelimiterList);
  while (entries.GetNext()) {
    auto rule = SchemeHostPortMatcherRule::FromUntrimmedRawString(
        entries.token_piece());
    if (rule) {
      result.AddAsLastRule(std::move(rule));
    }
  }

  return result;
}

void SchemeHostPortMatcher::AddAsFirstRule(
    std::unique_ptr<SchemeHostPortMatcherRule> rule) {
  DCHECK(rule);
  rules_.insert(rules_.begin(), std::move(rule));
}

void SchemeHostPortMatcher::AddAsLastRule(
    std::unique_ptr<SchemeHostPortMatcherRule> rule) {
  DCHECK(rule);
  rules_.push_back(std::move(rule));
}

void SchemeHostPortMatcher::ReplaceRule(
    size_t index,
    std::unique_ptr<SchemeHostPortMatcherRule> rule) {
  DCHECK_LT(index, rules_.size());
  rules_[index] = std::move(rule);
}

bool SchemeHostPortMatcher::Includes(const GURL& url) const {
  return Evaluate(url) == SchemeHostPortMatcherResult::kInclude;
}

SchemeHostPortMatcherResult SchemeHostPortMatcher::Evaluate(
    const GURL& url) const {
  // Later rules override earlier rules, so evaluating the rule list can be
  // done by iterating over it in reverse and short-circuiting when a match is
  // found.
  //
  // The order of evaluation generally doesn't matter if all the rules are
  // positive rules, so matches are just additive.
  //
  // However when mixing positive and negative rules, evaluation order makes a
  // difference.
  for (const auto& rule : std::views::reverse(rules_)) {
    SchemeHostPortMatcherResult result = rule->Evaluate(url);
    if (result != SchemeHostPortMatcherResult::kNoMatch) {
      return result;
    }
  }

  return SchemeHostPortMatcherResult::kNoMatch;
}

std::string SchemeHostPortMatcher::ToString() const {
  std::string result;
  for (const auto& rule : rules_) {
    DCHECK(!rule->ToString().contains(kParseRuleListDelimiterList));
    result += rule->ToString();
    result.push_back(kPrintRuleListDelimiter);
  }
  return result;
}

void SchemeHostPortMatcher::Clear() {
  rules_.clear();
}

#if !BUILDFLAG(CRONET_BUILD)
size_t SchemeHostPortMatcher::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(rules_);
}
#endif  // !BUILDFLAG(CRONET_BUILD)

}  // namespace net::proxy_matching_reference

// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/feature_list.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace net::proxy_matching_reference {

namespace {

// The immutable FeatureList service is shared to preserve feature identity.

// The <-loopback> rule corresponds with "remove the implicitly added bypass
// rules".
//
// The name <-loopback> is not a very precise name (as the implicit rules cover
// more than strictly loopback addresses), however this is the name that is
// used on Windows so re-used here.
//
// For platform-differences between implicit rules see
// ProxyResolverRules::MatchesImplicitRules().
const char kSubtractImplicitBypasses[] = "<-loopback>";

// The <local> rule bypasses any hostname that has no dots (and is not
// an IP literal). The name is misleading as it has nothing to do with
// localhost/loopback addresses, and would have better been called
// something like "simple hostnames". However this is the name used on
// Windows so is matched here.
const char kBypassSimpleHostnames[] = "<local>";

bool IsLinkLocalIP(const GURL& url) {
  // Quick fail if definitely not link-local, to avoid doing unnecessary work in
  // common case.
  if (!(url.host().starts_with("169.254.") || url.host().starts_with("["))) {
    return false;
  }

  IPAddress ip_address;
  if (!ip_address.AssignFromIPLiteral(url.HostNoBracketsPiece())) {
    return false;
  }

  return ip_address.IsLinkLocal();
}

// Returns true if the URL's host is an IPv6 literal in the range
// [::ffff:127.0.0.1]/104.
//
// Note that net::IsLocalhost() does not currently return true for such
// addresses. However for proxy resolving such URLs should bypass the use
// of a PAC script, since the destination is local.
bool IsIPv4MappedLoopback(const GURL& url) {
  if (!url.host().starts_with("[::ffff")) {
    return false;
  }

  IPAddress ip_address;
  if (!ip_address.AssignFromIPLiteral(url.HostNoBracketsPiece())) {
    return false;
  }

  if (!ip_address.IsIPv4MappedIPv6()) {
    return false;
  }

  return ip_address.bytes()[12] == 127;
}

// Returns true if the URL's host is an unspecified IP address (e.g., 0.0.0.0,
// [::], or [::ffff:0.0.0.0]). These route to loopback on POSIX systems and
// should implicitly bypass proxy resolution.
bool IsUnspecifiedIP(const GURL& url) {
  // Quick fail if definitely not an unspecified IP, to avoid doing unnecessary
  // work in the common case.
  if (!url.host().starts_with("0.") && !url.host().starts_with("[")) {
    return false;
  }

  IPAddress ip_address;
  if (!ip_address.AssignFromIPLiteral(url.HostNoBracketsPiece())) {
    return false;
  }

  if (ip_address.IsZero()) {
    return true;
  }

  return ip_address.IsIPv4MappedIPv6() &&
         ConvertIPv4MappedIPv6ToIPv4(ip_address).IsZero();
}

class BypassSimpleHostnamesRule : public SchemeHostPortMatcherRule {
 public:
  BypassSimpleHostnamesRule() = default;

  BypassSimpleHostnamesRule(const BypassSimpleHostnamesRule&) = delete;
  BypassSimpleHostnamesRule& operator=(const BypassSimpleHostnamesRule&) =
      delete;

  SchemeHostPortMatcherResult Evaluate(const GURL& url) const override {
    return ((url.host().find('.') == std::string::npos) &&
            !url.HostIsIPAddress())
               ? SchemeHostPortMatcherResult::kInclude
               : SchemeHostPortMatcherResult::kNoMatch;
  }

  std::string ToString() const override { return kBypassSimpleHostnames; }
};

class SubtractImplicitBypassesRule : public SchemeHostPortMatcherRule {
 public:
  SubtractImplicitBypassesRule() = default;

  SubtractImplicitBypassesRule(const SubtractImplicitBypassesRule&) = delete;
  SubtractImplicitBypassesRule& operator=(const SubtractImplicitBypassesRule&) =
      delete;

  SchemeHostPortMatcherResult Evaluate(const GURL& url) const override {
    return ProxyHostMatchingRules::MatchesImplicitRules(url)
               ? SchemeHostPortMatcherResult::kExclude
               : SchemeHostPortMatcherResult::kNoMatch;
  }

  std::string ToString() const override { return kSubtractImplicitBypasses; }
};

std::unique_ptr<SchemeHostPortMatcherRule> ParseRule(
    std::string_view raw_untrimmed) {
  std::string_view raw =
      base::TrimWhitespaceASCII(raw_untrimmed, base::TRIM_ALL);

  // <local> and <-loopback> are special syntax used by WinInet's bypass list
  // -- we allow it on all platforms and interpret it the same way.
  if (base::EqualsCaseInsensitiveASCII(raw, kBypassSimpleHostnames)) {
    return std::make_unique<BypassSimpleHostnamesRule>();
  }
  if (base::EqualsCaseInsensitiveASCII(raw, kSubtractImplicitBypasses)) {
    return std::make_unique<SubtractImplicitBypassesRule>();
  }

  return SchemeHostPortMatcherRule::FromUntrimmedRawString(raw_untrimmed);
}

}  // namespace

constexpr char ProxyHostMatchingRules::kBypassListDelimeter[];

ProxyHostMatchingRules::ProxyHostMatchingRules() = default;

ProxyHostMatchingRules::ProxyHostMatchingRules(
    const ProxyHostMatchingRules& rhs) {
  *this = rhs;
}

ProxyHostMatchingRules::ProxyHostMatchingRules(ProxyHostMatchingRules&& rhs) {
  *this = std::move(rhs);
}

ProxyHostMatchingRules::~ProxyHostMatchingRules() = default;

ProxyHostMatchingRules& ProxyHostMatchingRules::operator=(
    const ProxyHostMatchingRules& rhs) {
  ParseFromString(rhs.ToString());
  return *this;
}

ProxyHostMatchingRules& ProxyHostMatchingRules::operator=(
    ProxyHostMatchingRules&& rhs) {
  matcher_ = std::move(rhs.matcher_);
  return *this;
}

void ProxyHostMatchingRules::ReplaceRule(
    size_t index,
    std::unique_ptr<SchemeHostPortMatcherRule> rule) {
  matcher_.ReplaceRule(index, std::move(rule));
}

bool ProxyHostMatchingRules::Matches(const GURL& url, bool reverse) const {
  switch (matcher_.Evaluate(url)) {
    case SchemeHostPortMatcherResult::kInclude:
      return !reverse;
    case SchemeHostPortMatcherResult::kExclude:
      return reverse;
    case SchemeHostPortMatcherResult::kNoMatch:
      break;
  }

  // If none of the explicit rules matched, fall back to the implicit rules.
  bool matches_implicit = MatchesImplicitRules(url);
  if (matches_implicit) {
    return matches_implicit;
  }

  return reverse;
}

bool ProxyHostMatchingRules::operator==(
    const ProxyHostMatchingRules& other) const {
  if (rules().size() != other.rules().size()) {
    return false;
  }

  for (size_t i = 0; i < rules().size(); ++i) {
    if (rules()[i]->ToString() != other.rules()[i]->ToString()) {
      return false;
    }
  }
  return true;
}

void ProxyHostMatchingRules::ParseFromString(const std::string& raw) {
  Clear();

  base::StringTokenizer entries(
      raw, SchemeHostPortMatcher::kParseRuleListDelimiterList);
  while (entries.GetNext()) {
    AddRuleFromString(entries.token_piece());
  }
}

void ProxyHostMatchingRules::PrependRuleToBypassSimpleHostnames() {
  matcher_.AddAsFirstRule(std::make_unique<BypassSimpleHostnamesRule>());
}

bool ProxyHostMatchingRules::AddRuleFromString(std::string_view raw_untrimmed) {
  auto rule = ParseRule(raw_untrimmed);

  if (rule) {
    matcher_.AddAsLastRule(std::move(rule));
    return true;
  }

  return false;
}

void ProxyHostMatchingRules::AddRulesToSubtractImplicit() {
  matcher_.AddAsLastRule(std::make_unique<SubtractImplicitBypassesRule>());
}

std::string ProxyHostMatchingRules::GetRulesToSubtractImplicit() {
  ProxyHostMatchingRules rules;
  rules.AddRulesToSubtractImplicit();
  return rules.ToString();
}

std::string ProxyHostMatchingRules::ToString() const {
  return matcher_.ToString();
}

void ProxyHostMatchingRules::Clear() {
  matcher_.Clear();
}

bool ProxyHostMatchingRules::MatchesImplicitRules(const GURL& url) {
  // On Windows the implict rules are:
  //
  //     localhost
  //     loopback
  //     127.0.0.1
  //     [::1]
  //     169.254/16
  //     [FE80::]/10
  //
  // And on macOS they are:
  //
  //     localhost
  //     127.0.0.1/8
  //     [::1]
  //     169.254/16
  //
  // Our implicit rules are approximately:
  //
  //     localhost
  //     localhost.
  //     *.localhost
  //     loopback  [Windows only]
  //     loopback. [Windows only]
  //     [::1]
  //     127.0.0.1/8
  //     169.254/16
  //     [FE80::]/10
  //     0.0.0.0/32, [::]/128, [::ffff:0.0.0.0]/128
  return IsLocalhost(url) || IsIPv4MappedLoopback(url) || IsLinkLocalIP(url) ||
         (IsUnspecifiedIP(url) &&
          net::proxy_matching_rust::UnspecifiedEnabled())
#if BUILDFLAG(IS_WIN)
         // See http://crbug.com/904889
         || (url.host() == "loopback") || (url.host() == "loopback.")
#endif
      ;
}

}  // namespace net::proxy_matching_reference
