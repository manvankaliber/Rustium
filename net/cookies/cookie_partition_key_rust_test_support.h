// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_COOKIES_COOKIE_PARTITION_KEY_RUST_TEST_SUPPORT_H_
#define NET_COOKIES_COOKIE_PARTITION_KEY_RUST_TEST_SUPPORT_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "net/base/network_isolation_key.h"
#include "net/base/network_isolation_partition.h"
#include "net/cookies/cookie_partition_key.h"
#include "net/cookies/cookie_partition_key_rust_reference.h"
#include "net/cookies/cookie_partition_key_rust_reference_network_isolation_key.h"
#include "net/cookies/cookie_partition_key_rust_reference_network_isolation_partition.h"
#include "net/cookies/cookie_partition_key_rust_reference_site_for_cookies.h"
#include "net/cookies/site_for_cookies.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net::cookie_partition_key_test {
namespace original = net::cookie_partition_key_reference;

struct Production {
  using Key = net::CookiePartitionKey;
  using Site = net::SchemefulSite;
  using Sfc = net::SiteForCookies;
  using Nik = net::NetworkIsolationKey;
  using Partition = net::NetworkIsolationPartition;
};
struct Original {
  using Key = original::CookiePartitionKey;
  using Site = original::SchemefulSite;
  using Sfc = original::SiteForCookies;
  using Nik = original::NetworkIsolationKey;
  using Partition = original::NetworkIsolationPartition;
};

inline constexpr std::string_view kOpaqueError =
    "Cannot deserialize opaque origin to CookiePartitionKey";
inline constexpr std::string_view kMalformedError =
    "Cannot deserialize malformed top_level_site to CookiePartitionKey";
inline constexpr std::string_view kEmptyError =
    "top_level_site is unexpectedly empty";
inline constexpr std::string_view kSerializeError =
    "CookiePartitionKey is not serializeable";

// All origin/token fixtures are created before either implementation's run.
// The SAME opaque origins are copied into both independent native site types;
// a baseline cannot accidentally compare independently generated opaque IDs.
struct Inputs {
  const std::array<std::string, 7> canonical = {
      "https://example.com", "http://example.com", "https://localhost",
      "https://127.0.0.1", "https://[::1]", "file://", "file://server"};
  const std::array<std::string, 8> normalize = {
      "https://example.com/", "https://sub.example.com/path?q=v#f",
      "HTTPS://EXAMPLE.COM", "https://example.com:443",
      "https://user:pass@example.com/path", "file:///tmp/file.txt",
      "file://server/path/file.txt", "https://localhost:8000/path"};
  const std::array<std::string, 8> rejected = {
      "Invalid", "example.com", "abc123foobar!!", "null",
      "data:text/plain,hello", "about:blank", "https://", ""};
  const std::array<url::Origin, 5> origins = {
      url::Origin::Create(GURL("https://example.com/path")),
      url::Origin::Create(GURL("http://other.test/path")),
      url::Origin::Create(GURL("file://server/path")),
      url::Origin(), url::Origin()};
  const base::UnguessableToken nonce1 =
      base::UnguessableToken::Deserialize(0x1111111111111111ULL, 7).value();
  const base::UnguessableToken nonce2 =
      base::UnguessableToken::Deserialize(0x2222222222222222ULL, 9).value();
};

inline uint64_t Mix(uint64_t hash, uint64_t value) {
  return (hash ^ value) * 1099511628211ULL;
}
inline uint64_t Bytes(uint64_t hash, std::string_view value) {
  hash = Mix(hash, value.size());
  for (unsigned char c : value) {
    hash = Mix(hash, c);
  }
  return hash;
}

