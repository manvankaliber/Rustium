// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/dns/dns_query_rust_test_support.h"

namespace {
enum class Kind { Construct, Parse, Copy, Clone, Assign, Getters, Flags };
struct Case {
  const char* key;
  Kind kind;
  std::vector<uint8_t> name;
  scoped_refptr<net::IOBufferWithSize> packet;
  raw_ptr<const net::OptRecordRdata> opt = nullptr;
  bool padding = false;
  bool ede = false;
  size_t valid = 0;
};

template <bool Rust>
uint64_t Run(const Case& test, size_t iterations) {
  using Query =
      std::conditional_t<Rust, net::DnsQuery, net::DnsQueryRustReference>;
  const auto strategy = test.padding ? Query::PaddingStrategy::BLOCK_LENGTH_128
                                     : Query::PaddingStrategy::NONE;
  // Source fixtures and feature state are warm and outside timed loops. They
  // are constructed inside Run, so their small fixed cost is included per
  // measured batch and amortized across the published iteration count.
  Query source(0x1234, test.name, 28, test.opt, strategy);
  Query destination(source);
  uint64_t checksum = 0;
  for (size_t i = 0; i < iterations; ++i) {
    switch (test.kind) {
      case Kind::Construct: {
        Query query(static_cast<uint16_t>(i), test.name, 28, test.opt,
                    strategy);
        for (uint8_t byte : query.io_buffer()->span()) {
          checksum += byte;
        }
        break;
      }
      case Kind::Parse: {
        Query query(test.packet);
        bool ok = query.Parse(test.valid);
        checksum += ok + query.question_size();
        if (ok) {
          checksum += query.qtype() + query.id();
        }
        break;
      }
      case Kind::Copy: {
        Query query(test.packet);
        Query copy(query);
        checksum += copy.io_buffer()->size();
        checksum += copy.io_buffer()->span().back();
        break;
      }
      case Kind::Clone: {
        auto copy = source.CloneWithNewId(static_cast<uint16_t>(i));
        checksum += copy->id() + copy->io_buffer()->span().back();
        break;
      }
      case Kind::Assign:
        destination = source;
        checksum += destination.id() + destination.io_buffer()->span().back();
        break;
      case Kind::Getters:
        checksum += source.id() + source.qtype() + source.qname().size() +
                    source.question().size() + source.question_size();
        // Exercise mutable backing so a future cached ID/type is caught.
        source.io_buffer()->span()[0] ^= static_cast<uint8_t>(i);
        break;
      case Kind::Flags:
        source.set_flags(static_cast<uint16_t>(i));
        checksum +=
            source.io_buffer()->span()[2] + source.io_buffer()->span()[3];
        break;
    }
  }
  return checksum;
}
volatile uint64_t sink = 0;
template <bool Rust>
double Measure(const Case& test, size_t iterations) {
  const auto start = std::chrono::steady_clock::now();
  const auto checksum = Run<Rust>(test, iterations);
  const auto end = std::chrono::steady_clock::now();
  sink = checksum;
  return std::chrono::duration<double, std::nano>(end - start).count() /
         iterations;
}
}  // namespace

int main(int argc, char** argv) {
  const bool verify_only =
      argc > 1 && std::string_view(argv[1]) == "--verify-only";
  using net::dns_query_rust_test::Packet;
  const std::vector<uint8_t> short_name{3,   'w', 'w', 'w', 7,   'e',
                                        'x', 'a', 'm', 'p', 'l', 'e',
                                        4,   't', 'e', 's', 't', 0};
  std::vector<uint8_t> maximum;
  for (int i = 0; i < 4; ++i) {
    maximum.push_back(i == 3 ? 61 : 63);
    maximum.insert(maximum.end(), i == 3 ? 61 : 63, 'a');
  }
  maximum.push_back(0);
  net::DnsQueryRustReference short_query(0x1234, short_name, 28);
  net::DnsQueryRustReference max_query(0x1234, maximum, 28);
  auto short_wire = Packet(short_query.io_buffer()->span());
  auto max_wire = Packet(max_query.io_buffer()->span());
  auto tail_wire =
      Packet(short_query.io_buffer()->span(), 512 - short_wire->span().size());
  std::vector<uint8_t> malformed(12, 0);
  malformed[5] = 1;
  malformed.push_back(254);
  malformed.insert(malformed.end(), 254, 0xc0);
  malformed.insert(malformed.end(), {0, 0, 28, 0, 1});
  auto malformed_wire = Packet(malformed);
  auto response = Packet(short_wire->span());
  response->span()[2] = 0x80;
  const std::vector<uint8_t> unknown{0, 99, 0, 4, 0xde, 0xad, 0xbe, 0xef};
  auto opt = net::OptRecordRdata::Create(unknown);
  const std::vector<Case> cases = {
      {"dns_query_construct_short", Kind::Construct, short_name},
      {"dns_query_construct_max", Kind::Construct, maximum},
      {"dns_query_construct_opt", Kind::Construct, short_name, nullptr,
       opt.get()},
      {"dns_query_construct_padding", Kind::Construct, short_name, nullptr,
       nullptr, true},
      {"dns_query_construct_ede", Kind::Construct, short_name, nullptr, nullptr,
       false, true},
      {"dns_query_parse_short", Kind::Parse, short_name, short_wire, nullptr,
       false, false, short_wire->span().size()},
      {"dns_query_parse_max", Kind::Parse, maximum, max_wire, nullptr, false,
       false, max_wire->span().size()},
      {"dns_query_parse_label254", Kind::Parse, short_name, malformed_wire,
       nullptr, false, false, malformed_wire->span().size()},
      {"dns_query_parse_truncated", Kind::Parse, short_name, short_wire,
       nullptr, false, false, short_wire->span().size() - 1},
      {"dns_query_parse_response", Kind::Parse, short_name, response, nullptr,
       false, false, response->span().size()},
      {"dns_query_copy_capacity512", Kind::Copy, short_name, tail_wire},
      {"dns_query_clone", Kind::Clone, short_name},
      {"dns_query_assign", Kind::Assign, maximum},
      {"dns_query_getters", Kind::Getters, short_name},
      {"dns_query_set_flags", Kind::Flags, short_name},
  };
  std::mt19937 random(20261112);
  constexpr size_t kIterations = 20000;
  for (const auto& test : cases) {
    base::test::ScopedFeatureList features;
    features.InitWithFeatureState(net::features::kUseStructuredDnsErrors,
                                  test.ede);
    CHECK_EQ(Run<false>(test, 200), Run<true>(test, 200));
    if (verify_only) {
      continue;
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double baseline, rust;
      if (rust_first) {
        rust = Measure<true>(test, kIterations);
        baseline = Measure<false>(test, kIterations);
      } else {
        baseline = Measure<false>(test, kIterations);
        rust = Measure<true>(test, kIterations);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          test.key, repeat, baseline, rust, kIterations,
          rust_first ? "true" : "false"));
    }
  }
  return 0;
}
