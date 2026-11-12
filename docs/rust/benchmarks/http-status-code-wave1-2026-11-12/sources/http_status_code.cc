// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_status_code.h"
#include "net/http/http_status_code_ffi.h"
namespace net {
std::string_view GetHttpReasonPhrase(HttpStatusCode code,
                                    std::string_view default_value) {
  const auto reason = rustium_http_status_reason(static_cast<int>(code));
  if (!reason.data) {
    return default_value;
  }
  return {reinterpret_cast<const char*>(reason.data), reason.len};
}
const std::optional<HttpStatusCode> TryToGetHttpStatusCode(int response_code) {
  if (!rustium_http_status_known(response_code)) {
    return std::nullopt;
  }
  return static_cast<HttpStatusCode>(response_code);
}
}  // namespace net
