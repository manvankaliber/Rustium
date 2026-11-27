// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/connection_auth_original_parameters.h"

#include "base/check.h"

namespace net::auth_challenge_reference::frozen {

namespace {

// From the original HttpUtil required-value, lenient-quote iterator at
// 478cd0e02a3a22af2ce2b64bd5c440ba87c5943c.
std::string_view TrimParameterWhitespace(std::string_view value) {
  const size_t begin = value.find_first_not_of(" \t");
  if (begin == std::string_view::npos) {
    return {};
  }
  return value.substr(begin, value.find_last_not_of(" \t") - begin + 1);
}

// Frozen lenient unquoting for required Basic parameters. Accept an opening
// quote without a matching end quote and retain its remaining bytes.
std::string_view UnquoteParameter(std::string_view value, std::string& storage) {
  if (value.front() != '"') {
    return value;
  }
  value.remove_prefix(1);
  if (value.empty() || value.back() != '"') {
    return value;
  }
  value.remove_suffix(1);
  bool escaped = false;
  for (char c : value) {
    if (c == '\\' && !escaped) {
      escaped = true;
      continue;
    }
    escaped = false;
    storage.push_back(c);
  }
  return storage;
}

}  // namespace

ParameterValues::ParameterValues(std::string_view input)
    : tokenizer_(input.begin(), input.end(), ",") {
  tokenizer_.set_quote_chars("\"");
}

ParameterValues::~ParameterValues() = default;

bool ParameterValues::GetNext() {
  while (const auto token = tokenizer_.GetNextTokenView()) {
    value_ = TrimParameterWhitespace(*token);
    if (!value_.empty()) {
      return true;
    }
  }
  return false;
}

Parameters::Parameters(std::string_view input) : values_(input) {}

Parameters::~Parameters() = default;

bool Parameters::GetNext() {
  CHECK(valid_);
  name_ = {};
  value_ = {};
  unquoted_.clear();
  if (!values_.GetNext()) {
    return false;
  }

  const std::string_view pair = values_.value();
  const size_t equals = pair.find('=');
  if (equals == 0 || equals == std::string_view::npos) {
    valid_ = false;
    return false;
  }
  const std::string_view name = TrimParameterWhitespace(pair.substr(0, equals));
  const std::string_view value = TrimParameterWhitespace(pair.substr(equals + 1));
  if (name.find('"') != std::string_view::npos || value.empty()) {
    valid_ = false;
    return false;
  }
  name_ = name;
  value_ = UnquoteParameter(value, unquoted_);
  return true;
}

std::string QuoteParameter(std::string_view str) {
  std::string escaped;
  escaped.reserve(2 + str.size());

  // Escape any backslashes or quotation marks within the string, and
  // then surround with quotes.
  escaped.push_back('"');
  for (char c : str) {
    if (c == '"' || c == '\\') {
      escaped.push_back('\\');
    }
    escaped.push_back(c);
  }
  escaped.push_back('"');
  return escaped;
}

}  // namespace net::auth_challenge_reference::frozen
