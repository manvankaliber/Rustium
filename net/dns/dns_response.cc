// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_response.h"

#include <stddef.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/containers/span_writer.h"
#include "base/logging.h"
#include "base/numerics/byte_conversions.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "base/sys_byteorder.h"
#include "base/types/optional_util.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_response_result_extractor.h"
#include "net/dns/dns_response_rust_ffi.h"
#include "net/dns/dns_response_rust_ffi.rs.h"
#include "net/dns/dns_util.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/record_rdata.h"

namespace net {

namespace {
rust::Slice<const uint8_t> Bytes(base::span<const uint8_t> bytes) {
  return {bytes.empty() ? nullptr : bytes.data(), bytes.size()};
}
rust::Slice<const uint8_t> Bytes(std::string_view bytes) {
  return Bytes(base::as_byte_span(bytes));
}
std::string String(const rust::Vec<uint8_t>& bytes) {
  return bytes.empty()
             ? std::string()
             : std::string(reinterpret_cast<const char*>(bytes.data()),
                           bytes.size());
}
}  // namespace
namespace dns_response_rust {
struct ParserContext {
  base::raw_span<const uint8_t> bytes;
  raw_ptr<std::string> name;
  raw_ptr<DnsResourceRecord> record = nullptr;
  raw_ptr<uint16_t> qtype = nullptr;
};
rust::Slice<const uint8_t> packet(const ParserContext& ctx) {
  return Bytes(ctx.bytes);
}
void clear_name(ParserContext& ctx) {
  ctx.name->clear();
  ctx.name->reserve(dns_protocol::kMaxCharNameLength);
}
bool name_empty(const ParserContext& ctx) {
  return ctx.name->empty();
}
void append_name(ParserContext& ctx, rust::Slice<const uint8_t> bytes) {
  if (!bytes.empty()) {
    ctx.name->append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  }
}
void field(ParserContext& ctx, uint8_t index, uint32_t value) {
  if (ctx.qtype) {
    *ctx.qtype = value;
    return;
  }
  switch (index) {
    case 0:
      ctx.record->type = value;
      break;
    case 1:
      ctx.record->klass = value;
      break;
    case 2:
      ctx.record->ttl = value;
      break;
    default:
      NOTREACHED();
  }
}
void rdata(ParserContext& ctx, size_t offset, size_t len) {
  ctx.record->rdata = ctx.bytes.subspan(offset, len);
}
bool aliased(const ParserContext& ctx) {
  auto overlaps = [&](const void* data, size_t size) {
    const auto a = reinterpret_cast<uintptr_t>(ctx.bytes.data());
    const auto b = reinterpret_cast<uintptr_t>(data);
    return size && ctx.bytes.size() &&
           (a <= b ? b - a < ctx.bytes.size() : a - b < size);
  };
  return (ctx.name && (overlaps(ctx.name.get(), sizeof(std::string)) ||
                       overlaps(ctx.name->data(), ctx.name->capacity() + 1))) ||
         (ctx.record &&
          overlaps(ctx.record.get(), sizeof(DnsResourceRecord))) ||
         (ctx.qtype && overlaps(ctx.qtype.get(), sizeof(uint16_t)));
}
void commit(ParserContext& ctx, const ParseOutput& output) {
  if (output.touched) {
    clear_name(ctx);
    append_name(ctx, {output.name.data(), output.len});
  }
  if (output.fields & 1) {
    field(ctx, 0, output.kind);
  }
  if (output.fields & 2) {
    field(ctx, 1, output.klass);
  }
  if (output.fields & 4) {
    field(ctx, 2, output.ttl);
  }
  if (output.has_rdata) {
    rdata(ctx, output.rdata_offset, output.rdata_len);
  }
}

}  // namespace dns_response_rust

DnsResourceRecord::DnsResourceRecord() = default;

DnsResourceRecord::DnsResourceRecord(const DnsResourceRecord& other)
    : name(other.name),
      type(other.type),
      klass(other.klass),
      ttl(other.ttl),
      owned_rdata(other.owned_rdata) {
  if (!owned_rdata.empty()) {
    rdata = owned_rdata;
  } else {
    rdata = other.rdata;
  }
}

DnsResourceRecord::DnsResourceRecord(DnsResourceRecord&& other)
    : name(std::move(other.name)),
      type(other.type),
      klass(other.klass),
      ttl(other.ttl),
      owned_rdata(std::move(other.owned_rdata)) {
  if (!owned_rdata.empty()) {
    rdata = owned_rdata;
  } else {
    rdata = other.rdata;
  }
  // Ensure the moved-from object has no dangling reference.
  other.rdata = base::span<const uint8_t>();
}

DnsResourceRecord::~DnsResourceRecord() = default;

DnsResourceRecord& DnsResourceRecord::operator=(
    const DnsResourceRecord& other) {
  name = other.name;
  type = other.type;
  klass = other.klass;
  ttl = other.ttl;
  owned_rdata = other.owned_rdata;

  if (!owned_rdata.empty()) {
    rdata = owned_rdata;
  } else {
    rdata = other.rdata;
  }

  return *this;
}

DnsResourceRecord& DnsResourceRecord::operator=(DnsResourceRecord&& other) {
  name = std::move(other.name);
  type = other.type;
  klass = other.klass;
  ttl = other.ttl;
  owned_rdata = std::move(other.owned_rdata);

  if (!owned_rdata.empty()) {
    rdata = owned_rdata;
  } else {
    rdata = other.rdata;
  }
  // Ensure the moved-from object has no dangling reference.
  other.rdata = base::span<const uint8_t>();

  return *this;
}

void DnsResourceRecord::SetOwnedRdata(base::span<const uint8_t> value) {
  DCHECK(!value.empty());
  owned_rdata.assign(value.begin(), value.end());
  rdata = owned_rdata;
  DCHECK_EQ(owned_rdata.data(), rdata.data());
}

size_t DnsResourceRecord::CalculateRecordSize() const {
  CHECK(type == dns_protocol::kTypeOPT ? name.empty() : !name.empty());
  return dns_response_rust::record_size(Bytes(name), type, owned_rdata.size(),
                                        rdata.size());
}

DnsRecordParser::DnsRecordParser() = default;

DnsRecordParser::~DnsRecordParser() = default;

DnsRecordParser::DnsRecordParser(const DnsRecordParser&) = default;

DnsRecordParser::DnsRecordParser(DnsRecordParser&&) = default;

DnsRecordParser& DnsRecordParser::operator=(const DnsRecordParser&) = default;

DnsRecordParser& DnsRecordParser::operator=(DnsRecordParser&&) = default;

DnsRecordParser::DnsRecordParser(base::span<const uint8_t> packet,
                                 size_t offset,
                                 size_t num_records)
    : packet_(packet), num_records_(num_records), cur_(offset) {
  CHECK_LE(offset, packet_.size());
}

unsigned DnsRecordParser::ReadName(const void* const vpos,
                                   std::string* out) const {
  CHECK_LE(packet_.data(), vpos);
  CHECK_LE(vpos, packet_.last(0u).data());
  // SAFETY: the range CHECKs establish that both pointers belong to packet_.
  const size_t offset =
      packet_.empty()
          ? 0
          : UNSAFE_BUFFERS(static_cast<const uint8_t*>(vpos) - packet_.data());
  dns_response_rust::ParserContext context{packet_, out};
  return dns_response_rust::parse(context, 0, offset, out != nullptr);
}

bool DnsRecordParser::ReadRecord(DnsResourceRecord* out) {
  CHECK(!packet_.empty());
  dns_response_rust::ParserContext context{packet_, &out->name, out};
  const size_t consumed = dns_response_rust::parse(
      context, 1, cur_, num_records_parsed_ < num_records_);
  if (!consumed) {
    return false;
  }
  cur_ += consumed;
  ++num_records_parsed_;
  return true;
}

bool DnsRecordParser::ReadQuestion(std::string& name, uint16_t& type) {
  dns_response_rust::ParserContext context{packet_, &name, nullptr, &type};
  const size_t consumed = dns_response_rust::parse(context, 2, cur_, true);
  if (!consumed) {
    return false;
  }
  cur_ += consumed;
  return true;
}

DnsResponse::DnsResponse(
    uint16_t id,
    bool is_authoritative,
    const std::vector<DnsResourceRecord>& answers,
    const std::vector<DnsResourceRecord>& authority_records,
    const std::vector<DnsResourceRecord>& additional_records,
    const std::optional<DnsQuery>& query,
    uint8_t rcode,
    bool validate_records,
    bool validate_names_as_internet_hostnames) {
  DCHECK(dns_response_rust::valid_rcode(rcode));
  std::vector<dns_response_rust::Record> records;
  auto append = [&](const auto& group) {
    for (const auto& record : group) {
      CHECK_EQ(record.rdata.data(), record.owned_rdata.data());
      CHECK(record.type == dns_protocol::kTypeOPT ? record.name.empty()
                                                  : !record.name.empty());
      dns_response_rust::Record value;
      for (uint8_t byte : base::as_byte_span(record.name)) {
        value.name.push_back(byte);
      }
      for (uint8_t byte : record.owned_rdata) {
        value.data.push_back(byte);
      }
      value.kind = record.type;
      value.klass = record.klass;
      value.ttl = record.ttl;
      records.push_back(std::move(value));
    }
  };
  append(answers);
  append(authority_records);
  append(additional_records);
  auto result = dns_response_rust::build(
      id, is_authoritative, rcode, query ? query->id() : 0,
      query ? query->qtype() : 0,
      Bytes(query ? query->question() : std::string_view()), query.has_value(),
      rust::Slice<const dns_response_rust::Record>(records), answers.size(),
      authority_records.size(), validate_records,
      validate_names_as_internet_hostnames);
  DCHECK(result.success);
  if (!result.success) {
    return;
  }
  io_buffer_size_ = result.bytes.size();
  io_buffer_ = base::MakeRefCounted<IOBufferWithSize>(io_buffer_size_);
  io_buffer_->span().copy_from(base::span(result.bytes));
  if (query) {
    InitParse(io_buffer_size_, *query);
  } else {
    InitParseWithoutQuery(io_buffer_size_);
  }
}

DnsResponse::DnsResponse()
    : io_buffer_(base::MakeRefCounted<IOBufferWithSize>(
          dns_protocol::kMaxUDPSize + 1)),
      io_buffer_size_(dns_protocol::kMaxUDPSize + 1) {}

DnsResponse::DnsResponse(scoped_refptr<IOBuffer> buffer, size_t size)
    : io_buffer_(std::move(buffer)), io_buffer_size_(size) {}

DnsResponse::DnsResponse(size_t length)
    : io_buffer_(base::MakeRefCounted<IOBufferWithSize>(length)),
      io_buffer_size_(length) {}

DnsResponse::DnsResponse(base::span<const uint8_t> data, size_t answer_offset)
    : io_buffer_(base::MakeRefCounted<IOBufferWithSize>(data.size())),
      io_buffer_size_(data.size()),
      parser_(io_buffer_->span(),
              answer_offset,
              std::numeric_limits<size_t>::max()) {
  io_buffer_->span().copy_from(data);
}

// static
DnsResponse DnsResponse::CreateEmptyNoDataResponse(
    uint16_t id,
    bool is_authoritative,
    base::span<const uint8_t> qname,
    uint16_t qtype) {
  return DnsResponse(id, is_authoritative,
                     /*answers=*/{},
                     /*authority_records=*/{},
                     /*additional_records=*/{}, DnsQuery(id, qname, qtype));
}

DnsResponse::DnsResponse(DnsResponse&& other) = default;
DnsResponse& DnsResponse::operator=(DnsResponse&& other) = default;

DnsResponse::~DnsResponse() = default;

bool DnsResponse::InitParse(size_t nbytes, const DnsQuery& query) {
  // Expose only initialized bytes, never the unused allocation tail.
  if (nbytes > io_buffer_size_ || !io_buffer_ ||
      nbytes > io_buffer_->span().size()) {
    return false;
  }
  auto result = dns_response_rust::init_parse(
      Bytes(io_buffer_->first(nbytes)), Bytes(query.question()),
      Bytes(query.qname()), query.id(), query.qtype(), true);
  id_available_ |= result.id_available;
  if (!result.success) {
    return false;
  }
  for (const auto& question : result.questions) {
    dotted_qnames_.push_back(String(question.name));
    qtypes_.push_back(question.qtype);
  }
  parser_ =
      DnsRecordParser(io_buffer_->first(nbytes), result.offset, result.records);
  return true;
}
bool DnsResponse::InitParseWithoutQuery(size_t nbytes) {
  if (nbytes > io_buffer_size_ || !io_buffer_ ||
      nbytes > io_buffer_->span().size()) {
    return false;
  }
  auto result = dns_response_rust::init_parse(Bytes(io_buffer_->first(nbytes)),
                                              {}, {}, 0, 0, false);
  id_available_ |= result.id_available;
  if (!result.success) {
    if (result.reset_parser) {
      parser_ = DnsRecordParser();
    }
    return false;
  }
  std::vector<std::string> names;
  std::vector<uint16_t> types;
  for (const auto& question : result.questions) {
    names.push_back(String(question.name));
    types.push_back(question.qtype);
  }
  dotted_qnames_ = std::move(names);
  qtypes_ = std::move(types);
  parser_ =
      DnsRecordParser(io_buffer_->first(nbytes), result.offset, result.records);
  return true;
}

std::optional<uint16_t> DnsResponse::id() const {
  if (!id_available_) {
    return std::nullopt;
  }

  return dns_response_rust::field_value(Bytes(io_buffer_->first(12)), 0);
}

bool DnsResponse::IsValid() const {
  return parser_.IsValid();
}

uint16_t DnsResponse::flags() const {
  DCHECK(parser_.IsValid());
  return dns_response_rust::field_value(Bytes(io_buffer_->first(12)), 1);
}

uint8_t DnsResponse::rcode() const {
  DCHECK(parser_.IsValid());
  return dns_response_rust::field_value(Bytes(io_buffer_->first(12)), 6);
}

unsigned DnsResponse::question_count() const {
  DCHECK(parser_.IsValid());
  return dns_response_rust::field_value(Bytes(io_buffer_->first(12)), 2);
}

unsigned DnsResponse::answer_count() const {
  DCHECK(parser_.IsValid());
  return dns_response_rust::field_value(Bytes(io_buffer_->first(12)), 3);
}

unsigned DnsResponse::authority_count() const {
  DCHECK(parser_.IsValid());
  return dns_response_rust::field_value(Bytes(io_buffer_->first(12)), 4);
}

unsigned DnsResponse::additional_answer_count() const {
  DCHECK(parser_.IsValid());
  return dns_response_rust::field_value(Bytes(io_buffer_->first(12)), 5);
}

uint16_t DnsResponse::GetSingleQType() const {
  DCHECK_EQ(qtypes().size(), 1u);
  return qtypes().front();
}

std::string_view DnsResponse::GetSingleDottedName() const {
  DCHECK_EQ(dotted_qnames().size(), 1u);
  return dotted_qnames().front();
}

DnsRecordParser DnsResponse::Parser() const {
  DCHECK(parser_.IsValid());
  // Return a copy of the parser.
  return parser_;
}

}  // namespace net
