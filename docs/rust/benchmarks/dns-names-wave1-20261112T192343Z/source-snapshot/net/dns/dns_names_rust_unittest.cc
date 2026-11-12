// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <string>
#include <vector>

#include "net/dns/dns_names_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::dns_names_test {
TEST(DnsNamesRustTest, ExhaustiveTwoBytes) {
  Check({});
  std::array<uint8_t, 2> bytes;
  for (unsigned a = 0; a < 256; ++a) {
    bytes[0] = a;
    Check(base::span(bytes).first(1u));
    for (unsigned b = 0; b < 256; ++b) {
      bytes[1] = b;
      Check(bytes, false);
    }
  }
}
TEST(DnsNamesRustTest, ExhaustiveLabelLengthsAndAvailableBytes) {
  for (unsigned len = 0; len < 256; ++len) {
    for (size_t available = 0; available < 260; ++available) {
      std::vector<uint8_t> bytes(available + 1, 'a');
      bytes[0] = len;
      Check(bytes, false);
      bytes.insert(bytes.begin(), {1, 0});
      Check(bytes, false);
    }
  }
}
TEST(DnsNamesRustTest, ExhaustiveU16Lengths) {
  std::vector<uint8_t> bytes(65538, 0x91);
  for (unsigned len = 0; len < 65536; ++len) {
    bytes[0] = len >> 8;
    bytes[1] = len;
    for (size_t count : {size_t(len + 2), size_t(len + 1), size_t(len + 3)}) {
      const auto input = base::span(bytes).first(count);
      base::SpanReader<const uint8_t> a(input), b(input);
      base::span<const uint8_t> x, y;
      EXPECT_EQ(dns_names_util::ReadU16LengthPrefixed(a, &x),
                dns_names_reference::ReadU16LengthPrefixed(b, &y));
      EXPECT_EQ(a.num_read(), b.num_read());
      EXPECT_EQ(x.data(), y.data());
      EXPECT_EQ(x.size(), y.size());
    }
  }
}
TEST(DnsNamesRustTest, AllByteComparatorPolicy) {
  for (unsigned a = 0; a < 256; ++a) {
    for (unsigned b = 0; b < 256; ++b) {
      const std::string x(1, a), y(1, b);
      EXPECT_EQ(dns_names_util::DomainNameComparator()(x, y),
                dns_names_reference::DomainNameComparator()(x, y));
    }
  }
}
TEST(DnsNamesRustTest, NamesLimitsRootsAndServices) {
  for (size_t size = 0; size < 520; ++size) {
    for (uint8_t byte : {0, 46, 65, 127, 128, 255}) {
      std::string text(size, byte);
      Check(base::as_byte_span(text));
      for (size_t pos = 0; pos < size; pos += 63) {
        text[pos] = '.';
      }
      Check(base::as_byte_span(text));
    }
    std::vector<uint8_t> wire;
    for (size_t i = 0; i < size; ++i) {
      wire.insert(wire.end(), {1, uint8_t(i)});
    }
    Check(wire, false);
    wire.push_back(0);
    wire.push_back(0xc0);
    Check(wire, false);
  }
  for (std::string_view text :
       {"localhost", "LOCALHOST.", "a.localhost", "127.0.0.1", "0x7f.1",
        "[::1]", "::1", "GOOGLE.test", "G<OO>GLE.test", "g\xc3\xbcgle.test",
        "1.2.3.4", "_tcp.local"}) {
    Check(base::as_byte_span(text));
  }
}
TEST(DnsNamesRustTest, NonzeroInitialCursorAndAliasedOutputView) {
  const std::array<uint8_t, 10> bytes{8, 9, 1, 'A', 0, 0xc0, 3, 0, 1, 2};
  for (size_t offset = 0; offset <= bytes.size(); ++offset) {
    base::SpanReader a{base::span(bytes)}, b{base::span(bytes)};
    ASSERT_TRUE(a.Skip(offset));
    ASSERT_TRUE(b.Skip(offset));
    EXPECT_EQ(dns_names_util::NetworkToDottedName(a),
              dns_names_reference::NetworkToDottedName(b));
    EXPECT_EQ(a.num_read(), b.num_read());
    Check(base::span(bytes).subspan(offset));
  }
}
TEST(DnsNamesRustTest, CursorObjectCanBeInput) {
  // Reuse the same address, since pointer representation is part of the input.
  base::SpanReader<const uint8_t> reader;
  auto object = base::byte_span_from_ref(reader);
  for (size_t offset = 0; offset < object.size(); ++offset) {
    for (bool complete : {false, true}) {
      reader = base::SpanReader<const uint8_t>(object.subspan(offset));
      auto expected =
          dns_names_reference::NetworkToDottedName(reader, complete);
      auto count = reader.num_read();
      reader = base::SpanReader<const uint8_t>(object.subspan(offset));
      EXPECT_EQ(dns_names_util::NetworkToDottedName(reader, complete),
                expected);
      EXPECT_EQ(reader.num_read(), count);
    }
  }
}
TEST(DnsNamesRustTest, OutputObjectCanBeInput) {
  base::span<const uint8_t> out;
  auto object = base::byte_span_from_ref(out);
  const std::array<uint8_t, 20> storage{};
  for (size_t len = 0; len <= storage.size(); ++len) {
    for (size_t offset = 0; offset < object.size(); ++offset) {
      for (bool wide : {false, true}) {
        out = base::span(storage).first(len);
        base::SpanReader<const uint8_t> ref(object.subspan(offset));
        bool expected =
            wide ? dns_names_reference::ReadU16LengthPrefixed(ref, &out)
                 : dns_names_reference::ReadU8LengthPrefixed(ref, &out);
        auto saved = out;
        out = base::span(storage).first(len);
        base::SpanReader<const uint8_t> actual(object.subspan(offset));
        bool ok = wide ? dns_names_util::ReadU16LengthPrefixed(actual, &out)
                       : dns_names_util::ReadU8LengthPrefixed(actual, &out);
        EXPECT_EQ(ok, expected);
        EXPECT_EQ(out.data(), saved.data());
        EXPECT_EQ(out.size(), saved.size());
        EXPECT_EQ(actual.num_read(), ref.num_read());
      }
    }
  }
}
}  // namespace net::dns_names_test
