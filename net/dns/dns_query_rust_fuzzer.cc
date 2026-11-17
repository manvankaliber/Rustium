// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/compiler_specific.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/dns/dns_query_rust_test_support.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // LibFuzzer guarantees initialized size-byte input; empty input may be null.
  const auto bytes = size == 0 ? base::span<const uint8_t>()
                               : UNSAFE_BUFFERS(base::span(data, size));
  base::test::ScopedFeatureList features;
  features.InitWithFeatureState(net::features::kUseStructuredDnsErrors,
                                !bytes.empty() && (bytes[0] & 1));
  net::dns_query_rust_test::State(bytes);
  net::dns_query_rust_test::OptCases(bytes);
  return 0;
}
