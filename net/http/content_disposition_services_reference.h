// Frozen legacy Content-Disposition services; never linked to migrated helpers.
#ifndef NET_HTTP_CONTENT_DISPOSITION_SERVICES_REFERENCE_H_
#define NET_HTTP_CONTENT_DISPOSITION_SERVICES_REFERENCE_H_
#include <string>
#include <string_view>

#include "base/base64.h"
#include "base/strings/escape.h"
namespace net::content_disposition_reference {
bool Base64Decode(
    std::string_view input,
    std::string* output,
    base::Base64DecodePolicy policy = base::Base64DecodePolicy::kStrict);
std::string UnescapeBinaryURLComponent(std::string_view input,
                                       base::UnescapeRule::Type rules);
bool ConvertToUtf8(std::string_view input,
                   const char* charset,
                   std::string* output);
bool ConvertToUTF16(std::string_view input,
                    const char* charset,
                    std::u16string* output);
bool ConvertToUtf8AndNormalize(std::string_view input,
                               const char* charset,
                               std::string* output);
}  // namespace net::content_disposition_reference
#endif
