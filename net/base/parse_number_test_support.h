// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_PARSE_NUMBER_TEST_SUPPORT_H_
#define NET_BASE_PARSE_NUMBER_TEST_SUPPORT_H_

#include "net/base/parse_number.h"

// Frozen pre-migration C++ implementation. Tests/benchmarks only.
namespace net::parse_number_internal::test {
bool ParseInt32(std::string_view, ParseIntFormat, int32_t*, ParseIntError*);
bool ParseInt64(std::string_view, ParseIntFormat, int64_t*, ParseIntError*);
bool ParseUint32(std::string_view, ParseIntFormat, uint32_t*, ParseIntError*);
bool ParseUint64(std::string_view, ParseIntFormat, uint64_t*, ParseIntError*);
}  // namespace net::parse_number_internal::test

#endif  // NET_BASE_PARSE_NUMBER_TEST_SUPPORT_H_
