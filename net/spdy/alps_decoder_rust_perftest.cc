// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>

#include "base/feature_list.h"
#include "net/spdy/alps_decoder_rust_test_support.h"

namespace net::alps_rust_test {
namespace {
struct Fixture {
  const char* name;
  std::string input;
  int error = 0;
  int frames = 0;
  spdy::SettingsMap settings;
  std::vector<spdy::AcceptChOriginValuePair> accept;
  bool enabled = true;
};
std::vector<Fixture> Fixtures() {
  std::vector<Fixture> fixtures;
  fixtures.push_back({"empty", ""});
  Fixture settings{
      "settings_duplicates",
      Frame(4, 0, 0, Setting(7, 1) + Setting(0xffff, 9) + Setting(7, 42)),
      0,
      1,
      {{7, 42}, {0xffff, 9}}};
  fixtures.push_back(settings);
  const std::string origin("a\0\xff", 3);
  const std::string value("\x80\0z", 3);
  fixtures.push_back({"accept_bytes",
                      Frame(0x89, 0, 0, Pair(origin, value)),
                      0,
                      0,
                      {},
                      {{origin, value}}});
  Fixture many{"accept_many", ""};
  std::string payload;
  for (size_t i = 0; i < 64; ++i) {
    auto host = "https://h" + std::to_string(i) + ".test";
    const char* hints = "sec-ch-ua, sec-ch-ua-platform";
    payload += Pair(host, hints);
    many.accept.push_back({host, hints});
  }
  many.input = Frame(0x89, 0, 0, payload);
  fixtures.push_back(many);
  fixtures.push_back({"malformed_tail",
                      Frame(0x89, 0, 0, Pair("ok", "kept") + "x"),
                      0,
                      0,
                      {},
                      {{"ok", "kept"}}});
  fixtures.push_back({"disabled",
                      many.input + settings.input,
                      0,
                      1,
                      settings.settings,
                      {},
                      false});
  fixtures.push_back({"forbidden", settings.input + Frame(0, 1, 1, "body"), 2,
                      1, settings.settings});
  fixtures.push_back({"settings_ack", settings.input + Frame(4, 1, 0, ""), 4, 1,
                      settings.settings});
  fixtures.push_back(
      {"invalid_stream", Frame(0x89, 255, 1, Pair("x", "y")), 5});
  auto partial = settings.input;
  partial.pop_back();
  fixtures.push_back({"partial_frame", partial, 3, 1, {{7, 1}, {0xffff, 9}}});
  return fixtures;
}
uint64_t Mix(uint64_t sum, uint64_t value) {
  return (sum ^ value) * 0x100000001b3ull;
}
uint64_t Bytes(uint64_t sum, std::string_view value) {
  sum = Mix(sum, value.size());
  for (unsigned char ch : value) {
    sum = Mix(sum, ch);
  }
  return sum;
}
template <class Decoder>
uint64_t Run(const Fixture& fixture, size_t iterations) {
  uint64_t sum = 0xcbf29ce484222325ull;
  for (size_t i = 0; i < iterations; ++i) {
    Decoder decoder;
    sum = Mix(sum, static_cast<int>(decoder.Decode(fixture.input)));
    sum = Mix(sum, decoder.settings_frame_count());
    sum = Mix(sum, decoder.GetSettings().size());
    for (const auto& [id, value] : decoder.GetSettings()) {
      sum = Mix(Mix(sum, id), value);
    }
    sum = Mix(sum, decoder.GetAcceptCh().size());
    for (const auto& pair : decoder.GetAcceptCh()) {
      sum = Bytes(Bytes(sum, pair.origin), pair.value);
    }
  }
  return sum;
}
template <class Decoder>
void Verify(const Fixture& fixture) {
  Decoder decoder;
  CHECK(static_cast<int>(decoder.Decode(fixture.input)) == fixture.error);
  CHECK(decoder.settings_frame_count() == fixture.frames);
  CHECK(decoder.GetSettings() == fixture.settings);
  CHECK(decoder.GetAcceptCh() == fixture.accept);
}
volatile uint64_t sink;
template <class Decoder>
double Measure(const Fixture& fixture, size_t iterations, uint64_t expected) {
  const auto start = std::chrono::steady_clock::now();
  const auto checksum = Run<Decoder>(fixture, iterations);
  const auto stop = std::chrono::steady_clock::now();
  sink = checksum;
  CHECK(checksum == expected);
  return std::chrono::duration<double, std::nano>(stop - start).count() /
         iterations;
}
}  // namespace
}  // namespace net::alps_rust_test
int main(int argc, char** argv) {
  using namespace net;
  using namespace net::alps_rust_test;
  const bool verify =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) {
    UNSAFE_BUFFERS(std::fputs("Specify --verify-only or --timing\n", stderr));
    return 2;
  }
  std::mt19937 random(20261113);
  for (const auto& fixture : Fixtures()) {
    base::test::ScopedFeatureList feature;
    feature.InitWithFeatureState(features::kAlpsParsing, fixture.enabled);
    Verify<AlpsDecoder>(fixture);
    Verify<AlpsDecoderRustReference>(fixture);
    const auto preflight = Run<AlpsDecoderRustReference>(fixture, 256);
    CHECK(preflight == Run<AlpsDecoder>(fixture, 256));
    if (verify) {
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"verified\":true,\"iterations\":256,\"checksum\":"
          "\"%016llx\"}\n",
          fixture.name, static_cast<unsigned long long>(preflight)));
      continue;  // No clock calls occur on --verify-only.
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double baseline_ns, rust_ns;
      if (rust_first) {
        rust_ns = Measure<AlpsDecoder>(fixture, 256, preflight);
        baseline_ns =
            Measure<AlpsDecoderRustReference>(fixture, 256, preflight);
      } else {
        baseline_ns =
            Measure<AlpsDecoderRustReference>(fixture, 256, preflight);
        rust_ns = Measure<AlpsDecoder>(fixture, 256, preflight);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":256,\"rust_first\":%s,\"checksum\":\"%016llx\"}\n",
          fixture.name, repeat, baseline_ns, rust_ns,
          rust_first ? "true" : "false",
          static_cast<unsigned long long>(preflight)));
    }
  }
}
