// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_DNS_RESPONSE_RUST_TEST_SUPPORT_H_
#define NET_DNS_DNS_RESPONSE_RUST_TEST_SUPPORT_H_
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "net/base/io_buffer.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_response.h"
#include "net/dns/dns_response_rust_query_reference.h"
#include "net/dns/dns_response_rust_reference.h"
namespace net::dns_response_rust_test {
static_assert(sizeof(DnsResponse) == sizeof(DnsResponseRustReference));
static_assert(sizeof(DnsQuery) == sizeof(DnsResponseRustReferenceQuery));
static_assert(sizeof(DnsRecordParser) ==
              sizeof(DnsResponseRustReferenceParser));
static_assert(sizeof(DnsResourceRecord) ==
              sizeof(DnsResponseRustReferenceRecord));
inline auto Packet(base::span<const uint8_t> data, size_t tail = 0) {
  auto b = base::MakeRefCounted<IOBufferWithSize>(data.size() + tail);
  b->span().first(data.size()).copy_from(data);
  // Tail deliberately uninitialized: no Rust &[u8] may include it.
  return b;
}
template <class A, class B>
void EqualRecord(const A& a, const B& b) {
  CHECK_EQ(a.name, b.name);
  CHECK_EQ(a.type, b.type);
  CHECK_EQ(a.klass, b.klass);
  CHECK_EQ(a.ttl, b.ttl);
  CHECK(a.rdata == b.rdata);
  CHECK(a.owned_rdata == b.owned_rdata);
}
template <class A, class B>
void Parser(A a, B b) {
  CHECK_EQ(a.IsValid(), b.IsValid());
  CHECK_EQ(a.AtEnd(), b.AtEnd());
  CHECK_EQ(a.GetOffset(), b.GetOffset());
  if (!a.IsValid()) {
    return;
  }
  for (unsigned i = 0; i < 8; ++i) {
    DnsResourceRecord x;
    DnsResponseRustReferenceRecord y;
    x.name = y.name = "sentinel";
    x.type = y.type = 123;
    x.klass = y.klass = 456;
    x.ttl = y.ttl = 789;
    x.SetOwnedRdata(std::to_array<uint8_t>({7, 8, 9}));
    y.SetOwnedRdata(std::to_array<uint8_t>({7, 8, 9}));
    bool ok = a.ReadRecord(&x);
    CHECK_EQ(ok, b.ReadRecord(&y));
    EqualRecord(x, y);
    CHECK_EQ(a.GetOffset(), b.GetOffset());
    CHECK_EQ(a.AtEnd(), b.AtEnd());
    if (!ok) {
      break;
    }
  }
}
template <class A, class B>
void EqualResponse(const A& a, const B& b, bool vectors = true) {
  CHECK_EQ(a.IsValid(), b.IsValid());
  CHECK(a.id() == b.id());
  if (!a.IsValid()) {
    return;
  }
  CHECK_EQ(a.flags(), b.flags());
  CHECK_EQ(a.rcode(), b.rcode());
  CHECK_EQ(a.question_count(), b.question_count());
  CHECK_EQ(a.answer_count(), b.answer_count());
  CHECK_EQ(a.authority_count(), b.authority_count());
  CHECK_EQ(a.additional_answer_count(), b.additional_answer_count());
  if (vectors) {
    CHECK(a.qtypes() == b.qtypes());
    CHECK(a.dotted_qnames() == b.dotted_qnames());
    if (a.question_count() == 1) {
      CHECK_EQ(a.GetSingleQType(), b.GetSingleQType());
      CHECK_EQ(a.GetSingleDottedName(), b.GetSingleDottedName());
    }
  }
  Parser(a.Parser(), b.Parser());
}
inline void Differential(base::span<const uint8_t> data) {
  const size_t offset = data.empty() ? 0 : data.back() % (data.size() + 1);
  DnsRecordParser a(data, offset, 4);
  DnsResponseRustReferenceParser b(data, offset, 4);
  std::string x = "sentinel", y = x;
  CHECK_EQ(a.ReadName(data.subspan(offset).data(), &x),
           b.ReadName(data.subspan(offset).data(), &y));
  CHECK_EQ(x, y);
  CHECK_EQ(a.ReadName(data.subspan(offset).data(), nullptr),
           b.ReadName(data.subspan(offset).data(), nullptr));
  Parser(a, b);
  Parser(DnsRecordParser(data, 0, 4),
         DnsResponseRustReferenceParser(data, 0, 4));
  uint16_t xt = 888, yt = 888;
  x = y = "question";
  CHECK_EQ(a.ReadQuestion(x, xt), b.ReadQuestion(y, yt));
  CHECK_EQ(x, y);
  CHECK_EQ(xt, yt);
  CHECK_EQ(a.GetOffset(), b.GetOffset());
  auto ab = Packet(data, 37);
  auto bb = Packet(data, 37);
  DnsResponse ar(ab, ab->size());
  DnsResponseRustReference br(bb, bb->size());
  CHECK_EQ(ar.InitParseWithoutQuery(data.size()),
           br.InitParseWithoutQuery(data.size()));
  EqualResponse(ar, br);
  // Reparse failure preserves ID and prior parser unless question parsing
  // started.
  CHECK_EQ(ar.InitParseWithoutQuery(0), br.InitParseWithoutQuery(0));
  EqualResponse(ar, br);
  const auto qname = std::to_array<uint8_t>({1, 'a', 0});
  DnsQuery query(0x1234, qname, 1);
  DnsResponseRustReferenceQuery refquery(0x1234, qname, 1);
  DnsResponse matched(ab, ab->size());
  DnsResponseRustReference refmatched(bb, bb->size());
  CHECK_EQ(matched.InitParse(data.size(), query),
           refmatched.InitParse(data.size(), refquery));
  EqualResponse(matched, refmatched);
  // Raw-data constructor and response move preserve parser views of owned
  // backing.
  if (data.empty()) {
    return;
  }
  DnsResponse raw(data, offset);
  DnsResponseRustReference refraw(data, offset);
  Parser(raw.Parser(), refraw.Parser());
  DnsResponse moved(std::move(raw));
  DnsResponseRustReference refmoved(std::move(refraw));
  Parser(moved.Parser(), refmoved.Parser());
}
inline std::vector<std::vector<uint8_t>> Corpus() {
  std::vector<std::vector<uint8_t>> result{{},
                                           {0},
                                           {0xc0},
                                           {0xc0, 0},
                                           {0xc0, 2, 0},
                                           {0xc0, 255},
                                           {64, 0},
                                           {128, 0},
                                           {1, 'x', 0},
                                           {1, 'x', 0xc0, 0},
                                           {1, 'x', 0xc0, 2}};
  for (size_t n : {254u, 255u, 256u, 300u}) {
    std::vector<uint8_t> name;
    while (name.size() < n) {
      size_t len = std::min(size_t{63}, n - name.size() - 1);
      name.push_back(len);
      name.insert(name.end(), len, 'x');
    }
    name.push_back(0);
    result.push_back(std::move(name));
  }
  for (unsigned questions : {0, 1, 2, 255}) {
    std::vector<uint8_t> p{
        0x12, 0x34, 0x84, 0x83, 0, static_cast<uint8_t>(questions),
        0,    1,    0,    1,    0, 1};
    for (unsigned i = 0; i < questions; ++i) {
      p.insert(p.end(), {1, 'a', 0, 0, 1, 0, 1});
    }
    for (unsigned i = 0; i < 3; ++i) {
      p.insert(p.end(), {0xc0, 12, 0, 1, 0, 1, 0, 0, 0, 60, 0, 4, 1, 2, 3, 4});
    }
    result.push_back(p);
    for (size_t n = 0; n < std::min(p.size(), size_t{80}); ++n) {
      result.emplace_back(p.begin(), p.begin() + n);
    }
  }
  // Long chain and cycle: iterative, packet-size bounded, no recursion limit.
  std::vector<uint8_t> chain(4001, 0);
  for (size_t i = 0; i + 2 < chain.size(); i += 2) {
    chain[i] = 0xc0 | ((i + 2) >> 8);
    chain[i + 1] = (i + 2) & 255;
  }
  result.push_back(chain);
  chain.back() = 0xc0;
  result.push_back(chain);
  return result;
}
inline void Ownership() {
  DnsResourceRecord r;
  r.name = "example.com";
  r.SetOwnedRdata(std::to_array<uint8_t>({1, 2, 3, 4}));
  DnsResourceRecord copy(r);
  CHECK_EQ(copy.rdata.data(), copy.owned_rdata.data());
  CHECK_NE(copy.rdata.data(), r.rdata.data());
  DnsResourceRecord assigned;
  assigned = r;
  CHECK_EQ(assigned.rdata.data(), assigned.owned_rdata.data());
  DnsResourceRecord moved(std::move(r));
  CHECK(r.rdata.empty());
  CHECK_EQ(moved.rdata.data(), moved.owned_rdata.data());
  copy = std::move(moved);
  CHECK(moved.rdata.empty());
  CHECK_EQ(copy.rdata.data(), copy.owned_rdata.data());
  std::vector<uint8_t> bytes{1, 2, 3, 4};
  r.rdata = bytes;
  DnsResourceRecord borrow(r);
  CHECK_EQ(borrow.rdata.data(), bytes.data());
  DnsResourceRecord bm(std::move(borrow));
  CHECK(borrow.rdata.empty());
  bytes[0] = 99;
  CHECK_EQ(bm.rdata[0], 99);
  // Name output itself backs the packet: clear happens before first wire read.
  std::string a(300, 'x'), b = a;
  a[0] = b[0] = 1;
  a[1] = b[1] = 'a';
  a[2] = b[2] = 0;
  DnsRecordParser pa(base::as_byte_span(a), 0, 1);
  DnsResponseRustReferenceParser pb(base::as_byte_span(b), 0, 1);
  CHECK_EQ(pa.ReadName(a.data(), &a), pb.ReadName(b.data(), &b));
  CHECK_EQ(a, b);
  // A live alias outside the clear() terminator exercises incremental label
  // appends and partial output while retaining the original string allocation.
  a.assign(300, 'x');
  b = a;
  a[1] = b[1] = 1;
  a[2] = b[2] = 'a';
  a[3] = b[3] = 1;
  a[4] = b[4] = 'b';
  a[5] = b[5] = 0;
  DnsRecordParser pc(base::as_byte_span(a), 0, 1);
  DnsResponseRustReferenceParser pd(base::as_byte_span(b), 0, 1);
  CHECK_EQ(pc.ReadName(base::as_byte_span(a).subspan(1u).data(), &a),
           pd.ReadName(base::as_byte_span(b).subspan(1u).data(), &b));
  CHECK_EQ(a, b);
  // Field output overlaps input bytes, so each subsequent field sees commits.
  DnsResourceRecord xa;
  DnsResponseRustReferenceRecord xb;
  auto set = [&](auto& v) {
    v.type = 0;
    v.klass = 0;
    v.ttl = 0x01010101;
  };
  set(xa);
  set(xb);
  auto sa = base::byte_span_from_ref(xa.ttl);
  auto sb = base::byte_span_from_ref(xb.ttl);
  DnsRecordParser qa(sa, 0, 1);
  DnsResponseRustReferenceParser qb(sb, 0, 1);
  std::string na, nb;
  CHECK_EQ(qa.ReadQuestion(na, xa.type), qb.ReadQuestion(nb, xb.type));
  CHECK_EQ(na, nb);
  CHECK_EQ(xa.type, xb.type);
  std::array<uint16_t, 3> aq{0, 0, 0}, bq{0, 0, 0};
  auto av = base::as_writable_byte_span(aq);
  auto bv = base::as_writable_byte_span(bq);
  av[1] = bv[1] = 0x12;
  av[2] = bv[2] = 0x34;
  DnsRecordParser alias_question(av, 0, 0);
  DnsResponseRustReferenceParser ref_alias_question(bv, 0, 0);
  CHECK_EQ(alias_question.ReadQuestion(na, aq[2]),
           ref_alias_question.ReadQuestion(nb, bq[2]));
  CHECK_EQ(na, nb);
  CHECK_EQ(aq[2], 0x1234);
  CHECK(aq == bq);
}
inline void Serialize(unsigned seed) {
  const auto qname = std::to_array<uint8_t>({1, 'a', 0});
  DnsQuery query(0x1234, qname, 1);
  DnsResponseRustReferenceQuery refquery(0x1234, qname, 1);
  std::vector<DnsResourceRecord> a;
  std::vector<DnsResponseRustReferenceRecord> b;
  for (unsigned i = 0; i < 1 + seed % 7; ++i) {
    DnsResourceRecord x;
    DnsResponseRustReferenceRecord y;
    x.type = y.type = (i % 2 == 0 ? 1 : 5);
    x.klass = y.klass = 1;
    x.ttl = y.ttl = seed * 65537;
    x.name = y.name = (i % 2 == 0 ? "a" : "alias.example.");
    std::vector<uint8_t> data{static_cast<uint8_t>(seed), 2, 3, 4};
    x.SetOwnedRdata(data);
    y.SetOwnedRdata(data);
    a.push_back(std::move(x));
    b.push_back(std::move(y));
  }
  DnsResponse ar(0x1234, seed % 2, a, {}, a, query, seed % 16);
  DnsResponseRustReference br(0x1234, seed % 2, b, {}, b, refquery, seed % 16);
  CHECK(ar.io_buffer()->span() == br.io_buffer()->span());
  EqualResponse(ar, br);
  CHECK_EQ(a.front().CalculateRecordSize(), b.front().CalculateRecordSize());
  DnsResponse noquery(0, seed % 2, {}, a, {}, std::nullopt);
  DnsResponseRustReference refnoquery(0, seed % 2, {}, b, {}, std::nullopt);
  CHECK(noquery.io_buffer()->span() == refnoquery.io_buffer()->span());
  EqualResponse(noquery, refnoquery);
}
}  // namespace net::dns_response_rust_test
#endif