// Digest every observable field and exact output/error byte sequence. The
// nonce's native empty value is legal in direct key construction even though
// NIK rejects it; avoid invoking its serialization DCHECK for that value.
template <class Key>
uint64_t Digest(uint64_t hash, const std::optional<Key>& key) {
  hash = Mix(hash, key.has_value());
  hash = Mix(hash, Key::HasNonce(key));
  if (key) {
    hash = Mix(hash, key->site().opaque());
    hash = Bytes(hash, key->site().Serialize());
    hash = Mix(hash, key->IsThirdParty());
    hash = Mix(hash, key->IsSerializeable());
    hash = Mix(hash, key->ForbidsUnpartitionedCookieAccess());
    if (key->nonce()) {
      hash = Mix(hash, key->nonce()->is_empty());
      if (!key->nonce()->is_empty()) {
        hash = Mix(hash, key->nonce()->GetHighForSerialization());
        hash = Mix(hash, key->nonce()->GetLowForSerialization());
      }
    }
    std::ostringstream stream;
    stream << *key;
    hash = Bytes(hash, stream.str());
  }
  auto result = Key::Serialize(key);
  hash = Mix(hash, result.has_value());
  if (result) {
    hash = Bytes(hash, result->TopLevelSite());
    hash = Mix(hash, result->has_cross_site_ancestor());
    hash = Bytes(hash, result->GetDebugString());
  } else {
    hash = Bytes(hash, result.error());
  }
  return hash;
}

template <class Key>
uint64_t Digest(uint64_t hash, const Key& key) {
  return Digest(hash, std::optional<Key>(key));
}

enum class Workload {
  kStorageEmpty,
  kStorageCanonical,
  kStorageRejected,
  kUntrustedNormalize,
  kUntrustedRejected,
  kNikFirstParty,
  kNikCrossSite,
  kNikNonce,
  kNikRejected,
  kStorageComponents,
  kSerializeSites,
  kSerializeRejected,
  kComparison,
  kCopyMove,
  kDebug,
};
struct Case {
  const char* key;
  const char* label;
  Workload workload;
};
inline constexpr std::array<Case, 15> kCases = {{
    {"storage_empty", "Empty persisted sentinel", Workload::kStorageEmpty},
    {"storage_canonical", "Seven canonical storage sites", Workload::kStorageCanonical},
    {"storage_rejected", "Opaque and noncanonical storage", Workload::kStorageRejected},
    {"untrusted_normalize", "Eight loose input normalizations", Workload::kUntrustedNormalize},
    {"untrusted_rejected", "Eight invalid loose inputs", Workload::kUntrustedRejected},
    {"nik_first_party", "Top-site and main-frame derivation", Workload::kNikFirstParty},
    {"nik_cross_site", "Null, cross-site and revised ancestors", Workload::kNikCrossSite},
    {"nik_nonce_frame", "Nonce frame-site precedence", Workload::kNikNonce},
    {"nik_rejected", "Empty and special NIK partitions", Workload::kNikRejected},
    {"storage_components", "Wire/storage components with nonce and opaque sites", Workload::kStorageComponents},
    {"serialize_sites", "HTTP, local file and file-host serialization", Workload::kSerializeSites},
    {"serialize_rejected", "Opaque and nonce serialization failures", Workload::kSerializeRejected},
    {"comparison_order", "Equality, ordering and ordered storage", Workload::kComparison},
    {"copy_move", "Copy/move/assignment lifecycle", Workload::kCopyMove},
    {"debug_outputs", "Exact serialized and stream text", Workload::kDebug},
}};

