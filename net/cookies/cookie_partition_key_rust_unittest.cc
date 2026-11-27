// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/cookies/cookie_partition_key_rust_test_support.h"

#include <compare>
#include <functional>
#include <streambuf>
#include <tuple>

#include "testing/gtest/include/gtest/gtest.h"

namespace net::cookie_partition_key_test {
namespace {
using Key = Production::Key;
using RefKey = Original::Key;
using Bit = Key::AncestorChainBit;
using RefBit = RefKey::AncestorChainBit;

static_assert(static_cast<int>(Bit::kSameSite) == 0);
static_assert(static_cast<int>(Bit::kCrossSite) == 1);
static_assert(static_cast<int>(RefBit::kSameSite) == 0);
static_assert(static_cast<int>(RefBit::kCrossSite) == 1);

TEST(CookiePartitionKeyRustTest, PersistedAncestorIdsAndBooleanMapping) {
  EXPECT_EQ(Key::BoolToAncestorChainBit(false), Bit::kSameSite);
  EXPECT_EQ(Key::BoolToAncestorChainBit(true), Bit::kCrossSite);
}

TEST(CookiePartitionKeyRustTest, ExactStorageAndUntrustedErrors) {
  for (bool cross : {false, true}) {
    auto empty = Key::FromStorage("", cross);
    ASSERT_TRUE(empty.has_value());
    EXPECT_FALSE(empty->has_value());
    auto absent = Key::Serialize(std::nullopt);
    ASSERT_TRUE(absent.has_value());
    EXPECT_EQ(absent->TopLevelSite(), "");
    EXPECT_TRUE(absent->has_cross_site_ancestor());
    EXPECT_EQ(absent->GetDebugString(), ", cross-site");
    auto bad = Key::FromUntrustedInput("", cross);
    ASSERT_FALSE(bad.has_value());
    EXPECT_EQ(bad.error(), kEmptyError);
    for (const char* text : {"Invalid", "example.com", "null", "data:text/plain,a"}) {
      auto strict = Key::FromStorage(text, cross);
      ASSERT_FALSE(strict.has_value()) << text;
      EXPECT_EQ(strict.error(), kOpaqueError);
      auto loose = Key::FromUntrustedInput(text, cross);
      ASSERT_FALSE(loose.has_value()) << text;
      EXPECT_EQ(loose.error(), kOpaqueError);
    }
    for (const char* text : {"https://example.com/", "https://sub.example.com/path",
                             "HTTPS://EXAMPLE.COM", "https://example.com:443"}) {
      auto strict = Key::FromStorage(text, cross);
      ASSERT_FALSE(strict.has_value()) << text;
      EXPECT_EQ(strict.error(), kMalformedError);
      auto loose = Key::FromUntrustedInput(text, cross);
      ASSERT_TRUE(loose.has_value()) << text;
      EXPECT_EQ(loose->site().Serialize(), "https://example.com");
      EXPECT_EQ(loose->IsThirdParty(), cross);
    }
  }
}

TEST(CookiePartitionKeyRustTest, InputBytesMatchOriginalWithoutTruncation) {
  std::vector<std::string> inputs = {"", "https://example.com", "file://",
      "file://server", "https://[::1]", "https://localhost:9000/", "https://com",
      "https://a.github.io/path", "about:blank", "blob:https://example.com/id",
      "https://example.com\r\n/", "\thttps://example.com ",
      std::string("https://example.com\0.attacker.test", 34),
      std::string(1, '\0'), std::string(1, static_cast<char>(0xff))};
  for (const std::string& input : inputs) {
    for (bool cross : {false, true}) {
      SCOPED_TRACE(::testing::PrintToString(input));
      auto a = Key::FromStorage(input, cross);
      auto b = RefKey::FromStorage(input, cross);
      ASSERT_EQ(a.has_value(), b.has_value());
      if (a.has_value()) {
        EXPECT_EQ(Digest(0, *a), Digest(0, *b));
      } else {
        EXPECT_EQ(a.error(), b.error());
      }
      auto x = Key::FromUntrustedInput(input, cross);
      auto y = RefKey::FromUntrustedInput(input, cross);
      ASSERT_EQ(x.has_value(), y.has_value());
      if (x.has_value()) {
        EXPECT_EQ(Digest(0, *x), Digest(0, *y));
      } else {
        EXPECT_EQ(x.error(), y.error());
      }
    }
  }
}

TEST(CookiePartitionKeyRustTest, WirePreservesOpaqueNonceAndAncestor) {
  Inputs in;
  for (const auto& origin : in.origins) {
    for (const auto& nonce : {std::optional<base::UnguessableToken>(),
                             std::optional(in.nonce1),
                             std::optional(base::UnguessableToken())}) {
      for (int raw : {0, 1, 2, -1}) {
        const auto bit = static_cast<Bit>(raw);
        const SchemefulSite site(origin);
        const auto key = Key::FromWire(site, bit, nonce);
        auto components = Key::FromStorageKeyComponents(site, bit, nonce);
        ASSERT_TRUE(components.has_value());
        EXPECT_EQ(key, *components);
        EXPECT_EQ(key.site(), site);
        EXPECT_EQ(key.nonce(), nonce);
        EXPECT_EQ(key.IsThirdParty(), raw == 1);
        EXPECT_EQ(key.ForbidsUnpartitionedCookieAccess(), nonce.has_value());
        EXPECT_EQ(key.IsSerializeable(), !site.opaque() && !nonce);
        auto serialized = Key::Serialize(key);
        EXPECT_EQ(serialized.has_value(), !site.opaque() && !nonce);
        if (!serialized) EXPECT_EQ(serialized.error(), kSerializeError);
      }
    }
  }
}

TEST(CookiePartitionKeyRustTest, ExactFileHostDebugAndStreamBytes) {
  Inputs in;
  struct Fixture { const char* url; const char* serialized; const char* stream; };
  const Fixture cases[] = {
      {"https://sub.example.com/path", "https://example.com", "https://example.com"},
      {"file:///tmp/file.txt", "file://", "file://"},
      {"file://server/path/file.txt", "file://server", "file://"}};
  for (const auto& c : cases) {
    for (bool cross : {false, true}) {
      auto key = Key::FromURLForTesting(GURL(c.url), cross ? Bit::kCrossSite : Bit::kSameSite);
      auto serialized = Key::Serialize(key);
      ASSERT_TRUE(serialized.has_value());
      EXPECT_EQ(serialized->TopLevelSite(), c.serialized);
      EXPECT_EQ(serialized->GetDebugString(), std::string(c.serialized) +
          (cross ? ", cross-site" : ", same-site"));
      std::ostringstream out;
      out << key;
      EXPECT_EQ(out.str(), std::string(c.stream) + (cross ? ",cross_site" : ",same_site"));
      auto nonced = Key::FromWire(key.site(), cross ? Bit::kCrossSite : Bit::kSameSite, in.nonce1);
      std::ostringstream nonce_out;
      nonce_out << nonced;
      EXPECT_EQ(nonce_out.str(), std::string(c.stream) + ",nonced" +
          (cross ? ",cross_site" : ",same_site"));
    }
  }
}

// Stream writes are observable to custom buffers and on partial failures.
// Preserve the original site -> optional nonce -> ancestor insertion sequence,
// formatting reset, byte prefix and state bits, not only a successful string.
struct StreamObservation {
  std::vector<std::string> calls;
  std::string written;
  std::ios_base::iostate state;
  std::streamsize remaining_width;
  bool operator==(const StreamObservation&) const = default;
};
class RecordingBuffer : public std::streambuf {
 public:
  explicit RecordingBuffer(size_t budget) : budget_(budget) {}
  std::vector<std::string> calls;
  std::string written;

