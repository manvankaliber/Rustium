// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_security_headers.h"

#include "base/containers/span_rust.h"
#include "net/http/hsts_ffi.rs.h"

namespace net {

bool ParseHSTSHeader(std::string_view value,
                     base::TimeDelta* max_age,
                     bool* include_subdomains) {
  const uint64_t policy = hsts_internal::parse_hsts(
      base::SpanToRustSlice(base::as_byte_span(value)), kMaxHSTSAgeSecs);
  if ((policy & 1) == 0) {
    return false;
  }
  // The bridge returns only scalar metadata and borrows the input for this
  // call. Preserve both caller outputs on every invalid-header path.
  *max_age = base::Seconds(policy >> 2);
  *include_subdomains = (policy & 2) != 0;
  return true;
}

}  // namespace net
