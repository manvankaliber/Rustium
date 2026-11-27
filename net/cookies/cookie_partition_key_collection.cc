// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cookies/cookie_partition_key_collection.h"
#include "net/cookies/cookie_family_native.h"

#include <optional>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "net/base/schemeful_site.h"
#include "net/cookies/cookie_access_delegate.h"
#include "net/cookies/cookie_partition_key.h"

namespace net {
namespace cf=cookie_family;

CookiePartitionKeyCollection::CookiePartitionKeyCollection()
    : CookiePartitionKeyCollection(base::flat_set<CookiePartitionKey>()) {}

CookiePartitionKeyCollection::CookiePartitionKeyCollection(
    CookiePartitionKey key)
    : CookiePartitionKeyCollection(
          base::flat_set<CookiePartitionKey>({std::move(key)})) {}

CookiePartitionKeyCollection::CookiePartitionKeyCollection(
    base::flat_set<CookiePartitionKey> keys)
    : state_(std::move(keys)) {}

CookiePartitionKeyCollection::CookiePartitionKeyCollection(PrivateTag,
                                                           InternalState state)
    : state_(std::move(state)) {}

CookiePartitionKeyCollection::CookiePartitionKeyCollection(
    std::optional<CookiePartitionKey> opt_key)
    : state_(opt_key ? base::flat_set<CookiePartitionKey>(
                           {std::move(opt_key).value()})
                     : base::flat_set<CookiePartitionKey>()) {}

CookiePartitionKeyCollection::CookiePartitionKeyCollection(
    const CookiePartitionKeyCollection& other) = default;

CookiePartitionKeyCollection::CookiePartitionKeyCollection(
    CookiePartitionKeyCollection&& other) = default;

CookiePartitionKeyCollection& CookiePartitionKeyCollection::operator=(
    const CookiePartitionKeyCollection& other) = default;

CookiePartitionKeyCollection& CookiePartitionKeyCollection::operator=(
    CookiePartitionKeyCollection&& other) = default;

CookiePartitionKeyCollection::~CookiePartitionKeyCollection() = default;

bool CookiePartitionKeyCollection::Contains(
    const CookiePartitionKey& key) const {
  CookieFamilyRequest request{};request.scalar[0]=state_.has_value();
  cf::NativeContext native;native.collection=this;native.partition=&key;return cf::Run(52,request,&native).scalar;
}

CookiePartitionKeyCollection CookiePartitionKeyCollection::MatchesSite(
    const net::SchemefulSite& top_level_site) {
  base::flat_set<CookiePartitionKey> keys;CookieFamilyRequest request{};
  cf::NativeContext native;native.site=&top_level_site;native.key_output=&keys;cf::Run(53,request,&native);
  return CookiePartitionKeyCollection(std::move(keys));
}

std::ostream& operator<<(std::ostream& os,
                         const CookiePartitionKeyCollection& keys) {
  cf::NativeContext native;native.collection=&keys;native.stream=&os;
  CookieFamilyRequest request{};cf::Run(54,request,&native);return os;
}

bool CookiePartitionKeyCollection::IsEmpty() const {
  CookieFamilyRequest request{};request.scalar[0]=state_.has_value();
  cf::NativeContext native;native.collection=this;return cf::Run(50,request,&native).scalar;
}
bool CookiePartitionKeyCollection::ContainsAllKeys() const {
  CookieFamilyRequest request{};request.scalar[0]=state_.has_value();return cf::Run(51,request).scalar;
}
bool operator==(const CookiePartitionKeyCollection& left,const CookiePartitionKeyCollection& right) {
  CookieFamilyRequest request{};request.scalar[0]=left.state_.has_value();request.scalar[1]=right.state_.has_value();
  cf::NativeContext native;native.collection=&left;native.other_collection=&right;return cf::Run(55,request,&native).scalar;
}

}  // namespace net
