// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_CONNECTION_AUTH_ORIGINAL_PARAMETERS_H_
#define NET_HTTP_CONNECTION_AUTH_ORIGINAL_PARAMETERS_H_

#include <string>
#include <string_view>

#include "net/http/auth_challenge_reference_tokenizer.h"

namespace net::auth_challenge_reference::frozen {

// Comma-delimited HTTP values. Delimiters inside quotes are ignored.
class ParameterValues {
 public:
  explicit ParameterValues(std::string_view input);
  ~ParameterValues();

  bool GetNext();
  std::string_view value() const { return value_; }

 private:
  StringViewTokenizer tokenizer_;
  std::string_view value_;
};

// Required-value, lenient-quote form of the original HttpUtil iterator.
// The views returned by name() and value() expire on GetNext().
class Parameters {
 public:
  explicit Parameters(std::string_view input);
  Parameters(const Parameters&) = delete;
  Parameters& operator=(const Parameters&) = delete;
  ~Parameters();

  bool GetNext();
  bool valid() const { return valid_; }
  std::string_view name() const { return name_; }
  std::string_view value() const { return value_; }

 private:
  ParameterValues values_;
  std::string unquoted_;
  std::string_view name_;
  std::string_view value_;
  bool valid_ = true;
};

std::string QuoteParameter(std::string_view value);

}  // namespace net::auth_challenge_reference::frozen

#endif  // NET_HTTP_CONNECTION_AUTH_ORIGINAL_PARAMETERS_H_
