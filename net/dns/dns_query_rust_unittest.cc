// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/dns/dns_query_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
static_assert(sizeof(DnsQuery) == sizeof(DnsQueryRustReference));
static_assert(alignof(DnsQuery) == alignof(DnsQueryRustReference));
TEST(DnsQueryRustTest, AllBytesAndPrefixes) {
  for (bool enabled : {false, true}) {
    base::test::ScopedFeatureList features;
    features.InitWithFeatureState(features::kUseStructuredDnsErrors, enabled);
    dns_query_rust_test::State({});
    for (size_t value = 0; value < 256; ++value) {
      std::vector<uint8_t> bytes(290, static_cast<uint8_t>(value));
      dns_query_rust_test::State(bytes);
      dns_query_rust_test::OptCases(bytes);
      // ReadName accepts high length bytes rather than interpreting DNS
      // compression. Accept length 254 and total 255; reject 255 (total 256).
      std::vector<uint8_t> packet(12, 0);
      packet[5] = 1;
      if (value) {
        packet.push_back(static_cast<uint8_t>(value));
        packet.insert(packet.end(), value, static_cast<uint8_t>(value));
      }
      packet.insert(packet.end(), {0, 0, 28, 0, 1});
      DnsQuery query(dns_query_rust_test::Packet(packet));
      DnsQueryRustReference ref(dns_query_rust_test::Packet(packet));
      for (size_t valid = 0; valid <= packet.size(); ++valid) {
        EXPECT_EQ(query.Parse(valid), ref.Parse(valid));
        EXPECT_EQ(query.question_size(), ref.question_size());
      }
      EXPECT_EQ(query.Parse(packet.size()), value != 255);
      dns_query_rust_test::Equal(query, ref);
    }
  }
}
TEST(DnsQueryRustTest, NullEmptyAndUninitializedTail) {
  DnsQuery null_query(nullptr);
  DnsQueryRustReference null_ref(nullptr);
  EXPECT_FALSE(null_query.Parse(100));
  EXPECT_FALSE(null_ref.Parse(100));
  DnsQuery zero(base::MakeRefCounted<IOBufferWithSize>(0));
  EXPECT_FALSE(zero.Parse(100));
  DnsQuery zero_copy(zero);
  EXPECT_EQ(zero_copy.io_buffer()->size(), 0);
  const std::vector<uint8_t> name{1, 'a', 0};
  DnsQuery source(9, name, 1);
  auto backing = base::MakeRefCounted<IOBufferWithSize>(512);
  backing->span()
      .first(source.io_buffer()->span().size())
      .copy_from(source.io_buffer()->span());
  DnsQuery raw(backing);
  EXPECT_TRUE(raw.Parse(source.io_buffer()->size()));
  DnsQuery copy(raw);  // Do not read the deliberately uninitialized tail.
  EXPECT_EQ(copy.question(), source.question());
  EXPECT_EQ(copy.io_buffer()->size(), 512);
  auto clone = raw.CloneWithNewId(42);
  EXPECT_EQ(clone->id(), 42);
  EXPECT_EQ(clone->question(), source.question());
}
TEST(DnsQueryRustTest, NativeFlagsLiveViewsAndSelfAssignment) {
  const std::vector<uint8_t> name{1, 'a', 0, 0x55, 0xaa};
  DnsQuery query(0x1234, name, 0xffff);
  DnsQueryRustReference ref(0x1234, name, 0xffff);
  for (uint16_t flags : {0x0001, 0x0100, 0x8000, 0x0080, 0xffff}) {
    query.set_flags(flags);
    ref.set_flags(flags);
    dns_query_rust_test::Equal(query, ref);
  }
  auto bytes = std::vector<uint8_t>(query.io_buffer()->span().begin(),
                                    query.io_buffer()->span().end());
  const DnsQuery& alias = query;
  query = alias;  // Former self-assignment copied newly uninitialized storage.
  EXPECT_EQ(query.io_buffer()->span(), base::span(bytes));
}
TEST(DnsQueryRustTest, MaximumNameAndMultipleLabels) {
  std::vector<uint8_t> wire;
  for (int i = 0; i < 4; ++i) {
    wire.push_back(i == 3 ? 61 : 63);
    wire.insert(wire.end(), i == 3 ? 61 : 63, 'x');
  }
  wire.push_back(0);
  dns_query_rust_test::OptCases(wire);
  DnsQuery query(1, wire, 28);
  DnsQueryRustReference ref(1, wire, 28);
  dns_query_rust_test::Equal(query, ref);
  EXPECT_TRUE(query.Parse(query.io_buffer()->size()));
  for (bool ede : {false, true}) {
    base::test::ScopedFeatureList features;
    features.InitWithFeatureState(features::kUseStructuredDnsErrors, ede);
    DnsQuery padded(1, wire, 28, nullptr,
                    DnsQuery::PaddingStrategy::BLOCK_LENGTH_128);
    DnsQueryRustReference ref_padded(
        1, wire, 28, nullptr,
        DnsQueryRustReference::PaddingStrategy::BLOCK_LENGTH_128);
    dns_query_rust_test::Equal(padded, ref_padded);
    EXPECT_EQ(padded.io_buffer()->size() % 128, 0);
  }
}
TEST(DnsQueryRustDeathTest, Preconditions) {
  const std::vector<uint8_t> name{1, 'a', 0};
  const std::vector<uint8_t> padding{0, 12, 0, 0};
  auto opt = OptRecordRdata::Create(padding);
  EXPECT_DEATH_IF_SUPPORTED(
      (DnsQuery(1, name, 1, opt.get(),
                DnsQuery::PaddingStrategy::BLOCK_LENGTH_128)),
      "");
  OptRecordRdata empty;
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(features::kUseStructuredDnsErrors);
  EXPECT_DEATH_IF_SUPPORTED((DnsQuery(1, name, 1, &empty)), "");
  EXPECT_DEATH_IF_SUPPORTED((DnsQuery(1, base::span<const uint8_t>(), 1)), "");
  EXPECT_DEATH_IF_SUPPORTED(
      (DnsQueryRustReference(1, base::span<const uint8_t>(), 1)), "");
  EXPECT_DEATH_IF_SUPPORTED((DnsQueryRustReference(1, name, 1, &empty)), "");
  OptRecordRdata invalid_clone;
  invalid_clone.AddOpt(
      std::make_unique<OptRecordRdata::EdeOpt>(0, std::string(65534, 'x')));
  EXPECT_DEATH_IF_SUPPORTED((DnsQuery(1, name, 1, &invalid_clone)), "");
  EXPECT_DEATH_IF_SUPPORTED((DnsQueryRustReference(1, name, 1, &invalid_clone)),
                            "");
  DnsQuery small(dns_query_rust_test::Packet(name));
  DnsQueryRustReference ref_small(dns_query_rust_test::Packet(name));
  EXPECT_DEATH_IF_SUPPORTED(small.Parse(4), "");
  EXPECT_DEATH_IF_SUPPORTED(ref_small.Parse(4), "");
}
}  // namespace
}  // namespace net
