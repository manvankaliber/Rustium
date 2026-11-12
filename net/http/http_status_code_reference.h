#ifndef NET_HTTP_HTTP_STATUS_CODE_REFERENCE_H_
#define NET_HTTP_HTTP_STATUS_CODE_REFERENCE_H_
#include "net/http/http_status_code.h"
namespace net::http_status_code_reference {
std::string_view GetHttpReasonPhrase(HttpStatusCode code,
                                     std::string_view fallback);
const std::optional<HttpStatusCode> TryToGetHttpStatusCode(int code);
}  // namespace net::http_status_code_reference
#endif
