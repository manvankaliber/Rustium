// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/load_flags_to_string.h"

#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "net/base/net_diagnostics_debug_formatting_ffi.h"

namespace net {
namespace {

struct RustBufferDeleter {
  size_t length;
  void operator()(uint8_t* bytes) const {
    net_diagnostics_load_flags_free(bytes, length);
  }
};

}  // namespace

std::string LoadFlagsToString(int load_flags) {
  size_t length = 0;
  uint8_t* raw_bytes = net_diagnostics_load_flags_format(load_flags, &length);
  CHECK(raw_bytes) << "Invalid load flags: " << load_flags;
  std::unique_ptr<uint8_t, RustBufferDeleter> bytes(raw_bytes,
                                                    RustBufferDeleter{length});
  // SAFETY: `bytes` uniquely owns the allocation returned with this exact
  // length and remains alive through the copying constructor. RustBufferDeleter
  // frees it with the matching Rust allocator after the copy completes.
  return UNSAFE_BUFFERS(
      std::string(reinterpret_cast<const char*>(bytes.get()), length));
}

}  // namespace net
