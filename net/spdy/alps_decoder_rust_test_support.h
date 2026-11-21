// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_ALPS_DECODER_RUST_TEST_SUPPORT_H_
#define NET_SPDY_ALPS_DECODER_RUST_TEST_SUPPORT_H_
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/spdy/alps_decoder.h"
#include "net/spdy/alps_decoder_rust_reference.h"

namespace net::alps_rust_test {
inline constexpr char kHistogram[] =
    "Net.SpdySession.AlpsDecoderStatus.Bypassed";
inline std::string Frame(uint8_t type,
                         uint8_t flags,
                         uint32_t stream,
                         std::string_view payload) {
  std::string result;
  for (int shift : {16, 8, 0}) {
    result.push_back(static_cast<char>(payload.size() >> shift));
  }
  result.push_back(static_cast<char>(type));
  result.push_back(static_cast<char>(flags));
  for (int shift : {24, 16, 8, 0}) {
    result.push_back(static_cast<char>(stream >> shift));
  }
  result += payload;
  return result;
}
inline std::string Pair(std::string_view origin, std::string_view value) {
  std::string result;
  for (auto part : {origin, value}) {
    CHECK(part.size() <= 65535);
    result.push_back(static_cast<char>(part.size() >> 8));
    result.push_back(static_cast<char>(part.size()));
    result += part;
  }
  return result;
}
inline std::string Setting(uint16_t id, uint32_t value) {
  std::string out;
  out.push_back(static_cast<char>(id >> 8));
  out.push_back(static_cast<char>(id));
  for (int shift : {24, 16, 8, 0}) {
    out.push_back(static_cast<char>(value >> shift));
  }
  return out;
}

// Compare actual public APIs, including mutation visible after every Decode,
// order, duplicate settings and exact byte strings. Diagnostics are separately
// sampled around each call, so repeated calls cannot hide swapped samples.
inline void Compare(std::string_view input, bool enabled, size_t split) {
  base::test::ScopedFeatureList feature;
  feature.InitWithFeatureState(features::kAlpsParsing, enabled);
  AlpsDecoder rust;
  AlpsDecoderRustReference original;
  CHECK(rust.GetSettings().empty() && rust.GetAcceptCh().empty());
  CHECK(rust.settings_frame_count() == 0);
  std::vector<std::string_view> calls;
  if (split <= input.size()) {
    calls = {input.substr(0, split), input.substr(split), {}};
  } else {
    calls = {input};
  }
  for (auto part : calls) {
    base::HistogramTester histogram;
    const auto baseline = original.Decode(part);
    const auto baseline_samples = histogram.GetAllSamples(kHistogram);
    const auto actual = rust.Decode(part);
    const auto combined = histogram.GetAllSamples(kHistogram);
    CHECK(static_cast<int>(baseline) == static_cast<int>(actual));
    CHECK(original.GetSettings() == rust.GetSettings());
    CHECK(original.GetAcceptCh() == rust.GetAcceptCh());
    CHECK(original.settings_frame_count() == rust.settings_frame_count());
    CHECK(baseline_samples.size() == 1 && baseline_samples[0].count == 1);
    CHECK(combined.size() == 1 && combined[0].count == 2);
    CHECK(baseline_samples[0].min == combined[0].min);
  }
}

inline void Fuzz(std::string_view bytes) {
  if (bytes.empty()) {
    Compare({}, true, 1);
    return;
  }
  const uint8_t control = static_cast<uint8_t>(bytes.front());
  bytes.remove_prefix(1);
  std::string input(bytes);
  // Half the inputs exercise framing-valid extension/settings traffic rather
  // than stopping at a random invalid header. Byte mutations remain intact in
  // raw mode. Both paths still use the complete production frame adapter.
  if (control & 1) {
    input = Frame(0x89, control & 8 ? 1 : 0, control & 16 ? 1 : 0,
                  control & 32 ? std::string(bytes)
                               : Pair(bytes.substr(0, bytes.size() / 2),
                                      bytes.substr(bytes.size() / 2)));
    input += Frame(4, control & 64 ? 1 : 0, 0,
                   control & 64
                       ? ""
                       : Setting(control, static_cast<uint32_t>(bytes.size())) +
                             Setting(control, control));
    if (control & 128) {
      input += Frame(0, 1, 1, bytes);
    }
  }
  const size_t split =
      control & 4 ? std::min<size_t>(control, input.size()) : input.size() + 1;
  Compare(input, (control & 2) == 0, split);
}
}  // namespace net::alps_rust_test
#endif  // NET_SPDY_ALPS_DECODER_RUST_TEST_SUPPORT_H_
