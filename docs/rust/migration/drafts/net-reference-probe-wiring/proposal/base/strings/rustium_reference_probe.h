// Qualification-only bridge; compile and export only in the isolated probe build.
#ifndef BASE_STRINGS_RUSTIUM_REFERENCE_PROBE_H_
#define BASE_STRINGS_RUSTIUM_REFERENCE_PROBE_H_
#include <stdint.h>
#include "base/base_export.h"
namespace base::rustium_reference_probe {
BASE_EXPORT uint64_t StringsCount();
BASE_EXPORT uint64_t Base64Count();
}
#endif
