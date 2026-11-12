// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/mime_sniffer.h"

#include <limits>
#include <optional>

#include "base/check.h"
#include "base/check_op.h"
#include "net/base/mime_sniffer_ffi.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace net {

bool ShouldSniffMimeType(const GURL& url,
                         const HttpResponseHeaders* http_response_headers,
                         std::string_view mime_type) {
  // Classify Chromium-owned URL schemes and conditionally extract the first
  // header value. Rust applies MIME-type and nosniff rules to that metadata.
  const auto scheme_view = url.scheme();
  const uint8_t scheme = url.is_empty() || scheme_view == "http" ||
                         scheme_view == "https" || scheme_view == "file" ||
                         scheme_view == "filesystem";
  std::optional<std::string_view> first_options;
  if (scheme && http_response_headers) {
    first_options = http_response_headers->EnumerateHeader(
        /*iter=*/nullptr, "x-content-type-options");
  }
  const auto options = first_options.value_or("");
  return mime_sniffer_internal::rust_mime_should_sniff(
      reinterpret_cast<const uint8_t*>(mime_type.data()), mime_type.size(),
      scheme, reinterpret_cast<const uint8_t*>(options.data()), options.size());
}

bool SniffMimeType(std::string_view content,
                   const GURL& url,
                   const std::string& type_hint,
                   ForceSniffFileUrlsForHtml force_html,
                   std::string* result) {
  DCHECK_LT(content.length(), 1000000U);
  DCHECK(result);
  size_t content_offset = std::numeric_limits<size_t>::max();
  if (!content.empty()) {
    const auto input_address = reinterpret_cast<uintptr_t>(content.data());
    const auto output_address = reinterpret_cast<uintptr_t>(result->data());
    if (input_address >= output_address &&
        input_address - output_address <= result->size()) {
      content_offset = input_address - output_address;
    }
  }
  const auto path = url.path();
  const auto parsed = mime_sniffer_internal::rust_mime_sniff(
      reinterpret_cast<const uint8_t*>(content.data()), content.size(),
      reinterpret_cast<const uint8_t*>(path.data()), path.size(),
      reinterpret_cast<const uint8_t*>(type_hint.data()), type_hint.size(),
      url.scheme() == "file", force_html == ForceSniffFileUrlsForHtml::kEnabled,
      &type_hint == result, content_offset);
  if (parsed.flags & 1) {
    result->assign(type_hint);
  } else {
    result->assign(reinterpret_cast<const char*>(parsed.mime_data),
                   parsed.mime_size);
  }
  return (parsed.flags & 2) != 0;
}

bool SniffMimeTypeFromLocalData(std::string_view content, std::string* result) {
  if (content.empty()) {
    return false;
  }
  const auto parsed = mime_sniffer_internal::rust_mime_sniff_local(
      reinterpret_cast<const uint8_t*>(content.data()), content.size());
  if (parsed.flags & 2) {
    result->assign(reinterpret_cast<const char*>(parsed.mime_data),
                   parsed.mime_size);
  }
  return (parsed.flags & 2) != 0;
}

bool LooksLikeBinary(std::string_view content) {
  if (content.empty()) {
    return false;
  }
  return mime_sniffer_internal::rust_mime_looks_binary(
      reinterpret_cast<const uint8_t*>(content.data()), content.size());
}

}  // namespace net