template <class Types>
NOINLINE uint64_t Run(const Case& c, const Inputs& in, size_t iterations) {
  using Key = typename Types::Key;
  using Site = typename Types::Site;
  using Sfc = typename Types::Sfc;
  using Nik = typename Types::Nik;
  using Partition = typename Types::Partition;
  using Bit = typename Key::AncestorChainBit;
  uint64_t hash = 1469598103934665603ULL;
  for (size_t i = 0; i < iterations; ++i) {
    const bool cross = (i & 1) != 0;
    const Bit bit = cross ? Bit::kCrossSite : Bit::kSameSite;
    // All local keys/strings/native owners are destroyed on every iteration.
    switch (c.workload) {
      case Workload::kStorageEmpty: {
        auto result = Key::FromStorage(std::string(), cross);
        hash = Mix(hash, result.has_value());
        if (result.has_value()) hash = Digest(hash, *result);
        else hash = Bytes(hash, result.error());
        break;
      }
      case Workload::kStorageCanonical:
      case Workload::kStorageRejected: {
        const bool valid = c.workload == Workload::kStorageCanonical;
        const std::string input = valid ? in.canonical[i % in.canonical.size()]
            : ((i & 2) ? in.normalize[i % in.normalize.size()]
                       : in.rejected[i % (in.rejected.size() - 1)]);
        auto result = Key::FromStorage(input, cross);
        hash = Mix(hash, result.has_value());
        if (result.has_value()) hash = Digest(hash, *result);
        else hash = Bytes(hash, result.error());
        break;
      }
      case Workload::kUntrustedNormalize:
      case Workload::kUntrustedRejected: {
        const std::string input = c.workload == Workload::kUntrustedNormalize
            ? in.normalize[i % in.normalize.size()]
            : in.rejected[i % in.rejected.size()];
        auto result = Key::FromUntrustedInput(input, cross);
        hash = Mix(hash, result.has_value());
        if (result.has_value()) hash = Digest(hash, *result);
        else hash = Bytes(hash, result.error());
        break;
      }
      case Workload::kNikFirstParty:
      case Workload::kNikCrossSite:
      case Workload::kNikNonce:
      case Workload::kNikRejected: {
        const Site top(in.origins[0]);
        const Site frame(in.origins[1]);
        const bool nonced = c.workload == Workload::kNikNonce;
        const bool rejected = c.workload == Workload::kNikRejected;
        constexpr std::array<Partition, 4> partitions = {
            Partition::kGeneral,
            Partition::kFedCmUncredentialedRequests,
            Partition::kDnsOverHttps,
            Partition::kSharedWorkerSameSiteCookiesNone};
        const auto partition = rejected ? partitions[i % partitions.size()]
                                        : Partition::kGeneral;
        Nik nik = rejected && (i % partitions.size()) == 0 ? Nik()
            : Nik(top, frame, nonced ? std::optional(in.nonce1) : std::nullopt,
                  partition);
        Sfc sfc = c.workload == Workload::kNikCrossSite && (i % 3) == 0
                      ? Sfc() : Sfc(top);
        if (c.workload == Workload::kNikCrossSite && (i % 3) == 1)
          sfc.SetSchemefullySameForTesting(false);
        const Site request = c.workload == Workload::kNikCrossSite ? frame : top;
        auto key = Key::FromNetworkIsolationKey(nik, sfc, request, cross);
        hash = Digest(hash, key);
        break;
      }
      case Workload::kStorageComponents: {
        const Site site(in.origins[i % in.origins.size()]);
        const std::optional<base::UnguessableToken> nonce =
            (i % 3) == 0 ? std::optional(in.nonce1) : std::nullopt;
        const Key wire = Key::FromWire(site, bit, nonce);
        auto stored = Key::FromStorageKeyComponents(site, bit, nonce);
        hash = Mix(hash, stored.has_value());
        hash = Mix(hash, stored && *stored == wire);
        hash = Digest(hash, wire);
        hash = Digest(hash, stored);
        break;
      }
      case Workload::kSerializeSites: {
        const Key key = Key::FromURLForTesting(
            GURL(in.canonical[i % in.canonical.size()]), bit);
        hash = Digest(hash, key);
        break;
      }
      case Workload::kSerializeRejected: {
        const Site site(in.origins[cross ? 0 : 3]);
        const Key key = Key::FromWire(site, bit,
            cross ? std::optional(in.nonce1) : std::nullopt);
        hash = Digest(hash, key);
        break;
      }
      case Workload::kComparison: {
        std::vector<Key> keys;
        for (const auto& origin : in.origins) {
          for (const auto& nonce : {std::optional<base::UnguessableToken>(),
                                   std::optional(in.nonce1), std::optional(in.nonce2)}) {
            keys.push_back(Key::FromWire(Site(origin), Bit::kSameSite, nonce));
            keys.push_back(Key::FromWire(Site(origin), Bit::kCrossSite, nonce));
          }
        }
        std::rotate(keys.begin(), keys.begin() + i % keys.size(), keys.end());
        std::sort(keys.begin(), keys.end());
        for (size_t j = 0; j < keys.size(); ++j) {
          hash = Digest(hash, keys[j]);
          hash = Mix(hash, keys[j] == keys[j]);
          if (j) {
            hash = Mix(hash, keys[j - 1] == keys[j]);
            hash = Mix(hash, keys[j - 1] < keys[j]);
            hash = Mix(hash, keys[j] > keys[j - 1]);
          }
        }
        break;
      }
      case Workload::kCopyMove: {
        Key source = Key::FromWire(Site(in.origins[i % in.origins.size()]), bit,
            cross ? std::optional(in.nonce1) : std::nullopt);
        Key copied(source);
        Key moved(std::move(copied));
        Key assigned = Key::FromWire(Site(in.origins[1]), Bit::kCrossSite);
        assigned = source;
        Key move_assigned = source;
        move_assigned = std::move(moved);
        const Key* alias = &assigned;
        assigned = *alias;
        hash = Mix(hash, source == assigned);
        hash = Mix(hash, source == move_assigned);
        hash = Digest(hash, assigned);
        hash = Digest(hash, move_assigned);
        break;
      }
      case Workload::kDebug: {
        const Key key = Key::FromWire(Site(in.origins[i % in.origins.size()]), bit,
            cross ? std::optional(in.nonce1) : std::nullopt);
        hash = Digest(hash, key);
        break;
      }
    }
  }
  return hash;
}

