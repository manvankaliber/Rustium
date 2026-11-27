// Exact helper bodies extracted from frozen url_util.cc.
#include "net/cookies/cookie_family_original/url_helpers.h"
#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include <array>
#include "net/base/ip_address.h"
#include "url/url_canon_stdstring.h"
#include "url/url_canon_internal.h"
#include "url/url_canon_ip.h"
#include "url/url_constants.h"
#include "url/url_util.h"
namespace net::cookie_family_original {
namespace {
bool IsHostCharAlphanumeric(char c) {
  // We can just check lowercase because uppercase characters have already been
  // normalized.
  return ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9'));
}

bool IsNormalizedLocalhostTLD(std::string_view host) {
  return base::EndsWith(host, ".localhost",
                        base::CompareCase::INSENSITIVE_ASCII);
}

std::string CanonicalizeHost(std::string_view host,
                             bool is_file_scheme,
                             url::CanonHostInfo* host_info) {
  // Try to canonicalize the host.
  const url::Component raw_host_component(0, static_cast<int>(host.length()));
  std::string canon_host;
  url::StdStringCanonOutput canon_host_output(&canon_host);
  // A url::StdStringCanonOutput starts off with a zero length buffer. The
  // first time through Grow() immediately resizes it to 32 bytes, incurring
  // a malloc. With libcxx a 22 byte or smaller request can be accommodated
  // within the std::string itself (i.e. no malloc occurs). Start the buffer
  // off at the max size to avoid a malloc on short strings.
  // NOTE: To ensure the final size is correctly reflected, it's necessary
  // to call Complete() which will adjust the size to the actual bytes written.
  // This is handled below for success cases, while failure cases discard all
  // the output.
  const int kCxxMaxStringBufferSizeWithoutMalloc = 22;
  canon_host_output.Resize(kCxxMaxStringBufferSizeWithoutMalloc);
  if (is_file_scheme) {
    url::CanonicalizeFileHostVerbose(host, raw_host_component,
                                     canon_host_output, *host_info);
  } else {
    url::CanonicalizeSpecialHostVerbose(host, raw_host_component,
                                        canon_host_output, *host_info);
  }

  if (host_info->out_host.is_nonempty() &&
      host_info->family != url::CanonHostInfo::BROKEN) {
    // Success!  Assert that there's no extra garbage.
    canon_host_output.Complete();
    DCHECK_EQ(host_info->out_host.len, static_cast<int>(canon_host.length()));
  } else {
    // Empty host, or canonicalization failed.  We'll return empty.
    canon_host.clear();
  }

  return canon_host;
}
}
std::string CanonicalizeHost(std::string_view host,
                             url::CanonHostInfo* host_info) {
  return CanonicalizeHost(host, /*is_file_scheme=*/false, host_info);
}

bool IsSubdomainOf(std::string_view subdomain, std::string_view superdomain) {
  // Subdomain must be identical or have strictly more labels than the
  // superdomain.
  if (subdomain.length() <= superdomain.length()) {
    return subdomain == superdomain;
  }

  // Superdomain must be suffix of subdomain, and the last character not
  // included in the matching substring must be a dot.
  if (!subdomain.ends_with(superdomain)) {
    return false;
  }
  subdomain.remove_suffix(superdomain.length());
  return subdomain.back() == '.';
}

bool IsCanonicalizedHostCompliant(std::string_view host) {
  if (host.empty() || host.size() > 254 ||
      (host.back() != '.' && host.size() == 254)) {
    return false;
  }

  bool in_component = false;
  bool most_recent_component_started_alphanumeric = false;
  size_t label_size = 0;

  for (char c : host) {
    ++label_size;
    if (!in_component) {
      most_recent_component_started_alphanumeric = IsHostCharAlphanumeric(c);
      if (!most_recent_component_started_alphanumeric && (c != '-') &&
          (c != '_')) {
        return false;
      }
      in_component = true;
    } else if (c == '.') {
      in_component = false;
      if (label_size > 64 || label_size == 1) {
        // Label should not be empty or longer than 63 characters (+1 for '.'
        // character included in `label_size`).
        return false;
      } else {
        label_size = 0;
      }
    } else if (!IsHostCharAlphanumeric(c) && (c != '-') && (c != '_')) {
      return false;
    }
  }

  // Check for too-long label when not ended with final '.'.
  if (label_size > 63) {
    return false;
  }

  return most_recent_component_started_alphanumeric;
}

bool HostStringIsLocalhost(std::string_view host) {
  IPAddress ip_address;
  if (ip_address.AssignFromIPLiteral(host)) {
    return ip_address.IsLoopback();
  }
  return IsLocalHostname(host);
}

bool IsLocalHostname(std::string_view host) {
  // Remove any trailing '.'.
  if (!host.empty() && *host.rbegin() == '.') {
    host.remove_suffix(1);
  }

  return base::EqualsCaseInsensitiveASCII(host, "localhost") ||
         IsNormalizedLocalhostTLD(host);
}

bool IsLocalhost(const GURL& url) {
  return HostStringIsLocalhost(url.HostNoBracketsPiece());
}

GURL ChangeWebSocketSchemeToHttpScheme(const GURL& url) {
  DCHECK(url.SchemeIsWSOrWSS());
  GURL::Replacements replace_scheme;
  replace_scheme.SetSchemeStr(url.SchemeIs(url::kWssScheme) ? url::kHttpsScheme
                                                            : url::kHttpScheme);
  return url.ReplaceComponents(replace_scheme);
}

bool IsStandardSchemeWithNetworkHost(std::string_view scheme) {
  // file scheme is special. Windows file share origins can have network hosts.
  if (scheme == url::kFileScheme) {
    return true;
  }

  url::SchemeType scheme_type;
  if (!url::GetStandardSchemeType(scheme, &scheme_type)) {
    return false;
  }
  return scheme_type == url::SCHEME_WITH_HOST_PORT_AND_USER_INFORMATION ||
         scheme_type == url::SCHEME_WITH_HOST_AND_PORT;
}
bool CookieFamilyFrozenHttp::IsMethodSafe(std::string_view method) {
  return method == "GET" || method == "HEAD" || method == "OPTIONS" ||
         method == "TRACE";
}
std::string CookieFamilyFrozenHttp::TimeFormatHTTP(base::Time time) {
  static constexpr std::array<char[4], 7> kWeekdayName = {
      "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static constexpr std::array<char[4], 12> kMonthName = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);
  return base::StringPrintf(
      "%s, %02d %s %04d %02d:%02d:%02d GMT", kWeekdayName[exploded.day_of_week],
      exploded.day_of_month, kMonthName[exploded.month - 1], exploded.year,
      exploded.hour, exploded.minute, exploded.second);
}
bool CookieFamilyFrozenHttp::IsControlChar(char c) {
    return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
  }
}
