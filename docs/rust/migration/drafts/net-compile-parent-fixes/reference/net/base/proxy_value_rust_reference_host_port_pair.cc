#include "net/base/proxy_value_rust_reference_host_port_pair.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "url/url_canon_ip.h"
#include "url/third_party/mozilla/url_parse.h"
namespace net::proxy_value_rust_reference {
bool ParseHostAndPort(std::string_view input, std::string* host, int* port) {
  if (input.empty())
    return false;

  url::Component auth_component(0, input.size());
  url::Component username_component;
  url::Component password_component;
  url::Component hostname_component;
  url::Component port_component;

  url::ParseAuthority(input, auth_component, url::ParserMode::kSpecialURL,
                      &username_component, &password_component,
                      &hostname_component, &port_component);

  // There shouldn't be a username/password.
  if (username_component.is_valid() || password_component.is_valid())
    return false;

  if (hostname_component.is_empty())
    return false;  // Failed parsing.

  int parsed_port_number = -1;
  if (port_component.is_nonempty()) {
    parsed_port_number = url::ParsePort(input, port_component);

    // If parsing failed, port_number will be either PORT_INVALID or
    // PORT_UNSPECIFIED, both of which are negative.
    if (parsed_port_number < 0)
      return false;  // Failed parsing the port number.
  }

  if (port_component.len == 0)
    return false;  // Reject inputs like "foo:"

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
HostPortPair::HostPortPair() : port_(0) {}

HostPortPair::HostPortPair(std::string_view in_host, uint16_t in_port)
    : port_(in_port), host_(in_host) {}

HostPortPair::HostPortPair(const char* in_host, uint16_t in_port)
    : HostPortPair(std::string_view(in_host), in_port) {}

HostPortPair::HostPortPair(std::string&& in_host, uint16_t in_port)
    : port_(in_port), host_(std::move(in_host)) {}

HostPortPair HostPortPair::FromString(std::string_view str) {
  // Input with more than one ':' is ambiguous unless it contains an IPv6
  // literal (signified by starting with a '['). ParseHostAndPort() allows such
  // input and always uses the last ':' as the host/port delimiter, but because
  // HostPortPair often deals with IPv6 literals without brackets, disallow such
  // input here to prevent a common error.
  if (base::SplitStringPiece(str, ":", base::KEEP_WHITESPACE,
                             base::SPLIT_WANT_ALL)
              .size() > 2 &&
      str.front() != '[') {
    return HostPortPair();
  }

  std::string host;
  int port;
  if (!ParseHostAndPort(str, &host, &port))
    return HostPortPair();

  // Require a valid port.
  if (port == -1)
    return HostPortPair();
  DCHECK(base::IsValueInRangeForNumericType<uint16_t>(port));

  return HostPortPair(host, port);
}

std::string HostPortPair::ToString() const {
  std::string ret(HostForURL());
  ret += ':';
  ret += base::NumberToString(port_);
  return ret;
}

std::string HostPortPair::HostForURL() const {
  // TODO(rtenneti): Add support for |host| to have '\0'.
  if (host_.find('\0') != std::string::npos) {
    std::string host_for_log(host_);
    size_t nullpos;
    while ((nullpos = host_for_log.find('\0')) != std::string::npos) {
      host_for_log.replace(nullpos, 1, "%00");
    }
    LOG(DFATAL) << "Host has a null char: " << host_for_log;
  }
  // Check to see if the host is an IPv6 address.  If so, added brackets.
  if (host_.find(':') != std::string::npos) {
    DCHECK_NE(host_[0], '[');
    return base::StringPrintf("[%s]", host_.c_str());
  }

  return host_;
}


}
