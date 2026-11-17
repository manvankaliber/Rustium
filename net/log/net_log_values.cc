// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/log/net_log_values.h"

#include <utility>

#include "base/compiler_specific.h"
#include "base/notreached.h"
#include "base/values.h"
#include "net/log/net_log_values_rust_ffi.h"

namespace net {
namespace {

// Representation selection and all formatting run in Rust. This synchronous
// service only constructs the requested base::Value, retaining its ownership
// and UTF-8 diagnostic contracts. Text is copied before Rust storage expires.
extern "C" void NetLogValuesRustStoreValue(void* context,
                                           uint8_t kind,
                                           int32_t integer,
                                           double number,
                                           const uint8_t* text,
                                           size_t length) {
  auto& result = *static_cast<base::Value*>(context);
  switch (kind) {
    case 0:
      result = base::Value(integer);
      return;
    case 1:
      result = base::Value(number);
      return;
    case 2:
      result = base::Value(integer != 0);
      return;
    case 3:
      // SAFETY: Rust keeps initialized text alive until this callback returns.
      // The output Value is distinct from the public input storage.
      result = base::Value(
          length == 0 ? std::string_view()
                      : UNSAFE_BUFFERS(std::string_view(
                            reinterpret_cast<const char*>(text), length)));
      return;
  }
  NOTREACHED();
}

base::Value FromBytes(std::string_view bytes, uint8_t mode) {
  base::Value result;
  net_log_values_rust_bytes(reinterpret_cast<const uint8_t*>(bytes.data()),
                            bytes.size(), mode, &result,
                            NetLogValuesRustStoreValue);
  return result;
}

}  // namespace

base::Value NetLogStringValue(std::string_view raw) {
  return FromBytes(raw, 0);
}

base::Value NetLogBinaryValue(base::span<const uint8_t> bytes) {
  return NetLogBinaryValue(bytes.data(), bytes.size());
}

base::Value NetLogBinaryValue(const void* bytes, size_t length) {
  base::Value result;
  net_log_values_rust_bytes(static_cast<const uint8_t*>(bytes), length, 1,
                            &result, NetLogValuesRustStoreValue);
  return result;
}

base::Value NetLogNumberValue(int64_t num) {
  base::Value result;
  net_log_values_rust_signed(num, &result, NetLogValuesRustStoreValue);
  return result;
}

base::Value NetLogNumberValue(uint64_t num) {
  base::Value result;
  net_log_values_rust_unsigned(num, &result, NetLogValuesRustStoreValue);
  return result;
}

base::Value NetLogNumberValue(uint32_t num) {
  return NetLogNumberValue(static_cast<uint64_t>(num));
}

base::DictValue NetLogParamsWithInt(std::string_view name, int value) {
  base::DictValue params;
  params.Set(name, NetLogNumberValue(static_cast<int64_t>(value)));
  return params;
}

base::DictValue NetLogParamsWithInt64(std::string_view name, int64_t value) {
  base::DictValue params;
  params.Set(name, NetLogNumberValue(value));
  return params;
}

base::DictValue NetLogParamsWithBool(std::string_view name, bool value) {
  base::Value converted;
  net_log_values_rust_bool(value, &converted, NetLogValuesRustStoreValue);
  base::DictValue params;
  params.Set(name, std::move(converted));
  return params;
}

base::DictValue NetLogParamsWithString(std::string_view name,
                                       std::string_view value) {
  base::DictValue params;
  params.Set(name, FromBytes(value, 2));
  return params;
}

}  // namespace net
