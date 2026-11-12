// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <string_view>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/strings/string_view_util.h"
#include "net/http/hsts_test_support.h"
#include "net/http/http_security_headers.h"

namespace {
void Compare(std::string_view input) {
  for (bool initial : {false, true}) {
    auto actual_age = base::Seconds(-12345);
    auto expected_age = actual_age;
    bool actual_subdomains = initial;
    bool expected_subdomains = initial;
    const bool expected = net::hsts_internal::test::LegacyParseHSTSHeader(
        input, &expected_age, &expected_subdomains);
    CHECK_EQ(net::ParseHSTSHeader(input, &actual_age, &actual_subdomains),
             expected);
    CHECK_EQ(actual_age, expected_age);
    CHECK_EQ(actual_subdomains, expected_subdomains);
  }
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies a live byte buffer with exactly size bytes.
  const auto input = UNSAFE_BUFFERS(base::span(data, size));
  const std::string_view value = base::as_string_view(input);
  Compare(value);
  Compare("max-age=1;" + std::string(value));
  Compare("max-age=" + std::string(value));
  Compare("max-age=\"" + std::string(value) + "\"");
  return 0;
}
