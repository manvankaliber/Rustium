// Synchronous snapshot API. The caller owns the process-wide quiet window.
#ifndef NET_HTTP_RUSTIUM_REFERENCE_PROBE_TEST_SUPPORT_H_
#define NET_HTTP_RUSTIUM_REFERENCE_PROBE_TEST_SUPPORT_H_
#include <array>
#include <cstddef>
#include <stdint.h>
#include "base/strings/rustium_reference_probe.h"
#include "net/http/rustium_reference_probe.h"
namespace net::rustium_reference_probe {
using Snapshot = std::array<uint64_t, 7>;
inline Snapshot Read() {
  return {AuthCount(), AuthCacheCount(), RequestHeadersCount(), NetErrorCount(),
          NetLogCount(), base::rustium_reference_probe::StringsCount(),
          base::rustium_reference_probe::Base64Count()};
}
// Read only after callbacks, background provider work, and NetLog callbacks
// have settled. Do not reset counters and do not run competing test cases.
inline Snapshot Delta(const Snapshot& before, const Snapshot& after) {
  Snapshot result{};
  for (std::size_t i = 0; i < result.size(); ++i) result[i] = after[i] - before[i];
  return result;
}
}  // namespace net::rustium_reference_probe
#endif
