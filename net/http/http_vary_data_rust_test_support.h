// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_VARY_DATA_RUST_TEST_SUPPORT_H_
#define NET_HTTP_HTTP_VARY_DATA_RUST_TEST_SUPPORT_H_
#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>

#include "base/pickle.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_vary_data.h"
#include "net/http/http_vary_data_rust_reference.h"

namespace net::http_vary_data_rust_test {
inline void Require(bool ok) {
  if (!ok) {
    std::abort();
  }
}
inline auto Response(std::string_view vary) {
  std::string raw = "HTTP/1.1 200 OK\n";
  raw.append(vary);
  raw.append("\n\n");
  std::replace(raw.begin(), raw.end(), '\n', '\0');
  return base::MakeRefCounted<HttpResponseHeaders>(raw);
}
inline void Equal(const HttpVaryData& a, const HttpVaryDataOriginal& b) {
  Require(a.is_valid() == b.is_valid());
  Require(static_cast<int>(a.hash_type()) == static_cast<int>(b.hash_type()));
  if (a.is_valid()) {
    base::Pickle x, y;
    a.Persist(&x);
    b.Persist(&y);
    Require(x.size() == y.size());
    Require(base::span(x) == base::span(y));
  }
}
inline void Init(HttpVaryData& a,
                 HttpVaryDataOriginal& b,
                 const HttpRequestHeaders& request,
                 const HttpResponseHeaders& response,
                 int kind) {
  Require(
      a.Init(request, response, static_cast<HttpVaryData::HashType>(kind)) ==
      b.Init(request, response,
             static_cast<HttpVaryDataOriginal::HashType>(kind)));
  Equal(a, b);
  Require(a.MatchesRequest(request, response) ==
          b.MatchesRequest(request, response));
}
inline void Load(HttpVaryData& a,
                 HttpVaryDataOriginal& b,
                 const base::Pickle& p,
                 size_t skip = 0) {
  base::PickleIterator x(p), y(p);
  Require(x.ReadBytes(skip).has_value() == y.ReadBytes(skip).has_value());
  Require(a.InitFromPickle(&x) == b.InitFromPickle(&y));
  Equal(a, b);
  Require(x.RemainingBytes() == y.RemainingBytes());
  while (x.RemainingBytes()) {
    auto xv = x.ReadBytes(1), yv = y.ReadBytes(1);
    Require(xv.has_value() == yv.has_value());
    if (xv) {
      Require(*xv == *yv);
    }
    Require(x.RemainingBytes() == y.RemainingBytes());
  }
}
inline void Check(base::span<const uint8_t> input) {
  HttpVaryData a;
  HttpVaryDataOriginal b;
  Equal(a, b);
  HttpRequestHeaders request;
  std::string value(base::as_string_view(input));
  for (char& c : value) {
    if (c == '\0' || c == '\r' || c == '\n') {
      c = ' ';
    }
  }
  request.SetHeader("Foo", value);
  request.SetHeader("Bar", "");
  const std::string_view variants[] = {"",
                                       "Vary:",
                                       "Vary: foo",
                                       "Vary: missing",
                                       "Vary: foo,bar,missing",
                                       "Vary: *,foo",
                                       "Vary: foo\nVary: bar,foo",
                                       "Vary: , ,",
                                       "Vary: \"*\"",
                                       "Vary: foo,,bar",
                                       "Vary: foo\nVary: *",
                                       "Vary: Foo,FOO,foo"};
  const auto mode = input.empty() ? 0 : input.front();
  auto response = mode % (std::size(variants) + 1) == std::size(variants)
                      ? Response("Vary: " + value)
                      : Response(variants[mode % std::size(variants)]);
  const int kind = input.size() > 1 ? static_cast<int>(input[1] % 3) : 1;
  Init(a, b, request, *response, kind);
  // Matches never mutates stored state. Inputs are changed after
  // initialization.
  request.SetHeader("Foo", "changed");
  Require(a.MatchesRequest(request, *response) ==
          b.MatchesRequest(request, *response));
  Equal(a, b);
  if (a.is_valid()) {
    base::Pickle x;
    a.Persist(&x);
    HttpVaryData copy(a);
    HttpVaryDataOriginal original(b);
    Load(copy, original, x);
    // Embedded persistence: surrounding values remain consumable.
    base::Pickle embedded;
    embedded.WriteInt(17);
    a.Persist(&embedded);
    embedded.WriteInt(99);
    Load(copy, original, embedded, 4);
  }
  // Partial current records, exact 16-byte legacy records and arbitrary bytes.
  base::Pickle raw;
  raw.WriteBytes(input);
  Load(a, b, raw);
  // Exact, unpadded and deliberately unaligned serialized input. WithData
  // copies the base Pickle header and accepts a byte-aligned backing span.
  std::vector<uint8_t> unaligned(1 + sizeof(uint32_t) + input.size());
  uint32_t payload_size = static_cast<uint32_t>(input.size());
  auto serialized = base::span(unaligned).subspan<1>();
  serialized.first<4>().copy_from(base::byte_span_from_ref(payload_size));
  serialized.subspan<4>().copy_from(input);
  auto unaligned_a = base::PickleIterator::WithData(serialized);
  auto unaligned_b = base::PickleIterator::WithData(serialized);
  Require(a.InitFromPickle(&unaligned_a) == b.InitFromPickle(&unaligned_b));
  Equal(a, b);
  Require(unaligned_a.RemainingBytes() == unaligned_b.RemainingBytes());
  Require(a.MatchesRequest(request, *response) ==
          b.MatchesRequest(request, *response));
  for (int stored_kind : {0, 1, 2, -1}) {
    base::Pickle p;
    p.WriteInt(stored_kind);
    p.WriteBytes(input);
    Load(a, b, p);
  }
  // A failed Init retains the previous digest/type, even from a loaded record.
  auto empty = Response("");
  Init(a, b, request, *empty, 1);
  Require(a.MatchesRequest(request, *response) ==
          b.MatchesRequest(request, *response));
  Init(a, b, request, *response, 1);
  Equal(a, b);
}
}  // namespace net::http_vary_data_rust_test
#endif