 protected:
  std::streamsize xsputn(const char* source, std::streamsize length) override {
    calls.emplace_back("xsputn:" + std::string(source, static_cast<size_t>(length)));
    const size_t accepted = std::min(static_cast<size_t>(length), budget_);
    written.append(source, accepted);
    budget_ -= accepted;
    return static_cast<std::streamsize>(accepted);
  }
  int_type overflow(int_type character) override {
    if (traits_type::eq_int_type(character, traits_type::eof()))
      return traits_type::not_eof(character);
    const char c = traits_type::to_char_type(character);
    calls.emplace_back(std::string("overflow:") + c);
    if (!budget_) return traits_type::eof();
    written += c;
    --budget_;
    return character;
  }

 private:
  size_t budget_;
};

template <class KeyType>
StreamObservation ObserveInsertion(const KeyType& key, size_t budget,
                                   std::streamsize width) {
  RecordingBuffer buffer(budget);
  std::ostream out(&buffer);
  out.fill('_');
  out.width(width);
  out << key;
  return {std::move(buffer.calls), std::move(buffer.written), out.rdstate(), out.width()};
}

TEST(CookiePartitionKeyRustTest, StreamInsertionOrderAndPartialFailuresMatchOriginal) {
  Inputs in;
  for (const auto& origin : in.origins) {
    for (bool nonced : {false, true}) {
      for (bool cross : {false, true}) {
        const auto nonce = nonced ? std::optional(in.nonce1) : std::nullopt;
        const auto a = Key::FromWire(SchemefulSite(origin), cross ? Bit::kCrossSite : Bit::kSameSite, nonce);
        const auto b = RefKey::FromWire(Original::Site(origin), cross ? RefBit::kCrossSite : RefBit::kSameSite, nonce);
        for (std::streamsize width : {0, 40}) {
          for (size_t budget = 0; budget <= 72; ++budget) {
            EXPECT_EQ(ObserveInsertion(a, budget, width), ObserveInsertion(b, budget, width))
                << "budget=" << budget << ", width=" << width;
          }
        }
      }
    }
  }
  const auto a = Key::FromWire(SchemefulSite(in.origins[0]), Bit::kCrossSite, in.nonce1);
  const auto full = ObserveInsertion(a, 999, 0);
  EXPECT_EQ(full.written, "https://example.com,nonced,cross_site");
  EXPECT_EQ(full.state, std::ios_base::goodbit);
  EXPECT_EQ(full.calls, (std::vector<std::string>{
      "xsputn:https://example.com", "xsputn:,nonced", "xsputn:,cross_site"}));
}

class MutatingBuffer : public std::stringbuf {
 public:
  explicit MutatingBuffer(std::function<void()> mutate)
      : mutate_(std::move(mutate)) {}
  bool triggered = false;

