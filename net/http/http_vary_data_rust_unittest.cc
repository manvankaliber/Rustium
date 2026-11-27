// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_vary_data_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
TEST(HttpVaryDataRustTest, PublicApiSequencesAllBytesAndTruncations) {
  http_vary_data_rust_test::Check({});
  for (size_t size = 0; size <= 80; ++size) {
    std::vector<uint8_t> bytes(size);
    for (size_t i = 0; i < size; ++i) {
      bytes[i] = static_cast<uint8_t>(i * 17 + size);
    }
    http_vary_data_rust_test::Check(bytes);
  }
  for (unsigned byte = 0; byte < 256; ++byte) {
    std::vector<uint8_t> bytes(32, static_cast<uint8_t>(byte));
    http_vary_data_rust_test::Check(bytes);
  }
}
TEST(HttpVaryDataRustTest, OrderedFieldsMissingEmptyAndSeparators) {
  for (int kind : {0, 1, 2}) {
    HttpRequestHeaders first, same, collision;
    first.SetHeader("Foo", "1");
    first.SetHeader("Bar", "23");
    same.SetHeader("foo", "1");
    same.SetHeader("bar", "23");
    collision.SetHeader("foo", "12");
    collision.SetHeader("bar", "3");
    auto response = http_vary_data_rust_test::Response("Vary: Foo,bar,missing");
    auto reverse = http_vary_data_rust_test::Response("Vary: bar,foo,missing");
    HttpVaryData a;
    HttpVaryDataOriginal b;
    http_vary_data_rust_test::Init(a, b, first, *response, kind);
    EXPECT_TRUE(a.MatchesRequest(same, *response));
    EXPECT_FALSE(a.MatchesRequest(collision, *response));
    EXPECT_FALSE(a.MatchesRequest(same, *reverse));
    same.SetHeader("missing", "");
    EXPECT_TRUE(a.MatchesRequest(same, *response));
    auto before = a;
    same.Clear();
    EXPECT_FALSE(a.MatchesRequest(same, *response));
    base::Pickle p, q;
    a.Persist(&p);
    before.Persist(&q);
    EXPECT_EQ(base::span(p), base::span(q));
  }
}
TEST(HttpVaryDataRustTest, WildcardAlwaysShaAndDeterministic) {
  HttpRequestHeaders request;
  auto response = http_vary_data_rust_test::Response("Vary: foo,*,bar");
  for (int kind : {0, 1, 2, -1}) {
    HttpVaryData a;
    HttpVaryDataOriginal b;
    http_vary_data_rust_test::Init(a, b, request, *response, kind);
    EXPECT_EQ(a.hash_type(), HttpVaryData::HashType::kSHA256);
    base::Pickle p;
    a.Persist(&p);
    base::PickleIterator iter(p);
    int id;
    ASSERT_TRUE(iter.ReadInt(&id));
    EXPECT_EQ(id, 1);
    auto bytes = iter.ReadBytes(32);
    ASSERT_TRUE(bytes);
    for (auto byte : *bytes) {
      EXPECT_EQ(byte, 0);
    }
    EXPECT_FALSE(a.MatchesRequest(request, *response));
  }
}
TEST(HttpVaryDataRustTest, PickleFailureRetentionAndConsumption) {
  HttpRequestHeaders request;
  request.SetHeader("foo", "one");
  auto response = http_vary_data_rust_test::Response("Vary: foo");
  for (int kind : {0, 1}) {
    for (size_t len = 0; len <= 72; ++len) {
      HttpVaryData a;
      HttpVaryDataOriginal b;
      http_vary_data_rust_test::Init(a, b, request, *response, kind);
      base::Pickle p;
      std::vector<uint8_t> bytes(len, 0xff);
      p.WriteBytes(bytes);
      http_vary_data_rust_test::Load(a, b, p);
      EXPECT_EQ(a.MatchesRequest(request, *response),
                b.MatchesRequest(request, *response));
    }
  }
}
TEST(HttpVaryDataRustTest, OwnedDigestOutlivesContainersAndPickle) {
  HttpVaryData a;
  HttpVaryDataOriginal b;
  {
    HttpRequestHeaders request;
    request.SetHeader("foo", "borrowed before mutation");
    auto response = http_vary_data_rust_test::Response("Vary: foo");
    http_vary_data_rust_test::Init(a, b, request, *response, 1);
  }
  http_vary_data_rust_test::Equal(a, b);
  auto copied = a;
  auto original = b;
  {
    base::Pickle p;
    a.Persist(&p);
    http_vary_data_rust_test::Load(copied, original, p);
    // Append to the very same backing Pickle after loading it. Neither
    // implementation may retain a digest view into the iterator's storage.
    copied.Persist(&p);
  }
  a = std::move(copied);
  b = std::move(original);
  http_vary_data_rust_test::Equal(a, b);
}
TEST(HttpVaryDataRustTest, RepeatedLargeFieldsStreamInOrder) {
  HttpRequestHeaders request;
  request.SetHeader("foo", std::string(65536, 'a'));
  request.SetHeader("empty", "");
  std::string vary = "Vary: missing,empty";
  for (size_t i = 0; i < 64; ++i) {
    vary.append(",foo");
  }
  auto response = http_vary_data_rust_test::Response(vary);
  for (int kind : {0, 1}) {
    HttpVaryData a;
    HttpVaryDataOriginal b;
    http_vary_data_rust_test::Init(a, b, request, *response, kind);
    EXPECT_TRUE(a.MatchesRequest(request, *response));
    request.SetHeader("empty", "changed");
    EXPECT_FALSE(a.MatchesRequest(request, *response));
    request.SetHeader("empty", "");
  }
}
}  // namespace net
