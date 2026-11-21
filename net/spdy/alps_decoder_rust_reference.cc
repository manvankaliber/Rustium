// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/alps_decoder_rust_reference.h"

#include <string_view>

#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "net/base/features.h"

namespace net {
namespace {

bool ReadUint16PrefixedStringPiece(std::string_view* payload,
                                   std::string_view* output) {
  if (payload->size() < 2) {
    return false;
  }
  const uint16_t length = (static_cast<uint16_t>((*payload)[0]) << 8) +
                          (static_cast<uint8_t>((*payload)[1]));
  payload->remove_prefix(2);

  if (payload->size() < length) {
    return false;
  }
  *output = payload->substr(0, length);
  payload->remove_prefix(length);

  return true;
}

}  // anonymous namespace

AlpsDecoderRustReference::AlpsDecoderRustReference() {
  decoder_adapter_.set_visitor(&settings_parser_);
  decoder_adapter_.set_extension_visitor(&accept_ch_parser_);
}

AlpsDecoderRustReference::~AlpsDecoderRustReference() = default;

AlpsDecoderRustReference::Error AlpsDecoderRustReference::Decode(
    base::span<const char> data) {
  decoder_adapter_.ProcessInput(data.data(), data.size());

  // Log if any errors were bypassed.
  base::UmaHistogramEnumeration("Net.SpdySession.AlpsDecoderStatus.Bypassed",
                                accept_ch_parser_.error_bypass());

  if (decoder_adapter_.HasError()) {
    return Error::kFramingError;
  }

  if (settings_parser_.forbidden_frame_received()) {
    return Error::kForbiddenFrame;
  }

  if (settings_parser_.settings_ack_received()) {
    return Error::kSettingsWithAck;
  }

  if (decoder_adapter_.state() !=
      http2::Http2DecoderAdapter::SPDY_READY_FOR_FRAME) {
    return Error::kNotOnFrameBoundary;
  }

  return accept_ch_parser_.error();
}

int AlpsDecoderRustReference::settings_frame_count() const {
  return settings_parser_.settings_frame_count();
}

AlpsDecoderRustReference::SettingsParser::SettingsParser() = default;
AlpsDecoderRustReference::SettingsParser::~SettingsParser() = default;

void AlpsDecoderRustReference::SettingsParser::OnCommonHeader(
    spdy::SpdyStreamId /*stream_id*/,
    size_t /*length*/,
    uint8_t type,
    uint8_t /*flags*/) {
  if (type == static_cast<uint8_t>(http2::Http2FrameType::DATA) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::HEADERS) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::PRIORITY) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::RST_STREAM) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::PUSH_PROMISE) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::PING) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::GOAWAY) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::WINDOW_UPDATE) ||
      type == static_cast<uint8_t>(http2::Http2FrameType::CONTINUATION)) {
    forbidden_frame_received_ = true;
  }
}

void AlpsDecoderRustReference::SettingsParser::OnSettings() {
  settings_frame_count_++;
}
void AlpsDecoderRustReference::SettingsParser::OnSetting(
    spdy::SpdySettingsId id,
    uint32_t value) {
  settings_[id] = value;
}

void AlpsDecoderRustReference::SettingsParser::OnSettingsAck() {
  settings_ack_received_ = true;
}

AlpsDecoderRustReference::AcceptChParser::AcceptChParser() = default;
AlpsDecoderRustReference::AcceptChParser::~AcceptChParser() = default;

bool AlpsDecoderRustReference::AcceptChParser::OnFrameHeader(
    spdy::SpdyStreamId stream_id,
    size_t length,
    uint8_t type,
    uint8_t flags) {
  // Ignore data after an error has occurred.
  if (error_ != Error::kNoError) {
    return false;
  }
  // Stop all alps parsing if it's disabled.
  if (!base::FeatureList::IsEnabled(features::kAlpsParsing)) {
    return false;
  }

  // Handle per-type parsing.
  switch (type) {
    case static_cast<uint8_t>(spdy::SpdyFrameType::ACCEPT_CH): {
      // Check for issues with the frame.
      if (stream_id != 0) {
        error_ = Error::kAcceptChInvalidStream;
        return false;
      }
      if (flags != 0) {
        error_ = Error::kAcceptChWithFlags;
        return false;
      }
      // This frame can be parsed in OnFramePayload.
      return true;
    }
    default:
      // Ignore all other types.
      return false;
  }
}

void AlpsDecoderRustReference::AcceptChParser::OnFramePayload(const char* data,
                                                              size_t len) {
  DCHECK_EQ(Error::kNoError, error_);

  std::string_view payload(data, len);

  while (!payload.empty()) {
    std::string_view origin;
    std::string_view value;
    if (!ReadUint16PrefixedStringPiece(&payload, &origin) ||
        !ReadUint16PrefixedStringPiece(&payload, &value)) {
      // This logs that a session termination was bypassed.
      error_bypass_ = Error::kAcceptChMalformed;
      return;
    }
    accept_ch_.emplace_back(std::string(origin), std::string(value));
  }
}

}  // namespace net
