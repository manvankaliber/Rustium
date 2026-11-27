// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/cookies/cookie_partition_key.h"

#include <compare>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>

#include "base/check.h"
#include "base/logging.h"
#include "base/types/optional_util.h"
#include "net/base/cronet_buildflags.h"
#include "net/base/network_isolation_key.h"
#include "net/base/network_isolation_partition.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_partition_key_rust_ffi.h"
#include "net/cookies/site_for_cookies.h"
#include "url/gurl.h"

#if !BUILDFLAG(CRONET_BUILD)
#include "mojo/public/cpp/bindings/default_construct_tag.h"
#endif

namespace net {
namespace {

namespace policy = cookie_partition_key_rust;

static_assert(static_cast<int32_t>(CookiePartitionKey::AncestorChainBit::kSameSite) == 0);
static_assert(static_cast<int32_t>(CookiePartitionKey::AncestorChainBit::kCrossSite) == 1);
static_assert(static_cast<int32_t>(NetworkIsolationPartition::kGeneral) == 0);

// This is call-scoped adapter storage, not CookiePartitionKey's representation.
// All optional/string output is initialized before Rust can request a service.
// Native objects stay native; Rust selects primitive operations synchronously.
struct PrimitiveContext {
  raw_ptr<const SchemefulSite> site = nullptr;
  raw_ptr<const SchemefulSite> right_site = nullptr;
  raw_ptr<const std::optional<base::UnguessableToken>> nonce = nullptr;
  raw_ptr<const std::optional<base::UnguessableToken>> right_nonce = nullptr;
  raw_ptr<const std::string> input = nullptr;
  raw_ptr<const NetworkIsolationKey> nik = nullptr;
  raw_ptr<const SiteForCookies> site_for_cookies = nullptr;
  raw_ptr<const SchemefulSite> request_site = nullptr;
  const std::optional<SchemefulSite>& (*get_frame)(const NetworkIsolationKey&) = nullptr;
  std::optional<SchemefulSite> parsed;
  std::string serialized;
};

int32_t OrderingSign(std::strong_ordering order) {
  return (order > 0) - (order < 0);
}

// The switch only dispatches native value/access/ownership services. It does
// not decide CookiePartitionKey validation, ordering, site choice or ancestors.
// Rust invokes only primitives whose required native inputs have been bound.
extern "C" int32_t CookiePartitionKeyPrimitive(void* opaque, uint32_t operation) {
  auto& context = *static_cast<PrimitiveContext*>(opaque);
  switch (operation) {
    case policy::kSiteOpaque:
      return context.site->opaque();
    case policy::kSiteIsFile:
      return context.site->GetURL().SchemeIsFile();
    case policy::kSiteSerialize:
      context.serialized = context.site->Serialize();
      return 0;
    case policy::kSiteSerializeFileHost:
      context.serialized = context.site->SerializeFileSiteWithHost();
      return 0;
    case policy::kSiteDeserialize:
      context.parsed.emplace(SchemefulSite::Deserialize(*context.input));
      context.site = &*context.parsed;
      return 0;
    case policy::kSerializedEqualsInput:
      return context.serialized == *context.input;
    case policy::kNikPartition:
      return static_cast<int32_t>(context.nik->GetNetworkIsolationPartition());
    case policy::kNikNoncePresent:
      context.nonce = &context.nik->GetNonce();
      return context.nonce->has_value();
    case policy::kNikSelectTop: {
      const auto& site = context.nik->GetTopFrameSite();
      context.site = site ? &*site : nullptr;
      return context.site != nullptr;
    }
    case policy::kNikSelectFrame: {
      const auto& site = context.get_frame(*context.nik);
      context.site = site ? &*site : nullptr;
      return context.site != nullptr;
    }
    case policy::kSfcIsNull:
      return context.site_for_cookies->IsNull();
    case policy::kSfcIsFirstParty:
      return context.site_for_cookies->IsFirstParty(context.request_site->GetURL());
    case policy::kSiteEqual:
      return *context.site == *context.right_site;
    case policy::kSiteCompare:
      return OrderingSign(*context.site <=> *context.right_site);
    case policy::kLeftNoncePresent:
      return context.nonce->has_value();
    case policy::kRightNoncePresent:
      return context.right_nonce->has_value();
    case policy::kNonceEqual:
      return context.nonce->value() == context.right_nonce->value();
    case policy::kNonceCompare:
      return OrderingSign(context.nonce->value() <=> context.right_nonce->value());
  }
  CHECK(false) << "Unknown CookiePartitionKey Rust primitive: " << operation;
}

std::string_view TextView(CookiePartitionKeyRustText text) {
  return {reinterpret_cast<const char*>(text.data), text.length};
}

base::unexpected<std::string> NativeError(CookiePartitionKeyRustResult result) {
  const std::string message(TextView(cookie_partition_key_rust_message(result.error)));
  for (uint32_t i = 0; i < result.warning_count; ++i) {
    DLOG(WARNING) << message;
  }
  return base::unexpected(message);
}

}  // namespace

CookiePartitionKey::SerializedCookiePartitionKey::SerializedCookiePartitionKey(
    base::PassKey<CookiePartitionKey> key,
    const std::string& site,
    bool has_cross_site_ancestor)
    : top_level_site_(site),
      has_cross_site_ancestor_(has_cross_site_ancestor) {}

const std::string&
CookiePartitionKey::SerializedCookiePartitionKey::TopLevelSite() const {
  return top_level_site_;
}

bool CookiePartitionKey::SerializedCookiePartitionKey::has_cross_site_ancestor()
    const {
  return has_cross_site_ancestor_;
}

std::string CookiePartitionKey::SerializedCookiePartitionKey::GetDebugString()
    const {
  std::string out = top_level_site_;
  out.append(TextView(cookie_partition_key_rust_serialized_suffix(
      has_cross_site_ancestor_)));
  return out;
}

#if !BUILDFLAG(CRONET_BUILD)
CookiePartitionKey::CookiePartitionKey(mojo::DefaultConstruct::Tag) {}
#endif

CookiePartitionKey::AncestorChainBit CookiePartitionKey::BoolToAncestorChainBit(
    bool cross_site) {
  return static_cast<AncestorChainBit>(cookie_partition_key_rust_ancestor(cross_site));
}

CookiePartitionKey::CookiePartitionKey(
    const SchemefulSite& site,
    std::optional<base::UnguessableToken> nonce,
    AncestorChainBit ancestor_chain_bit)
    : site_(site),
      nonce_(nonce),
      ancestor_chain_bit_(ancestor_chain_bit) {}

CookiePartitionKey::CookiePartitionKey(const CookiePartitionKey& other) = default;
CookiePartitionKey::CookiePartitionKey(CookiePartitionKey&& other) = default;
CookiePartitionKey& CookiePartitionKey::operator=(const CookiePartitionKey& other) = default;
CookiePartitionKey& CookiePartitionKey::operator=(CookiePartitionKey&& other) = default;
CookiePartitionKey::~CookiePartitionKey() = default;

bool operator==(const CookiePartitionKey& left, const CookiePartitionKey& right) {
  PrimitiveContext context;
  context.site = &left.site_;
  context.right_site = &right.site_;
  context.nonce = &left.nonce_;
  context.right_nonce = &right.nonce_;
  return cookie_partition_key_rust_equals(
      static_cast<int32_t>(left.ancestor_chain_bit_),
      static_cast<int32_t>(right.ancestor_chain_bit_), &context,
      CookiePartitionKeyPrimitive);
}

std::strong_ordering operator<=>(const CookiePartitionKey& left,
                                 const CookiePartitionKey& right) {
  PrimitiveContext context;
  context.site = &left.site_;
  context.right_site = &right.site_;
  context.nonce = &left.nonce_;
  context.right_nonce = &right.nonce_;
  return cookie_partition_key_rust_compare(
             static_cast<int32_t>(left.ancestor_chain_bit_),
             static_cast<int32_t>(right.ancestor_chain_bit_), &context,
             CookiePartitionKeyPrimitive) <=> 0;
}

base::expected<CookiePartitionKey::SerializedCookiePartitionKey, std::string>
CookiePartitionKey::Serialize(base::optional_ref<const CookiePartitionKey> in) {
  PrimitiveContext context;
  if (in) {
    context.site = &in->site_;
  }
  const auto result = cookie_partition_key_rust_serialize(
      in.has_value(), in && in->nonce_.has_value(),
      in ? static_cast<int32_t>(in->ancestor_chain_bit_) : 0, &context,
      CookiePartitionKeyPrimitive);
  if (result.status == policy::kError) {
    return NativeError(result);
  }
  return base::ok(SerializedCookiePartitionKey(
      base::PassKey<CookiePartitionKey>(),
      result.status == policy::kAbsent ? kEmptyCookiePartitionKey : context.serialized,
      result.ancestor == static_cast<int32_t>(AncestorChainBit::kCrossSite)));
}

std::optional<CookiePartitionKey> CookiePartitionKey::FromNetworkIsolationKey(
    const NetworkIsolationKey& network_isolation_key,
    const SiteForCookies& site_for_cookies,
    const SchemefulSite& request_site,
    bool main_frame_navigation) {
  PrimitiveContext context;
  context.nik = &network_isolation_key;
  context.site_for_cookies = &site_for_cookies;
  context.request_site = &request_site;
  // PassKey construction stays in the authorized CookiePartitionKey member.
  // It runs lazily only when Rust selects nonced frame-site construction.
  context.get_frame = [](const NetworkIsolationKey& key)
      -> const std::optional<SchemefulSite>& {
    return key.GetFrameSiteForCookiePartitionKey(
        NetworkIsolationKey::CookiePartitionKeyPassKey());
  };
  const auto result = cookie_partition_key_rust_from_nik(
      main_frame_navigation, &context, CookiePartitionKeyPrimitive);
  if (result.status == policy::kAbsent) {
    return std::nullopt;
  }
  return CookiePartitionKey(*context.site, *context.nonce,
                            static_cast<AncestorChainBit>(result.ancestor));
}

std::optional<CookiePartitionKey> CookiePartitionKey::FromStorageKeyComponents(
    const SchemefulSite& site,
    AncestorChainBit ancestor_chain_bit,
    base::optional_ref<const base::UnguessableToken> nonce) {
  return FromWire(site, ancestor_chain_bit, nonce.CopyAsOptional());
}

base::expected<std::optional<CookiePartitionKey>, std::string>
CookiePartitionKey::FromStorage(const std::string& top_level_site,
                                bool has_cross_site_ancestor) {
  PrimitiveContext context;
  context.input = &top_level_site;
  const auto result = cookie_partition_key_rust_parse(
      policy::kStorage, top_level_site.empty(), static_cast<int32_t>(has_cross_site_ancestor), &context,
      CookiePartitionKeyPrimitive);
  if (result.status == policy::kError) {
    return NativeError(result);
  }
  if (result.status == policy::kAbsent) {
    return base::ok(std::nullopt);
  }
  return base::ok(CookiePartitionKey(
      *context.site, std::nullopt, static_cast<AncestorChainBit>(result.ancestor)));
}

base::expected<CookiePartitionKey, std::string>
CookiePartitionKey::FromUntrustedInput(const std::string& top_level_site,
                                       bool has_cross_site_ancestor) {
  PrimitiveContext context;
  context.input = &top_level_site;
  const auto result = cookie_partition_key_rust_parse(
      policy::kUntrusted, top_level_site.empty(), static_cast<int32_t>(has_cross_site_ancestor), &context,
      CookiePartitionKeyPrimitive);
  if (result.status == policy::kError) {
    return NativeError(result);
  }
  return base::ok(CookiePartitionKey(
      *context.site, std::nullopt, static_cast<AncestorChainBit>(result.ancestor)));
}

base::expected<CookiePartitionKey, std::string>
CookiePartitionKey::DeserializeInternal(const std::string& top_level_site,
                                       AncestorChainBit has_cross_site_ancestor,
                                       ParsingMode parsing_mode) {
  static_assert(static_cast<uint32_t>(ParsingMode::kStrict) == policy::kInternalStrict);
  static_assert(static_cast<uint32_t>(ParsingMode::kLoose) == policy::kInternalLoose);
  PrimitiveContext context;
  context.input = &top_level_site;
  const auto result = cookie_partition_key_rust_parse(
      static_cast<uint32_t>(parsing_mode), top_level_site.empty(),
      static_cast<int32_t>(has_cross_site_ancestor), &context,
      CookiePartitionKeyPrimitive);
  if (result.status == policy::kError) {
    return NativeError(result);
  }
  return base::ok(CookiePartitionKey(
      *context.site, std::nullopt, static_cast<AncestorChainBit>(result.ancestor)));
}

bool CookiePartitionKey::IsSerializeable() const {
  return cookie_partition_key_rust_query(
      policy::kSerializeable, true, site_.opaque(), nonce_.has_value(),
      static_cast<int32_t>(ancestor_chain_bit_));
}

bool CookiePartitionKey::ForbidsUnpartitionedCookieAccess() const {
  return cookie_partition_key_rust_query(policy::kForbidsUnpartitioned, true, false,
                                         nonce_.has_value(), 0);
}

bool CookiePartitionKey::HasNonce(base::optional_ref<const CookiePartitionKey> key) {
  return cookie_partition_key_rust_query(policy::kHasNonce, key.has_value(), false,
                                         key && key->nonce_.has_value(), 0);
}

bool CookiePartitionKey::IsThirdParty() const {
  return cookie_partition_key_rust_query(
      policy::kThirdParty, true, false, false,
      static_cast<int32_t>(ancestor_chain_bit_));
}

std::ostream& operator<<(std::ostream& os, const CookiePartitionKey& cpk) {
  os << cpk.site();
  const auto nonce_suffix =
      cookie_partition_key_rust_stream_suffix(cpk.nonce().has_value(), 0).nonce;
  if (nonce_suffix.length != 0) {
    os << TextView(nonce_suffix);
  }
  // Streambuf callbacks may mutate an originally nonconst key through another
  // alias during the nonce insertion. Read the ancestor only after that write,
  // as the original implementation did; no key borrow spans a Rust call/write.
  return os << TextView(cookie_partition_key_rust_stream_suffix(
                           false, cpk.IsThirdParty() ? 1 : 0)
                           .ancestor);
}

}  // namespace net
