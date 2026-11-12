// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/auth_challenge_test_support.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  net::auth_challenge_test::Check(
      std::string_view(reinterpret_cast<const char*>(data), size));
  return 0;
}
