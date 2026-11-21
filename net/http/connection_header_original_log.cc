// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/connection_header_original_policy.h"

#include <string_view>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/http/auth_challenge_reference.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/log/net_log_with_source.h"

namespace net {

namespace {

bool ShouldRedactChallenge(auth_challenge_reference::Tokenizer* challenge) {
  // Ignore lines with commas, as they may contain lists of schemes, and
  // the information we want to hide is Base64 encoded, so has no commas.
  if (challenge->challenge_text().find(',') != std::string::npos)
    return false;

  const std::string& scheme = challenge->auth_scheme();
  // Invalid input.
  if (scheme.empty())
    return false;

  // Ignore Basic and Digest authentication challenges, as they contain
  // public information.
  if (scheme == kBasicAuthScheme || scheme == kDigestAuthScheme)
    return false;

  return true;
}

}  // namespace

std::string ConnectionOriginalElideHeaderValueForNetLog(NetLogCaptureMode capture_mode,
                                      std::string_view header,
                                      std::string_view value) {
  std::string_view redact;

  if (!NetLogCaptureIncludesSensitive(capture_mode)) {
    if (base::EqualsCaseInsensitiveASCII(header, "set-cookie") ||
        base::EqualsCaseInsensitiveASCII(header, "set-cookie2") ||
        base::EqualsCaseInsensitiveASCII(header, "cookie") ||
        base::EqualsCaseInsensitiveASCII(header, "authorization") ||
        base::EqualsCaseInsensitiveASCII(header, "proxy-authorization")) {
      redact = value;
    } else if (base::EqualsCaseInsensitiveASCII(header, "www-authenticate") ||
               base::EqualsCaseInsensitiveASCII(header, "proxy-authenticate")) {
      // Look for authentication information from data received from the server
      // in multi-round Negotiate authentication.
      auth_challenge_reference::Tokenizer challenge(value);
      if (ShouldRedactChallenge(&challenge)) {
        redact = challenge.params();
      }
    }
  }

  if (redact.empty()) {
    return std::string(value);
  }

  // Create string_views that contain the part of `value` before the `redact`
  // substring, and the value after it. Need to use the data() field of the two
  // string_views to figure out where `redact` appears within `value`.
  size_t redact_offset = redact.data() - value.data();
  std::string_view value_before_redact = value.substr(0, redact_offset);
  std::string_view value_after_redact =
      value.substr(redact_offset + redact.length());

  return base::StrCat({value_before_redact,
                       base::StringPrintf("[%ld bytes were stripped]",
                                          static_cast<long>(redact.length())),
                       value_after_redact});
}

void ConnectionOriginalNetLogRequestHeaders(const NetLogWithSource& net_log,
                          NetLogEventType type,
                          const std::string& request_line,
                          const HttpRequestHeaders* headers) {
  net_log.AddEvent(type, [&](NetLogCaptureMode capture_mode) {
    return ConnectionHeaderOriginalPolicy::NetLogParams(*headers, request_line, capture_mode);
  });
}

}  // namespace net
