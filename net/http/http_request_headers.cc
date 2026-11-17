// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_request_headers.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "net/base/url_util.h"
#include "net/http/http_log_util.h"
#include "net/http/http_request_headers_rust_ffi.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_values.h"
#include "url/gurl.h"

namespace net {

namespace {

// C++ owns strings, vector capacity, iterator invalidation, and external
// services. This service only exposes indexed immutable fields during a Rust
// read call.
RequestHeadersRustReader Reader(
    const HttpRequestHeaders::HeaderVector& headers) {
  return {
      &headers, headers.size(), [](const void* context, size_t i, bool value) {
        const auto& pair =
            (*static_cast<const HttpRequestHeaders::HeaderVector*>(context))[i];
        const auto& field = value ? pair.value : pair.key;
        return RequestHeadersRustSpan{field.data(), field.size()};
      }};
}

struct HttpRequestHeadersRustValidation {
  static bool IsValidHeaderName(std::string_view input) {
    return rustium_request_headers_validate(input.data(), input.size(), 0);
  }
  static bool IsValidHeaderValue(std::string_view input) {
    return rustium_request_headers_validate(input.data(), input.size(), 1);
  }
};

}  // namespace

HttpRequestHeaders::HeaderKeyValuePair::HeaderKeyValuePair() = default;

HttpRequestHeaders::HeaderKeyValuePair::HeaderKeyValuePair(
    std::string_view key,
    std::string_view value)
    : HeaderKeyValuePair(key, std::string(value)) {}

HttpRequestHeaders::HeaderKeyValuePair::HeaderKeyValuePair(std::string_view key,
                                                           std::string&& value)
    : key(key), value(std::move(value)) {}

HttpRequestHeaders::Iterator::Iterator(const HttpRequestHeaders& headers)
    : curr_(headers.headers_.begin()), end_(headers.headers_.end()) {}

HttpRequestHeaders::Iterator::~Iterator() = default;

bool HttpRequestHeaders::Iterator::GetNext() {
  const auto step = rustium_request_headers_step(started_, curr_ == end_);
  started_ = true;
  if (step == 2) {
    ++curr_;
  }
  return step != 0 && curr_ != end_;
}

HttpRequestHeaders::HttpRequestHeaders() = default;
HttpRequestHeaders::HttpRequestHeaders(const HttpRequestHeaders& other) =
    default;
HttpRequestHeaders::HttpRequestHeaders(HttpRequestHeaders&& other) = default;
HttpRequestHeaders::~HttpRequestHeaders() = default;

HttpRequestHeaders& HttpRequestHeaders::operator=(
    const HttpRequestHeaders& other) = default;
HttpRequestHeaders& HttpRequestHeaders::operator=(HttpRequestHeaders&& other) =
    default;

std::optional<std::string_view> HttpRequestHeaders::GetHeaderView(
    std::string_view key) const {
  auto it = FindHeader(key);
  if (it == headers_.end()) {
    return std::nullopt;
  }
  return std::string_view(it->value);
}

std::optional<std::string> HttpRequestHeaders::GetHeader(
    std::string_view key) const {
  auto it = FindHeader(key);
  if (it == headers_.end()) {
    return std::nullopt;
  }
  return it->value;
}

void HttpRequestHeaders::Clear() {
  headers_.clear();
}

void HttpRequestHeaders::SetHeader(std::string_view key,
                                   std::string_view value) {
  SetHeader(key, std::string(value));
}

void HttpRequestHeaders::SetHeader(std::string_view key, std::string&& value) {
  using HttpUtil = HttpRequestHeadersRustValidation;
  // Invalid header names or values could mean clients can attach
  // browser-internal headers.
  CHECK(HttpUtil::IsValidHeaderName(key)) << key;
  CHECK(HttpUtil::IsValidHeaderValue(value)) << key << " has invalid value.";

  SetHeaderInternal(key, std::move(value));
}

void HttpRequestHeaders::SetHeaderWithoutCheckForTesting(
    std::string_view key,
    std::string_view value) {
  SetHeaderInternal(key, std::string(value));
}

void HttpRequestHeaders::SetHeaderIfMissing(std::string_view key,
                                            std::string_view value) {
  using HttpUtil = HttpRequestHeadersRustValidation;
  CHECK(HttpUtil::IsValidHeaderName(key));
  CHECK(HttpUtil::IsValidHeaderValue(value));
  const auto plan =
      rustium_request_headers_plan(Reader(headers_), key.data(), key.size(), 1);
  if (plan.action == 1) {
    headers_.emplace_back(key, value);
  }
}

void HttpRequestHeaders::RemoveHeader(std::string_view key) {
  const auto plan =
      rustium_request_headers_plan(Reader(headers_), key.data(), key.size(), 2);
  if (plan.action == 3) {
    headers_.erase(headers_.begin() + plan.index);
  }
}

void HttpRequestHeaders::AddHeaderFromString(std::string_view header_line) {
  // Check and logging remain native so severity, crash behavior and text match.
  const size_t crlf_index =
      rustium_request_headers_crlf(header_line.data(), header_line.size());
  DCHECK_EQ(std::string::npos, crlf_index)
      << "\"" << header_line << "\" contains CRLF.";
  const auto parsed =
      rustium_request_headers_parse(header_line.data(), header_line.size());
  switch (parsed.error) {
    case 1:
      LOG(DFATAL) << "\"" << header_line << "\" is missing colon delimiter.";
      return;
    case 2:
      LOG(DFATAL) << "\"" << header_line << "\" is missing header key.";
      return;
    case 3:
      LOG(DFATAL) << "\"" << header_line << "\" has invalid header key.";
      return;
    case 4:
      LOG(DFATAL) << "\"" << header_line << "\" has invalid header value.";
      return;
  }
  SetHeader(header_line.substr(0, parsed.key_len),
            header_line.substr(parsed.value_begin, parsed.value_len));
}

void HttpRequestHeaders::AddHeadersFromString(std::string_view headers) {
  struct Context {
    raw_ptr<HttpRequestHeaders> self;
    std::string_view input;
  } context{this, headers};
  rustium_request_headers_block(
      headers.data(), headers.size(), &context,
      [](void* opaque, size_t begin, size_t len) {
        auto& context = *static_cast<Context*>(opaque);
        context.self->AddHeaderFromString(context.input.substr(begin, len));
      });
}

void HttpRequestHeaders::MergeFrom(const HttpRequestHeaders& other) {
  struct Context {
    raw_ptr<HttpRequestHeaders> self;
    raw_ptr<const HttpRequestHeaders> other;
  } context{this, &other};
  rustium_request_headers_each(
      other.headers_.size(), &context, [](void* opaque, size_t i) {
        auto& context = *static_cast<Context*>(opaque);
        const auto& header = context.other->headers_[i];
        context.self->SetHeader(header.key, header.value);
      });
}

std::string HttpRequestHeaders::ToString() const {
  const size_t len =
      rustium_request_headers_serialize(Reader(headers_), nullptr, 0);
  std::string output(len, '\0');
  const size_t written = rustium_request_headers_serialize(
      Reader(headers_), output.data(), output.size());
  CHECK_EQ(written, len);
  return output;
}

base::DictValue HttpRequestHeaders::NetLogParams(
    const std::string& request_line,
    NetLogCaptureMode capture_mode) const {
  base::DictValue dict;
  dict.Set("line", NetLogStringValue(request_line));
  base::ListValue headers;
  struct Context {
    raw_ptr<const HeaderVector> source;
    raw_ptr<base::ListValue> output;
    NetLogCaptureMode mode;
  } context{&headers_, &headers, capture_mode};
  rustium_request_headers_each(
      headers_.size(), &context, [](void* opaque, size_t i) {
        auto& context = *static_cast<Context*>(opaque);
        const auto& header = (*context.source)[i];
        std::string log_value =
            ElideHeaderValueForNetLog(context.mode, header.key, header.value);
        // Formatting is owned by Rust as well; the elision and Value conversion
        // are existing external NetLog services.
        const size_t len = rustium_request_headers_format(
            header.key.data(), header.key.size(), log_value.data(),
            log_value.size(), nullptr);
        std::string line(len, '\0');
        rustium_request_headers_format(header.key.data(), header.key.size(),
                                       log_value.data(), log_value.size(),
                                       line.data());
        context.output->Append(NetLogStringValue(line));
      });
  dict.Set("headers", std::move(headers));
  return dict;
}

void HttpRequestHeaders::SetAcceptEncodingIfMissing(
    const GURL& url,
    const std::optional<base::flat_set<SourceStreamType>>&
        accepted_stream_types,
    bool enable_brotli,
    bool enable_zstd) {
  // Membership and URL classification are external library services. All
  // ordering, range/existing precedence, and enablement policy is Rust-owned.
  const uint32_t mode = rustium_request_headers_encoding_mode(Reader(headers_));
  if (mode == 0) {
    return;
  }
  uint32_t supported = 0;
  bool advanced = false;
  if (mode == 2) {
    const auto supports = [&](SourceStreamType type) {
      return !accepted_stream_types || accepted_stream_types->contains(type);
    };
    supported |= supports(SourceStreamType::kGzip) ? 1 : 0;
    supported |= supports(SourceStreamType::kDeflate) ? 2 : 0;
    supported |= supports(SourceStreamType::kBrotli) ? 4 : 0;
    supported |= supports(SourceStreamType::kZstd) ? 8 : 0;
    advanced = url.SchemeIsCryptographic() || IsLocalhost(url);
  }
  char output[32] = {};
  const size_t len = rustium_request_headers_encoding(
      mode, supported, advanced, enable_brotli, enable_zstd, output);
  if (len != 0) {
    SetHeader(kAcceptEncoding, std::string_view(output, len));
  }
}

HttpRequestHeaders::HeaderVector::iterator HttpRequestHeaders::FindHeader(
    std::string_view key) {
  return headers_.begin() +
         rustium_request_headers_find(Reader(headers_), key.data(), key.size());
}

HttpRequestHeaders::HeaderVector::const_iterator HttpRequestHeaders::FindHeader(
    std::string_view key) const {
  return headers_.begin() +
         rustium_request_headers_find(Reader(headers_), key.data(), key.size());
}

void HttpRequestHeaders::SetHeaderInternal(std::string_view key,
                                           std::string&& value) {
  const auto plan =
      rustium_request_headers_plan(Reader(headers_), key.data(), key.size(), 0);
  // Planning's read borrows have ended before any string
  // assignment/reallocation. Keep the original move and emplace primitives,
  // including self-move behavior.
  if (plan.action == 2) {
    headers_[plan.index].value = std::move(value);
  } else {
    headers_.emplace_back(key, std::move(value));
  }
}

}  // namespace net
