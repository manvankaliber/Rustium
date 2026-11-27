// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_MIME_SNIFFER_REFERENCE_H_
#define NET_BASE_MIME_SNIFFER_REFERENCE_H_

#include "net/base/mime_sniffer.h"

// Frozen pre-rewrite implementation for differential tests and measurements.
namespace net::mime_sniffer_test {
bool ShouldSniffMimeType(const GURL& url,
                         const HttpResponseHeaders* http_response_headers,
                         std::string_view mime_type);
bool SniffMimeType(std::string_view content,
                   const GURL& url,
                   const std::string& type_hint,
                   ForceSniffFileUrlsForHtml force_html,
                   std::string* result);
bool SniffMimeTypeFromLocalData(std::string_view content, std::string* result);
bool LooksLikeBinary(std::string_view content);
}  // namespace net::mime_sniffer_test

#endif  // NET_BASE_MIME_SNIFFER_REFERENCE_H_
