// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#ifndef NET_HTTP_CONNECTION_HEADER_ORIGINAL_POLICY_H_
#define NET_HTTP_CONNECTION_HEADER_ORIGINAL_POLICY_H_
#include <optional>
#include <string>
#include <string_view>
#include "net/http/http_request_headers.h"
#include "net/log/net_log_with_source.h"
namespace net {
class HttpAuthController;
// Reference-only C++ policy over exactly the native storage. No owner, adapter
// container, layout conversion, const_cast or thread-local dispatch override.
// Mutable operations require a genuinely mutable receiver. Const reads use the
// public inline vector view. Parent supplies the private storage friendship.
class ConnectionHeaderOriginalPolicy {
 public:
  static bool HasHeader(const HttpRequestHeaders&, std::string_view);
  static std::optional<std::string_view> GetHeaderView(const HttpRequestHeaders&, std::string_view);
  static std::optional<std::string> GetHeader(const HttpRequestHeaders&, std::string_view);
  static void Clear(HttpRequestHeaders&);
  static void SetHeader(HttpRequestHeaders&, std::string_view, std::string_view);
  static void SetHeader(HttpRequestHeaders&, std::string_view, std::string&&);
  static void SetHeader(HttpRequestHeaders& h, std::string_view k, const char* v) {
    SetHeader(h, k, std::string_view(v));
  }
  static void SetHeaderWithoutCheckForTesting(HttpRequestHeaders&, std::string_view, std::string_view);
  static void SetHeaderIfMissing(HttpRequestHeaders&, std::string_view, std::string_view);
  static void RemoveHeader(HttpRequestHeaders&, std::string_view);
  static void AddHeaderFromString(HttpRequestHeaders&, std::string_view);
  static void AddHeadersFromString(HttpRequestHeaders&, std::string_view);
  static void MergeFrom(HttpRequestHeaders&, const HttpRequestHeaders&);
  static std::string ToString(const HttpRequestHeaders&);
  static base::DictValue NetLogParams(const HttpRequestHeaders&, const std::string&, NetLogCaptureMode);
  // Exact AddAuthorizationHeader policy: copies the token once into the
  // existing destination, then clears it. It never moves from the controller
  // before validation succeeds. Same thread checks / HaveAuth preconditions.
  static void AddAuthorizationHeader(HttpAuthController&, HttpRequestHeaders*);
 private:
  static HttpRequestHeaders::HeaderVector::iterator FindHeader(HttpRequestHeaders&, std::string_view);
  static HttpRequestHeaders::HeaderVector::const_iterator FindHeader(const HttpRequestHeaders&, std::string_view);
  static void SetHeaderInternal(HttpRequestHeaders&, std::string_view, std::string&&);
};
void ConnectionOriginalNetLogRequestHeaders(const NetLogWithSource&, NetLogEventType,
                                           const std::string&, const HttpRequestHeaders*);
std::string ConnectionOriginalElideHeaderValueForNetLog(NetLogCaptureMode,
                                                       std::string_view, std::string_view);
}  // namespace net
#endif
