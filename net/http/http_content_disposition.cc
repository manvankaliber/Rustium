// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_content_disposition.h"

#include "base/compiler_specific.h"
#include "net/http/content_disposition_ffi.rs.h"
#include "net/http/http_response_headers.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace net {

HttpContentDisposition::HttpContentDisposition(
    const HttpResponseHeaders& headers,
    const std::string& referrer_charset) {
  if (auto header = headers.EnumerateHeader(nullptr, "Content-Disposition")) {
    Parse(*header, referrer_charset);
  }
}

HttpContentDisposition::HttpContentDisposition(
    std::string_view header,
    const std::string& referrer_charset) {
  Parse(header, referrer_charset, true);
}

HttpContentDisposition::~HttpContentDisposition() = default;

void HttpContentDisposition::Parse(std::string_view header,
                                   const std::string& referrer_charset,
                                   bool first_value) {
  auto result = content_disposition_internal::parse_disposition(
      rust::Slice<const uint8_t>(
          reinterpret_cast<const uint8_t*>(header.data()), header.size()),
      rust::Slice<const uint8_t>(
          reinterpret_cast<const uint8_t*>(referrer_charset.data()),
          referrer_charset.size()),
      first_value);
  type_ = result.attachment ? ATTACHMENT : INLINE;
  parse_result_flags_ = static_cast<int>(result.flags);
  if (result.filename.empty()) {
    filename_.clear();
  } else {
    // SAFETY: The owned Rust vector remains live throughout this copy.
    UNSAFE_BUFFERS(
        filename_.assign(reinterpret_cast<const char*>(result.filename.data()),
                         result.filename.size()));
  }
}

}  // namespace net
