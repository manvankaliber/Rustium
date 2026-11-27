// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_query.h"

#include <limits>
#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_view_util.h"
#include "net/base/features.h"
#include "net/dns/dns_query_rust_ffi.h"
#include "net/dns/dns_query_rust_ffi.rs.h"
#include "net/dns/opt_record_rdata.h"

namespace net {
namespace {
// DNS's public layout is fixed; there is no typed Header object in IOBuffer.
constexpr size_t kHeaderSize = 12;

rust::Slice<const uint8_t> Bytes(base::span<const uint8_t> bytes) {
  // CXX accepts null/empty spans without making a Rust reference to null.
  return {bytes.data(), bytes.size()};
}
}  // namespace

namespace dns_query_rust {
struct OptContext {
  raw_ptr<const OptRecordRdata> input;
  std::unique_ptr<OptRecordRdata> merged;
};
bool opt_clone_or_create(OptContext& context, bool clone_input) {
  context.merged =
      clone_input
          ? OptRecordRdata::Create(base::as_byte_span(context.input->buf()))
          : std::make_unique<OptRecordRdata>();
  return context.merged != nullptr;
}
bool opt_contains(const OptContext& context, uint16_t code) {
  return context.merged->ContainsOptCode(code);
}
void opt_add_ede(OptContext& context) {
  context.merged->AddOpt(
      OptRecordRdata::EdeOpt::CreateStructuredErrorsRequest());
}
void opt_add_padding(OptContext& context, uint16_t len) {
  context.merged->AddOpt(std::make_unique<OptRecordRdata::PaddingOpt>(len));
}
size_t opt_len(const OptContext& context) {
  return context.merged->buf().size();
}
size_t opt_count(const OptContext& context) {
  return context.merged->OptCount();
}
}  // namespace dns_query_rust

DnsQuery::DnsQuery(uint16_t id,
                   base::span<const uint8_t> qname,
                   uint16_t qtype,
                   const OptRecordRdata* opt_rdata,
                   PaddingStrategy padding_strategy)
    : qname_size_(qname.size()) {
  dns_query_rust::OptContext context{opt_rdata, nullptr};
  const auto construction = dns_query_rust::prepare_query(
      Bytes(qname), opt_rdata != nullptr,
      padding_strategy == PaddingStrategy::BLOCK_LENGTH_128,
      base::FeatureList::IsEnabled(features::kUseStructuredDnsErrors), context);
  io_buffer_ = base::MakeRefCounted<IOBufferWithSize>(construction.size);
  base::span<const uint8_t> opt_bytes;
  if (context.merged) {
    opt_bytes = base::as_byte_span(context.merged->buf());
  }
  // Exclusive fresh storage, disjoint from the borrowed qname and OPT bytes.
  // Rust initializes every byte without reading the uninitialized allocation.
  dns_query_rust::write_query(io_buffer_->span().data(), construction.size, id,
                              Bytes(qname), qtype, construction.has_opt,
                              Bytes(opt_bytes));
}

DnsQuery::DnsQuery(scoped_refptr<IOBufferWithSize> buffer)
    : io_buffer_(std::move(buffer)) {}
DnsQuery::DnsQuery(const DnsQuery& query) {
  CopyFrom(query);
}
DnsQuery& DnsQuery::operator=(const DnsQuery& query) {
  CopyFrom(query);
  return *this;
}
DnsQuery::DnsQuery(DnsQuery&& query) = default;
DnsQuery& DnsQuery::operator=(DnsQuery&& query) = default;
DnsQuery::~DnsQuery() = default;

std::unique_ptr<DnsQuery> DnsQuery::CloneWithNewId(uint16_t id) const {
  return base::WrapUnique(new DnsQuery(*this, id));
}

bool DnsQuery::Parse(size_t valid_bytes) {
  if (!io_buffer_ || io_buffer_->span().empty()) {
    return false;
  }
  // Only this initialized prefix crosses into Rust. first() retains the
  // original bounds check if valid_bytes exceeds the allocation.
  const size_t name_size =
      dns_query_rust::parse_query(Bytes(io_buffer_->first(valid_bytes)));
  if (name_size == std::numeric_limits<size_t>::max()) {
    VLOG(1) << "Not supporting parsing a DNS query with multiple (or zero) "
               "questions.";
    return false;
  }
  if (name_size == 0) {
    return false;
  }
  qname_size_ = name_size;
  return true;
}

uint16_t DnsQuery::id() const {
  CHECK(io_buffer_ && !io_buffer_->span().empty());
  return dns_query_rust::read_u16(Bytes(io_buffer_->first(2u)));
}
base::span<const uint8_t> DnsQuery::qname() const {
  return io_buffer_->span().subspan(kHeaderSize, qname_size_);
}
uint16_t DnsQuery::qtype() const {
  return dns_query_rust::read_u16(
      Bytes(io_buffer_->span().subspan(kHeaderSize + qname_size_).first<2u>()));
}
std::string_view DnsQuery::question() const {
  return base::as_string_view(
      io_buffer_->span().subspan(kHeaderSize, question_size()));
}
size_t DnsQuery::question_size() const {
  return dns_query_rust::question_size(qname_size_);
}
void DnsQuery::set_flags(uint16_t flags) {
  CHECK(io_buffer_ && !io_buffer_->span().empty());
  // Bounds are checked in C++, but no reference to potentially uninitialized
  // bytes is manufactured by the Rust adapter.
  dns_query_rust::set_flags(io_buffer_->span().subspan(2u, 2u).data(), flags);
}
DnsQuery::DnsQuery(const DnsQuery& orig, uint16_t id) {
  CopyFrom(orig, true, id);
}
void DnsQuery::CopyFrom(const DnsQuery& orig, bool replace_id, uint16_t id) {
  qname_size_ = orig.qname_size_;
  // Keep orig alive until the byte copy is complete, including self-assignment.
  // The old self-assignment read its freshly allocated uninitialized buffer.
  auto buffer =
      base::MakeRefCounted<IOBufferWithSize>(orig.io_buffer()->size());
  dns_query_rust::copy_query(orig.io_buffer()->span().data(),
                             buffer->span().data(), buffer->size(), replace_id,
                             id);
  io_buffer_ = std::move(buffer);
}
}  // namespace net
