// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/icu_util.h"
#include "net/dns/dns_names_test_support.h"

extern "C" int LLVMFuzzerInitialize(int*, char***) {
  CHECK(base::i18n::InitializeICU());
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  net::dns_names_test::Check(UNSAFE_BUFFERS(base::span(data, size)));
  return 0;
}
