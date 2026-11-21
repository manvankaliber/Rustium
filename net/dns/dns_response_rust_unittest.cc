// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/dns/dns_response_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::dns_response_rust_test {
TEST(DnsResponseRustTest, AllBytesAndTruncations) {
  for (unsigned b = 0; b < 256; ++b) {
    std::vector<uint8_t> data{
        1, static_cast<uint8_t>(b), 0, 0, 1, 0, 1, 0, 0, 0, 42, 0, 4, 1, 2, 3,
        4};
    for (size_t n = 0; n <= data.size(); ++n) {
      Differential(base::span(data).first(n));
    }
  }
}
TEST(DnsResponseRustTest, Corpus) {
  for (const auto& packet : Corpus()) {
    Differential(packet);
  }
}
TEST(DnsResponseRustTest, OwnershipAndMutableBacking) {
  Ownership();
}
TEST(DnsResponseRustTest, Serialization) {
  for (unsigned i = 0; i < 256; ++i) {
    Serialize(i);
  }
}
TEST(DnsResponseRustTest, AliasedRecordAndLivePacket) {
  DnsResourceRecord a;
  DnsResponseRustReferenceRecord b;
  a.name.assign(300, 'x');
  b.name = a.name;
  const std::vector<uint8_t> wire{1, 'a', 0, 0, 1, 0, 1, 0, 0,
                                  0, 42,  0, 4, 1, 2, 3, 4};
  std::ranges::copy(wire, a.name.begin() + 1);
  std::ranges::copy(wire, b.name.begin() + 1);
  DnsRecordParser pa(base::as_byte_span(a.name), 1, 1);
  DnsResponseRustReferenceParser pb(base::as_byte_span(b.name), 1, 1);
  CHECK_EQ(pa.ReadRecord(&a), pb.ReadRecord(&b));
  EqualRecord(a, b);
  CHECK_EQ(pa.GetOffset(), pb.GetOffset());
  auto bytes = Corpus()[0];
  bytes = {0x12, 0x34, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  auto packet = Packet(bytes, 200);
  DnsResponse response(packet, packet->size());
  ASSERT_TRUE(response.InitParseWithoutQuery(bytes.size()));
  packet->span()[0] = 0xab;
  EXPECT_EQ(response.id(), 0xab34);
  packet->span()[3] = 3;
  EXPECT_EQ(response.rcode(), 3);
  DnsResponse moved(1);
  moved = std::move(response);
  EXPECT_EQ(moved.io_buffer(), packet.get());
  EXPECT_EQ(moved.id(), 0xab34);
  EXPECT_TRUE(moved.Parser().AtEnd());
  EXPECT_FALSE(moved.InitParseWithoutQuery(0));
  EXPECT_EQ(moved.id(), 0xab34);
  EXPECT_TRUE(moved.IsValid());
}
TEST(DnsResponseRustTest, InitializedExtentAndIdFailures) {
  const auto bytes = std::to_array<uint8_t>(
      {0x12, 0x34, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1});
  for (size_t n = 0; n <= bytes.size(); ++n) {
    auto packet = Packet(base::span(bytes).first(n), 100);
    DnsResponse response(packet, packet->size());
    EXPECT_FALSE(response.InitParseWithoutQuery(n));
    EXPECT_EQ(response.id().has_value(), n >= 12);
  }
  DnsResponse response;
  EXPECT_EQ(response.io_buffer_size(), dns_protocol::kMaxUDPSize + 1u);
  EXPECT_FALSE(response.IsValid());
  EXPECT_FALSE(response.id().has_value());
}
TEST(DnsResponseRustTest, NullEmpty) {
  Differential({});
}
}  // namespace net::dns_response_rust_test
