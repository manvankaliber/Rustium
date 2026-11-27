// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_status_code.h"

#include "base/compiler_specific.h"
#include "net/http/http_status_code_ffi.h"
namespace net {
std::string_view GetHttpReasonPhrase(HttpStatusCode code,
                                     std::string_view default_value) {
  const uint8_t* reason =
      rustium_http_status_reason_record(static_cast<int>(code));
  if (!reason) {
    return default_value;
  }
  // SAFETY: A non-null Rust record has one length byte followed by exactly that
  // many immutable static phrase bytes. The generator checks the length bound.
  return UNSAFE_BUFFERS(
      std::string_view(reinterpret_cast<const char*>(reason + 1), reason[0]));
}
const std::optional<HttpStatusCode> TryToGetHttpStatusCode(int response_code) {
  const int32_t parsed = rustium_http_status_parse(response_code);
  if (parsed < 0) {
    return std::nullopt;
  }
  return static_cast<HttpStatusCode>(parsed);
}
}  // namespace net
