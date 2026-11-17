// Frozen original helper declarations; source-only reference.
#ifndef COOKIE_FAMILY_ORIGINAL_URL_HELPERS_H_
#define COOKIE_FAMILY_ORIGINAL_URL_HELPERS_H_
#include <string>
#include <string_view>
#include "url/gurl.h"
#include "url/url_canon.h"
#include "base/time/time.h"
namespace net::cookie_family_original {
class CookieFamilyFrozenHttp {
 public:
  static bool IsMethodSafe(std::string_view);
  static bool IsControlChar(char);
  static std::string TimeFormatHTTP(base::Time);
};
std::string CanonicalizeHost(std::string_view,url::CanonHostInfo*);
bool IsCanonicalizedHostCompliant(std::string_view);
bool IsSubdomainOf(std::string_view subdomain,
                   std::string_view superdomain);
bool HostStringIsLocalhost(std::string_view);
bool IsLocalHostname(std::string_view);
bool IsLocalhost(const GURL&);
GURL ChangeWebSocketSchemeToHttpScheme(const GURL&);
bool IsStandardSchemeWithNetworkHost(std::string_view);
}
#endif
