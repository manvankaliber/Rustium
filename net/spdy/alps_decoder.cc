// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/spdy/alps_decoder.h"

#include <array>

#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "net/base/features.h"
#include "net/spdy/alps_decoder_rust_ffi.h"

namespace net {
namespace {
extern "C" bool AlpsParsingEnabled() {
  return base::FeatureList::IsEnabled(features::kAlpsParsing);
}
}  // namespace
AlpsDecoder::AlpsDecoder() {
  decoder_adapter_.set_visitor(&settings_parser_);
  decoder_adapter_.set_extension_visitor(&accept_ch_parser_);
}
AlpsDecoder::~AlpsDecoder() = default;

AlpsDecoder::Error AlpsDecoder::Decode(base::span<const char> data) {
  decoder_adapter_.ProcessInput(data.data(), data.size());
  base::UmaHistogramEnumeration("Net.SpdySession.AlpsDecoderStatus.Bypassed",
                                accept_ch_parser_.error_bypass());
  return static_cast<Error>(alps_decoder_rust_finish(
      decoder_adapter_.HasError(), settings_parser_.forbidden_frame_received(),
      settings_parser_.settings_ack_received(),
      decoder_adapter_.state() ==
          http2::Http2DecoderAdapter::SPDY_READY_FOR_FRAME,
      static_cast<int32_t>(accept_ch_parser_.error())));
}
int AlpsDecoder::settings_frame_count() const {
  return settings_parser_.settings_frame_count();
}
AlpsDecoder::SettingsParser::SettingsParser() = default;
AlpsDecoder::SettingsParser::~SettingsParser() = default;
void AlpsDecoder::SettingsParser::OnCommonHeader(spdy::SpdyStreamId,
                                                 size_t,
                                                 uint8_t type,
                                                 uint8_t) {
  forbidden_frame_received_ =
      alps_decoder_rust_common_header(forbidden_frame_received_, type);
}
void AlpsDecoder::SettingsParser::OnSettings() {
  settings_frame_count_ =
      alps_decoder_rust_settings_start(settings_frame_count_);
}
void AlpsDecoder::SettingsParser::OnSetting(spdy::SpdySettingsId id,
                                            uint32_t value) {
  const auto action = alps_decoder_rust_setting(id, value);
  // Native map ownership service: Rust requests an unconditional upsert,
  // including duplicates and unknown IDs. Map ordering is the public ABI.
  settings_[action.id] = action.value;
}
void AlpsDecoder::SettingsParser::OnSettingsAck() {
  settings_ack_received_ = alps_decoder_rust_settings_ack();
}
AlpsDecoder::AcceptChParser::AcceptChParser() = default;
AlpsDecoder::AcceptChParser::~AcceptChParser() = default;
bool AlpsDecoder::AcceptChParser::OnFrameHeader(spdy::SpdyStreamId stream_id,
                                                size_t,
                                                uint8_t type,
                                                uint8_t flags) {
  const auto decision = alps_decoder_rust_header(
      static_cast<int32_t>(error_), AlpsParsingEnabled, stream_id, type, flags);
  error_ = static_cast<Error>(decision.error);
  return decision.accept;
}
void AlpsDecoder::AcceptChParser::OnFramePayload(const char* data, size_t len) {
  DCHECK_EQ(Error::kNoError, error_);
  std::array<AlpsRustPair, 8> inline_pairs;
  const auto result = alps_decoder_rust_payload_into(
      reinterpret_cast<const uint8_t*>(data), len, inline_pairs.data(),
      inline_pairs.size(), static_cast<int32_t>(error_bypass_));
  // QUICHE supplies initialized bytes. Rust returns only validated ranges;
  // its input borrow has ended before native containers mutate. No C++
  // parser, payload buffering, per-pair FFI, or old implementation fallback.
  for (const auto& pair :
       UNSAFE_BUFFERS(base::span(result.pairs, result.count))) {
    accept_ch_.emplace_back(
        std::string(UNSAFE_BUFFERS(data + pair.origin), pair.origin_len),
        std::string(UNSAFE_BUFFERS(data + pair.value), pair.value_len));
  }
  error_bypass_ = static_cast<Error>(result.bypass);
  if (result.owner != nullptr) {
    alps_decoder_rust_payload_free(result.owner);
  }
}
}  // namespace net
