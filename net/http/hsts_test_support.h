// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HSTS_TEST_SUPPORT_H_
#define NET_HTTP_HSTS_TEST_SUPPORT_H_

#include <string_view>

#include "base/time/time.h"

namespace net::hsts_internal::test {

// Frozen pre-rewrite implementations. The header baseline uses this C++ numeric
// parser even when production ParseUint32 has independently migrated to Rust.
bool LegacyParseHSTSHeader(std::string_view value,
                           base::TimeDelta* max_age,
                           bool* include_subdomains);

}  // namespace net::hsts_internal::test

#endif  // NET_HTTP_HSTS_TEST_SUPPORT_H_