template <class Types>
void VerifyCanonicalOutcomes(const Inputs& inputs) {
  using Key = typename Types::Key;
  using Site = typename Types::Site;
  using Bit = typename Key::AncestorChainBit;
  for (bool cross : {false, true}) {
    auto absent = Key::FromStorage("", cross);
    CHECK(absent.has_value());
    CHECK(!absent->has_value());
    auto absent_serialized = Key::Serialize(std::nullopt);
    CHECK(absent_serialized.has_value());
    CHECK(absent_serialized->TopLevelSite().empty());
    CHECK(absent_serialized->has_cross_site_ancestor());
    for (const std::string& input : inputs.canonical) {
      auto parsed = Key::FromStorage(input, cross);
      CHECK(parsed.has_value()) << input;
      CHECK(parsed->has_value());
      CHECK_EQ((*parsed)->IsThirdParty(), cross);
      auto serialized = Key::Serialize(*parsed);
      CHECK(serialized.has_value());
      CHECK_EQ(serialized->TopLevelSite(), input);
      CHECK_EQ(serialized->has_cross_site_ancestor(), cross);
    }
    auto empty_error = Key::FromUntrustedInput("", cross);
    CHECK(!empty_error.has_value());
    CHECK_EQ(empty_error.error(), kEmptyError);
    auto invalid = Key::FromStorage("Invalid", cross);
    CHECK(!invalid.has_value());
    CHECK_EQ(invalid.error(), kOpaqueError);
    auto malformed = Key::FromStorage("https://example.com/", cross);
    CHECK(!malformed.has_value());
    CHECK_EQ(malformed.error(), kMalformedError);
    auto normalized = Key::FromUntrustedInput("https://sub.example.com/path", cross);
    CHECK(normalized.has_value());
    CHECK_EQ(normalized->site().Serialize(), "https://example.com");
    const Key nonce_key = Key::FromWire(Site(inputs.origins[0]),
        cross ? Bit::kCrossSite : Bit::kSameSite, inputs.nonce1);
    CHECK(nonce_key.ForbidsUnpartitionedCookieAccess());
    CHECK(!nonce_key.IsSerializeable());
    CHECK_EQ(nonce_key.IsThirdParty(), cross);
    auto nonce_serialized = Key::Serialize(nonce_key);
    CHECK(!nonce_serialized.has_value());
    CHECK_EQ(nonce_serialized.error(), kSerializeError);
  }
}

inline void VerifyAllWorkloads(const Inputs& inputs) {
  VerifyCanonicalOutcomes<Original>(inputs);
  VerifyCanonicalOutcomes<Production>(inputs);
  for (const Case& c : kCases) {
    CHECK_EQ(Run<Original>(c, inputs, 37), Run<Production>(c, inputs, 37)) << c.key;
  }
}
}  // namespace net::cookie_partition_key_test
#endif
