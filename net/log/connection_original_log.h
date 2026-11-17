#ifndef NET_LOG_CONNECTION_ORIGINAL_LOG_H_
#define NET_LOG_CONNECTION_ORIGINAL_LOG_H_
#include <string_view>
#include <utility>
#include "base/check_op.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/net_errors.h"
#include "net/http/auth_challenge_reference.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_response_headers.h"
#include "net/log/net_log_values_rust_reference.h"
#include "net/log/net_log_with_source.h"
#include "net/third_party/quiche/src/quiche/common/http/http_header_block.h"
namespace net::connection_original_logging {


inline bool ShouldRedactChallenge(auth_challenge_reference::Tokenizer* challenge) {
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


inline std::string ElideHeaderValueForNetLog(NetLogCaptureMode capture_mode,
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

inline base::DictValue ResponseParams(const HttpResponseHeaders& self,
    NetLogCaptureMode capture_mode) {
  base::DictValue dict;
  base::ListValue headers;
  headers.Append(net_log_values_rust_reference::NetLogStringValue(self.GetStatusLine()));
  size_t iterator = 0;
  std::string name;
  std::string value;
  while (self.EnumerateHeaderLines(&iterator, &name, &value)) {
    std::string log_value =
        ElideHeaderValueForNetLog(capture_mode, name, value);
    headers.Append(net_log_values_rust_reference::NetLogStringValue(base::StrCat({name, ": ", log_value})));
  }
  dict.Set("headers", std::move(headers));
  return dict;
}
}  // namespace net::connection_original_logging
namespace net {
inline void ConnectionOriginalNetLogResponseHeaders(const NetLogWithSource& net_log,
                                      NetLogEventType type,
                                      const HttpResponseHeaders* headers) {
  net_log.AddEvent(type, [&](NetLogCaptureMode capture_mode) {
    return connection_original_logging::ResponseParams(*headers, capture_mode);
  });
}
inline base::Value ConnectionOriginalElideGoAwayDebugDataForNetLog(NetLogCaptureMode capture_mode,
                                          std::string_view debug_data) {
  if (NetLogCaptureIncludesSensitive(capture_mode))
    return net_log_values_rust_reference::NetLogStringValue(debug_data);

  return net_log_values_rust_reference::NetLogStringValue(base::StrCat(
      {"[", base::NumberToString(debug_data.size()), " bytes were stripped]"}));
}

inline base::ListValue ConnectionOriginalElideHttpHeaderBlockForNetLog(
    const quiche::HttpHeaderBlock& headers,
    NetLogCaptureMode capture_mode) {
  base::ListValue headers_list;
  for (const auto& [key, value] : headers) {
    headers_list.Append(net_log_values_rust_reference::NetLogStringValue(base::StrCat(
        {key, ": ", connection_original_logging::ElideHeaderValueForNetLog(capture_mode, key, value)})));
  }
  return headers_list;
}

inline base::DictValue ConnectionOriginalHttpHeaderBlockNetLogParams(
    const quiche::HttpHeaderBlock* headers,
    NetLogCaptureMode capture_mode) {
  return base::DictValue().Set(
      "headers", ConnectionOriginalElideHttpHeaderBlockForNetLog(*headers, capture_mode));
}


inline base::DictValue ConnectionOriginalAuthorizationResultParams(
    const char* name,
    HttpAuth::AuthorizationResult authorization_result) {
  return net_log_values_rust_reference::NetLogParamsWithString(
      name, HttpAuth::AuthorizationResultToString(authorization_result));
}
class ConnectionOriginalLog {
 public:
  static void AddEventWithStringParams(const NetLogWithSource& self,
    NetLogEventType type,
                                                std::string_view name,
                                                std::string_view value);
  static void AddEventWithIntParams(const NetLogWithSource& self,
    NetLogEventType type,
                                             std::string_view name,
                                             int value);
  static void BeginEventWithIntParams(const NetLogWithSource& self,
    NetLogEventType type,
                                               std::string_view name,
                                               int value);
  static void EndEventWithIntParams(const NetLogWithSource& self,
    NetLogEventType type,
                                             std::string_view name,
                                             int value);
  static void AddEventWithInt64Params(const NetLogWithSource& self,
    NetLogEventType type,
                                               std::string_view name,
                                               int64_t value);
  static void BeginEventWithStringParams(const NetLogWithSource& self,
    NetLogEventType type,
    std::string_view name,
    std::string_view value);
  static void AddEventWithNetErrorCode(const NetLogWithSource& self,
    NetLogEventType event_type,
                                                int net_error);
  static void EndEventWithNetErrorCode(const NetLogWithSource& self,
    NetLogEventType event_type,
                                                int net_error);
  static void AddEntryWithBoolParams(const NetLogWithSource& self,
    NetLogEventType type,
                                              NetLogEventPhase phase,
                                              std::string_view name,
                                              bool value);
  static void AddByteTransferEvent(const NetLogWithSource& self,
    NetLogEventType event_type,
    base::span<const uint8_t> bytes);
  static void AddByteTransferEvent(const NetLogWithSource& self,
    NetLogEventType event_type,
                                            int byte_count,
                                            const char* bytes);
};
inline base::DictValue BytesTransferredParams(int byte_count,
                                       const char* bytes,
                                       NetLogCaptureMode capture_mode) {
  base::DictValue dict;
  dict.Set("byte_count", byte_count);
  if (NetLogCaptureIncludesSocketBytes(capture_mode) && byte_count > 0) {
    dict.Set("bytes", net_log_values_rust_reference::NetLogBinaryValue(bytes, byte_count));
  }
  return dict;
}
inline void ConnectionOriginalLog::AddEventWithStringParams(const NetLogWithSource& self,
    NetLogEventType type,
                                                std::string_view name,
                                                std::string_view value) {
  self.AddEvent(type, [&] { return net_log_values_rust_reference::NetLogParamsWithString(name, value); });
}

inline void ConnectionOriginalLog::AddEventWithIntParams(const NetLogWithSource& self,
    NetLogEventType type,
                                             std::string_view name,
                                             int value) {
  self.AddEvent(type, [&] { return net_log_values_rust_reference::NetLogParamsWithInt(name, value); });
}

inline void ConnectionOriginalLog::BeginEventWithIntParams(const NetLogWithSource& self,
    NetLogEventType type,
                                               std::string_view name,
                                               int value) {
  self.BeginEvent(type, [&] { return net_log_values_rust_reference::NetLogParamsWithInt(name, value); });
}

inline void ConnectionOriginalLog::EndEventWithIntParams(const NetLogWithSource& self,
    NetLogEventType type,
                                             std::string_view name,
                                             int value) {
  self.EndEvent(type, [&] { return net_log_values_rust_reference::NetLogParamsWithInt(name, value); });
}

inline void ConnectionOriginalLog::AddEventWithInt64Params(const NetLogWithSource& self,
    NetLogEventType type,
                                               std::string_view name,
                                               int64_t value) {
  self.AddEvent(type, [&] { return net_log_values_rust_reference::NetLogParamsWithInt64(name, value); });
}

inline void ConnectionOriginalLog::BeginEventWithStringParams(const NetLogWithSource& self,
    NetLogEventType type,
    std::string_view name,
    std::string_view value) {
  self.BeginEvent(type, [&] { return net_log_values_rust_reference::NetLogParamsWithString(name, value); });
}

inline void ConnectionOriginalLog::AddEventWithNetErrorCode(const NetLogWithSource& self,
    NetLogEventType event_type,
                                                int net_error) {
  DCHECK_NE(ERR_IO_PENDING, net_error);
  if (net_error >= 0) {
    self.AddEvent(event_type);
  } else {
    AddEventWithIntParams(self, event_type, "net_error", net_error);
  }
}

inline void ConnectionOriginalLog::EndEventWithNetErrorCode(const NetLogWithSource& self,
    NetLogEventType event_type,
                                                int net_error) {
  DCHECK_NE(ERR_IO_PENDING, net_error);
  if (net_error >= 0) {
    self.EndEvent(event_type);
  } else {
    EndEventWithIntParams(self, event_type, "net_error", net_error);
  }
}

inline void ConnectionOriginalLog::AddEntryWithBoolParams(const NetLogWithSource& self,
    NetLogEventType type,
                                              NetLogEventPhase phase,
                                              std::string_view name,
                                              bool value) {
  self.AddEntry(type, phase, [&] { return net_log_values_rust_reference::NetLogParamsWithBool(name, value); });
}

inline void ConnectionOriginalLog::AddByteTransferEvent(const NetLogWithSource& self,
    NetLogEventType event_type,
    base::span<const uint8_t> bytes) {
  AddByteTransferEvent(self, event_type, base::checked_cast<int>(bytes.size()),
                       base::as_chars(bytes).data());
}

inline void ConnectionOriginalLog::AddByteTransferEvent(const NetLogWithSource& self,
    NetLogEventType event_type,
                                            int byte_count,
                                            const char* bytes) {
  self.AddEvent(event_type, [&](NetLogCaptureMode capture_mode) {
    return BytesTransferredParams(byte_count, bytes, capture_mode);
  });
}
}  // namespace net

#endif
