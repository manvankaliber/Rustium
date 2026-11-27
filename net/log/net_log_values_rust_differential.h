// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_LOG_NET_LOG_VALUES_RUST_DIFFERENTIAL_H_
#define NET_LOG_NET_LOG_VALUES_RUST_DIFFERENTIAL_H_

#include <bit>
#include <limits>
#include <string>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log_values.h"
#include "net/log/net_log_values_rust_reference.h"

namespace net::net_log_values_rust_test {

inline void Equal(const base::Value& actual, const base::Value& expected) {
  CHECK(actual.type() == expected.type());
  CHECK(actual == expected);
  if (actual.is_double()) {
    CHECK(std::bit_cast<uint64_t>(actual.GetDouble()) ==
          std::bit_cast<uint64_t>(expected.GetDouble()));
  }
}

inline void Check(std::string_view raw, uint64_t bits) {
  namespace ref = net_log_values_rust_reference;
  const int64_t signed_value = std::bit_cast<int64_t>(bits);
  const auto small = static_cast<uint32_t>(bits);
  Equal(NetLogNumberValue(signed_value), ref::NetLogNumberValue(signed_value));
  Equal(NetLogNumberValue(bits), ref::NetLogNumberValue(bits));
  Equal(NetLogNumberValue(small), ref::NetLogNumberValue(small));
  Equal(NetLogStringValue(raw), ref::NetLogStringValue(raw));
  Equal(NetLogBinaryValue(base::as_byte_span(raw)),
        ref::NetLogBinaryValue(base::as_byte_span(raw)));
  Equal(NetLogBinaryValue(raw.data(), raw.size()),
        ref::NetLogBinaryValue(raw.data(), raw.size()));

  // Raw strings and keys retain base::Value's UTF-8 diagnostics. Exercise all
  // valid arbitrary UTF-8 inputs, including noncharacters and embedded NULs.
  const bool valid = base::IsStringUTF8AllowingNoncharacters(raw);
  const std::string_view literal = valid ? raw : std::string_view("k\0z", 3);
  const std::string_view key = literal;
  CHECK(NetLogParamsWithInt(key, static_cast<int32_t>(small)) ==
        ref::NetLogParamsWithInt(key, static_cast<int32_t>(small)));
  CHECK(NetLogParamsWithInt64(key, signed_value) ==
        ref::NetLogParamsWithInt64(key, signed_value));
  CHECK(NetLogParamsWithBool(key, (bits & 1) != 0) ==
        ref::NetLogParamsWithBool(key, (bits & 1) != 0));
  CHECK(NetLogParamsWithString(key, literal) ==
        ref::NetLogParamsWithString(key, literal));

  // All returned objects own copies. Key and literal may overlap one another;
  // no output aliases caller storage after a mutation or destruction.
  std::string backing(raw);
  auto text = NetLogStringValue(backing);
  auto bytes = NetLogBinaryValue(backing.data(), backing.size());
  auto dictionary = NetLogParamsWithString(literal, literal);
  auto dictionary_reference = ref::NetLogParamsWithString(literal, literal);
  backing.assign(backing.size(), 'x');
  Equal(text, ref::NetLogStringValue(raw));
  Equal(bytes, ref::NetLogBinaryValue(raw.data(), raw.size()));
  CHECK(dictionary == dictionary_reference);
}

inline void Boundaries() {
  constexpr int64_t signed_cases[] = {std::numeric_limits<int64_t>::min(),
                                      -9007199254740993LL,
                                      -9007199254740992LL,
                                      -9007199254740991LL,
                                      -9007199254740990LL,
                                      -2147483649LL,
                                      -2147483648LL,
                                      -2147483647LL,
                                      -1,
                                      0,
                                      1,
                                      2147483646LL,
                                      2147483647LL,
                                      2147483648LL,
                                      4294967295LL,
                                      9007199254740990LL,
                                      9007199254740991LL,
                                      9007199254740992LL,
                                      9007199254740993LL,
                                      std::numeric_limits<int64_t>::max()};
  for (int64_t value : signed_cases) {
    Check({}, std::bit_cast<uint64_t>(value));
    Equal(NetLogNumberValue(value),
          net_log_values_rust_reference::NetLogNumberValue(value));
    CHECK(SerializeNetLogValueToJson(NetLogNumberValue(value)) ==
          SerializeNetLogValueToJson(
              net_log_values_rust_reference::NetLogNumberValue(value)));
  }
  for (uint64_t value :
       {uint64_t{0}, uint64_t{2147483647}, uint64_t{2147483648},
        uint64_t{4294967295}, uint64_t{9007199254740991},
        uint64_t{9007199254740992}, std::numeric_limits<uint64_t>::max()}) {
    Check({}, value);
  }
  Equal(NetLogBinaryValue(nullptr, 0),
        net_log_values_rust_reference::NetLogBinaryValue(nullptr, 0));
  Equal(NetLogBinaryValue(base::span<const uint8_t>()),
        net_log_values_rust_reference::NetLogBinaryValue(
            base::span<const uint8_t>()));
  std::string all_bytes;
  for (unsigned i = 0; i < 256; ++i) {
    all_bytes.push_back(static_cast<char>(i));
    Check(std::string_view(all_bytes).substr(i, 1), i);
  }
  Check(all_bytes, 0);
  Check("%", 0);
  Check("%ESCAPED:\xE2\x80\x8B %00", 0);
  Check(std::string_view("a\0b\x7F", 4), 0);
  Check("\xEF\xBF\xBF\xF4\x8F\xBF\xBF", 0);  // UTF-8 noncharacters.
}

}  // namespace net::net_log_values_rust_test
#endif  // NET_LOG_NET_LOG_VALUES_RUST_DIFFERENTIAL_H_
