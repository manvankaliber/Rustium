// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/header_coalescer.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/compiler_specific.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "base/values.h"
#include "net/http/http_log_util.h"
#include "net/log/net_log_values.h"
#include "net/spdy/header_coalescer_rust_ffi.h"

namespace net {
namespace {

void NetLogInvalidHeader(const NetLogWithSource& net_log,
                         std::string_view header_name,
                         std::string_view header_value,
                         const char* error_message) {
  net_log.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_INVALID_HEADER,
                   [&](NetLogCaptureMode capture_mode) {
                     return base::DictValue()
                         .Set("header_name", NetLogStringValue(header_name))
                         .Set("header_value",
                              NetLogStringValue(ElideHeaderValueForNetLog(
                                  capture_mode, header_name, header_value)))
                         .Set("error", error_message);
                   });
}

// Keep diagnostic string ownership out of the append/ignored-header path.
NOINLINE void NetLogHeaderError(const NetLogWithSource& net_log,
                                std::string_view key,
                                std::string_view value,
                                HeaderCoalescerRustAction action,
                                uint8_t invalid_byte) {
  using Action = HeaderCoalescerRustAction;
  const char* message = nullptr;
  switch (action) {
    case Action::kEmptyName:
      message = "Header name must not be empty.";
      break;
    case Action::kPseudoAfterRegular:
      message = "Pseudo header must not follow regular headers.";
      break;
    case Action::kInvalidName:
      message = "Invalid character in header name.";
      break;
    case Action::kUppercaseName:
      message = "Upper case characters in header name.";
      break;
    case Action::kTooLarge:
      message = "Header list too large.";
      break;
    case Action::kInvalidValue: {
      std::string error_line;
      base::StringAppendF(&error_line,
                          "Invalid character 0x%02X in header value.",
                          invalid_byte);
      NetLogInvalidHeader(net_log, key, value, error_line.c_str());
      return;
    }
    case Action::kIgnore:
    case Action::kAppend:
    case Action::kReleaseValid:
    case Action::kReleaseInvalid:
      NOTREACHED();
  }
  NetLogInvalidHeader(net_log, key, value, message);
}

}  // namespace

HeaderCoalescer::HeaderCoalescer(uint32_t max_header_list_size,
                                 const NetLogWithSource& net_log)
    : max_header_list_size_(max_header_list_size), net_log_(net_log) {}

void HeaderCoalescer::OnHeader(std::string_view key, std::string_view value) {
  const uint32_t decision = header_coalescer_rust_header_fast(
      &header_list_size_, reinterpret_cast<uint8_t*>(&regular_header_seen_),
      error_seen_, reinterpret_cast<const uint8_t*>(key.data()), key.size(),
      reinterpret_cast<const uint8_t*>(value.data()), value.size(),
      max_header_list_size_);
  // Rust commits the partial size/order state before logging, in the original
  // order, including when the input views alias those native fields.
  using Action = HeaderCoalescerRustAction;
  const auto action = static_cast<Action>(decision & 0xff);
  switch (action) {
    case Action::kIgnore:
      return;
    case Action::kAppend:
      headers_.AppendValueOrAddHeader(key, value);
      return;
    case Action::kEmptyName:
    case Action::kPseudoAfterRegular:
    case Action::kInvalidName:
    case Action::kUppercaseName:
    case Action::kTooLarge:
    case Action::kInvalidValue:
      break;
    case Action::kReleaseValid:
    case Action::kReleaseInvalid:
      NOTREACHED();
  }
  NetLogHeaderError(net_log_, key, value, action,
                    static_cast<uint8_t>(decision >> 8));
  // Reentrant log callbacks can mutate other state; do not overwrite it with
  // the pre-callback snapshot. Only the original post-log error latch changes.
  error_seen_ = (decision & (1u << 16)) != 0;
}

quiche::HttpHeaderBlock HeaderCoalescer::release_headers() {
  const uint16_t decision = header_coalescer_rust_release_fast(headers_valid_);
  DCHECK(static_cast<HeaderCoalescerRustAction>(decision & 0xff) ==
         HeaderCoalescerRustAction::kReleaseValid);
  headers_valid_ = (decision & (1u << 8)) != 0;
  return std::move(headers_);
}

}  // namespace net
