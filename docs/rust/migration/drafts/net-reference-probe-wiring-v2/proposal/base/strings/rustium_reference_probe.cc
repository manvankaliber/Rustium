#define RUSTIUM_REFERENCE_PROBE 1
#include "base/strings/rustium_reference_probe.h"
#if !defined(RUSTIUM_REFERENCE_PROBE)
#error This file belongs only in the isolated qualification build.
#endif
extern "C" uint64_t rustium_probe_strings_count();
extern "C" uint64_t rustium_probe_base64_count();
namespace base::rustium_reference_probe {
uint64_t StringsCount() { return rustium_probe_strings_count(); }
uint64_t Base64Count() { return rustium_probe_base64_count(); }
}
