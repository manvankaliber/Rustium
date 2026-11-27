// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/proxy_chain.h"

#include <algorithm>
#include <ostream>
#include <vector>

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/pickle.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "build/buildflag.h"
#include "net/base/proxy_server.h"
#include "net/base/proxy_string_util.h"
#include "net/base/proxy_value_rust_ffi.h"
#include "net/base/proxy_value_rust_ffi.rs.h"
#include "net/net_buildflags.h"

namespace net {

namespace pv = proxy_value_rust;

ProxyChain::ProxyChain() = default;

ProxyChain::ProxyChain(const ProxyChain& other) = default;
ProxyChain::ProxyChain(ProxyChain&& other) noexcept = default;

ProxyChain& ProxyChain::operator=(const ProxyChain& other) = default;
ProxyChain& ProxyChain::operator=(ProxyChain&& other) noexcept = default;
ProxyChain::~ProxyChain() = default;

ProxyChain::ProxyChain(ProxyServer proxy_server)
    : ProxyChain(std::vector<ProxyServer>{std::move(proxy_server)}) {}

ProxyChain::ProxyChain(ProxyServer::Scheme scheme,
                       const HostPortPair& host_port_pair)
    : ProxyChain(ProxyServer(scheme, host_port_pair)) {}

ProxyChain::ProxyChain(std::vector<ProxyServer> proxy_server_list)
    : proxy_server_list_(std::move(proxy_server_list)) {
  if (!IsValidInternal()) {
    proxy_server_list_ = std::nullopt;
  }
}

// static
std::optional<ProxyChain> ProxyChain::InitFromPickle(
    base::PickleIterator& pickle_iter) {
  pv::Reader reader{&pickle_iter};
  auto result = pv::read_chain(reader, pv::QuicEnabled(), pv::MultiEnabled());
  if (!result.valid) {
    return std::nullopt;
  }
  return pv::Restore(result);
}

void ProxyChain::Persist(base::Pickle* pickle) const {
  DCHECK(IsValid());
  pv::Writer writer{pickle};
  pv::persist_chain(writer, pv::Slice(pv::Snapshot(*this)));
}

const ProxyServer& ProxyChain::GetProxyServer(size_t chain_index) const {
  DCHECK(IsValid());
  CHECK_LT(chain_index, proxy_server_list_.value().size());
  return proxy_server_list_.value().at(chain_index);
}

const std::vector<ProxyServer>& ProxyChain::proxy_servers() const {
  DCHECK(IsValid());
  return proxy_server_list_.value();
}

std::pair<ProxyChain, const ProxyServer&> ProxyChain::SplitLast() const {
  DCHECK(IsValid());
  DCHECK_NE(length(), 0u);
  ProxyChain new_chain =
      ProxyChain({proxy_server_list_->begin(), proxy_server_list_->end() - 1},
                 opaque_data_);
  CHECK(new_chain.IsValid());
  return std::make_pair(std::move(new_chain),
                        std::ref(proxy_server_list_->back()));
}

ProxyChain ProxyChain::Prefix(size_t len) const {
  DCHECK(IsValid());
  DCHECK_LE(len, length());
  auto new_chain = ProxyChain(
      {proxy_server_list_->begin(), proxy_server_list_->begin() + len},
      opaque_data_);
  CHECK(new_chain.IsValid());
  return new_chain;
}

const ProxyServer& ProxyChain::First() const {
  DCHECK(IsValid());
  DCHECK_NE(length(), 0u);
  return proxy_server_list_->front();
}

const ProxyServer& ProxyChain::Last() const {
  DCHECK(IsValid());
  DCHECK_NE(length(), 0u);
  return proxy_server_list_->back();
}

std::string ProxyChain::ToDebugString() const {
  return pv::String(pv::debug_chain(IsValid(), pv::Slice(pv::Snapshot(*this)),
                                    opaque_data_.has_value(),
                                    opaque_data_.value_or(0)));
}

ProxyChain::ProxyChain(std::vector<ProxyServer> proxy_server_list,
                       std::optional<int> opaque_data)
    : proxy_server_list_(std::move(proxy_server_list)),
      opaque_data_(opaque_data) {
  if (!IsValidInternal()) {
    *this = ProxyChain();
  }
}

bool ProxyChain::IsValidInternal() const {
  return proxy_server_list_.has_value() &&
         pv::valid_chain(pv::Slice(pv::Snapshot(*this)), pv::QuicEnabled(),
                         pv::MultiEnabled());
}

bool ProxyChain::is_get_to_proxy_allowed() const {
  return pv::get_allowed(length(), is_single_proxy()
                                       ? First().scheme()
                                       : ProxyServer::SCHEME_INVALID);
}

std::strong_ordering operator<=>(const ProxyChain& a, const ProxyChain& b) {
  const int c = pv::compare_chain(
      a.IsValid(), pv::Slice(pv::Snapshot(a)), a.opaque_data_.has_value(),
      a.opaque_data_.value_or(0), b.IsValid(), pv::Slice(pv::Snapshot(b)),
      b.opaque_data_.has_value(), b.opaque_data_.value_or(0));
  return c < 0   ? std::strong_ordering::less
         : c > 0 ? std::strong_ordering::greater
                 : std::strong_ordering::equal;
}
bool operator==(const ProxyChain& a, const ProxyChain& b) {
  return (a <=> b) == 0;
}

std::ostream& operator<<(std::ostream& os, const ProxyChain& proxy_chain) {
  return os << proxy_chain.ToDebugString();
}

}  // namespace net
