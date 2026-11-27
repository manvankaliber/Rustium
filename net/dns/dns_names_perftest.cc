// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/i18n/icu_util.h"
#include "net/dns/dns_names_test_support.h"

namespace {
enum class Kind { Dotted, Network, U8, U16, Record, Canonical, Compare };
struct Case {
  const char* key;
  std::string input;
  Kind kind;
  bool option = false;
};
template <bool Rust>
uint64_t Run(const Case& test, size_t iterations) {
  namespace current = net::dns_names_util;
  namespace old = net::dns_names_reference;
  uint64_t checksum = 0;
  for (size_t i = 0; i < iterations; ++i) {
    switch (test.kind) {
      case Kind::Dotted: {
        auto result =
            Rust ? current::DottedNameToNetwork(test.input, test.option)
                 : old::DottedNameToNetwork(test.input, test.option);
        checksum += result.has_value();
        if (result) {
          for (uint8_t byte : *result) {
            checksum += byte;
          }
        }
        break;
      }
      case Kind::Network: {
        base::SpanReader reader(base::as_byte_span(test.input));
        auto result = Rust ? current::NetworkToDottedName(reader, test.option)
                           : old::NetworkToDottedName(reader, test.option);
        checksum += result.has_value() + reader.num_read();
        if (result) {
          for (uint8_t byte : *result) {
            checksum += byte;
          }
        }
        break;
      }
      case Kind::U8:
      case Kind::U16: {
        base::SpanReader reader(base::as_byte_span(test.input));
        base::span<const uint8_t> output;
        const bool wide = test.kind == Kind::U16;
        const bool ok =
            Rust ? (wide ? current::ReadU16LengthPrefixed(reader, &output)
                         : current::ReadU8LengthPrefixed(reader, &output))
                 : (wide ? old::ReadU16LengthPrefixed(reader, &output)
                         : old::ReadU8LengthPrefixed(reader, &output));
        checksum += ok + reader.num_read() + output.size();
        if (!output.empty()) {
          checksum += output.front();
        }
        break;
      }
      case Kind::Record:
        checksum += Rust ? current::IsValidDnsRecordName(test.input)
                         : old::IsValidDnsRecordName(test.input);
        break;
      case Kind::Canonical: {
        auto result = Rust ? current::UrlCanonicalizeNameIfAble(test.input)
                           : old::UrlCanonicalizeNameIfAble(test.input);
        for (uint8_t byte : result) {
          checksum += byte;
        }
        break;
      }
      case Kind::Compare:
        checksum +=
            Rust ? current::DomainNameComparator()(test.input, "Example.test")
                 : old::DomainNameComparator()(test.input, "Example.test");
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
  CHECK(base::i18n::InitializeICU());
  std::string maximum;
  for (size_t i = 0; i < 4; ++i) {
    if (i) {
      maximum += '.';
    }
    maximum.append(i == 3 ? 61 : 63, 'a');
  }
  const auto maximum_wire =
      net::dns_names_reference::DottedNameToNetwork(maximum);
  CHECK(maximum_wire);
  const std::string wire(maximum_wire->begin(), maximum_wire->end());
  const std::vector<Case> cases = {
      {"dns_dotted_short", "www.example.test", Kind::Dotted},
      {"dns_dotted_max", maximum, Kind::Dotted},
      {"dns_dotted_invalid", "a..test", Kind::Dotted},
      {"dns_dotted_bytes", std::string("a\0\xff.test", 9), Kind::Dotted},
      {"dns_dotted_strict", "www.example.test", Kind::Dotted, true},
      {"dns_network_root", std::string(1, 0), Kind::Network, true},
      {"dns_network_short", std::string("\3www\7example\4test\0", 18),
       Kind::Network, true},
      {"dns_network_max", wire, Kind::Network, true},
      {"dns_network_truncated", std::string("\1a\3bc", 5), Kind::Network, true},
      {"dns_network_oversize", std::string(1, 64) + std::string(64, 'a'),
       Kind::Network},
      {"dns_read_u8", std::string("\3abc", 4), Kind::U8},
      {"dns_read_u16", std::string("\0\3abc", 5), Kind::U16},
      {"dns_read_truncated", std::string("\0\x40\0", 3), Kind::U16},
      {"dns_record_valid", "_service._tcp.example.test", Kind::Record},
      {"dns_record_localhost", "LOCALHOST.", Kind::Record},
      {"dns_record_ip", "[2001:db8::1]", Kind::Record},
      {"dns_canonical_ascii", "WWW.EXAMPLE.TEST", Kind::Canonical},
      {"dns_canonical_unicode", "g\xc3\xbcgle.test", Kind::Canonical},
      {"dns_canonical_broken", "G<OO>GLE.test", Kind::Canonical},
      {"dns_compare_ascii", "eXAMPLE.Test", Kind::Compare},
      {"dns_compare_bytes", std::string("\xff\0A", 3), Kind::Compare},
  };
  const bool verify_only =
      argc > 1 && std::string_view(argv[1]) == "--verify-only";
  std::mt19937 random(20261112);
  constexpr size_t kIterations = 20000;
  for (const auto& test : cases) {
    net::dns_names_test::Check(base::as_byte_span(test.input));
    CHECK(Run<false>(test, 200) == Run<true>(test, 200));
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
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,"
                      "\"rust_ns\":%.9f,"
                      "\"iterations\":%zu,\"rust_first\":%s}\n",
                      test.key, repeat, baseline, rust, kIterations,
                      rust_first ? "true" : "false"));
    }
  }
  return 0;
}
