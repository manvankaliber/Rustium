// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_vary_data.h"

#include <algorithm>
#include <optional>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/stack_allocated.h"
#include "base/pickle.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_vary_data_rust_ffi.h"

namespace net {

crypto::obsolete::Md5 MakeMd5HasherForHttpVaryData() {
  return {};
}

namespace {
using Hash = std::variant<Md5Hash, Sha256Hash>;
using Hasher = std::variant<crypto::obsolete::Md5, crypto::hash::Hasher>;

HttpVaryDataRustState Snapshot(const Hash& hash, bool valid) {
  HttpVaryDataRustState state{};
  state.valid = valid;
  state.kind = std::holds_alternative<Sha256Hash>(hash) ? 1 : 0;
  std::visit([&](const auto& bytes) { std::ranges::copy(bytes, state.digest); },
             hash);
  return state;
}

void Commit(const HttpVaryDataRustState& state, Hash& hash, bool& valid) {
  if (state.kind == 1) {
    Sha256Hash bytes;
    base::span(bytes).copy_from(base::span(state.digest));
    hash = bytes;
  } else {
    Md5Hash bytes;
    base::span(bytes).copy_from(base::span(state.digest).first<16>());
    hash = bytes;
  }
  valid = state.valid != 0;
}

// Only explicit library/container primitives live here. Rust chooses the
// algorithm, fields, feed order, separators, format, probe and read/write
// order.
struct Services {
  STACK_ALLOCATED();

 public:
  raw_ptr<const HttpRequestHeaders> request = nullptr;
  raw_ptr<const HttpResponseHeaders> response = nullptr;
  raw_ptr<base::PickleIterator> iter = nullptr;
  raw_ptr<base::Pickle> pickle = nullptr;
  std::optional<base::PickleIterator> probe;
  std::array<uint8_t, 32> digest{};
  std::optional<Hasher> context;
};

HttpVaryDataRustReply View(std::optional<std::string_view> view,
                           size_t value = 0) {
  if (!view) {
    return {nullptr, 0, value};
  }
  // Present empty views need a non-null marker distinct from absence.
  static constexpr uint8_t kEmpty = 0;
  return {
      view->empty() ? &kEmpty : reinterpret_cast<const uint8_t*>(view->data()),
      view->size(), value};
}

HttpVaryDataRustReply Service(void* opaque,
                              uint32_t op,
                              const uint8_t* input,
                              size_t length,
                              size_t arg) {
  auto& services = *static_cast<Services*>(opaque);
  // SAFETY: Rust passes an initialized readable range for this synchronous
  // callback. Empty spans are normalized without constructing a null range.
  auto bytes = length ? UNSAFE_BUFFERS(base::span(input, length))
                      : base::span<const uint8_t>();
  switch (op) {
    case 0:
      return {nullptr, 0, services.response->HasHeaderValue("vary", "*")};
    case 1: {
      size_t cursor = arg;
      auto field = services.response->EnumerateHeader(&cursor, "vary");
      return View(field, cursor);
    }
    case 2:
      return View(services.request->GetHeaderView(base::as_string_view(bytes)));
    case 3:
      if (arg == 1) {
        services.context.emplace(std::in_place_type<crypto::hash::Hasher>,
                                 crypto::hash::HashKind::kSha256);
      } else {
        services.context.emplace(MakeMd5HasherForHttpVaryData());
      }
      return {};
    case 4:
      services.probe.emplace(*services.iter);
      return {};
    case 5:
      return {nullptr, 0, services.probe->ReadBytes(arg).has_value()};
    case 6: {
      int value = 0;
      bool ok = services.iter->ReadInt(&value);
      return {nullptr, ok ? 1u : 0u, static_cast<size_t>(value)};
    }
    case 7: {
      auto value = services.iter->ReadBytes(arg);
      return value ? HttpVaryDataRustReply{value->data(), value->size(), 0}
                   : HttpVaryDataRustReply{};
    }
    case 8:
      services.pickle->WriteInt(static_cast<int32_t>(arg));
      return {};
    case 9:
      services.pickle->WriteBytes(bytes);
      return {};
    case 10:
      std::visit([&](auto& context) { context.Update(bytes); },
                 *services.context);
      return {};
    case 11:
      if (arg == 1) {
        std::get<crypto::hash::Hasher>(*services.context)
            .Finish(services.digest);
        return {services.digest.data(), 32, 0};
      } else {
        std::get<crypto::obsolete::Md5>(*services.context)
            .Finish(base::span(services.digest).first<16>());
        return {services.digest.data(), 16, 0};
      }
  }
  CHECK(false);
}
}  // namespace

HttpVaryData::HttpVaryData() = default;

bool HttpVaryData::Init(const HttpRequestHeaders& request_headers,
                        const HttpResponseHeaders& response_headers,
                        HashType hash_type) {
  Services services;
  services.request = &request_headers;
  services.response = &response_headers;
  auto state =
      rustium_http_vary_data(Snapshot(hash_, is_valid_), &services, Service, 0,
                             static_cast<int32_t>(hash_type));
  Commit(state, hash_, is_valid_);
  return is_valid_;
}

bool HttpVaryData::InitFromPickle(base::PickleIterator* iter) {
  Services services;
  services.iter = iter;
  auto state = rustium_http_vary_data(Snapshot(hash_, is_valid_), &services,
                                      Service, 2, 0);
  Commit(state, hash_, is_valid_);
  return is_valid_;
}

void HttpVaryData::Persist(base::Pickle* pickle) const {
  DCHECK(is_valid());
  Services services;
  services.pickle = pickle;
  rustium_http_vary_data(Snapshot(hash_, is_valid_), &services, Service, 3, 0);
}

bool HttpVaryData::MatchesRequest(
    const HttpRequestHeaders& request_headers,
    const HttpResponseHeaders& cached_response_headers) const {
  Services services;
  services.request = &request_headers;
  services.response = &cached_response_headers;
  return rustium_http_vary_data(Snapshot(hash_, is_valid_), &services, Service,
                                1, 0)
             .valid != 0;
}
}  // namespace net
