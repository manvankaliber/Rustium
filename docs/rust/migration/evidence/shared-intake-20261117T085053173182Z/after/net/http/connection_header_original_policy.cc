// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/http/connection_header_original_policy.h"
#include <utility>
#include "base/check.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "net/http/connection_header_original_helpers.h"
#include "net/http/http_auth_controller.h"
#include "net/log/net_log_values_rust_reference.h"
#include "third_party/abseil-cpp/absl/container/inlined_vector.h"
namespace net {
bool ConnectionHeaderOriginalPolicy::HasHeader(const HttpRequestHeaders& headers,
                                              std::string_view key) {
  return FindHeader(headers, key) != headers.GetHeaderVector().end();
}
std::optional<std::string_view> ConnectionHeaderOriginalPolicy::GetHeaderView(const HttpRequestHeaders& self,
    std::string_view key) {
  auto it = FindHeader(self, key);
  if (it == self.headers_.end()) {
    return std::nullopt;
  }
  return std::string_view(it->value);
}

std::optional<std::string> ConnectionHeaderOriginalPolicy::GetHeader(const HttpRequestHeaders& self,
    std::string_view key) {
  auto it = FindHeader(self, key);
  if (it == self.headers_.end()) {
    return std::nullopt;
  }
  return it->value;
}

void ConnectionHeaderOriginalPolicy::Clear(HttpRequestHeaders& self) {
  self.headers_.clear();
}

void ConnectionHeaderOriginalPolicy::SetHeader(HttpRequestHeaders& self,
    std::string_view key,
                                               std::string_view value) {
  SetHeader(self, key, std::string(value));
}

void ConnectionHeaderOriginalPolicy::SetHeader(HttpRequestHeaders& self,
    std::string_view key,
                                               std::string&& value) {
  // Invalid header names or values could mean clients can attach
  // browser-internal headers.
  CHECK(connection_header_original_helpers::HttpUtil::IsValidHeaderName(
      key))
      << key;
  CHECK(
      connection_header_original_helpers::HttpUtil::IsValidHeaderValue(
          value))
      << key << " has invalid value.";

  SetHeaderInternal(self, key, std::move(value));
}

void ConnectionHeaderOriginalPolicy::SetHeaderWithoutCheckForTesting(HttpRequestHeaders& self,
    std::string_view key,
    std::string_view value) {
  SetHeaderInternal(self, key, std::string(value));
}

void ConnectionHeaderOriginalPolicy::SetHeaderIfMissing(HttpRequestHeaders& self,
    std::string_view key,
    std::string_view value) {
  // Invalid header names or values could mean clients can attach
  // browser-internal headers.
  CHECK(connection_header_original_helpers::HttpUtil::IsValidHeaderName(
      key));
  CHECK(
      connection_header_original_helpers::HttpUtil::IsValidHeaderValue(
          value));
  auto it = FindHeader(self, key);
  if (it == self.headers_.end()) {
    self.headers_.emplace_back(key, value);
  }
}

void ConnectionHeaderOriginalPolicy::RemoveHeader(HttpRequestHeaders& self,
    std::string_view key) {
  auto it = FindHeader(self, key);
  if (it != self.headers_.end()) {
    self.headers_.erase(it);
  }
}

void ConnectionHeaderOriginalPolicy::AddHeaderFromString(HttpRequestHeaders& self,
    std::string_view header_line) {
  DCHECK_EQ(std::string::npos, header_line.find("\r\n"))
      << "\"" << header_line << "\" contains CRLF.";

  const std::string::size_type key_end_index = header_line.find(":");
  if (key_end_index == std::string::npos) {
    LOG(DFATAL) << "\"" << header_line << "\" is missing colon delimiter.";
    return;
  }

  if (key_end_index == 0) {
    LOG(DFATAL) << "\"" << header_line << "\" is missing header key.";
    return;
  }

  const std::string_view header_key = header_line.substr(0, key_end_index);
  if (!connection_header_original_helpers::HttpUtil::IsValidHeaderName(
          header_key)) {
    LOG(DFATAL) << "\"" << header_line << "\" has invalid header key.";
    return;
  }

  const std::string::size_type value_index = key_end_index + 1;

  if (value_index < header_line.size()) {
    std::string_view header_value = header_line.substr(value_index);
    header_value =
        connection_header_original_helpers::HttpUtil::TrimLWS(
            header_value);
    if (!connection_header_original_helpers::HttpUtil::
            IsValidHeaderValue(header_value)) {
      LOG(DFATAL) << "\"" << header_line << "\" has invalid header value.";
      return;
    }
    SetHeader(self, header_key, header_value);
  } else if (value_index == header_line.size()) {
    SetHeader(self, header_key, "");
  } else {
    NOTREACHED();
  }
}

void ConnectionHeaderOriginalPolicy::AddHeadersFromString(HttpRequestHeaders& self,
    std::string_view headers) {
  for (std::string_view header :
       connection_header_original_helpers::Split(headers)) {
    AddHeaderFromString(self, header);
  }
}

void ConnectionHeaderOriginalPolicy::MergeFrom(HttpRequestHeaders& self,
    const HttpRequestHeaders& other) {
  for (const auto& header : other.GetHeaderVector()) {
    SetHeader(self, header.key, header.value);
  }
}

std::string ConnectionHeaderOriginalPolicy::ToString(const HttpRequestHeaders& self) {
  static constexpr std::string_view kColon = ": ";
  static constexpr std::string_view kCrNl = "\r\n";

  // As of January 2024, 99% of of HttpRequestHeaders objects had 27
  // headers or less. Allow space for 128 string pieces without heap allocation
  // as it is a nice round number.
  absl::InlinedVector<std::string_view, 128> pieces;
  const size_t expected_size = self.headers_.size() * 4 + 1;

  pieces.reserve(expected_size);
  for (const auto& header : self.headers_) {
    pieces.insert(pieces.end(), {header.key, kColon, header.value, kCrNl});
  }
  pieces.push_back(kCrNl);
  CHECK_EQ(pieces.size(), expected_size);
  return base::StrCat(pieces);
}

base::DictValue ConnectionHeaderOriginalPolicy::NetLogParams(const HttpRequestHeaders& self,
    const std::string& request_line,
    NetLogCaptureMode capture_mode) {
  base::DictValue dict;
  dict.Set("line", net_log_values_rust_reference::NetLogStringValue(request_line));
  base::ListValue headers;
  for (const auto& header : self.headers_) {
    std::string log_value =
        ConnectionOriginalElideHeaderValueForNetLog(capture_mode, header.key, header.value);
    headers.Append(
        net_log_values_rust_reference::NetLogStringValue(base::StrCat({header.key, ": ", log_value})));
  }
  dict.Set("headers", std::move(headers));
  return dict;
}

HttpRequestHeaders::HeaderVector::iterator
ConnectionHeaderOriginalPolicy::FindHeader(HttpRequestHeaders& self,
    std::string_view key) {
  for (auto it = self.headers_.begin(); it != self.headers_.end(); ++it) {
    if (connection_header_original_helpers::EqualsCaseInsensitiveASCII(
            key, it->key)) {
      return it;
    }
  }

  return self.headers_.end();
}

HttpRequestHeaders::HeaderVector::const_iterator
ConnectionHeaderOriginalPolicy::FindHeader(const HttpRequestHeaders& self,
    std::string_view key) {
  for (auto it = self.headers_.begin(); it != self.headers_.end(); ++it) {
    if (connection_header_original_helpers::EqualsCaseInsensitiveASCII(
            key, it->key)) {
      return it;
    }
  }

  return self.headers_.end();
}

void ConnectionHeaderOriginalPolicy::SetHeaderInternal(HttpRequestHeaders& self,
    std::string_view key,
                                                       std::string&& value) {
  auto it = FindHeader(self, key);
  if (it != self.headers_.end()) {
    it->value = std::move(value);
  } else {
    self.headers_.emplace_back(key, std::move(value));
  }
}

void ConnectionHeaderOriginalPolicy::AddAuthorizationHeader(
    HttpAuthController& auth, HttpRequestHeaders* authorization_headers) {
  DCHECK_CALLED_ON_VALID_THREAD(auth.thread_checker_);
  DCHECK(auth.HaveAuth());
  // auth.auth_token_ can be empty if we encountered a permanent error with
  // the auth scheme and want to retry.
  if (!auth.auth_token_.empty()) {
    SetHeader(*authorization_headers, 
        HttpAuth::GetAuthorizationHeaderName(auth.target_), auth.auth_token_);
    auth.auth_token_.clear();
  }
}
}  // namespace net
