#ifndef NET_HTTP_HTTP_STATUS_CODE_DIFFERENTIAL_H_
#define NET_HTTP_HTTP_STATUS_CODE_DIFFERENTIAL_H_
#include <cstdlib>
#include <limits>

#include "net/http/http_status_code_reference.h"
namespace net::http_status_code_test {
inline void Check(int code) {
  const char bytes[] = {'a', '\0', 'z'};
  for (auto fallback : {std::string_view(), std::string_view(bytes, 0),
                        std::string_view(bytes, 3)}) {
    auto before = http_status_code_reference::GetHttpReasonPhrase(
        static_cast<HttpStatusCode>(code), fallback);
    auto after = GetHttpReasonPhrase(code, fallback);
    if (before != after) {
      std::abort();
    }
    if (!http_status_code_reference::TryToGetHttpStatusCode(code) &&
        (after.data() != fallback.data() || after.size() != fallback.size())) {
      std::abort();
    }
    if (http_status_code_reference::TryToGetHttpStatusCode(code) !=
        TryToGetHttpStatusCode(code)) {
      std::abort();
    }
  }
}
inline void Exhaustive() {
#define HTTP_STATUS_ENUM_VALUE(label, code, reason) Check(code);
#include "net/http/http_status_code_reference_list.h"
#undef HTTP_STATUS_ENUM_VALUE
  for (int code = -65536; code <= 65536; ++code) {
    Check(code);
  }
  Check(std::numeric_limits<int>::min());
  Check(std::numeric_limits<int>::max());
}
}  // namespace net::http_status_code_test
#endif