 protected:
  std::streamsize xsputn(const char* source, std::streamsize length) override {
    const auto accepted = std::stringbuf::xsputn(source, length);
    if (!triggered && str().ends_with(",nonced")) {
      triggered = true;
      mutate_();
    }
    return accepted;
  }

 private:
  std::function<void()> mutate_;
};

template <class Types>
std::pair<std::string, bool> ObserveMutatingInsertion(const Inputs& in, bool cross) {
  using KeyType = typename Types::Key;
  using Site = typename Types::Site;
  using Ancestor = typename KeyType::AncestorChainBit;
  const Site site(in.origins[0]);
  KeyType key = KeyType::FromWire(site,
      cross ? Ancestor::kCrossSite : Ancestor::kSameSite, in.nonce1);
  // The source object is nonconst. An ostream callback may mutate it through
  // this other alias; the insertion's const reference does not prohibit that.
  MutatingBuffer buffer([&] {
    key = KeyType::FromWire(site,
        cross ? Ancestor::kSameSite : Ancestor::kCrossSite, std::nullopt);
  });
  std::ostream out(&buffer);
  out << key;
  return {buffer.str(), buffer.triggered};
}

TEST(CookiePartitionKeyRustTest, StreamCallbackObservesAncestorAfterNonceInsertion) {
  Inputs in;
  for (bool cross : {false, true}) {
    const auto actual = ObserveMutatingInsertion<Production>(in, cross);
    const auto expected = ObserveMutatingInsertion<Original>(in, cross);
    EXPECT_EQ(actual, expected);
    EXPECT_TRUE(actual.second);
    EXPECT_EQ(actual.first, std::string("https://example.com,nonced") +
        (cross ? ",same_site" : ",cross_site"));
  }
}

TEST(CookiePartitionKeyRustTest, NetworkIsolationSelectionAndAncestorPrecedence) {
  Inputs in;
  const SchemefulSite top(in.origins[0]), frame(in.origins[1]);
  for (bool nonced : {false, true}) {
    NetworkIsolationKey nik(top, frame, nonced ? std::optional(in.nonce1) : std::nullopt);
    for (bool main : {false, true}) {
      for (unsigned mode = 0; mode != 4; ++mode) {
        SiteForCookies sfc = mode == 0 ? SiteForCookies() : SiteForCookies(top);
        if (mode == 3) sfc.SetSchemefullySameForTesting(false);
        const SchemefulSite request = mode == 2 ? frame : top;
        auto key = Key::FromNetworkIsolationKey(nik, sfc, request, main);
        ASSERT_TRUE(key.has_value());
        EXPECT_EQ(key->site(), nonced ? frame : top);
        EXPECT_EQ(key->nonce(), nik.GetNonce());
        EXPECT_EQ(key->IsThirdParty(), nonced || (!main && (mode == 0 || mode == 2 || mode == 3)));
      }
    }
  }
  for (auto p : {NetworkIsolationPartition::kFedCmUncredentialedRequests,
                 NetworkIsolationPartition::kDnsOverHttps,
                 NetworkIsolationPartition::kSharedWorkerSameSiteCookiesNone}) {
    EXPECT_FALSE(Key::FromNetworkIsolationKey(NetworkIsolationKey(top, frame, in.nonce1, p),
        SiteForCookies(top), top, true));
    EXPECT_FALSE(Key::FromNetworkIsolationKey(NetworkIsolationKey::CreateEmptyWithPartition(p),
        SiteForCookies(top), top, false));
  }
  EXPECT_FALSE(Key::FromNetworkIsolationKey(NetworkIsolationKey(), SiteForCookies(), top, true));
}

TEST(CookiePartitionKeyRustTest, EqualityAndOrderingFollowFieldOrder) {
  Inputs in;
  std::vector<Key> a;
  std::vector<RefKey> b;
  for (const auto& origin : in.origins) {
    for (const auto& nonce : {std::optional<base::UnguessableToken>(),
                             std::optional(in.nonce1), std::optional(in.nonce2)}) {
      for (int raw : {0, 1, 2, -1}) {
        a.push_back(Key::FromWire(SchemefulSite(origin), static_cast<Bit>(raw), nonce));
        b.push_back(RefKey::FromWire(Original::Site(origin), static_cast<RefBit>(raw), nonce));
      }
    }
  }
  for (size_t i = 0; i < a.size(); ++i) {
    for (size_t j = 0; j < a.size(); ++j) {
      EXPECT_EQ(a[i] == a[j], b[i] == b[j]) << i << "," << j;
      EXPECT_EQ(a[i] < a[j], b[i] < b[j]) << i << "," << j;
      EXPECT_EQ(a[i] > a[j], b[i] > b[j]) << i << "," << j;
      EXPECT_EQ((a[i] <=> a[j]) == 0, (b[i] <=> b[j]) == 0);
    }
  }
  auto site = SchemefulSite(in.origins[0]);
  auto same = Key::FromWire(site, Bit::kSameSite);
  auto cross = Key::FromWire(site, Bit::kCrossSite);
  EXPECT_LT(same, cross);
  EXPECT_NE(same, cross);
  EXPECT_LT(cross, Key::FromWire(site, Bit::kSameSite, in.nonce1));
}

TEST(CookiePartitionKeyRustTest, CopyMoveSelfAssignmentAndAllPreparedWorkloads) {
  Inputs in;
  VerifyAllWorkloads(in);
}
}  // namespace
}  // namespace net::cookie_partition_key_test
