// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "http_request_headers_rust_original.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "http_request_headers_rust_original_helpers.h"
#include "net/base/url_util.h"
#include "net/http/http_log_util.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_values.h"
#include "third_party/abseil-cpp/absl/container/inlined_vector.h"
#include "url/gurl.h"

namespace net {

namespace {

constexpr char kEncodingGzip[] = "gzip";
constexpr char kEncodingDeflate[] = "deflate";
constexpr char kEncodingBrotli[] = "br";
constexpr char kEncodingZstd[] = "zstd";

bool SupportsStreamType(const std::optional<base::flat_set<SourceStreamType>>&
                            accepted_stream_types,
                        SourceStreamType type) {
  if (!accepted_stream_types) {
    return true;
  }
  return accepted_stream_types->contains(type);
}

}  // namespace

HttpRequestHeadersRustOriginal::HeaderKeyValuePair::HeaderKeyValuePair() =
    default;

HttpRequestHeadersRustOriginal::HeaderKeyValuePair::HeaderKeyValuePair(
    std::string_view key,
    std::string_view value)
    : HeaderKeyValuePair(key, std::string(value)) {}

HttpRequestHeadersRustOriginal::HeaderKeyValuePair::HeaderKeyValuePair(
    std::string_view key,
    std::string&& value)
    : key(key), value(std::move(value)) {}

HttpRequestHeadersRustOriginal::Iterator::Iterator(
    const HttpRequestHeadersRustOriginal& headers)
    : curr_(headers.headers_.begin()), end_(headers.headers_.end()) {}

HttpRequestHeadersRustOriginal::Iterator::~Iterator() = default;

bool HttpRequestHeadersRustOriginal::Iterator::GetNext() {
  if (!started_) {
    started_ = true;
    return curr_ != end_;
  }

  if (curr_ == end_) {
    return false;
  }

  ++curr_;
  return curr_ != end_;
}

HttpRequestHeadersRustOriginal::HttpRequestHeadersRustOriginal() = default;
HttpRequestHeadersRustOriginal::HttpRequestHeadersRustOriginal(
    const HttpRequestHeadersRustOriginal& other) = default;
HttpRequestHeadersRustOriginal::HttpRequestHeadersRustOriginal(
    HttpRequestHeadersRustOriginal&& other) = default;
HttpRequestHeadersRustOriginal::~HttpRequestHeadersRustOriginal() = default;

HttpRequestHeadersRustOriginal& HttpRequestHeadersRustOriginal::operator=(
    const HttpRequestHeadersRustOriginal& other) = default;
HttpRequestHeadersRustOriginal& HttpRequestHeadersRustOriginal::operator=(
    HttpRequestHeadersRustOriginal&& other) = default;

std::optional<std::string_view> HttpRequestHeadersRustOriginal::GetHeaderView(
    std::string_view key) const {
  auto it = FindHeader(key);
  if (it == headers_.end()) {
    return std::nullopt;
  }
  return std::string_view(it->value);
}

std::optional<std::string> HttpRequestHeadersRustOriginal::GetHeader(
    std::string_view key) const {
  auto it = FindHeader(key);
  if (it == headers_.end()) {
    return std::nullopt;
  }
  return it->value;
}

void HttpRequestHeadersRustOriginal::Clear() {
  headers_.clear();
}

void HttpRequestHeadersRustOriginal::SetHeader(std::string_view key,
                                               std::string_view value) {
  SetHeader(key, std::string(value));
}

void HttpRequestHeadersRustOriginal::SetHeader(std::string_view key,
                                               std::string&& value) {
  // Invalid header names or values could mean clients can attach
  // browser-internal headers.
  CHECK(http_request_headers_rust_original_helpers::HttpUtil::IsValidHeaderName(
      key))
      << key;
  CHECK(
      http_request_headers_rust_original_helpers::HttpUtil::IsValidHeaderValue(
          value))
      << key << " has invalid value.";

  SetHeaderInternal(key, std::move(value));
}

void HttpRequestHeadersRustOriginal::SetHeaderWithoutCheckForTesting(
    std::string_view key,
    std::string_view value) {
  SetHeaderInternal(key, std::string(value));
}

void HttpRequestHeadersRustOriginal::SetHeaderIfMissing(
    std::string_view key,
    std::string_view value) {
  // Invalid header names or values could mean clients can attach
  // browser-internal headers.
  CHECK(http_request_headers_rust_original_helpers::HttpUtil::IsValidHeaderName(
      key));
  CHECK(
      http_request_headers_rust_original_helpers::HttpUtil::IsValidHeaderValue(
          value));
  auto it = FindHeader(key);
  if (it == headers_.end()) {
    headers_.emplace_back(key, value);
  }
}

void HttpRequestHeadersRustOriginal::RemoveHeader(std::string_view key) {
  auto it = FindHeader(key);
  if (it != headers_.end()) {
    headers_.erase(it);
  }
}

void HttpRequestHeadersRustOriginal::AddHeaderFromString(
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
  if (!http_request_headers_rust_original_helpers::HttpUtil::IsValidHeaderName(
          header_key)) {
    LOG(DFATAL) << "\"" << header_line << "\" has invalid header key.";
    return;
  }

  const std::string::size_type value_index = key_end_index + 1;

  if (value_index < header_line.size()) {
    std::string_view header_value = header_line.substr(value_index);
    header_value =
        http_request_headers_rust_original_helpers::HttpUtil::TrimLWS(
            header_value);
    if (!http_request_headers_rust_original_helpers::HttpUtil::
            IsValidHeaderValue(header_value)) {
      LOG(DFATAL) << "\"" << header_line << "\" has invalid header value.";
      return;
    }
    SetHeader(header_key, header_value);
  } else if (value_index == header_line.size()) {
    SetHeader(header_key, "");
  } else {
    NOTREACHED();
  }
}

void HttpRequestHeadersRustOriginal::AddHeadersFromString(
    std::string_view headers) {
  for (std::string_view header :
       http_request_headers_rust_original_helpers::Split(headers)) {
    AddHeaderFromString(header);
  }
}

void HttpRequestHeadersRustOriginal::MergeFrom(
    const HttpRequestHeadersRustOriginal& other) {
  for (const auto& header : other.headers_) {
    SetHeader(header.key, header.value);
  }
}

std::string HttpRequestHeadersRustOriginal::ToString() const {
  static constexpr std::string_view kColon = ": ";
  static constexpr std::string_view kCrNl = "\r\n";

  // As of January 2024, 99% of of HttpRequestHeadersRustOriginal objects had 27
  // headers or less. Allow space for 128 string pieces without heap allocation
  // as it is a nice round number.
  absl::InlinedVector<std::string_view, 128> pieces;
  const size_t expected_size = headers_.size() * 4 + 1;

  pieces.reserve(expected_size);
  for (const auto& header : headers_) {
    pieces.insert(pieces.end(), {header.key, kColon, header.value, kCrNl});
  }
  pieces.push_back(kCrNl);
  CHECK_EQ(pieces.size(), expected_size);
  return base::StrCat(pieces);
}

base::DictValue HttpRequestHeadersRustOriginal::NetLogParams(
    const std::string& request_line,
    NetLogCaptureMode capture_mode) const {
  base::DictValue dict;
  dict.Set("line", NetLogStringValue(request_line));
  base::ListValue headers;
  for (const auto& header : headers_) {
    std::string log_value =
        ElideHeaderValueForNetLog(capture_mode, header.key, header.value);
    headers.Append(
        NetLogStringValue(base::StrCat({header.key, ": ", log_value})));
  }
  dict.Set("headers", std::move(headers));
  return dict;
}

void HttpRequestHeadersRustOriginal::SetAcceptEncodingIfMissing(
    const GURL& url,
    const std::optional<base::flat_set<SourceStreamType>>&
        accepted_stream_types,
    bool enable_brotli,
    bool enable_zstd) {
  if (HasHeader(kAcceptEncoding)) {
    return;
  }

  // If a range is specifically requested, set the "Accepted Encoding" header to
  // "identity".
  if (HasHeader(kRange)) {
    SetHeader(kAcceptEncoding, "identity");
    return;
  }

  // Supply Accept-Encoding headers first so that it is more likely that they
  // will be in the first transmitted packet. This can sometimes make it easier
  // to filter and analyze the streams to assure that a proxy has not damaged
  // these headers. Some proxies deliberately corrupt Accept-Encoding headers.
  std::vector<std::string_view> advertised_encoding_names;
  advertised_encoding_names.reserve(4u);
  if (SupportsStreamType(accepted_stream_types, SourceStreamType::kGzip)) {
    advertised_encoding_names.emplace_back(kEncodingGzip);
  }
  if (SupportsStreamType(accepted_stream_types, SourceStreamType::kDeflate)) {
    advertised_encoding_names.emplace_back(kEncodingDeflate);
  }

  const bool can_use_advanced_encodings =
      (url.SchemeIsCryptographic() || IsLocalhost(url));

  // Advertise "br" encoding only if transferred data is opaque to proxy.
  if (enable_brotli &&
      SupportsStreamType(accepted_stream_types, SourceStreamType::kBrotli) &&
      can_use_advanced_encodings) {
    advertised_encoding_names.emplace_back(kEncodingBrotli);
  }
  // Advertise "zstd" encoding only if transferred data is opaque to proxy.
  if (enable_zstd &&
      SupportsStreamType(accepted_stream_types, SourceStreamType::kZstd) &&
      can_use_advanced_encodings) {
    advertised_encoding_names.emplace_back(kEncodingZstd);
  }
  if (!advertised_encoding_names.empty()) {
    // Tell the server what compression formats are supported.
    SetHeader(kAcceptEncoding,
              base::JoinString(advertised_encoding_names, ", "));
  }
}

HttpRequestHeadersRustOriginal::HeaderVector::iterator
HttpRequestHeadersRustOriginal::FindHeader(std::string_view key) {
  for (auto it = headers_.begin(); it != headers_.end(); ++it) {
    if (http_request_headers_rust_original_helpers::EqualsCaseInsensitiveASCII(
            key, it->key)) {
      return it;
    }
  }

  return headers_.end();
}

HttpRequestHeadersRustOriginal::HeaderVector::const_iterator
HttpRequestHeadersRustOriginal::FindHeader(std::string_view key) const {
  for (auto it = headers_.begin(); it != headers_.end(); ++it) {
    if (http_request_headers_rust_original_helpers::EqualsCaseInsensitiveASCII(
            key, it->key)) {
      return it;
    }
  }

  return headers_.end();
}

void HttpRequestHeadersRustOriginal::SetHeaderInternal(std::string_view key,
                                                       std::string&& value) {
  auto it = FindHeader(key);
  if (it != headers_.end()) {
    it->value = std::move(value);
  } else {
    headers_.emplace_back(key, std::move(value));
  }
}

}  // namespace net
