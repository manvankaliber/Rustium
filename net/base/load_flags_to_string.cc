// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/load_flags_to_string.h"

#include <limits>
#include <string>

#include "base/check_op.h"
#include "net/base/net_diagnostics_debug_formatting_ffi.h"

namespace net {
std::string LoadFlagsToString(int load_flags) {
  const size_t length = net_diagnostics_load_flags_length(load_flags);
  CHECK_NE(length, std::numeric_limits<size_t>::max())
      << "Invalid load flags: " << load_flags;
  std::string result(length, '\0');
  // Rust composes the text directly into initialized C++-owned string bytes.
  // The string and its allocation stay live and unique throughout the call.
  CHECK(net_diagnostics_load_flags_write(
      load_flags, reinterpret_cast<uint8_t*>(result.data()), result.size()));
  return result;
}

}  // namespace net
