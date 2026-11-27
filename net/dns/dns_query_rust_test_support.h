// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_DNS_QUERY_RUST_TEST_SUPPORT_H_
#define NET_DNS_DNS_QUERY_RUST_TEST_SUPPORT_H_

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_query_rust_reference.h"
#include "net/dns/opt_record_rdata.h"

namespace net::dns_query_rust_test {
inline void Equal(const DnsQuery& rust, const DnsQueryRustReference& cpp) {
  CHECK_EQ(rust.question_size(), cpp.question_size());
  CHECK_EQ(rust.io_buffer() == nullptr, cpp.io_buffer() == nullptr);
  if (!rust.io_buffer()) {
    return;
  }
  CHECK(rust.io_buffer()->span() == cpp.io_buffer()->span());
  const size_t size = rust.io_buffer()->span().size();
  if (size >= 2) {
    CHECK_EQ(rust.id(), cpp.id());
  }
  if (size >= 12 + rust.question_size()) {
    CHECK(rust.qname() == cpp.qname());
    CHECK_EQ(rust.qtype(), cpp.qtype());
    CHECK_EQ(rust.question(), cpp.question());
    CHECK_EQ(rust.qname().data(), rust.io_buffer()->span().subspan(12u).data());
    CHECK_EQ(rust.question().data(),
             base::as_chars(rust.io_buffer()->span()).subspan(12u).data());
  }
}
inline scoped_refptr<IOBufferWithSize> Packet(base::span<const uint8_t> data,
                                              size_t tail = 0) {
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(data.size() + tail);
  buffer->span().first(data.size()).copy_from(data);
  std::ranges::fill(buffer->span().subspan(data.size()), 0xa5);
  return buffer;
}
inline std::vector<uint8_t> Name(base::span<const uint8_t> data) {
  size_t len = data.empty() ? 1 : 1 + data[0] % 63;
  std::vector<uint8_t> name(len + 2, 0);
  name[0] = static_cast<uint8_t>(len);
  for (size_t i = 0; i < len; ++i) {
    name[i + 1] = data.empty() ? 'a' : data[i % data.size()];
  }
  return name;
}
inline void State(base::span<const uint8_t> data) {
  DnsQuery rust(Packet(data, 19));
  DnsQueryRustReference cpp(Packet(data, 19));
  const size_t prefix = data.empty() ? 0 : data.back() % (data.size() + 1);
  CHECK_EQ(rust.Parse(prefix), cpp.Parse(prefix));
  Equal(rust, cpp);
  CHECK_EQ(rust.Parse(data.size()), cpp.Parse(data.size()));
  Equal(rust, cpp);
  // Copy includes trailing bytes even when parsing failed; clones preserve
  // those bytes and the old qname_size_. Copies have independent storage.
  DnsQuery copy(rust);
  DnsQueryRustReference ref_copy(cpp);
  Equal(copy, ref_copy);
  CHECK_NE(copy.io_buffer(), rust.io_buffer());
  auto clone = rust.CloneWithNewId(0xcafe);
  auto ref_clone = cpp.CloneWithNewId(0xcafe);
  Equal(*clone, *ref_clone);
  rust.set_flags(0x0001);
  cpp.set_flags(0x0001);
  CHECK_EQ(rust.Parse(data.size()), cpp.Parse(data.size()));
  Equal(rust, cpp);
  Equal(copy, ref_copy);
  DnsQuery moved(std::move(copy));
  DnsQueryRustReference ref_moved(std::move(ref_copy));
  Equal(moved, ref_moved);
  Equal(copy, ref_copy);  // moved-from buffer null; name size preserved
  copy = rust;
  ref_copy = cpp;
  Equal(copy, ref_copy);
  auto* prior = copy.io_buffer();
  moved = std::move(copy);
  ref_moved = std::move(ref_copy);
  CHECK_EQ(prior, moved.io_buffer());
  Equal(moved, ref_moved);
  Equal(copy, ref_copy);

  const auto name = Name(data);
  uint16_t id = data.empty() ? 0 : data.front() * 257u;
  uint16_t type = data.empty() ? 28 : data.back() * 257u;
  DnsQuery constructed(id, name, type);
  DnsQueryRustReference ref_constructed(id, name, type);
  Equal(constructed, ref_constructed);
  // Caller-visible qname span can itself be the next constructor's input.
  DnsQuery aliased(id, constructed.qname(), type);
  DnsQueryRustReference ref_aliased(id, ref_constructed.qname(), type);
  Equal(aliased, ref_aliased);
  // A successful parse followed by failed reparses preserves the last name
  // size. Views remain live views of the externally mutable backing buffer.
  auto wire = Packet(constructed.io_buffer()->span(), 7);
  auto ref_wire = Packet(ref_constructed.io_buffer()->span(), 7);
  DnsQuery parsed(wire);
  DnsQueryRustReference ref_parsed(ref_wire);
  CHECK(parsed.Parse(constructed.io_buffer()->size()));
  CHECK(ref_parsed.Parse(ref_constructed.io_buffer()->size()));
  auto view = parsed.qname();
  wire->span()[13] ^= 0xff;
  ref_wire->span()[13] ^= 0xff;
  CHECK_EQ(view[1], wire->span()[13]);
  Equal(parsed, ref_parsed);
  CHECK(!parsed.Parse(12));
  CHECK(!ref_parsed.Parse(12));
  Equal(parsed, ref_parsed);
  wire->span()[4] = ref_wire->span()[4] = 1;
  CHECK(!parsed.Parse(wire->size()));
  CHECK(!ref_parsed.Parse(ref_wire->size()));
  Equal(parsed, ref_parsed);
}
inline void OptCases(base::span<const uint8_t> data) {
  auto name = Name(data);
  // Unknown option, EDE request and existing padding; creation services stay
  // unchanged and all DnsQuery selection is differential-tested above them.
  const std::vector<std::vector<uint8_t>> options = {
      {0, 99, 0, 2, 0xfe, 0xff},
      {0, 15, 0, 2, 0, 0},
      {0, 12, 0, 1, 0xaa},
      {0, 99, 0, 0, 0, 15, 0, 2, 0, 0, 0, 99, 0, 1, 0xff}};
  for (const auto& bytes : options) {
    auto opt = OptRecordRdata::Create(bytes);
    CHECK(opt);
    for (bool padding : {false, true}) {
      if (padding && opt->ContainsOptCode(12)) {
        continue;  // debug precondition; covered by death test
      }
      auto strategy = padding ? DnsQuery::PaddingStrategy::BLOCK_LENGTH_128
                              : DnsQuery::PaddingStrategy::NONE;
      auto ref_strategy =
          padding ? DnsQueryRustReference::PaddingStrategy::BLOCK_LENGTH_128
                  : DnsQueryRustReference::PaddingStrategy::NONE;
      DnsQuery rust(0x1234, name, 28, opt.get(), strategy);
      DnsQueryRustReference cpp(0x1234, name, 28, opt.get(), ref_strategy);
      Equal(rust, cpp);
      CHECK(base::as_byte_span(opt->buf()) == base::span(bytes));
    }
  }
  DnsQuery rust(0x1234, name, 28, nullptr,
                DnsQuery::PaddingStrategy::BLOCK_LENGTH_128);
  DnsQueryRustReference cpp(
      0x1234, name, 28, nullptr,
      DnsQueryRustReference::PaddingStrategy::BLOCK_LENGTH_128);
  Equal(rust, cpp);
}
}  // namespace net::dns_query_rust_test
#endif
