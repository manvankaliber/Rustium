// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>
#include <type_traits>

#include "net/dns/dns_response_rust_test_support.h"
namespace {
enum class Kind {
  Name,
  Skip,
  Record,
  Question,
  Response,
  Match,
  Build,
  Copy,
  Move,
  Size,
  Getters
};
struct Case {
  const char* key;
  Kind kind;
  std::vector<uint8_t> bytes;
  size_t offset = 0;
  size_t count = 1;
};
template <bool Rust>
uint64_t Run(const Case& test, size_t iterations) {
  using Parser = std::conditional_t<Rust, net::DnsRecordParser,
                                    net::DnsResponseRustReferenceParser>;
  using Response =
      std::conditional_t<Rust, net::DnsResponse, net::DnsResponseRustReference>;
  using Record = std::conditional_t<Rust, net::DnsResourceRecord,
                                    net::DnsResponseRustReferenceRecord>;
  using Query = std::conditional_t<Rust, net::DnsQuery,
                                   net::DnsResponseRustReferenceQuery>;
  auto packet = net::dns_response_rust_test::Packet(test.bytes);
  Query query(0x1234, std::to_array<uint8_t>({1, 'a', 0}), 1);
  std::vector<Record> records(test.count);
  for (auto& record : records) {
    record.name = "a";
    record.type = 1;
    record.klass = 1;
    record.ttl = 300;
    record.SetOwnedRdata(std::to_array<uint8_t>({1, 2, 3, 4}));
  }
  uint64_t sum = 0;
  for (size_t i = 0; i < iterations; ++i) {
    switch (test.kind) {
      case Kind::Name:
      case Kind::Skip: {
        Parser parser(test.bytes, test.offset, 1);
        std::string name;
        sum +=
            parser.ReadName(base::span(test.bytes).subspan(test.offset).data(),
                            test.kind == Kind::Skip ? nullptr : &name);
        sum += name.size();
        break;
      }
      case Kind::Record: {
        Parser parser(test.bytes, test.offset, test.count);
        Record record;
        for (size_t n = 0; n < test.count; ++n) {
          sum += parser.ReadRecord(&record);
          sum += record.name.size() + record.type + record.ttl +
                 record.rdata.size();
        }
        sum += parser.GetOffset();
        break;
      }
      case Kind::Question: {
        Parser parser(test.bytes, test.offset, 0);
        std::string name;
        uint16_t type = 0;
        sum += parser.ReadQuestion(name, type);
        sum += name.size() + type + parser.GetOffset();
        break;
      }
      case Kind::Response:
      case Kind::Match: {
        Response response(packet, test.bytes.size());
        sum += (test.kind == Kind::Match
                    ? response.InitParse(test.bytes.size(), query)
                    : response.InitParseWithoutQuery(test.bytes.size()));
        sum += response.id().value_or(0);
        if (response.IsValid()) {
          sum += response.flags() + response.question_count() +
                 response.Parser().GetOffset();
        }
        break;
      }
      case Kind::Build: {
        Response response(0x1234, true, records, {}, records, query);
        sum += response.io_buffer_size() + response.answer_count() +
               response.GetSingleQType();
        break;
      }
      case Kind::Copy: {
        Record copy(records.front());
        sum += copy.CalculateRecordSize() + copy.rdata.front();
        break;
      }
      case Kind::Move: {
        Record copy(records.front());
        Record moved(std::move(copy));
        Record target;
        target = std::move(moved);
        sum += target.CalculateRecordSize() + target.rdata.front() +
               copy.rdata.size();
        break;
      }
      case Kind::Size: {
        sum += records.front().CalculateRecordSize();
        break;
      }
      case Kind::Getters: {
        Response response(packet, test.bytes.size());
        CHECK(response.InitParseWithoutQuery(test.bytes.size()));
        sum += response.flags() + response.rcode() + response.question_count() +
               response.answer_count() + response.authority_count() +
               response.additional_answer_count();
        packet->span()[0] ^= static_cast<uint8_t>(i);
        sum += response.id().value_or(0);
        break;
      }
    }
  }
  return sum;
}
volatile uint64_t sink = 0;
template <bool Rust>
double Measure(const Case& test, size_t iterations) {
  auto start = std::chrono::steady_clock::now();
  sink = Run<Rust>(test, iterations);
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::nano>(end - start).count() /
         iterations;
}
}  // namespace
int main(int argc, char** argv) {
  const bool verify = argc > 1 && std::string_view(argv[1]) == "--verify-only";
  const std::vector<uint8_t> short_name{1,   'a', 7,   'e', 'x', 'a',
                                        'm', 'p', 'l', 'e', 0};
  std::vector<uint8_t> maximum;
  for (unsigned i = 0; i < 4; ++i) {
    maximum.push_back(i == 3 ? 61 : 63);
    maximum.insert(maximum.end(), i == 3 ? 61 : 63, 'a');
  }
  maximum.push_back(0);
  std::vector<uint8_t> direct{0x12, 0x34, 0x84, 0,   0, 1, 0, 1, 0, 0,
                              0,    0,    1,    'a', 0, 0, 1, 0, 1};
  direct.insert(direct.end(),
                {0xc0, 12, 0, 1, 0, 1, 0, 0, 1, 44, 0, 4, 1, 2, 3, 4});
  auto multi = direct;
  multi[5] = 2;
  multi.insert(multi.begin() + 19, {0xc0, 12, 0, 28, 0, 1});
  multi[26] = 12;
  auto truncated = direct;
  truncated.resize(17);
  auto flags = direct;
  flags[2] = 0;
  const std::vector<Case> cases{
      {"dns_response_name_direct", Kind::Name, short_name},
      {"dns_response_name_max", Kind::Name, maximum},
      {"dns_response_name_compressed", Kind::Name, direct, 19},
      {"dns_response_name_skip", Kind::Skip, direct, 19},
      {"dns_response_name_cycle", Kind::Name, {1, 'x', 0xc0, 0}},
      {"dns_response_record_compressed", Kind::Record, direct, 19},
      {"dns_response_record_truncated",
       Kind::Record,
       {1, 'a', 0, 0, 1, 0, 1, 0, 0, 1, 44, 0, 16, 1, 2}},
      {"dns_response_question", Kind::Question, direct, 12},
      {"dns_response_parse", Kind::Response, direct},
      {"dns_response_match", Kind::Match, direct},
      {"dns_response_multi_question", Kind::Response, multi},
      {"dns_response_truncated", Kind::Response, truncated},
      {"dns_response_invalid_flags", Kind::Response, flags},
      {"dns_response_build_one", Kind::Build, {}, 0, 1},
      {"dns_response_build_eight", Kind::Build, {}, 0, 8},
      {"dns_response_record_copy", Kind::Copy, {}},
      {"dns_response_record_move", Kind::Move, {}},
      {"dns_response_record_size", Kind::Size, {}},
      {"dns_response_live_getters", Kind::Getters, direct},
  };
  std::mt19937 random(20261113);
  constexpr size_t iterations = 20000;
  for (const auto& test : cases) {
    CHECK_EQ(Run<false>(test, 200), Run<true>(test, 200));
    if (verify) {
      continue;
    }
    for (unsigned pair = 0; pair < 20; ++pair) {
      bool rust_first = (random() & 1) != 0;
      double cpp, rust;
      if (rust_first) {
        rust = Measure<true>(test, iterations);
        cpp = Measure<false>(test, iterations);
      } else {
        cpp = Measure<false>(test, iterations);
        rust = Measure<true>(test, iterations);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          test.key, pair, cpp, rust, iterations,
          rust_first ? "true" : "false"));
    }
  }
}
