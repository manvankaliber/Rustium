// Copyright 2026 The Chromium Authors
#ifndef NET_BASE_NET_DIAGNOSTICS_LOAD_FLAGS_REFERENCE_H_
#define NET_BASE_NET_DIAGNOSTICS_LOAD_FLAGS_REFERENCE_H_
#include <string>

#include "base/component_export.h"
namespace net::net_diagnostics_load_flags_reference {
COMPONENT_EXPORT(NET_DIAGNOSTICS_REFERENCE)
std::string LoadFlagsToString(int);
}  // namespace net::net_diagnostics_load_flags_reference
#endif
