// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/i18n/icu_util.h"
#include "base/logging.h"
#include "net/base/host_mapping_rust_ffi.rs.h"
#include "net/base/host_mapping_rust_test_support.h"
extern "C" int LLVMFuzzerInitialize(int*, char***) {
  CHECK(base::i18n::InitializeICU());
  return 0;
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static const bool initialized = [] {
    logging::SetMinLogLevel(logging::LOGGING_FATAL);
    return true;
  }();
  (void)initialized;
  // SAFETY: libFuzzer supplies size initialized bytes for this synchronous
  // call.
  auto view = size ? UNSAFE_BUFFERS(std::string_view(
                         reinterpret_cast<const char*>(data), size))
                   : std::string_view();
  net::host_mapping_rust_test::State(view);
  auto lhs = view.substr(0, size / 2), rhs = view.substr(size / 2);
  auto bytes = [](std::string_view s) {
    return s.empty()
               ? rust::Slice<const uint8_t>()
               : UNSAFE_BUFFERS(rust::Slice<const uint8_t>(
                     reinterpret_cast<const uint8_t*>(s.data()), s.size()));
  };
  CHECK_EQ(net::HostMappingRustOriginalMatchPattern(lhs, rhs),
           net::host_mapping_rust::match_pattern(bytes(lhs), bytes(rhs)));
  return 0;
}
