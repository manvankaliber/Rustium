// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_session.h"

#include "net/spdy/spdy_session_rust_ffi.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "net/base/features.h"
#include "net/base/privacy_mode.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_string_util.h"
#include "net/base/session_usage.h"
#include "net/base/url_util.h"
#include "net/cert/asn1_util.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/ct_policy_status.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties.h"
#include "net/http/http_util.h"
#include "net/http/http_vary_data.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/quic/quic_http_utils.h"
#include "net/socket/socket.h"
#include "net/socket/ssl_client_socket.h"
#include "net/spdy/alps_decoder.h"
#include "net/spdy/header_coalescer.h"
#include "net/spdy/spdy_buffer_producer.h"
#include "net/spdy/spdy_http_utils.h"
#include "net/spdy/spdy_log_util.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/spdy/spdy_stream.h"
#include "net/ssl/ssl_cipher_suite_names.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_frame_builder.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

namespace net {

namespace {

constexpr net::NetworkTrafficAnnotationTag
    kSpdySessionCommandsTrafficAnnotation =
        net::DefineNetworkTrafficAnnotation("spdy_session_control", R"(
        semantics {
          sender: "Spdy Session"
          description:
            "Sends commands to control an HTTP/2 session."
          trigger:
            "Required control commands like initiating stream, requesting "
            "stream reset, changing priorities, etc."
          data: "No user data."
          destination: OTHER
          destination_other:
            "Any destination the HTTP/2 session is connected to."
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled in settings."
          policy_exception_justification: "Essential for network access."
        }
    )");

const int kReadBufferSize = 8 * 1024;
const int kHungIntervalSeconds = 10;

// Default initial value for HTTP/2 SETTINGS.
const uint32_t kDefaultInitialHeaderTableSize = 4096;
const uint32_t kDefaultInitialEnablePush = 1;
const uint32_t kDefaultInitialInitialWindowSize = 65535;
const uint32_t kDefaultInitialMaxFrameSize = 16384;


// Bind protocol/default scalar constants used directly by the safe Rust core.
static_assert(spdy::SETTINGS_HEADER_TABLE_SIZE==1);
static_assert(spdy::SETTINGS_ENABLE_PUSH==2);
static_assert(spdy::SETTINGS_INITIAL_WINDOW_SIZE==4);
static_assert(spdy::SETTINGS_MAX_FRAME_SIZE==5);
static_assert(spdy::SETTINGS_ENABLE_CONNECT_PROTOCOL==8);
static_assert(kDefaultInitialHeaderTableSize==4096);
static_assert(kDefaultInitialEnablePush==1);
static_assert(kDefaultInitialInitialWindowSize==65535);
static_assert(kDefaultInitialMaxFrameSize==16384);

// These values are persisted to logs. Entries should not be renumbered, and
// numeric values should never be reused.
enum class SpdyAcceptChEntries {
  kNoEntries = 0,
  kOnlyValidEntries = 1,
  kOnlyInvalidEntries = 2,
  kBothValidAndInvalidEntries = 3,
  kMaxValue = kBothValidAndInvalidEntries,
};

// A SpdyBufferProducer implementation that creates an HTTP/2 frame by adding
// stream ID to greased frame parameters.
class GreasedBufferProducer : public SpdyBufferProducer {
 public:
  GreasedBufferProducer() = delete;
  GreasedBufferProducer(
      base::WeakPtr<SpdyStream> stream,
      const SpdySessionPool::GreasedHttp2Frame* greased_http2_frame,
      BufferedSpdyFramer* buffered_spdy_framer)
      : stream_(stream),
        greased_http2_frame_(greased_http2_frame),
        buffered_spdy_framer_(buffered_spdy_framer) {}

  ~GreasedBufferProducer() override = default;

  std::unique_ptr<SpdyBuffer> ProduceBuffer() override;

 private:
  base::WeakPtr<SpdyStream> stream_;
  const raw_ptr<const SpdySessionPool::GreasedHttp2Frame> greased_http2_frame_;
  raw_ptr<BufferedSpdyFramer> buffered_spdy_framer_;
};

std::unique_ptr<SpdyBuffer> GreasedBufferProducer::ProduceBuffer() {
spdy::SpdyStreamId stream_id=0;
  std::optional<spdy::SpdyUnknownIR> frame;
  std::unique_ptr<spdy::SpdySerializedFrame> serialized;
  std::unique_ptr<SpdyBuffer> result;

  RunSpdySessionRust(129,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return !!stream_; }
      case 1: { return stream_->stream_id(); }
      case 2: { stream_id=static_cast<spdy::SpdyStreamId>(a);
return 0; }
      case 3: { frame.emplace(stream_id,greased_http2_frame_->type,greased_http2_frame_->flags,greased_http2_frame_->payload);
return 0; }
      case 4: { serialized=std::make_unique<spdy::SpdySerializedFrame>(buffered_spdy_framer_->SerializeFrame(*frame));
return 0; }
      case 5: { result=std::make_unique<SpdyBuffer>(std::move(serialized));
return 0; }
    }
    NOTREACHED();
  });
return result;
}

[[maybe_unused]] bool IsSpdySettingAtDefaultInitialValue(spdy::SpdySettingsId setting_id,
                                        uint32_t value) {
const int64_t rust_result = RunSpdySessionRust(93,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return setting_id; }
      case 1: { return value; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

void LogSpdyAcceptChForOriginHistogram(bool value) {
  base::UmaHistogramBoolean("Net.SpdySession.AcceptChForOrigin", value);
}

void LogSessionCreationInitiatorToHistogram(
    MultiplexedSessionCreationInitiator session_creation,
    bool is_used) {
std::string suffix;
  RunSpdySessionRust(125,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return is_used; }
      case 1: { suffix=".Used";
return 0; }
      case 2: { suffix=".Unused";
return 0; }
      case 3: { const std::string histogram_name=base::StrCat({"Net.SpdySession.GoogleSearch.SessionCreationInitiator",suffix}); base::UmaHistogramEnumeration(histogram_name,session_creation);
return 0; }
    }
    NOTREACHED();
  });
}

base::DictValue NetLogSpdyHeadersSentParams(
    const quiche::HttpHeaderBlock* headers,
    bool fin,
    spdy::SpdyStreamId stream_id,
    bool has_priority,
    int weight,
    spdy::SpdyStreamId parent_stream_id,
    bool exclusive,
    NetLogSource source_dependency,
    NetLogCaptureMode capture_mode) {
base::DictValue dict;
  RunSpdySessionRust(126,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { dict=base::DictValue().Set("headers",ElideHttpHeaderBlockForNetLog(*headers,capture_mode)).Set("fin",fin).Set("stream_id",static_cast<int>(stream_id)).Set("has_priority",has_priority);
return 0; }
      case 1: { return has_priority; }
      case 2: { dict.Set("parent_stream_id",static_cast<int>(parent_stream_id)); dict.Set("weight",weight); dict.Set("exclusive",exclusive);
return 0; }
      case 3: { return source_dependency.IsValid(); }
      case 4: { source_dependency.AddToEventParameters(dict);
return 0; }
    }
    NOTREACHED();
  });
return dict;
}

base::DictValue NetLogSpdyHeadersReceivedParams(
    const quiche::HttpHeaderBlock* headers,
    bool fin,
    spdy::SpdyStreamId stream_id,
    NetLogCaptureMode capture_mode) {
  return base::DictValue()
      .Set("headers", ElideHttpHeaderBlockForNetLog(*headers, capture_mode))
      .Set("fin", fin)
      .Set("stream_id", static_cast<int>(stream_id));
}

base::DictValue NetLogSpdySessionCloseParams(int net_error,
                                             const std::string& description) {
  return base::DictValue()
      .Set("net_error", net_error)
      .Set("description", description);
}

base::DictValue NetLogSpdyInitializedParams(NetLogSource source) {
base::DictValue dict;
  RunSpdySessionRust(127,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return source.IsValid(); }
      case 1: { source.AddToEventParameters(dict);
return 0; }
      case 2: { dict.Set("protocol",NextProtoToString(NextProto::kProtoHTTP2));
return 0; }
    }
    NOTREACHED();
  });
return dict;
}

base::DictValue NetLogSpdySendSettingsParams(
    const spdy::SettingsMap* settings) {
base::ListValue settings_list; spdy::SettingsMap::const_iterator it;
  RunSpdySessionRust(128,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { it=settings->begin();
return 0; }
      case 1: { return it!=settings->end(); }
      case 2: { settings_list.Append(base::StringPrintf("[id:%u (%s) value:%u]",it->first,spdy::SettingsIdToString(it->first).c_str(),it->second));
return 0; }
      case 3: { ++it;
return 0; }
    }
    NOTREACHED();
  });
return base::DictValue().Set("settings",std::move(settings_list));
}

base::DictValue NetLogSpdyRecvAcceptChParams(
    spdy::AcceptChOriginValuePair entry) {
  return base::DictValue()
      .Set("origin", entry.origin)
      .Set("accept_ch", entry.value);
}

base::DictValue NetLogSpdyRecvSettingParams(spdy::SpdySettingsId id,
                                            uint32_t value) {
  return base::DictValue()
      .Set("id", base::StringPrintf("%u (%s)", id,
                                    spdy::SettingsIdToString(id).c_str()))
      .Set("value", static_cast<int>(value));
}

base::DictValue NetLogSpdyWindowUpdateFrameParams(spdy::SpdyStreamId stream_id,
                                                  uint32_t delta) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("delta", static_cast<int>(delta));
}

base::DictValue NetLogSpdySessionWindowUpdateParams(int32_t delta,
                                                    int32_t window_size) {
  return base::DictValue().Set("delta", delta).Set("window_size", window_size);
}

base::DictValue NetLogSpdyDataParams(spdy::SpdyStreamId stream_id,
                                     int size,
                                     bool fin) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("size", size)
      .Set("fin", fin);
}

base::DictValue NetLogSpdyRecvRstStreamParams(spdy::SpdyStreamId stream_id,
                                              spdy::SpdyErrorCode error_code) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("error_code", base::StringPrintf("%u (%s)", error_code,
                                            ErrorCodeToString(error_code)));
}

base::DictValue NetLogSpdySendRstStreamParams(spdy::SpdyStreamId stream_id,
                                              spdy::SpdyErrorCode error_code,
                                              const std::string& description) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("error_code", base::StringPrintf("%u (%s)", error_code,
                                            ErrorCodeToString(error_code)))
      .Set("description", description);
}

base::DictValue NetLogSpdyPingParams(spdy::SpdyPingId unique_id,
                                     bool is_ack,
                                     const char* type) {
  return base::DictValue()
      .Set("unique_id", static_cast<int>(unique_id))
      .Set("type", type)
      .Set("is_ack", is_ack);
}

base::DictValue NetLogSpdyRecvGoAwayParams(spdy::SpdyStreamId last_stream_id,
                                           int active_streams,
                                           spdy::SpdyErrorCode error_code,
                                           std::string_view debug_data,
                                           NetLogCaptureMode capture_mode) {
  return base::DictValue()
      .Set("last_accepted_stream_id", static_cast<int>(last_stream_id))
      .Set("active_streams", active_streams)
      .Set("error_code", base::StringPrintf("%u (%s)", error_code,
                                            ErrorCodeToString(error_code)))
      .Set("debug_data",
           ElideGoAwayDebugDataForNetLog(capture_mode, debug_data));
}

base::DictValue NetLogSpdySessionStalledParams(size_t num_active_streams,
                                               size_t num_created_streams,
                                               size_t max_concurrent_streams,
                                               const std::string& url) {
  return base::DictValue()
      .Set("num_active_streams", static_cast<int>(num_active_streams))
      .Set("num_created_streams", static_cast<int>(num_created_streams))
      .Set("max_concurrent_streams", static_cast<int>(max_concurrent_streams))
      .Set("url", url);
}

base::DictValue NetLogSpdyPriorityParams(spdy::SpdyStreamId stream_id,
                                         spdy::SpdyStreamId parent_stream_id,
                                         int weight,
                                         bool exclusive) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("parent_stream_id", static_cast<int>(parent_stream_id))
      .Set("weight", weight)
      .Set("exclusive", exclusive);
}

base::DictValue NetLogSpdyGreasedFrameParams(spdy::SpdyStreamId stream_id,
                                             uint8_t type,
                                             uint8_t flags,
                                             size_t length,
                                             RequestPriority priority) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("type", type)
      .Set("flags", flags)
      .Set("length", static_cast<int>(length))
      .Set("priority", RequestPriorityToString(priority));
}

// Helper function to return the total size of an array of objects
// with .size() member functions.
template <typename T>
size_t GetTotalSize(const T& container_of_containers) {
const int64_t rust_result = RunSpdySessionRust(88,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return container_of_containers.size(); }
      case 1: { return base::span(container_of_containers)[a].size(); }
    }
    NOTREACHED();
  });
return static_cast<size_t>(rust_result);
}

// The maximum number of concurrent streams we will ever create.  Even if
// the server permits more, we will never exceed this limit.
const size_t kMaxConcurrentStreamLimit = 256;

}  // namespace

SpdyProtocolErrorDetails MapFramerErrorToProtocolError(
    http2::Http2DecoderAdapter::SpdyFramerError err) {
const std::array<std::pair<int64_t,int64_t>,28> mapping = {{{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_NO_ERROR),static_cast<int64_t>(SPDY_ERROR_NO_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_STREAM_ID),static_cast<int64_t>(SPDY_ERROR_INVALID_STREAM_ID)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_CONTROL_FRAME),static_cast<int64_t>(SPDY_ERROR_INVALID_CONTROL_FRAME)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_CONTROL_PAYLOAD_TOO_LARGE),static_cast<int64_t>(SPDY_ERROR_CONTROL_PAYLOAD_TOO_LARGE)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_DECOMPRESS_FAILURE),static_cast<int64_t>(SPDY_ERROR_DECOMPRESS_FAILURE)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_PADDING),static_cast<int64_t>(SPDY_ERROR_INVALID_PADDING)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_DATA_FRAME_FLAGS),static_cast<int64_t>(SPDY_ERROR_INVALID_DATA_FRAME_FLAGS)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_UNEXPECTED_FRAME),static_cast<int64_t>(SPDY_ERROR_UNEXPECTED_FRAME)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INTERNAL_FRAMER_ERROR),static_cast<int64_t>(SPDY_ERROR_INTERNAL_FRAMER_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_CONTROL_FRAME_SIZE),static_cast<int64_t>(SPDY_ERROR_INVALID_CONTROL_FRAME_SIZE)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_OVERSIZED_PAYLOAD),static_cast<int64_t>(SPDY_ERROR_OVERSIZED_PAYLOAD)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INDEX_VARINT_ERROR),static_cast<int64_t>(SPDY_ERROR_HPACK_INDEX_VARINT_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_NAME_LENGTH_VARINT_ERROR),static_cast<int64_t>(SPDY_ERROR_HPACK_NAME_LENGTH_VARINT_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_VALUE_LENGTH_VARINT_ERROR),static_cast<int64_t>(SPDY_ERROR_HPACK_VALUE_LENGTH_VARINT_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_NAME_TOO_LONG),static_cast<int64_t>(SPDY_ERROR_HPACK_NAME_TOO_LONG)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_VALUE_TOO_LONG),static_cast<int64_t>(SPDY_ERROR_HPACK_VALUE_TOO_LONG)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_NAME_HUFFMAN_ERROR),static_cast<int64_t>(SPDY_ERROR_HPACK_NAME_HUFFMAN_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_VALUE_HUFFMAN_ERROR),static_cast<int64_t>(SPDY_ERROR_HPACK_VALUE_HUFFMAN_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_MISSING_DYNAMIC_TABLE_SIZE_UPDATE),static_cast<int64_t>(SPDY_ERROR_HPACK_MISSING_DYNAMIC_TABLE_SIZE_UPDATE)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INVALID_INDEX),static_cast<int64_t>(SPDY_ERROR_HPACK_INVALID_INDEX)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INVALID_NAME_INDEX),static_cast<int64_t>(SPDY_ERROR_HPACK_INVALID_NAME_INDEX)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_NOT_ALLOWED),static_cast<int64_t>(SPDY_ERROR_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_NOT_ALLOWED)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INITIAL_DYNAMIC_TABLE_SIZE_UPDATE_IS_ABOVE_LOW_WATER_MARK),static_cast<int64_t>(SPDY_ERROR_HPACK_INITIAL_DYNAMIC_TABLE_SIZE_UPDATE_IS_ABOVE_LOW_WATER_MARK)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_IS_ABOVE_ACKNOWLEDGED_SETTING),static_cast<int64_t>(SPDY_ERROR_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_IS_ABOVE_ACKNOWLEDGED_SETTING)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_TRUNCATED_BLOCK),static_cast<int64_t>(SPDY_ERROR_HPACK_TRUNCATED_BLOCK)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_FRAGMENT_TOO_LONG),static_cast<int64_t>(SPDY_ERROR_HPACK_FRAGMENT_TOO_LONG)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_COMPRESSED_HEADER_SIZE_EXCEEDS_LIMIT),static_cast<int64_t>(SPDY_ERROR_HPACK_COMPRESSED_HEADER_SIZE_EXCEEDS_LIMIT)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_STOP_PROCESSING),static_cast<int64_t>(SPDY_ERROR_STOP_PROCESSING)}}};
  const int64_t rust_result = RunSpdySessionRust(89,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return err; }
      case 1: { return mapping.size(); }
      case 2: { return base::span(mapping)[a].first; }
      case 3: { return base::span(mapping)[a].second; }
      case 4: { NOTREACHED(); }
    }
    NOTREACHED();
  });
return static_cast<SpdyProtocolErrorDetails>(rust_result);
}

Error MapFramerErrorToNetError(
    http2::Http2DecoderAdapter::SpdyFramerError err) {
const std::array<std::pair<int64_t,int64_t>,28> mapping = {{{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_NO_ERROR),static_cast<int64_t>(OK)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_CONTROL_FRAME),static_cast<int64_t>(ERR_HTTP2_PROTOCOL_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_CONTROL_PAYLOAD_TOO_LARGE),static_cast<int64_t>(ERR_HTTP2_FRAME_SIZE_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_DECOMPRESS_FAILURE),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INDEX_VARINT_ERROR),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_NAME_LENGTH_VARINT_ERROR),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_VALUE_LENGTH_VARINT_ERROR),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_NAME_TOO_LONG),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_VALUE_TOO_LONG),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_NAME_HUFFMAN_ERROR),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_VALUE_HUFFMAN_ERROR),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_MISSING_DYNAMIC_TABLE_SIZE_UPDATE),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INVALID_INDEX),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INVALID_NAME_INDEX),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_NOT_ALLOWED),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_INITIAL_DYNAMIC_TABLE_SIZE_UPDATE_IS_ABOVE_LOW_WATER_MARK),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_IS_ABOVE_ACKNOWLEDGED_SETTING),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_TRUNCATED_BLOCK),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_FRAGMENT_TOO_LONG),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_HPACK_COMPRESSED_HEADER_SIZE_EXCEEDS_LIMIT),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_STOP_PROCESSING),static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_PADDING),static_cast<int64_t>(ERR_HTTP2_PROTOCOL_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_DATA_FRAME_FLAGS),static_cast<int64_t>(ERR_HTTP2_PROTOCOL_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_UNEXPECTED_FRAME),static_cast<int64_t>(ERR_HTTP2_PROTOCOL_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INTERNAL_FRAMER_ERROR),static_cast<int64_t>(ERR_HTTP2_PROTOCOL_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_CONTROL_FRAME_SIZE),static_cast<int64_t>(ERR_HTTP2_FRAME_SIZE_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_INVALID_STREAM_ID),static_cast<int64_t>(ERR_HTTP2_PROTOCOL_ERROR)},{static_cast<int64_t>(http2::Http2DecoderAdapter::SPDY_OVERSIZED_PAYLOAD),static_cast<int64_t>(ERR_HTTP2_FRAME_SIZE_ERROR)}}};
  const int64_t rust_result = RunSpdySessionRust(90,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return err; }
      case 1: { return mapping.size(); }
      case 2: { return base::span(mapping)[a].first; }
      case 3: { return base::span(mapping)[a].second; }
      case 4: { NOTREACHED(); }
    }
    NOTREACHED();
  });
return static_cast<Error>(rust_result);
}

SpdyProtocolErrorDetails MapRstStreamStatusToProtocolError(
    spdy::SpdyErrorCode error_code) {
const std::array<std::pair<int64_t,int64_t>,14> mapping = {{{static_cast<int64_t>(spdy::ERROR_CODE_NO_ERROR),static_cast<int64_t>(STATUS_CODE_NO_ERROR)},{static_cast<int64_t>(spdy::ERROR_CODE_PROTOCOL_ERROR),static_cast<int64_t>(STATUS_CODE_PROTOCOL_ERROR)},{static_cast<int64_t>(spdy::ERROR_CODE_INTERNAL_ERROR),static_cast<int64_t>(STATUS_CODE_INTERNAL_ERROR)},{static_cast<int64_t>(spdy::ERROR_CODE_FLOW_CONTROL_ERROR),static_cast<int64_t>(STATUS_CODE_FLOW_CONTROL_ERROR)},{static_cast<int64_t>(spdy::ERROR_CODE_SETTINGS_TIMEOUT),static_cast<int64_t>(STATUS_CODE_SETTINGS_TIMEOUT)},{static_cast<int64_t>(spdy::ERROR_CODE_STREAM_CLOSED),static_cast<int64_t>(STATUS_CODE_STREAM_CLOSED)},{static_cast<int64_t>(spdy::ERROR_CODE_FRAME_SIZE_ERROR),static_cast<int64_t>(STATUS_CODE_FRAME_SIZE_ERROR)},{static_cast<int64_t>(spdy::ERROR_CODE_REFUSED_STREAM),static_cast<int64_t>(STATUS_CODE_REFUSED_STREAM)},{static_cast<int64_t>(spdy::ERROR_CODE_CANCEL),static_cast<int64_t>(STATUS_CODE_CANCEL)},{static_cast<int64_t>(spdy::ERROR_CODE_COMPRESSION_ERROR),static_cast<int64_t>(STATUS_CODE_COMPRESSION_ERROR)},{static_cast<int64_t>(spdy::ERROR_CODE_CONNECT_ERROR),static_cast<int64_t>(STATUS_CODE_CONNECT_ERROR)},{static_cast<int64_t>(spdy::ERROR_CODE_ENHANCE_YOUR_CALM),static_cast<int64_t>(STATUS_CODE_ENHANCE_YOUR_CALM)},{static_cast<int64_t>(spdy::ERROR_CODE_INADEQUATE_SECURITY),static_cast<int64_t>(STATUS_CODE_INADEQUATE_SECURITY)},{static_cast<int64_t>(spdy::ERROR_CODE_HTTP_1_1_REQUIRED),static_cast<int64_t>(STATUS_CODE_HTTP_1_1_REQUIRED)}}};
  const int64_t rust_result = RunSpdySessionRust(91,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return error_code; }
      case 1: { return mapping.size(); }
      case 2: { return base::span(mapping)[a].first; }
      case 3: { return base::span(mapping)[a].second; }
      case 4: { NOTREACHED(); }
    }
    NOTREACHED();
  });
return static_cast<SpdyProtocolErrorDetails>(rust_result);
}

spdy::SpdyErrorCode MapNetErrorToGoAwayStatus(Error err) {
const std::array<std::pair<int64_t,int64_t>,6> mapping = {{{static_cast<int64_t>(OK),static_cast<int64_t>(spdy::ERROR_CODE_NO_ERROR)},{static_cast<int64_t>(ERR_HTTP2_PROTOCOL_ERROR),static_cast<int64_t>(spdy::ERROR_CODE_PROTOCOL_ERROR)},{static_cast<int64_t>(ERR_HTTP2_FLOW_CONTROL_ERROR),static_cast<int64_t>(spdy::ERROR_CODE_FLOW_CONTROL_ERROR)},{static_cast<int64_t>(ERR_HTTP2_FRAME_SIZE_ERROR),static_cast<int64_t>(spdy::ERROR_CODE_FRAME_SIZE_ERROR)},{static_cast<int64_t>(ERR_HTTP2_COMPRESSION_ERROR),static_cast<int64_t>(spdy::ERROR_CODE_COMPRESSION_ERROR)},{static_cast<int64_t>(ERR_HTTP2_INADEQUATE_TRANSPORT_SECURITY),static_cast<int64_t>(spdy::ERROR_CODE_INADEQUATE_SECURITY)}}};
  const int64_t rust_result = RunSpdySessionRust(92,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return err; }
      case 1: { return mapping.size(); }
      case 2: { return base::span(mapping)[a].first; }
      case 3: { return base::span(mapping)[a].second; }
      case 4: { return spdy::ERROR_CODE_PROTOCOL_ERROR; }
    }
    NOTREACHED();
  });
return static_cast<spdy::SpdyErrorCode>(rust_result);
}

SpdyStreamRequest::SpdyStreamRequest() {
  Reset();
}

SpdyStreamRequest::~SpdyStreamRequest() {
  CancelRequest();
}

int SpdyStreamRequest::StartRequest(
    SpdyStreamType type,
    const base::WeakPtr<SpdySession>& session,
    const GURL& url,
    bool can_send_early,
    RequestPriority priority,
    const SocketTag& socket_tag,
    const NetLogWithSource& net_log,
    CompletionOnceCallback callback,
    const NetworkTrafficAnnotationTag& traffic_annotation,
    bool detect_broken_connection,
    base::TimeDelta heartbeat_interval) {
base::WeakPtr<SpdyStream> created_stream;
  const int64_t rust_result = RunSpdySessionRust(44,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(session); DCHECK(!session_); DCHECK(!stream_); DCHECK(callback_.is_null()); DCHECK(url.is_valid())<<url.possibly_invalid_spec();
return 0; }
      case 1: { type_=type; session_=session; url_=SimplifyUrlForRequest(url); priority_=priority; socket_tag_=socket_tag; net_log_=net_log; callback_=std::move(callback); traffic_annotation_=MutableNetworkTrafficAnnotationTag(traffic_annotation); detect_broken_connection_=detect_broken_connection; heartbeat_interval_=heartbeat_interval;
return 0; }
      case 2: { return can_send_early; }
      case 3: { return session_->ConfirmHandshake(base::BindOnce(&SpdyStreamRequest::OnConfirmHandshakeComplete,weak_ptr_factory_.GetWeakPtr())); }
      case 4: { return session->TryCreateStream(weak_ptr_factory_.GetWeakPtr(),&created_stream); }
      case 5: { Reset();
return 0; }
      case 6: { stream_=created_stream;
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

void SpdyStreamRequest::CancelRequest() {
RunSpdySessionRust(45,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return !!session_; }
      case 1: { session_->CancelStreamRequest(weak_ptr_factory_.GetWeakPtr());
return 0; }
      case 2: { Reset();
return 0; }
      case 3: { weak_ptr_factory_.InvalidateWeakPtrs();
return 0; }
    }
    NOTREACHED();
  });
}

base::WeakPtr<SpdyStream> SpdyStreamRequest::ReleaseStream() {
base::WeakPtr<SpdyStream> result;
  RunSpdySessionRust(46,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(!session_); result=stream_; DCHECK(result);
return 0; }
      case 1: { Reset();
return 0; }
    }
    NOTREACHED();
  });
return result;
}

void SpdyStreamRequest::SetPriority(RequestPriority priority) {
RunSpdySessionRust(47,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return priority_==priority; }
      case 1: { return !!stream_; }
      case 2: { stream_->SetPriority(priority);
return 0; }
      case 3: { return !!session_; }
      case 4: { session_->ChangeStreamRequestPriority(weak_ptr_factory_.GetWeakPtr(),priority);
return 0; }
      case 5: { priority_=priority;
return 0; }
    }
    NOTREACHED();
  });
}

void SpdyStreamRequest::OnRequestCompleteSuccess(
    const base::WeakPtr<SpdyStream>& stream) {
CompletionOnceCallback callback;
  RunSpdySessionRust(48,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(session_); DCHECK(!stream_); DCHECK(!callback_.is_null()); callback=std::move(callback_);
return 0; }
      case 1: { Reset();
return 0; }
      case 2: { DCHECK(stream); stream_=stream;
return 0; }
      case 3: { std::move(callback).Run(OK);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdyStreamRequest::OnRequestCompleteFailure(int rv) {
CompletionOnceCallback callback;
  RunSpdySessionRust(49,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(session_); DCHECK(!stream_); DCHECK(!callback_.is_null()); callback=std::move(callback_);
return 0; }
      case 1: { Reset();
return 0; }
      case 2: { DCHECK_NE(rv,OK);
return 0; }
      case 3: { std::move(callback).Run(rv);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdyStreamRequest::Reset() {
RunSpdySessionRust(50,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { type_=SPDY_BIDIRECTIONAL_STREAM;
return 0; }
      case 1: { session_.reset();
return 0; }
      case 2: { stream_.reset();
return 0; }
      case 3: { url_=GURL();
return 0; }
      case 4: { priority_=MINIMUM_PRIORITY;
return 0; }
      case 5: { socket_tag_=SocketTag();
return 0; }
      case 6: { net_log_=NetLogWithSource();
return 0; }
      case 7: { callback_.Reset();
return 0; }
      case 8: { traffic_annotation_.reset();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdyStreamRequest::OnConfirmHandshakeComplete(int rv) {
base::WeakPtr<SpdyStream> stream;
  RunSpdySessionRust(51,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_NE(ERR_IO_PENDING,rv);
return 0; }
      case 1: { return !!session_; }
      case 2: { return rv; }
      case 3: { OnRequestCompleteFailure(static_cast<int>(a));
return 0; }
      case 4: { confirm_handshake_end_=base::TimeTicks::Now();
return 0; }
      case 5: { return ERR_CONNECTION_CLOSED; }
      case 6: { rv=session_->TryCreateStream(weak_ptr_factory_.GetWeakPtr(),&stream); return rv; }
      case 7: { OnRequestCompleteSuccess(stream);
return 0; }
    }
    NOTREACHED();
  });
}

// static
bool SpdySession::CanPool(TransportSecurityState* transport_security_state,
                          const SSLInfo& ssl_info,
                          const SSLConfigService& ssl_config_service,
                          std::string_view old_hostname,
                          std::string_view new_hostname) {
ct::CTRequirementsStatus ct_requirement_status;
  const int64_t rust_result = RunSpdySessionRust(87,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return IsCertStatusError(ssl_info.cert_status); }
      case 1: { return ssl_info.client_cert_sent; }
      case 2: { return ssl_config_service.CanShareConnectionWithClientCerts(old_hostname); }
      case 3: { return ssl_config_service.CanShareConnectionWithClientCerts(new_hostname); }
      case 4: { return ssl_info.cert->VerifyNameMatch(new_hostname); }
      case 5: { return transport_security_state->CheckPublicKeyPins(new_hostname,ssl_info.is_issued_by_known_root,ssl_info.public_key_hashes)==TransportSecurityState::PKPStatus::VIOLATED; }
      case 6: { ct_requirement_status=transport_security_state->CheckCTRequirements(new_hostname,ssl_info.is_issued_by_known_root,ssl_info.public_key_hashes,ssl_info.cert.get(),ssl_info.ct_policy_compliance); base::UmaHistogramEnumeration("Net.CanPool.CTRequirementStatus",ct_requirement_status);
return 0; }
      case 7: { return ct_requirement_status==ct::CTRequirementsStatus::CT_REQUIREMENTS_NOT_MET; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

SpdySession::PendingStreamRequest::PendingStreamRequest(
    base::WeakPtr<SpdyStreamRequest> request,
    base::TimeTicks queue_first_enqueued_time)
    : request(std::move(request)),
      queue_first_enqueued_time(queue_first_enqueued_time) {}

SpdySession::PendingStreamRequest::~PendingStreamRequest() = default;

SpdySession::PendingStreamRequest::PendingStreamRequest(
    const PendingStreamRequest& other) = default;

SpdySession::PendingStreamRequest::PendingStreamRequest(
    PendingStreamRequest&& other) = default;

SpdySession::PendingStreamRequest& SpdySession::PendingStreamRequest::operator=(
    const PendingStreamRequest& other) = default;

SpdySession::PendingStreamRequest& SpdySession::PendingStreamRequest::operator=(
    PendingStreamRequest&& other) = default;

SpdySession::SpdySession(
    const SpdySessionKey& spdy_session_key,
    HttpServerProperties* http_server_properties,
    TransportSecurityState* transport_security_state,
    SSLConfigService* ssl_config_service,
    const quic::ParsedQuicVersionVector& quic_supported_versions,
    bool enable_sending_initial_data,
    bool enable_ping_based_connection_checking,
    bool is_http2_enabled,
    bool is_quic_enabled,
    size_t session_max_recv_window_size,
    int session_max_queued_capped_frames,
    const spdy::SettingsMap& initial_settings,
    bool enable_http2_settings_grease,
    const std::optional<SpdySessionPool::GreasedHttp2Frame>&
        greased_http2_frame,
    bool http2_end_stream_with_data_frame,
    bool enable_priority_update,
    TimeFunc time_func,
    NetworkQualityEstimator* network_quality_estimator,
    NetLog* net_log,
    MultiplexedSessionCreationInitiator session_creation_initiator,
    SpdySessionInitiator spdy_session_initiator)
    : spdy_session_key_(spdy_session_key),
      http_server_properties_(http_server_properties),
      transport_security_state_(transport_security_state),
      ssl_config_service_(ssl_config_service),
      stream_hi_water_mark_(kFirstStreamId),
      initial_settings_(initial_settings),
      enable_http2_settings_grease_(enable_http2_settings_grease),
      greased_http2_frame_(greased_http2_frame),
      http2_end_stream_with_data_frame_(http2_end_stream_with_data_frame),
      enable_priority_update_(enable_priority_update),
      max_concurrent_streams_(kInitialMaxConcurrentStreams),
      last_read_time_(time_func()),
      session_max_recv_window_size_(session_max_recv_window_size),
      session_max_queued_capped_frames_(session_max_queued_capped_frames),
      last_recv_window_update_(base::TimeTicks::Now()),
      time_to_buffer_small_window_updates_(
          kDefaultTimeToBufferSmallWindowUpdates),
      stream_initial_send_window_size_(kDefaultInitialWindowSize),
      max_header_table_size_(
          initial_settings.at(spdy::SETTINGS_HEADER_TABLE_SIZE)),
      stream_max_recv_window_size_(
          initial_settings.at(spdy::SETTINGS_INITIAL_WINDOW_SIZE)),
      net_log_(
          NetLogWithSource::Make(net_log, NetLogSourceType::HTTP2_SESSION)),
      quic_supported_versions_(quic_supported_versions),
      enable_sending_initial_data_(enable_sending_initial_data),
      enable_ping_based_connection_checking_(
          enable_ping_based_connection_checking),
      is_http2_enabled_(is_http2_enabled),
      is_quic_enabled_(is_quic_enabled),
      connection_at_risk_of_loss_time_(
          base::Seconds(kSpdyDefaultConnectionAtRiskOfLossSeconds)),
      hung_interval_(base::Seconds(kHungIntervalSeconds)),
      time_func_(time_func),
      network_quality_estimator_(network_quality_estimator),
      session_creation_initiator_(session_creation_initiator),
      spdy_session_initiator_(spdy_session_initiator) {
RunSpdySessionRust(99,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { net_log_.BeginEvent(NetLogEventType::HTTP2_SESSION,[&] { return base::DictValue().Set("host",host_port_pair().ToString()).Set("proxy",spdy_session_key_.proxy_chain().ToDebugString()); });
return 0; }
      case 1: { DCHECK(initial_settings_.contains(spdy::SETTINGS_HEADER_TABLE_SIZE)); DCHECK(initial_settings_.contains(spdy::SETTINGS_INITIAL_WINDOW_SIZE));
return 0; }
      case 2: { return greased_http2_frame_.has_value(); }
      case 3: { DCHECK_EQ(0x0b,greased_http2_frame_.value().type%0x1f);
return 0; }
    }
    NOTREACHED();
  });
}

SpdySession::~SpdySession() {
RunSpdySessionRust(100,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_);
return 0; }
      case 1: { DcheckDraining();
return 0; }
      case 2: { DCHECK(waiting_for_confirmation_callbacks_.empty()); DCHECK_EQ(broken_connection_detection_requests_,0); DCHECK(socket_);
return 0; }
      case 3: { socket_->Disconnect();
return 0; }
      case 4: { RecordHistograms();
return 0; }
      case 5: { net_log_.EndEvent(NetLogEventType::HTTP2_SESSION);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::InitializeWithSocketHandle(
    std::unique_ptr<StreamSocketHandle> stream_socket_handle,
    SpdySessionPool* pool) {
RunSpdySessionRust(105,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(!stream_socket_handle_); DCHECK(!owned_stream_socket_); DCHECK(!socket_); DCHECK(stream_socket_handle->socket());
return 0; }
      case 1: { stream_socket_handle_=std::move(stream_socket_handle);
return 0; }
      case 2: { socket_=stream_socket_handle_->socket();
return 0; }
      case 3: { stream_socket_handle_->AddHigherLayeredPool(this);
return 0; }
      case 4: { InitializeInternal(pool);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::InitializeWithSocket(
    std::unique_ptr<StreamSocket> stream_socket,
    const LoadTimingInfo::ConnectTiming& connect_timing,
    SpdySessionPool* pool) {
RunSpdySessionRust(106,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(!stream_socket_handle_); DCHECK(!owned_stream_socket_); DCHECK(!socket_); DCHECK(stream_socket);
return 0; }
      case 1: { owned_stream_socket_=std::move(stream_socket);
return 0; }
      case 2: { socket_=owned_stream_socket_.get();
return 0; }
      case 3: { connect_timing_=std::make_unique<LoadTimingInfo::ConnectTiming>(connect_timing);
return 0; }
      case 4: { InitializeInternal(pool);
return 0; }
    }
    NOTREACHED();
  });
}

int SpdySession::ParseAlps() {
decltype(socket_->GetPeerApplicationSettings()) alps_data;
  std::optional<AlpsDecoder> alps_decoder;
  AlpsDecoder::Error error=AlpsDecoder::Error::kNoError;
  spdy::SettingsMap::const_iterator setting;
  std::optional<url::SchemeHostPort> origin;
  std::string serialized;

  const int64_t rust_result = RunSpdySessionRust(52,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { alps_data=socket_->GetPeerApplicationSettings(); return alps_data.has_value(); }
      case 1: { alps_decoder.emplace(); error=alps_decoder->Decode(alps_data.value()); base::UmaHistogramEnumeration("Net.SpdySession.AlpsDecoderStatus",error); return error==AlpsDecoder::Error::kNoError; }
      case 2: { DoDrainSession(ERR_HTTP2_PROTOCOL_ERROR,base::StrCat({"Error parsing ALPS: ",base::NumberToString(static_cast<int>(error))})); return ERR_HTTP2_PROTOCOL_ERROR; }
      case 3: { base::UmaHistogramCounts100("Net.SpdySession.AlpsSettingParameterCount",alps_decoder->GetSettings().size()); setting=alps_decoder->GetSettings().begin();
return 0; }
      case 4: { return setting!=alps_decoder->GetSettings().end(); }
      case 5: { spdy::SpdySettingsId identifier=setting->first; uint32_t value=setting->second; net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_SETTING,[&] { return NetLogSpdyRecvSettingParams(identifier,value); }); HandleSetting(identifier,value);
return 0; }
      case 6: { ++setting;
return 0; }
      case 7: { return alps_decoder->GetAcceptCh().size(); }
      case 8: { origin.emplace(GURL(base::span(alps_decoder->GetAcceptCh())[a].origin)); serialized=origin->Serialize(); return serialized.empty(); }
      case 9: { return base::span(alps_decoder->GetAcceptCh())[a].origin==serialized; }
      case 10: { accept_ch_entries_received_via_alps_.emplace(std::move(*origin),base::span(alps_decoder->GetAcceptCh())[a].value);
return 0; }
      case 11: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_ACCEPT_CH,[&] { return NetLogSpdyRecvAcceptChParams(base::span(alps_decoder->GetAcceptCh())[a]); });
return 0; }
      case 12: { base::UmaHistogramEnumeration("Net.SpdySession.AlpsAcceptChEntries",static_cast<SpdyAcceptChEntries>(a));
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

bool SpdySession::VerifyDomainAuthentication(std::string_view domain) const {
std::optional<SSLInfo> ssl_info;
  const int64_t rust_result = RunSpdySessionRust(2,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_DRAINING; }
      case 1: { ssl_info.emplace(); return GetSSLInfo(&*ssl_info); }
      case 2: { return CanPool(transport_security_state_,*ssl_info,*ssl_config_service_,host_port_pair().host(),domain); }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

void SpdySession::EnqueueStreamWrite(
    const base::WeakPtr<SpdyStream>& stream,
    spdy::SpdyFrameType frame_type,
    std::unique_ptr<SpdyBufferProducer> producer) {
RunSpdySessionRust(107,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(frame_type==spdy::SpdyFrameType::HEADERS || frame_type==spdy::SpdyFrameType::DATA);
return 0; }
      case 1: { EnqueueWrite(stream->priority(),frame_type,std::move(producer),stream,stream->traffic_annotation());
return 0; }
    }
    NOTREACHED();
  });
}

bool SpdySession::GreasedFramesEnabled() const {
  return greased_http2_frame_.has_value();
}

void SpdySession::EnqueueGreasedFrame(const base::WeakPtr<SpdyStream>& stream) {
RunSpdySessionRust(102,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_DRAINING; }
      case 1: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_GREASED_FRAME,[&] { return NetLogSpdyGreasedFrameParams(stream->stream_id(),greased_http2_frame_.value().type,greased_http2_frame_.value().flags,greased_http2_frame_.value().payload.length(),stream->priority()); });
return 0; }
      case 2: { EnqueueWrite(stream->priority(),static_cast<spdy::SpdyFrameType>(greased_http2_frame_.value().type),std::make_unique<GreasedBufferProducer>(stream,&greased_http2_frame_.value(),buffered_spdy_framer_.get()),stream,stream->traffic_annotation());
return 0; }
    }
    NOTREACHED();
  });
}

bool SpdySession::ShouldSendHttp2Priority() const {
const int64_t rust_result = RunSpdySessionRust(0,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return enable_priority_update_; }
      case 1: { return deprecate_http2_priorities_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

bool SpdySession::ShouldSendPriorityUpdate() const {
const int64_t rust_result = RunSpdySessionRust(1,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return enable_priority_update_; }
      case 1: { return settings_frame_received_; }
      case 2: { return deprecate_http2_priorities_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

int SpdySession::ConfirmHandshake(CompletionOnceCallback callback) {
const int64_t rust_result = RunSpdySessionRust(3,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_DRAINING; }
      case 1: { return drain_session_pending_; }
      case 2: { return ERR_CONNECTION_CLOSED; }
      case 3: { return availability_state_==STATE_GOING_AWAY; }
      case 4: { return ERR_FAILED; }
      case 5: { return in_confirm_handshake_; }
      case 6: { return socket_->ConfirmHandshake(base::BindOnce(&SpdySession::NotifyRequestsOfConfirmation,weak_factory_.GetWeakPtr())); }
      case 7: { in_confirm_handshake_=true; waiting_for_confirmation_callbacks_.push_back(std::move(callback));
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

std::unique_ptr<spdy::SpdySerializedFrame> SpdySession::CreateHeaders(
    spdy::SpdyStreamId stream_id,
    RequestPriority priority,
    spdy::SpdyControlFlags flags,
    quiche::HttpHeaderBlock block,
    NetLogSource source_dependency) {
ActiveStreamMap::const_iterator it;
  spdy::SpdyPriority spdy_priority=0;
  int weight=0;
  spdy::SpdyStreamId parent_stream_id=0;
  bool exclusive=false;
  bool has_priority=true;
  std::optional<spdy::SpdyHeadersIR> headers;
  std::unique_ptr<spdy::SpdySerializedFrame> result;

  RunSpdySessionRust(61,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { it=active_streams_.find(stream_id); CHECK(it!=active_streams_.end()); CHECK_EQ(it->second->stream_id(),stream_id);
return 0; }
      case 1: { MaybeSendPrefacePing();
return 0; }
      case 2: { DCHECK(buffered_spdy_framer_.get()); spdy_priority=ConvertRequestPriorityToSpdyPriority(priority);
return 0; }
      case 3: { priority_dependency_state_.OnStreamCreation(stream_id,spdy_priority,&parent_stream_id,&weight,&exclusive);
return 0; }
      case 4: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_HEADERS,[&](NetLogCaptureMode capture_mode) { return NetLogSpdyHeadersSentParams(&block,(flags&spdy::CONTROL_FLAG_FIN)!=0,stream_id,has_priority,weight,parent_stream_id,exclusive,source_dependency,capture_mode); });
return 0; }
      case 5: { headers.emplace(stream_id,std::move(block)); headers->set_has_priority(has_priority); headers->set_weight(weight); headers->set_parent_stream_id(parent_stream_id); headers->set_exclusive(exclusive);
return 0; }
      case 6: { return flags; }
      case 7: { return spdy::CONTROL_FLAG_FIN; }
      case 8: { headers->set_fin(a!=0);
return 0; }
      case 9: { return streams_initiated_count_; }
      case 10: { streams_initiated_count_=static_cast<int>(a);
return 0; }
      case 11: { result=std::make_unique<spdy::SpdySerializedFrame>(buffered_spdy_framer_->SerializeFrame(*headers));
return 0; }
    }
    NOTREACHED();
  });
return result;
}

std::unique_ptr<SpdyBuffer> SpdySession::CreateDataBuffer(
    spdy::SpdyStreamId stream_id,
    IOBuffer* data,
    int len,
    spdy::SpdyDataFlags flags,
    int* effective_len,
    bool* end_stream) {
ActiveStreamMap::const_iterator it;
  SpdyStream* stream=nullptr;
  std::unique_ptr<spdy::SpdySerializedFrame> frame;
  std::unique_ptr<SpdyBuffer> result;
  enum SpdyFrameFlowControlState { SEND_NOT_STALLED,SEND_STALLED_BY_STREAM,SEND_STALLED_BY_SESSION,SEND_STALLED_BY_STREAM_AND_SESSION };

  RunSpdySessionRust(59,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_DRAINING; }
      case 1: { it=active_streams_.find(stream_id); CHECK(it!=active_streams_.end()); stream=it->second; CHECK_EQ(stream->stream_id(),stream_id);
return 0; }
      case 2: { return len; }
      case 3: { NOTREACHED(); }
      case 4: { return kMaxSpdyFrameChunkSize; }
      case 5: { *effective_len=static_cast<int>(a);
return 0; }
      case 6: { return stream->send_window_size(); }
      case 7: { return IsSendStalled(); }
      case 8: { UMA_HISTOGRAM_ENUMERATION("Net.SpdyFrameStreamAndSessionFlowControlState",static_cast<SpdyFrameFlowControlState>(a),SEND_STALLED_BY_STREAM_AND_SESSION+1);
return 0; }
      case 9: { stream->set_send_stalled_by_flow_control(true);
return 0; }
      case 10: { QueueSendStalledStream(*stream);
return 0; }
      case 11: { net_log_.AddEventWithIntParams(NetLogEventType::HTTP2_SESSION_STREAM_STALLED_BY_STREAM_SEND_WINDOW,"stream_id",stream_id);
return 0; }
      case 12: { return *effective_len; }
      case 13: { net_log_.AddEventWithIntParams(NetLogEventType::HTTP2_SESSION_STREAM_STALLED_BY_SESSION_SEND_WINDOW,"stream_id",stream_id);
return 0; }
      case 14: { return session_send_window_size_; }
      case 15: { DCHECK_GE(*effective_len,0);
return 0; }
      case 16: { return flags; }
      case 17: { return spdy::DATA_FLAG_FIN; }
      case 18: { flags=static_cast<spdy::SpdyDataFlags>(a);
return 0; }
      case 19: { MaybeSendPrefacePing();
return 0; }
      case 20: { DCHECK(buffered_spdy_framer_.get()); frame = buffered_spdy_framer_->CreateDataFrame(stream_id,data->data(),static_cast<uint32_t>(*effective_len),flags);
return 0; }
      case 21: { result=std::make_unique<SpdyBuffer>(std::move(frame));
return 0; }
      case 22: { DecreaseSendWindowSize(static_cast<int32_t>(*effective_len));
return 0; }
      case 23: { result->AddConsumeCallback(base::BindRepeating(&SpdySession::OnWriteBufferConsumed,weak_factory_.GetWeakPtr(),static_cast<size_t>(*effective_len)));
return 0; }
      case 24: { *end_stream=a!=0;
return 0; }
    }
    NOTREACHED();
  });
return result;
}

void SpdySession::UpdateStreamPriority(SpdyStream* stream,
                                       RequestPriority old_priority,
                                       RequestPriority new_priority) {
spdy::SpdyStreamId stream_id=0;
  decltype(priority_dependency_state_.OnStreamUpdate(0,0)) updates;

  RunSpdySessionRust(60,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { write_queue_.ChangePriorityOfWritesForStream(stream,old_priority,new_priority);
return 0; }
      case 1: { stream_id=stream->stream_id(); return stream_id; }
      case 2: { DCHECK(IsStreamActive(stream_id));
return 0; }
      case 3: { updates=priority_dependency_state_.OnStreamUpdate(stream_id,ConvertRequestPriorityToSpdyPriority(new_priority)); return updates.size(); }
      case 4: { const auto& u=base::span(updates)[a]; DCHECK(IsStreamActive(u.id)); EnqueuePriorityFrame(u.id,u.parent_stream_id,u.weight,u.exclusive);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::CloseActiveStream(spdy::SpdyStreamId stream_id, int status) {
ActiveStreamMap::iterator it;
  RunSpdySessionRust(56,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_NE(stream_id,0u);
return 0; }
      case 1: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 2: { NOTREACHED(); }
      case 3: { CloseActiveStreamIterator(it,status);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::CloseCreatedStream(const base::WeakPtr<SpdyStream>& stream,
                                     int status) {
CreatedStreamSet::iterator it;
  RunSpdySessionRust(57,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_EQ(stream->stream_id(),0u);
return 0; }
      case 1: { it=created_streams_.find(stream.get()); return it!=created_streams_.end(); }
      case 2: { NOTREACHED(); }
      case 3: { CloseCreatedStreamIterator(it,status);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::ResetStream(spdy::SpdyStreamId stream_id,
                              int error,
                              const std::string& description) {
ActiveStreamMap::iterator it;
  RunSpdySessionRust(58,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_NE(stream_id,0u);
return 0; }
      case 1: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 2: { NOTREACHED(); }
      case 3: { ResetStreamIterator(it,error,description);
return 0; }
    }
    NOTREACHED();
  });
}

bool SpdySession::IsStreamActive(spdy::SpdyStreamId stream_id) const {
  return active_streams_.contains(stream_id);
}

LoadState SpdySession::GetLoadState() const {
  // Just report that we're idle since the session could be doing
  // many things concurrently.
  return LOAD_STATE_IDLE;
}

int SpdySession::GetRemoteEndpoint(IPEndPoint* endpoint) {
int rv=OK;
  const int64_t rust_result = RunSpdySessionRust(53,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { rv=GetPeerAddress(endpoint); base::UmaHistogramSparse("Net.SpdySession.GetRemoteEndpointResult",-rv); return rv; }
      case 1: { return ERR_SOCKET_NOT_CONNECTED; }
      case 2: { return base::FeatureList::IsEnabled(features::kDrainSpdySessionSynchronouslyOnRemoteEndpointDisconnect); }
      case 3: { DoDrainSessionAsync(ERR_CONNECTION_CLOSED,"Remote endpoint disconnected");
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

bool SpdySession::GetSSLInfo(SSLInfo* ssl_info) const {
  return socket_->GetSSLInfo(ssl_info);
}

std::optional<ResolutionDetails> SpdySession::GetResolutionDetails() const {
std::optional<ResolutionDetails> result;
  RunSpdySessionRust(54,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return !!stream_socket_handle_; }
      case 1: { result=stream_socket_handle_->resolution_details();
return 0; }
    }
    NOTREACHED();
  });
return result;
}

std::string_view SpdySession::GetAcceptChViaAlps(
    const url::SchemeHostPort& scheme_host_port) const {
decltype(accept_ch_entries_received_via_alps_)::const_iterator it;
  std::string_view result;

  RunSpdySessionRust(55,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { it=accept_ch_entries_received_via_alps_.find(scheme_host_port); return it!=accept_ch_entries_received_via_alps_.end(); }
      case 1: { LogSpdyAcceptChForOriginHistogram(false);
return 0; }
      case 2: { LogSpdyAcceptChForOriginHistogram(true); result=it->second;
return 0; }
    }
    NOTREACHED();
  });
return result;
}

NextProto SpdySession::GetNegotiatedProtocol() const {
  return socket_->GetNegotiatedProtocol();
}

void SpdySession::SendStreamWindowUpdate(spdy::SpdyStreamId stream_id,
                                         uint32_t delta_window_size) {
ActiveStreamMap::const_iterator it;
  RunSpdySessionRust(104,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { it=active_streams_.find(stream_id); CHECK(it!=active_streams_.end()); CHECK_EQ(it->second->stream_id(),stream_id);
return 0; }
      case 1: { SendWindowUpdateFrame(stream_id,delta_window_size,it->second->priority());
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::CloseSessionOnError(Error err,
                                      const std::string& description,
                                      bool force_send_go_away) {
RunSpdySessionRust(108,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_LT(err,ERR_IO_PENDING);
return 0; }
      case 1: { DoDrainSession(err,description,force_send_go_away);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::MakeUnavailable(Error error) {
RunSpdySessionRust(4,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_AVAILABLE; }
      case 1: { availability_state_=STATE_GOING_AWAY; error_on_unavailable_=error;
return 0; }
      case 2: { pool_->MakeSessionUnavailable(GetWeakPtr());
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::StartGoingAway(spdy::SpdyStreamId last_good_stream_id,
                                 Error status) {
size_t old_size=0;
  base::WeakPtr<SpdyStreamRequest> pending_request;
  ActiveStreamMap::iterator active;
  CreatedStreamSet::iterator created;

  RunSpdySessionRust(5,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GE(availability_state_,STATE_GOING_AWAY); DCHECK_NE(OK,status); DCHECK_NE(ERR_IO_PENDING,status); last_good_stream_id_=last_good_stream_id;
return 0; }
      case 1: { NotifyRequestsOfConfirmation(status);
return 0; }
      case 2: { old_size=GetTotalSize(pending_create_stream_queues_); pending_request=GetNextPendingStreamRequest(); return !!pending_request; }
      case 3: { DCHECK_GT(old_size,GetTotalSize(pending_create_stream_queues_));
return 0; }
      case 4: { pending_request->OnRequestCompleteFailure(status);
return 0; }
      case 5: { old_size=active_streams_.size(); active=active_streams_.lower_bound(last_good_stream_id+1); return active!=active_streams_.end(); }
      case 6: { LogAbandonedActiveStream(active,status);
return 0; }
      case 7: { CloseActiveStreamIterator(active,status);
return 0; }
      case 8: { DCHECK_GT(old_size,active_streams_.size());
return 0; }
      case 9: { return !created_streams_.empty(); }
      case 10: { old_size=created_streams_.size(); created=created_streams_.begin();
return 0; }
      case 11: { LogAbandonedStream(*created,status);
return 0; }
      case 12: { CloseCreatedStreamIterator(created,status);
return 0; }
      case 13: { DCHECK_GT(old_size,created_streams_.size());
return 0; }
      case 14: { write_queue_.RemovePendingWritesForStreamsAfter(last_good_stream_id);
return 0; }
      case 15: { DcheckGoingAway();
return 0; }
      case 16: { MaybeFinishGoingAway();
return 0; }
      case 17: { pending_request.reset();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::MaybeFinishGoingAway() {
RunSpdySessionRust(6,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return active_streams_.empty(); }
      case 1: { return created_streams_.empty(); }
      case 2: { return availability_state_==STATE_GOING_AWAY; }
      case 3: { return drain_session_pending_; }
      case 4: { DoDrainSession(OK,"Finished going away");
return 0; }
    }
    NOTREACHED();
  });
}

base::DictValue SpdySession::GetInfoAsValue() const {
int pending_create_stream_request_count=0;
  base::DictValue dict,key_dict;
  base::ListValue alias_list,active_stream_details;
  std::set<SpdySessionKey>::const_iterator alias;
  ActiveStreamMap::const_iterator active;

  RunSpdySessionRust(95,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return pending_create_stream_queues_.size(); }
      case 1: { return base::span(pending_create_stream_queues_)[a].size(); }
      case 2: { pending_create_stream_request_count=static_cast<int>(a);
return 0; }
      case 3: { dict=base::DictValue().Set("source_id",static_cast<int>(net_log_.source().id)).Set("host_port_pair",host_port_pair().ToString()).Set("proxy",spdy_session_key_.proxy_chain().ToDebugString()).Set("network_anonymization_key",spdy_session_key_.network_anonymization_key().ToDebugString()).Set("active_streams",static_cast<int>(active_streams_.size())).Set("created_streams",static_cast<int>(created_streams_.size())).Set("pending_create_stream_request_count",pending_create_stream_request_count).Set("negotiated_protocol",NextProtoToString(socket_->GetNegotiatedProtocol())).Set("error",error_on_close_).Set("error_on_unavailable",error_on_unavailable_).Set("max_concurrent_streams",static_cast<int>(max_concurrent_streams_)).Set("streams_initiated_count",streams_initiated_count_).Set("streams_abandoned_count",streams_abandoned_count_).Set("stream_hi_water_mark",static_cast<int>(stream_hi_water_mark_));
return 0; }
      case 4: { return !!buffered_spdy_framer_; }
      case 5: { return buffered_spdy_framer_->frames_received(); }
      case 6: { dict.Set("frames_received",static_cast<int>(a)); dict.Set("send_window_size",session_send_window_size_); dict.Set("recv_window_size",session_recv_window_size_); dict.Set("unacked_recv_window_bytes",session_unacked_recv_window_bytes_); dict.Set("support_websocket",support_websocket_); dict.Set("availability_state",AvailabilityStateToString(availability_state_)); dict.Set("last_good_stream_id",static_cast<int>(last_good_stream_id_));
return 0; }
      case 7: { key_dict.Set("privacy_mode",PrivacyModeToDebugString(spdy_session_key_.privacy_mode())); key_dict.Set("secure_dns_policy",SecureDnsPolicyToDebugString(spdy_session_key_.secure_dns_policy())); key_dict.Set("disable_cert_verification_network_fetches",spdy_session_key_.disable_cert_verification_network_fetches()); dict.Set("spdy_session_key",std::move(key_dict));
return 0; }
      case 8: { return drain_error_.has_value(); }
      case 9: { CHECK(!drain_description_.empty()); dict.Set("drain_error",*drain_error_); dict.Set("drain_description",drain_description_);
return 0; }
      case 10: { return go_away_error_.has_value(); }
      case 11: { dict.Set("go_away_error",static_cast<int>(*go_away_error_)); dict.Set("go_away_debug_data",go_away_debug_data_);
return 0; }
      case 12: { return pooled_aliases_.empty(); }
      case 13: { alias=pooled_aliases_.begin();
return 0; }
      case 14: { return alias!=pooled_aliases_.end(); }
      case 15: { alias_list.Append(alias->host_port_pair().ToString());
return 0; }
      case 16: { ++alias;
return 0; }
      case 17: { dict.Set("aliases",std::move(alias_list));
return 0; }
      case 18: { active=active_streams_.begin();
return 0; }
      case 19: { return active!=active_streams_.end(); }
      case 20: { active_stream_details.Append(active->second->GetInfoAsValue());
return 0; }
      case 21: { ++active;
return 0; }
      case 22: { dict.Set("active_stream_details",std::move(active_stream_details));
return 0; }
    }
    NOTREACHED();
  });
return dict;
}

bool SpdySession::IsReused() const {
const int64_t rust_result = RunSpdySessionRust(62,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return buffered_spdy_framer_->frames_received(); }
      case 1: { return !!owned_stream_socket_; }
      case 2: { return stream_socket_handle_->reuse_type()==StreamSocketHandle::SocketReuseType::kUnusedIdle; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

bool SpdySession::GetLoadTimingInfo(spdy::SpdyStreamId stream_id,
                                    LoadTimingInfo* load_timing_info) const {
const int64_t rust_result = RunSpdySessionRust(63,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return !!stream_socket_handle_; }
      case 1: { DCHECK(!connect_timing_);
return 0; }
      case 2: { return stream_socket_handle_->GetLoadTimingInfo(a!=0,load_timing_info); }
      case 3: { return stream_id!=kFirstStreamId; }
      case 4: { DCHECK(connect_timing_); DCHECK(socket_);
return 0; }
      case 5: { load_timing_info->socket_reused=a!=0;
return 0; }
      case 6: { return load_timing_info->socket_reused; }
      case 7: { load_timing_info->connect_timing=*connect_timing_;
return 0; }
      case 8: { load_timing_info->socket_log_id=socket_->NetLog().source().id;
return 0; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

int SpdySession::GetPeerAddress(IPEndPoint* address) const {
const int64_t rust_result = RunSpdySessionRust(64,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return !!socket_; }
      case 1: { return socket_->GetPeerAddress(address); }
      case 2: { return ERR_SOCKET_NOT_CONNECTED; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

int SpdySession::GetLocalAddress(IPEndPoint* address) const {
const int64_t rust_result = RunSpdySessionRust(65,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return !!socket_; }
      case 1: { return socket_->GetLocalAddress(address); }
      case 2: { return ERR_SOCKET_NOT_CONNECTED; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

void SpdySession::AddPooledAlias(const SpdySessionKey& alias_key) {
  pooled_aliases_.insert(alias_key);
}

void SpdySession::RemovePooledAlias(const SpdySessionKey& alias_key) {
  pooled_aliases_.erase(alias_key);
}

bool SpdySession::HasAcceptableTransportSecurity() const {
SSLInfo ssl_info;
  const int64_t rust_result = RunSpdySessionRust(66,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(GetSSLInfo(&ssl_info));
return 0; }
      case 1: { return SSLConnectionStatusToVersion(ssl_info.connection_status); }
      case 2: { return SSL_CONNECTION_VERSION_TLS1_2; }
      case 3: { return IsTLSCipherSuiteAllowedByHTTP2(SSLConnectionStatusToCipherSuite(ssl_info.connection_status)); }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

base::WeakPtr<SpdySession> SpdySession::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool SpdySession::CloseOneIdleConnection() {
RunSpdySessionRust(67,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_); DCHECK(pool_);
return 0; }
      case 1: { return active_streams_.empty(); }
      case 2: { DoDrainSession(ERR_CONNECTION_CLOSED,"Closing idle connection.");
return 0; }
    }
    NOTREACHED();
  });
return false;
}

bool SpdySession::ChangeSocketTag(const SocketTag& new_tag) {
const int64_t rust_result = RunSpdySessionRust(68,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return IsAvailable(); }
      case 1: { return !!socket_; }
      case 2: { return is_active(); }
      case 3: { socket_->ApplySocketTag(new_tag);
return 0; }
      case 4: { SpdySessionKey new_key(spdy_session_key_.host_port_pair(),spdy_session_key_.privacy_mode(),spdy_session_key_.proxy_chain(),spdy_session_key_.session_usage(),new_tag,spdy_session_key_.network_anonymization_key(),spdy_session_key_.secure_dns_policy(),spdy_session_key_.disable_cert_verification_network_fetches(),spdy_session_key_.target_network()); spdy_session_key_=new_key;
return 0; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

void SpdySession::EnableBrokenConnectionDetection(
    base::TimeDelta heartbeat_interval) {
RunSpdySessionRust(69,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GE(broken_connection_detection_requests_,0); return broken_connection_detection_requests_; }
      case 1: { broken_connection_detection_requests_=static_cast<int>(a);
return 0; }
      case 2: { DCHECK(!IsBrokenConnectionDetectionEnabled());
return 0; }
      case 3: { NetworkChangeNotifier::AddDefaultNetworkActiveObserver(this);
return 0; }
      case 4: { heartbeat_interval_=heartbeat_interval;
return 0; }
      case 5: { heartbeat_timer_.Start(FROM_HERE,heartbeat_interval_,base::BindOnce(&SpdySession::MaybeCheckConnectionStatus,weak_factory_.GetWeakPtr()));
return 0; }
    }
    NOTREACHED();
  });
}

bool SpdySession::IsBrokenConnectionDetectionEnabled() const {
  return heartbeat_timer_.IsRunning();
}

// static
std::string_view SpdySession::AvailabilityStateToString(
    AvailabilityState state) {
std::string_view result;
  RunSpdySessionRust(96,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { static_assert(STATE_AVAILABLE==0 && STATE_GOING_AWAY==1 && STATE_DRAINING==2); return state; }
      case 1: { result="Available";
return 0; }
      case 2: { result="GoingAway";
return 0; }
      case 3: { result="Draining";
return 0; }
      case 4: { NOTREACHED(); }
    }
    NOTREACHED();
  });
return result;
}

void SpdySession::InitializeInternal(SpdySessionPool* pool) {
RunSpdySessionRust(70,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_); DCHECK_EQ(availability_state_,STATE_AVAILABLE); DCHECK_EQ(read_state_,READ_STATE_DO_READ); DCHECK_EQ(write_state_,WRITE_STATE_IDLE);
return 0; }
      case 1: { session_send_window_size_=kDefaultInitialWindowSize; session_recv_window_size_=kDefaultInitialWindowSize;
return 0; }
      case 2: { buffered_spdy_framer_=std::make_unique<BufferedSpdyFramer>(initial_settings_.find(spdy::SETTINGS_MAX_HEADER_LIST_SIZE)->second,net_log_,time_func_); buffered_spdy_framer_->set_visitor(this); buffered_spdy_framer_->set_debug_visitor(this); buffered_spdy_framer_->UpdateHeaderDecoderTableSize(max_header_table_size_);
return 0; }
      case 3: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_INITIALIZED,[&] { return NetLogSpdyInitializedParams(socket_->NetLog().source()); });
return 0; }
      case 4: { DCHECK_EQ(availability_state_,STATE_AVAILABLE); return enable_sending_initial_data_; }
      case 5: { SendInitialData();
return 0; }
      case 6: { pool_=pool;
return 0; }
      case 7: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&SpdySession::PumpReadLoop,weak_factory_.GetWeakPtr(),READ_STATE_DO_READ,OK));
return 0; }
    }
    NOTREACHED();
  });
}

// {,Try}CreateStream() can be called with |in_io_loop_| set if a stream is
// being created in response to another being closed due to received data.

int SpdySession::TryCreateStream(
    const base::WeakPtr<SpdyStreamRequest>& request,
    base::WeakPtr<SpdyStream>* stream) {
const int64_t rust_result = RunSpdySessionRust(7,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(request);
return 0; }
      case 1: { return availability_state_==STATE_DRAINING; }
      case 2: { return drain_session_pending_; }
      case 3: { return ERR_CONNECTION_CLOSED; }
      case 4: { return availability_state_==STATE_GOING_AWAY; }
      case 5: { return ERR_FAILED; }
      case 6: { return request->socket_tag_!=spdy_session_key_.socket_tag(); }
      case 7: { return active_streams_.size(); }
      case 8: { return created_streams_.size(); }
      case 9: { return max_concurrent_streams_; }
      case 10: { return CreateStream(*request,stream); }
      case 11: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_STALLED_MAX_STREAMS,[&] {
      return NetLogSpdySessionStalledParams(active_streams_.size(),created_streams_.size(),max_concurrent_streams_,request->url().spec()); });
return 0; }
      case 12: { RequestPriority priority=request->priority(); CHECK_GE(priority,MINIMUM_PRIORITY); CHECK_LE(priority,MAXIMUM_PRIORITY);
       base::span(pending_create_stream_queues_)[priority].emplace_back(request,base::TimeTicks::Now());
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

int SpdySession::CreateStream(const SpdyStreamRequest& request,
                              base::WeakPtr<SpdyStream>* stream) {
std::unique_ptr<SpdyStream> new_stream;
  const int64_t rust_result = RunSpdySessionRust(8,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GE(request.priority(),MINIMUM_PRIORITY); DCHECK_LE(request.priority(),MAXIMUM_PRIORITY); CHECK_EQ(availability_state_,STATE_AVAILABLE); CHECK(!drain_session_pending_); DCHECK(socket_);
return 0; }
      case 1: { UMA_HISTOGRAM_BOOLEAN("Net.SpdySession.CreateStreamWithSocketConnected",socket_->IsConnected());
return 0; }
      case 2: { return socket_->IsConnected(); }
      case 3: { DoDrainSessionAsync(ERR_CONNECTION_CLOSED,"Tried to create SPDY stream for a closed socket connection."); return ERR_CONNECTION_CLOSED; }
      case 4: { new_stream=std::make_unique<SpdyStream>(request.type(),GetWeakPtr(),request.url(),request.priority(),stream_initial_send_window_size_,stream_max_recv_window_size_,request.net_log(),request.traffic_annotation(),request.detect_broken_connection_);
       *stream=new_stream->GetWeakPtr();
return 0; }
      case 5: { InsertCreatedStream(std::move(new_stream));
return 0; }
      case 6: { return request.detect_broken_connection_; }
      case 7: { EnableBrokenConnectionDetection(request.heartbeat_interval_);
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

bool SpdySession::CancelStreamRequest(
    const base::WeakPtr<SpdyStreamRequest>& request) {
RequestPriority priority=MINIMUM_PRIORITY;
  PendingStreamRequestQueue* queue=nullptr;
  PendingStreamRequestQueue::iterator it;

  const int64_t rust_result = RunSpdySessionRust(94,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(request); priority=request->priority(); CHECK_GE(priority,MINIMUM_PRIORITY); CHECK_LE(priority,MAXIMUM_PRIORITY);
#if DCHECK_IS_ON()
       for(int i=MINIMUM_PRIORITY;i<=MAXIMUM_PRIORITY;++i) {
         if(priority==i) continue;
         DCHECK(!std::ranges::contains(base::span(pending_create_stream_queues_)[i],request.get(),[](const PendingStreamRequest& item){return item.request.get();}));
       }
#endif
    
return 0; }
      case 1: { queue=&base::span(pending_create_stream_queues_)[priority]; it=queue->begin();
return 0; }
      case 2: { return it!=queue->end(); }
      case 3: { return it->request.get()==request.get(); }
      case 4: { ++it;
return 0; }
      case 5: { it=queue->erase(it); DCHECK(std::ranges::find(it,queue->end(),request.get(),[](const PendingStreamRequest& item){return item.request.get();})==queue->end());
return 0; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

void SpdySession::ChangeStreamRequestPriority(
    const base::WeakPtr<SpdyStreamRequest>& request,
    RequestPriority priority) {
RunSpdySessionRust(9,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_NE(priority,request->priority());
return 0; }
      case 1: { return CancelStreamRequest(request); }
      case 2: { base::span(pending_create_stream_queues_)[priority].emplace_back(request,base::TimeTicks::Now());
return 0; }
    }
    NOTREACHED();
  });
}

base::WeakPtr<SpdyStreamRequest> SpdySession::GetNextPendingStreamRequest() {
std::optional<PendingStreamRequest> pending;
  base::WeakPtr<SpdyStreamRequest> result;

  RunSpdySessionRust(10,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return MAXIMUM_PRIORITY; }
      case 1: { return MINIMUM_PRIORITY; }
      case 2: { return base::span(pending_create_stream_queues_)[a].empty(); }
      case 3: { pending.emplace(std::move(base::span(pending_create_stream_queues_)[a].front())); base::span(pending_create_stream_queues_)[a].pop_front(); CHECK(pending->request);
return 0; }
      case 4: { return !!pending->request; }
      case 5: { pending->request->max_stream_limit_pending_delay_=base::TimeTicks::Now()-pending->queue_first_enqueued_time;
return 0; }
      case 6: { result=pending->request;
return 0; }
    }
    NOTREACHED();
  });
return result;
}

void SpdySession::ProcessPendingStreamRequests() {
base::WeakPtr<SpdyStreamRequest> pending_request;
  RunSpdySessionRust(11,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_AVAILABLE; }
      case 1: { return max_concurrent_streams_; }
      case 2: { return active_streams_.size(); }
      case 3: { return created_streams_.size(); }
      case 4: { pending_request=GetNextPendingStreamRequest(); return !!pending_request; }
      case 5: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&SpdySession::CompleteStreamRequest,weak_factory_.GetWeakPtr(),pending_request));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::CloseActiveStreamIterator(ActiveStreamMap::iterator it,
                                            int status) {
std::unique_ptr<SpdyStream> owned_stream;
  RunSpdySessionRust(109,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { owned_stream.reset(it->second);
return 0; }
      case 1: { active_streams_.erase(it);
return 0; }
      case 2: { priority_dependency_state_.OnStreamDestruction(owned_stream->stream_id());
return 0; }
      case 3: { DeleteStream(std::move(owned_stream),status);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::CloseCreatedStreamIterator(CreatedStreamSet::iterator it,
                                             int status) {
std::unique_ptr<SpdyStream> owned_stream;
  RunSpdySessionRust(110,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { owned_stream.reset(*it);
return 0; }
      case 1: { created_streams_.erase(it);
return 0; }
      case 2: { DeleteStream(std::move(owned_stream),status);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::ResetStreamIterator(ActiveStreamMap::iterator it,
                                      int error,
                                      const std::string& description) {
spdy::SpdyStreamId stream_id=0; RequestPriority priority=MINIMUM_PRIORITY;
  RunSpdySessionRust(71,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return error; }
      case 1: { return ERR_FAILED; }
      case 2: { return ERR_ABORTED; }
      case 3: { return ERR_HTTP2_FLOW_CONTROL_ERROR; }
      case 4: { return ERR_TIMED_OUT; }
      case 5: { return ERR_HTTP2_STREAM_CLOSED; }
      case 6: { return spdy::ERROR_CODE_PROTOCOL_ERROR; }
      case 7: { return spdy::ERROR_CODE_INTERNAL_ERROR; }
      case 8: { return spdy::ERROR_CODE_CANCEL; }
      case 9: { return spdy::ERROR_CODE_FLOW_CONTROL_ERROR; }
      case 10: { return spdy::ERROR_CODE_REFUSED_STREAM; }
      case 11: { return spdy::ERROR_CODE_STREAM_CLOSED; }
      case 12: { stream_id=it->first; priority=it->second->priority();
return 0; }
      case 13: { EnqueueResetStreamFrame(stream_id,priority,static_cast<spdy::SpdyErrorCode>(a),description);
return 0; }
      case 14: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 15: { CloseActiveStreamIterator(it,error);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::EnqueueResetStreamFrame(spdy::SpdyStreamId stream_id,
                                          RequestPriority priority,
                                          spdy::SpdyErrorCode error_code,
                                          const std::string& description) {
std::unique_ptr<spdy::SpdySerializedFrame> frame;
  RunSpdySessionRust(111,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_NE(stream_id,0u);
return 0; }
      case 1: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_RST_STREAM,[&] { return NetLogSpdySendRstStreamParams(stream_id,error_code,description); });
return 0; }
      case 2: { DCHECK(buffered_spdy_framer_.get()); frame=buffered_spdy_framer_->CreateRstStream(stream_id,error_code);
return 0; }
      case 3: { EnqueueSessionWrite(priority,spdy::SpdyFrameType::RST_STREAM,std::move(frame));
return 0; }
      case 4: { RecordProtocolErrorHistogram(MapRstStreamStatusToProtocolError(error_code));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::EnqueuePriorityFrame(spdy::SpdyStreamId stream_id,
                                       spdy::SpdyStreamId dependency_id,
                                       int weight,
                                       bool exclusive) {
std::unique_ptr<spdy::SpdySerializedFrame> frame;
  RunSpdySessionRust(112,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { net_log_.AddEvent(NetLogEventType::HTTP2_STREAM_SEND_PRIORITY,[&] { return NetLogSpdyPriorityParams(stream_id,dependency_id,weight,exclusive); });
return 0; }
      case 1: { DCHECK(buffered_spdy_framer_.get()); frame=buffered_spdy_framer_->CreatePriority(stream_id,dependency_id,weight,exclusive);
return 0; }
      case 2: { EnqueueWrite(HIGHEST,spdy::SpdyFrameType::PRIORITY,std::make_unique<SimpleBufferProducer>(std::make_unique<SpdyBuffer>(std::move(frame))),base::WeakPtr<SpdyStream>(),kSpdySessionCommandsTrafficAnnotation);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::PumpReadLoop(ReadState expected_read_state, int result) {
RunSpdySessionRust(12,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_);
return 0; }
      case 1: { return availability_state_==STATE_DRAINING; }
      case 2: { return drain_session_pending_; }
      case 3: { std::ignore=DoReadLoop(expected_read_state,result);
return 0; }
    }
    NOTREACHED();
  });
}

int SpdySession::DoReadLoop(ReadState expected_read_state, int result) {
base::TimeTicks yield_after_time;
  const int64_t rust_result = RunSpdySessionRust(13,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_); CHECK_EQ(read_state_,expected_read_state); in_io_loop_=true;
return 0; }
      case 1: { yield_after_time=time_func_()+base::Milliseconds(kYieldAfterDurationMilliseconds); return result; }
      case 2: { return read_state_==READ_STATE_DO_READ; }
      case 3: { CHECK_EQ(result,OK); result=DoRead(); return result; }
      case 4: { return read_state_==READ_STATE_DO_READ_COMPLETE; }
      case 5: { result=DoReadComplete(result); return result; }
      case 6: { NOTREACHED()<<"read_state_: "<<read_state_; }
      case 7: { return availability_state_==STATE_DRAINING; }
      case 8: { return drain_session_pending_; }
      case 9: { return kYieldAfterBytesRead; }
      case 10: { return time_func_()>yield_after_time; }
      case 11: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&SpdySession::PumpReadLoop,weak_factory_.GetWeakPtr(),READ_STATE_DO_READ,OK)); result=ERR_IO_PENDING;
return 0; }
      case 12: { CHECK(in_io_loop_); in_io_loop_=false;
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

int SpdySession::DoRead() {
int rv=OK;
  const int64_t rust_result = RunSpdySessionRust(14,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(!read_buffer_); CHECK(in_io_loop_); CHECK(socket_); read_state_=READ_STATE_DO_READ_COMPLETE;
return 0; }
      case 1: { read_buffer_=base::MakeRefCounted<IOBufferWithSize>(kReadBufferSize);
return 0; }
      case 2: { rv=socket_->ReadIfReady(read_buffer_.get(),kReadBufferSize,base::BindOnce(&SpdySession::PumpReadLoop,weak_factory_.GetWeakPtr(),READ_STATE_DO_READ)); return rv; }
      case 3: { read_buffer_=nullptr; read_state_=READ_STATE_DO_READ;
return 0; }
      case 4: { return ERR_READ_IF_READY_NOT_IMPLEMENTED; }
      case 5: { return socket_->Read(read_buffer_.get(),kReadBufferSize,base::BindOnce(&SpdySession::PumpReadLoop,weak_factory_.GetWeakPtr(),READ_STATE_DO_READ_COMPLETE)); }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

int SpdySession::DoReadComplete(int result) {
base::span<uint8_t> available_data;
  const int64_t rust_result = RunSpdySessionRust(15,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(read_buffer_); CHECK(in_io_loop_); return result; }
      case 1: { DoDrainSession(ERR_CONNECTION_CLOSED,"Connection closed"); return ERR_CONNECTION_CLOSED; }
      case 2: { DoDrainSession(static_cast<Error>(result),base::StringPrintf("Error %d reading from socket.",-result));
return 0; }
      case 3: { CHECK_LE(result,kReadBufferSize); last_read_time_=time_func_(); DCHECK(buffered_spdy_framer_.get()); available_data=read_buffer_->first(result);
return 0; }
      case 4: { return !available_data.empty(); }
      case 5: { size_t bytes_processed=buffered_spdy_framer_->ProcessInput(reinterpret_cast<char*>(available_data.data()),available_data.size()); available_data=available_data.subspan(bytes_processed);
return 0; }
      case 6: { return availability_state_==STATE_DRAINING; }
      case 7: { return drain_session_pending_; }
      case 8: { return ERR_CONNECTION_CLOSED; }
      case 9: { DCHECK_EQ(buffered_spdy_framer_->spdy_framer_error(),http2::Http2DecoderAdapter::SPDY_NO_ERROR);
return 0; }
      case 10: { read_buffer_=nullptr; read_state_=READ_STATE_DO_READ;
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

void SpdySession::PumpWriteLoop(WriteState expected_write_state, int result) {
RunSpdySessionRust(16,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_); DCHECK_EQ(write_state_,expected_write_state);
return 0; }
      case 1: { DoWriteLoop(expected_write_state,result);
return 0; }
      case 2: { return availability_state_==STATE_DRAINING; }
      case 3: { return !!in_flight_write_; }
      case 4: { return write_queue_.IsEmpty(); }
      case 5: { pool_->RemoveUnavailableSession(GetWeakPtr());
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::MaybePostWriteLoop() {
RunSpdySessionRust(17,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return write_state_==WRITE_STATE_IDLE; }
      case 1: { CHECK(!in_flight_write_); write_state_=WRITE_STATE_DO_WRITE;
return 0; }
      case 2: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&SpdySession::PumpWriteLoop,weak_factory_.GetWeakPtr(),WRITE_STATE_DO_WRITE,OK));
return 0; }
    }
    NOTREACHED();
  });
}

int SpdySession::DoWriteLoop(WriteState expected_write_state, int result) {
const int64_t rust_result = RunSpdySessionRust(18,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_); DCHECK_NE(write_state_,WRITE_STATE_IDLE); DCHECK_EQ(write_state_,expected_write_state); in_io_loop_=true;
return 0; }
      case 1: { return result; }
      case 2: { return write_state_==WRITE_STATE_DO_WRITE; }
      case 3: { DCHECK_EQ(result,OK); result=DoWrite(); return result; }
      case 4: { return write_state_==WRITE_STATE_DO_WRITE_COMPLETE; }
      case 5: { result=DoWriteComplete(result); return result; }
      case 6: { NOTREACHED()<<"write_state_: "<<write_state_; }
      case 7: { return write_state_==WRITE_STATE_IDLE; }
      case 8: { DCHECK_EQ(result,ERR_IO_PENDING);
return 0; }
      case 9: { CHECK(in_io_loop_); in_io_loop_=false;
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

int SpdySession::DoWrite() {
spdy::SpdyFrameType frame_type=spdy::SpdyFrameType::DATA;
  std::unique_ptr<SpdyBufferProducer> producer;
  base::WeakPtr<SpdyStream> stream;
  std::unique_ptr<SpdyStream> owned_stream;
  scoped_refptr<IOBuffer> write_io_buffer;

  const int64_t rust_result = RunSpdySessionRust(19,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); DCHECK(buffered_spdy_framer_);
return 0; }
      case 1: { return !!in_flight_write_; }
      case 2: { DCHECK_GT(in_flight_write_->GetRemainingSize(),0u);
return 0; }
      case 3: { return write_queue_.Dequeue(&frame_type,&producer,&stream,&in_flight_write_traffic_annotation_); }
      case 4: { write_state_=WRITE_STATE_IDLE;
return 0; }
      case 5: { return !!stream; }
      case 6: { CHECK(!stream->IsClosed());
return 0; }
      case 7: { return frame_type==spdy::SpdyFrameType::HEADERS; }
      case 8: { CHECK(stream.get()); CHECK_EQ(stream->stream_id(),0u); owned_stream=ActivateCreatedStream(stream.get());
return 0; }
      case 9: { InsertActivatedStream(std::move(owned_stream));
return 0; }
      case 10: { return stream_hi_water_mark_>kLastStreamId; }
      case 11: { CHECK_EQ(stream->stream_id(),kLastStreamId);
return 0; }
      case 12: { MakeUnavailable(ERR_HTTP2_PROTOCOL_ERROR);
return 0; }
      case 13: { StartGoingAway(kLastStreamId,ERR_HTTP2_PROTOCOL_ERROR);
return 0; }
      case 14: { in_flight_write_=producer->ProduceBuffer(); return !!in_flight_write_; }
      case 15: { NOTREACHED(); }
      case 16: { in_flight_write_frame_type_=frame_type; in_flight_write_frame_size_=in_flight_write_->GetRemainingSize(); DCHECK_GE(in_flight_write_frame_size_,spdy::kFrameMinimumSize); in_flight_write_stream_=stream;
return 0; }
      case 17: { stream.reset(); producer.reset();
return 0; }
      case 18: { write_state_=WRITE_STATE_DO_WRITE_COMPLETE; write_io_buffer=in_flight_write_->GetIOBufferForRemainingData();
return 0; }
      case 19: { return socket_->Write(write_io_buffer.get(),in_flight_write_->GetRemainingSize(),base::BindOnce(&SpdySession::PumpWriteLoop,weak_factory_.GetWeakPtr(),WRITE_STATE_DO_WRITE_COMPLETE),NetworkTrafficAnnotationTag(in_flight_write_traffic_annotation_)); }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

int SpdySession::DoWriteComplete(int result) {
const int64_t rust_result = RunSpdySessionRust(20,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); DCHECK_NE(result,ERR_IO_PENDING); DCHECK_GT(in_flight_write_->GetRemainingSize(),0u); return result; }
      case 1: { DCHECK_NE(result,ERR_IO_PENDING); in_flight_write_.reset(); in_flight_write_frame_type_=spdy::SpdyFrameType::DATA; in_flight_write_frame_size_=0; in_flight_write_stream_.reset(); in_flight_write_traffic_annotation_.reset(); write_state_=WRITE_STATE_DO_WRITE;
return 0; }
      case 2: { DoDrainSession(static_cast<Error>(result),"Write error");
return 0; }
      case 3: { DCHECK_LE(static_cast<size_t>(result),in_flight_write_->GetRemainingSize());
return 0; }
      case 4: { in_flight_write_->Consume(static_cast<size_t>(result));
return 0; }
      case 5: { return !!in_flight_write_stream_; }
      case 6: { in_flight_write_stream_->AddRawSentBytes(static_cast<size_t>(result));
return 0; }
      case 7: { return in_flight_write_->GetRemainingSize(); }
      case 8: { DCHECK_GT(in_flight_write_frame_size_,0u); in_flight_write_stream_->OnFrameWriteComplete(in_flight_write_frame_type_,in_flight_write_frame_size_);
return 0; }
      case 9: { in_flight_write_.reset(); in_flight_write_frame_type_=spdy::SpdyFrameType::DATA; in_flight_write_frame_size_=0; in_flight_write_stream_.reset();
return 0; }
      case 10: { write_state_=WRITE_STATE_DO_WRITE;
return 0; }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

void SpdySession::NotifyRequestsOfConfirmation(int rv) {
RunSpdySessionRust(21,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return waiting_for_confirmation_callbacks_.size(); }
      case 1: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(std::move(base::span(waiting_for_confirmation_callbacks_)[a]),rv));
return 0; }
      case 2: { waiting_for_confirmation_callbacks_.clear();
return 0; }
      case 3: { in_confirm_handshake_=false;
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::SendInitialData() {
spdy::SettingsMap settings_map;
  spdy::SettingsMap::const_iterator setting;
  int32_t delta_window_size=0;
  std::unique_ptr<spdy::SpdySerializedFrame> settings_frame,window_update_frame,initial_frame;
  size_t initial_frame_size=0;
  std::unique_ptr<char[]> initial_frame_data;
  std::optional<base::SpanWriter<char>> initial_data_writer;

  RunSpdySessionRust(86,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(enable_sending_initial_data_); DCHECK(buffered_spdy_framer_.get()); setting=initial_settings_.begin();
return 0; }
      case 1: { return setting!=initial_settings_.end(); }
      case 2: { return setting->first; }
      case 3: { return setting->second; }
      case 4: { settings_map.insert(*setting);
return 0; }
      case 5: { ++setting;
return 0; }
      case 6: { return enable_http2_settings_grease_; }
      case 7: { return base::RandGenerator(16); }
      case 8: { return base::RandGenerator(static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())+1); }
      case 9: { settings_map.emplace(static_cast<spdy::SpdySettingsId>(a),static_cast<uint32_t>(b));
return 0; }
      case 10: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_SETTINGS,[&] { return NetLogSpdySendSettingsParams(&settings_map); }); settings_frame=buffered_spdy_framer_->CreateSettings(settings_map);
return 0; }
      case 11: { DCHECK_GE(session_max_recv_window_size_,session_recv_window_size_); DCHECK_GE(session_recv_window_size_,0); DCHECK_EQ(0,session_unacked_recv_window_bytes_);
return 0; }
      case 12: { return session_max_recv_window_size_; }
      case 13: { return session_recv_window_size_; }
      case 14: { delta_window_size=static_cast<int32_t>(a); session_recv_window_size_=static_cast<int32_t>(b);
return 0; }
      case 15: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_UPDATE_RECV_WINDOW,[&] { return NetLogSpdySessionWindowUpdateParams(delta_window_size,session_recv_window_size_); }); last_recv_window_update_=base::TimeTicks::Now();
return 0; }
      case 16: { return session_unacked_recv_window_bytes_; }
      case 17: { session_unacked_recv_window_bytes_=static_cast<int32_t>(a);
return 0; }
      case 18: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_WINDOW_UPDATE,[&] { return NetLogSpdyWindowUpdateFrameParams(spdy::kSessionFlowControlStreamId,session_unacked_recv_window_bytes_); }); window_update_frame=buffered_spdy_framer_->CreateWindowUpdate(spdy::kSessionFlowControlStreamId,session_unacked_recv_window_bytes_); session_unacked_recv_window_bytes_=0;
return 0; }
      case 19: { return spdy::kHttp2ConnectionHeaderPrefixSize; }
      case 20: { return settings_frame->size(); }
      case 21: { return window_update_frame->size(); }
      case 22: { initial_frame_size=a; initial_frame_data=std::make_unique<char[]>(initial_frame_size); initial_data_writer.emplace(UNSAFE_TODO(base::span(initial_frame_data.get(),initial_frame_size)));
return 0; }
      case 23: { initial_data_writer->Write(UNSAFE_TODO(base::span(spdy::kHttp2ConnectionHeaderPrefix,static_cast<size_t>(spdy::kHttp2ConnectionHeaderPrefixSize)))); initial_data_writer->Write(*settings_frame);
return 0; }
      case 24: { initial_data_writer->Write(*window_update_frame);
return 0; }
      case 25: { CHECK_EQ(initial_data_writer->remaining(),0u); initial_frame=std::make_unique<spdy::SpdySerializedFrame>(std::move(initial_frame_data),initial_frame_size);
return 0; }
      case 26: { EnqueueSessionWrite(HIGHEST,spdy::SpdyFrameType::SETTINGS,std::move(initial_frame));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::HandleSetting(uint32_t id, uint32_t value) {
int32_t delta_window_size=0;
  RunSpdySessionRust(22,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return id; }
      case 1: { return value; }
      case 2: { buffered_spdy_framer_->UpdateHeaderEncoderTableSize(value);
return 0; }
      case 3: { return kMaxConcurrentStreamLimit; }
      case 4: { max_concurrent_streams_=a;
return 0; }
      case 5: { ProcessPendingStreamRequests();
return 0; }
      case 6: { net_log_.AddEventWithIntParams(NetLogEventType::HTTP2_SESSION_INITIAL_WINDOW_SIZE_OUT_OF_RANGE,"initial_window_size",value);
return 0; }
      case 7: { return stream_initial_send_window_size_; }
      case 8: { delta_window_size=static_cast<int32_t>(a); stream_initial_send_window_size_=static_cast<int32_t>(value);
return 0; }
      case 9: { UpdateStreamsSendWindowSize(delta_window_size);
return 0; }
      case 10: { net_log_.AddEventWithIntParams(NetLogEventType::HTTP2_SESSION_UPDATE_STREAMS_SEND_WINDOW_SIZE,"delta_window_size",delta_window_size);
return 0; }
      case 11: { return support_websocket_; }
      case 12: { DoDrainSession(ERR_HTTP2_PROTOCOL_ERROR,"Invalid value for spdy::SETTINGS_ENABLE_CONNECT_PROTOCOL.");
return 0; }
      case 13: { support_websocket_=true;
return 0; }
      case 14: { return settings_frame_received_; }
      case 15: { return deprecate_http2_priorities_; }
      case 16: { DoDrainSession(ERR_HTTP2_PROTOCOL_ERROR,"Invalid value for spdy::SETTINGS_DEPRECATE_HTTP2_PRIORITIES.");
return 0; }
      case 17: { DoDrainSession(ERR_HTTP2_PROTOCOL_ERROR,"spdy::SETTINGS_DEPRECATE_HTTP2_PRIORITIES value changed after first SETTINGS frame.");
return 0; }
      case 18: { deprecate_http2_priorities_=true;
return 0; }
      case 19: { return spdy::SETTINGS_HEADER_TABLE_SIZE; }
      case 20: { return spdy::SETTINGS_MAX_CONCURRENT_STREAMS; }
      case 21: { return spdy::SETTINGS_INITIAL_WINDOW_SIZE; }
      case 22: { return spdy::SETTINGS_ENABLE_CONNECT_PROTOCOL; }
      case 23: { return spdy::SETTINGS_DEPRECATE_HTTP2_PRIORITIES; }
    }
    NOTREACHED();
  });
}

void SpdySession::UpdateStreamsSendWindowSize(int32_t delta_window_size) {
ActiveStreamMap::const_iterator active;
  CreatedStreamSet::const_iterator created;

  RunSpdySessionRust(23,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { active=active_streams_.begin();
return 0; }
      case 1: { return active!=active_streams_.end(); }
      case 2: { return active->second->AdjustSendWindowSize(delta_window_size); }
      case 3: { DoDrainSession(ERR_HTTP2_FLOW_CONTROL_ERROR,base::StringPrintf("New spdy::SETTINGS_INITIAL_WINDOW_SIZE value overflows flow control window of stream %d.",active->second->stream_id()));
return 0; }
      case 4: { ++active;
return 0; }
      case 5: { created=created_streams_.begin();
return 0; }
      case 6: { return created!=created_streams_.end(); }
      case 7: { return (*created)->AdjustSendWindowSize(delta_window_size); }
      case 8: { DoDrainSession(ERR_HTTP2_FLOW_CONTROL_ERROR,base::StringPrintf("New spdy::SETTINGS_INITIAL_WINDOW_SIZE value overflows flow control window of stream %d.",(*created)->stream_id()));
return 0; }
      case 9: { ++created;
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::MaybeCheckConnectionStatus() {
RunSpdySessionRust(24,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return NetworkChangeNotifier::IsDefaultNetworkActive(); }
      case 1: { CheckConnectionStatus();
return 0; }
      case 2: { check_connection_on_radio_wakeup_=true;
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::MaybeSendPrefacePing() {
RunSpdySessionRust(25,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return ping_in_flight_; }
      case 1: { return check_ping_status_pending_; }
      case 2: { return enable_ping_based_connection_checking_; }
      case 3: { return time_func_()>last_read_time_+connection_at_risk_of_loss_time_; }
      case 4: { WritePingFrame(next_ping_id_,false);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::SendWindowUpdateFrame(spdy::SpdyStreamId stream_id,
                                        uint32_t delta_window_size,
                                        RequestPriority priority) {
ActiveStreamMap::const_iterator it; std::unique_ptr<spdy::SpdySerializedFrame> frame;
  RunSpdySessionRust(103,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 1: { CHECK_EQ(it->second->stream_id(),stream_id);
return 0; }
      case 2: { CHECK_EQ(stream_id,spdy::kSessionFlowControlStreamId);
return 0; }
      case 3: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_WINDOW_UPDATE,[&] { return NetLogSpdyWindowUpdateFrameParams(stream_id,delta_window_size); });
return 0; }
      case 4: { DCHECK(buffered_spdy_framer_.get()); frame=buffered_spdy_framer_->CreateWindowUpdate(stream_id,delta_window_size);
return 0; }
      case 5: { EnqueueSessionWrite(priority,spdy::SpdyFrameType::WINDOW_UPDATE,std::move(frame));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::WritePingFrame(spdy::SpdyPingId unique_id, bool is_ack) {
std::unique_ptr<spdy::SpdySerializedFrame> ping_frame;
  RunSpdySessionRust(26,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(buffered_spdy_framer_.get()); ping_frame = buffered_spdy_framer_->CreatePingFrame(unique_id,is_ack);
return 0; }
      case 1: { EnqueueSessionWrite(HIGHEST,spdy::SpdyFrameType::PING,std::move(ping_frame));
return 0; }
      case 2: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_PING,[&] { return NetLogSpdyPingParams(unique_id,is_ack,"sent"); });
return 0; }
      case 3: { return is_ack; }
      case 4: { DCHECK(!ping_in_flight_); ping_in_flight_=true; return next_ping_id_; }
      case 5: { next_ping_id_=a;
return 0; }
      case 6: { PlanToCheckPingStatus();
return 0; }
      case 7: { last_ping_sent_time_=time_func_();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::PlanToCheckPingStatus() {
RunSpdySessionRust(27,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return check_ping_status_pending_; }
      case 1: { check_ping_status_pending_=true;
return 0; }
      case 2: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(FROM_HERE,base::BindOnce(&SpdySession::CheckPingStatus,weak_factory_.GetWeakPtr(),time_func_()),hung_interval_);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::CheckPingStatus(base::TimeTicks last_check_time) {
base::TimeTicks now;
  RunSpdySessionRust(28,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(!in_io_loop_); DCHECK(check_ping_status_pending_);
return 0; }
      case 1: { return ping_in_flight_; }
      case 2: { check_ping_status_pending_=false;
return 0; }
      case 3: { now=time_func_();
return 0; }
      case 4: { return now>last_read_time_+hung_interval_; }
      case 5: { return last_read_time_<last_check_time; }
      case 6: { DoDrainSession(ERR_HTTP2_PING_FAILED,"Failed ping.");
return 0; }
      case 7: { const base::TimeDelta delay=last_read_time_+hung_interval_-now; base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(FROM_HERE,base::BindOnce(&SpdySession::CheckPingStatus,weak_factory_.GetWeakPtr(),now),delay);
return 0; }
    }
    NOTREACHED();
  });
}

spdy::SpdyStreamId SpdySession::GetNewStreamId() {
const int64_t rust_result = RunSpdySessionRust(29,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK_LE(stream_hi_water_mark_,kLastStreamId); return stream_hi_water_mark_; }
      case 1: { stream_hi_water_mark_=static_cast<spdy::SpdyStreamId>(a);
return 0; }
    }
    NOTREACHED();
  });
return static_cast<spdy::SpdyStreamId>(rust_result);
}

void SpdySession::EnqueueSessionWrite(
    RequestPriority priority,
    spdy::SpdyFrameType frame_type,
    std::unique_ptr<spdy::SpdySerializedFrame> frame) {
std::unique_ptr<SpdyBuffer> buffer;
  RunSpdySessionRust(101,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(frame_type==spdy::SpdyFrameType::RST_STREAM || frame_type==spdy::SpdyFrameType::SETTINGS || frame_type==spdy::SpdyFrameType::WINDOW_UPDATE || frame_type==spdy::SpdyFrameType::PING || frame_type==spdy::SpdyFrameType::GOAWAY); DCHECK(IsSpdyFrameTypeWriteCapped(frame_type));
return 0; }
      case 1: { return write_queue_.num_queued_capped_frames(); }
      case 2: { return session_max_queued_capped_frames_; }
      case 3: { LOG(WARNING)<<"Draining session due to exceeding max queued capped frames";
return 0; }
      case 4: { DoDrainSessionAsync(ERR_CONNECTION_CLOSED,"Exceeded max queued capped frames");
return 0; }
      case 5: { buffer=std::make_unique<SpdyBuffer>(std::move(frame));
return 0; }
      case 6: { EnqueueWrite(priority,frame_type,std::make_unique<SimpleBufferProducer>(std::move(buffer)),base::WeakPtr<SpdyStream>(),kSpdySessionCommandsTrafficAnnotation);
return 0; }
      case 7: { return greased_http2_frame_.has_value(); }
      case 8: { return frame_type==spdy::SpdyFrameType::SETTINGS; }
      case 9: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_GREASED_FRAME,[&] { return NetLogSpdyGreasedFrameParams(0,greased_http2_frame_.value().type,greased_http2_frame_.value().flags,greased_http2_frame_.value().payload.length(),priority); });
return 0; }
      case 10: { EnqueueWrite(priority,static_cast<spdy::SpdyFrameType>(greased_http2_frame_.value().type),std::make_unique<GreasedBufferProducer>(base::WeakPtr<SpdyStream>(),&greased_http2_frame_.value(),buffered_spdy_framer_.get()),base::WeakPtr<SpdyStream>(),kSpdySessionCommandsTrafficAnnotation);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::EnqueueWrite(
    RequestPriority priority,
    spdy::SpdyFrameType frame_type,
    std::unique_ptr<SpdyBufferProducer> producer,
    const base::WeakPtr<SpdyStream>& stream,
    const NetworkTrafficAnnotationTag& traffic_annotation) {
RunSpdySessionRust(30,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_DRAINING; }
      case 1: { write_queue_.Enqueue(priority,frame_type,std::move(producer),stream,traffic_annotation);
return 0; }
      case 2: { MaybePostWriteLoop();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::InsertCreatedStream(std::unique_ptr<SpdyStream> stream) {
CreatedStreamSet::iterator it;
  RunSpdySessionRust(113,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK_EQ(stream->stream_id(),0u); it=created_streams_.lower_bound(stream.get()); CHECK(it==created_streams_.end() || *it!=stream.get());
return 0; }
      case 1: { created_streams_.insert(it,stream.release());
return 0; }
    }
    NOTREACHED();
  });
}

std::unique_ptr<SpdyStream> SpdySession::ActivateCreatedStream(
    SpdyStream* stream) {
CreatedStreamSet::iterator it; std::unique_ptr<SpdyStream> owned_stream;
  RunSpdySessionRust(114,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK_EQ(stream->stream_id(),0u); it=created_streams_.find(stream); CHECK(it!=created_streams_.end());
return 0; }
      case 1: { stream->set_stream_id(GetNewStreamId());
return 0; }
      case 2: { owned_stream.reset(stream); created_streams_.erase(it);
return 0; }
    }
    NOTREACHED();
  });
return owned_stream;
}

void SpdySession::InsertActivatedStream(std::unique_ptr<SpdyStream> stream) {
spdy::SpdyStreamId stream_id=0;
  RunSpdySessionRust(115,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { stream_id=stream->stream_id(); CHECK_NE(stream_id,0u);
return 0; }
      case 1: { const auto inserted=active_streams_.emplace(stream_id,stream.get()); CHECK(inserted.second);
return 0; }
      case 2: { std::ignore=stream.release();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::DeleteStream(std::unique_ptr<SpdyStream> stream, int status) {
RunSpdySessionRust(31,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return in_flight_write_stream_.get()==stream.get(); }
      case 1: { in_flight_write_stream_.reset();
return 0; }
      case 2: { write_queue_.RemovePendingWritesForStream(stream.get());
return 0; }
      case 3: { return stream->detect_broken_connection(); }
      case 4: { MaybeDisableBrokenConnectionDetection();
return 0; }
      case 5: { stream->OnClose(status);
return 0; }
      case 6: { return active_streams_.empty(); }
      case 7: { return created_streams_.empty(); }
      case 8: { return drain_session_pending_; }
      case 9: { return !!stream_socket_handle_; }
      case 10: { return stream_socket_handle_->IsPoolStalled(); }
      case 11: { DoDrainSessionAsync(ERR_CONNECTION_CLOSED,"Closing idle connection.");
return 0; }
      case 12: { MaybeFinishGoingAway();
return 0; }
      case 13: { return availability_state_==STATE_AVAILABLE; }
      case 14: { ProcessPendingStreamRequests();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::RecordHistograms() {
RunSpdySessionRust(97,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { UMA_HISTOGRAM_CUSTOM_COUNTS("Net.SpdyStreamsPerSession",streams_initiated_count_,1,300,50); UMA_HISTOGRAM_CUSTOM_COUNTS("Net.SpdyStreamsAbandonedPerSession",streams_abandoned_count_,1,300,50); UMA_HISTOGRAM_BOOLEAN("Net.SpdySession.ServerSupportsWebSocket",support_websocket_);
return 0; }
      case 1: { return IsGoogleHostWithAlpnH3(spdy_session_key_.host_port_pair().host()); }
      case 2: { return streams_initiated_count_; }
      case 3: { LogSessionCreationInitiatorToHistogram(session_creation_initiator_,a!=0);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::RecordProtocolErrorHistogram(
    SpdyProtocolErrorDetails details) {
RunSpdySessionRust(98,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { UMA_HISTOGRAM_ENUMERATION("Net.SpdySessionErrorDetails2",details,NUM_SPDY_PROTOCOL_ERROR_DETAILS);
return 0; }
      case 1: { return base::EndsWith(host_port_pair().host(),"google.com",base::CompareCase::INSENSITIVE_ASCII); }
      case 2: { UMA_HISTOGRAM_ENUMERATION("Net.SpdySessionErrorDetails_Google2",details,NUM_SPDY_PROTOCOL_ERROR_DETAILS);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::DcheckGoingAway() const {
#if DCHECK_IS_ON()
  DCHECK_GE(availability_state_, STATE_GOING_AWAY);
  for (int i = MINIMUM_PRIORITY; i <= MAXIMUM_PRIORITY; ++i) {
    DCHECK(pending_create_stream_queues_[i].empty());
  }
  DCHECK(created_streams_.empty());
#endif
}

void SpdySession::DcheckDraining() const {
  DcheckGoingAway();
  DCHECK_EQ(availability_state_, STATE_DRAINING);
  DCHECK(active_streams_.empty());
}

void SpdySession::DoDrainSession(Error err,
                                 const std::string& description,
                                 bool force_send_go_away) {
std::unique_ptr<spdy::SpdySerializedFrame> frame;
std::optional<spdy::SpdyGoAwayIR> goaway_ir;
  RunSpdySessionRust(32,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { drain_session_pending_=false;
return 0; }
      case 1: { return availability_state_==STATE_DRAINING; }
      case 2: { MakeUnavailable(err);
return 0; }
      case 3: { drain_error_=err; drain_description_=description;
return 0; }
      case 4: { return err==ERR_HTTP_1_1_REQUIRED; }
      case 5: { http_server_properties_->SetHTTP11Required(url::SchemeHostPort(url::kHttpsScheme,host_port_pair().host(),host_port_pair().port()),spdy_session_key_.network_anonymization_key());
return 0; }
      case 6: { return force_send_go_away; }
      case 7: { return err; }
      case 8: { return ERR_ABORTED; }
      case 9: { return ERR_NETWORK_CHANGED; }
      case 10: { return ERR_SOCKET_NOT_CONNECTED; }
      case 11: { return ERR_HTTP_1_1_REQUIRED; }
      case 12: { return ERR_CONNECTION_CLOSED; }
      case 13: { return ERR_CONNECTION_RESET; }
      case 14: { goaway_ir.emplace(0,MapNetErrorToGoAwayStatus(err),description); frame=std::make_unique<spdy::SpdySerializedFrame>(buffered_spdy_framer_->SerializeFrame(*goaway_ir));
return 0; }
      case 15: { EnqueueSessionWrite(HIGHEST,spdy::SpdyFrameType::GOAWAY,std::move(frame)); goaway_ir.reset();
return 0; }
      case 16: { availability_state_=STATE_DRAINING; error_on_close_=err;
return 0; }
      case 17: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_CLOSE,[&] { return NetLogSpdySessionCloseParams(err,description); });
return 0; }
      case 18: { base::UmaHistogramSparse("Net.SpdySession.ClosedOnError",-err);
return 0; }
      case 19: { DcheckGoingAway();
return 0; }
      case 20: { StartGoingAway(0,err);
return 0; }
      case 21: { DcheckDraining();
return 0; }
      case 22: { MaybePostWriteLoop();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::DoDrainSessionAsync(Error err,
                                      std::string description,
                                      bool force_send_go_away) {
RunSpdySessionRust(33,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_DRAINING; }
      case 1: { return drain_session_pending_; }
      case 2: { MakeUnavailable(err);
return 0; }
      case 3: { drain_session_pending_=true;
return 0; }
      case 4: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&SpdySession::DoDrainSession,weak_factory_.GetWeakPtr(),err,std::move(description),force_send_go_away));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::LogAbandonedStream(SpdyStream* stream, Error status) {
RunSpdySessionRust(116,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(stream);
return 0; }
      case 1: { stream->LogStreamError(status,"Abandoned.");
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::LogAbandonedActiveStream(ActiveStreamMap::const_iterator it,
                                           Error status) {
RunSpdySessionRust(117,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GT(it->first,0u);
return 0; }
      case 1: { LogAbandonedStream(it->second,status);
return 0; }
      case 2: { return streams_abandoned_count_; }
      case 3: { streams_abandoned_count_=static_cast<int>(a);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::CompleteStreamRequest(
    const base::WeakPtr<SpdyStreamRequest>& pending_request) {
base::WeakPtr<SpdyStream> stream; int rv=OK;
  RunSpdySessionRust(34,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return !!pending_request; }
      case 1: { rv=TryCreateStream(pending_request,&stream); return rv; }
      case 2: { DCHECK(stream); pending_request->OnRequestCompleteSuccess(stream);
return 0; }
      case 3: { DCHECK(!stream);
return 0; }
      case 4: { pending_request->OnRequestCompleteFailure(rv);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnError(
    http2::Http2DecoderAdapter::SpdyFramerError spdy_framer_error) {
std::string description;
  RunSpdySessionRust(118,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_);
return 0; }
      case 1: { RecordProtocolErrorHistogram(MapFramerErrorToProtocolError(spdy_framer_error));
return 0; }
      case 2: { description=base::StringPrintf("Framer error: %d (%s).",spdy_framer_error,http2::Http2DecoderAdapter::SpdyFramerErrorToString(spdy_framer_error));
return 0; }
      case 3: { DoDrainSession(MapFramerErrorToNetError(spdy_framer_error),description);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnStreamError(spdy::SpdyStreamId stream_id,
                                const std::string& description) {
ActiveStreamMap::iterator it;
  RunSpdySessionRust(72,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 1: { EnqueueResetStreamFrame(stream_id,IDLE,spdy::ERROR_CODE_PROTOCOL_ERROR,description);
return 0; }
      case 2: { ResetStreamIterator(it,ERR_HTTP2_PROTOCOL_ERROR,description);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnPing(spdy::SpdyPingId unique_id, bool is_ack) {
base::TimeDelta ping_duration;
  RunSpdySessionRust(73,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_PING,[&] { return NetLogSpdyPingParams(unique_id,is_ack,"received"); });
return 0; }
      case 1: { return is_ack; }
      case 2: { WritePingFrame(unique_id,true);
return 0; }
      case 3: { return ping_in_flight_; }
      case 4: { RecordProtocolErrorHistogram(PROTOCOL_ERROR_UNEXPECTED_PING);
return 0; }
      case 5: { DoDrainSession(ERR_HTTP2_PROTOCOL_ERROR,"Unexpected PING ACK.");
return 0; }
      case 6: { ping_in_flight_=false; ping_duration=time_func_()-last_ping_sent_time_;
return 0; }
      case 7: { return !!network_quality_estimator_; }
      case 8: { network_quality_estimator_->RecordSpdyPingLatency(host_port_pair(),ping_duration);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnRstStream(spdy::SpdyStreamId stream_id,
                              spdy::SpdyErrorCode error_code) {
ActiveStreamMap::iterator it;
  std::optional<quiche::HttpHeaderBlock::const_iterator> status_it;
  int status=0;

  RunSpdySessionRust(74,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); base::UmaHistogramSparse("Net.SpdySession.RstStreamReceived",error_code); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_RST_STREAM,[&] { return NetLogSpdyRecvRstStreamParams(stream_id,error_code); });
return 0; }
      case 1: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 2: { DLOG(WARNING)<<"Received RST for invalid stream "<<stream_id;
return 0; }
      case 3: { DCHECK(it->second); CHECK_EQ(it->second->stream_id(),stream_id);
return 0; }
      case 4: { return it->second->IsRemoteClosed(); }
      case 5: { return error_code; }
      case 6: { return spdy::ERROR_CODE_NO_ERROR; }
      case 7: { return spdy::ERROR_CODE_CANCEL; }
      case 8: { return spdy::ERROR_CODE_STREAM_CLOSED; }
      case 9: { return spdy::ERROR_CODE_REFUSED_STREAM; }
      case 10: { return spdy::ERROR_CODE_HTTP_1_1_REQUIRED; }
      case 11: { CloseActiveStreamIterator(it,OK);
return 0; }
      case 12: { status_it.emplace(it->second->response_headers().find(spdy::kHttp2StatusHeader)); return *status_it!=it->second->response_headers().end(); }
      case 13: { return base::StringToInt((*status_it)->second,&status); }
      case 14: { return status; }
      case 15: { CloseActiveStreamIterator(it,ERR_HTTP2_RST_STREAM_NO_ERROR_RECEIVED);
return 0; }
      case 16: { CloseActiveStreamIterator(it,ERR_HTTP2_SERVER_REFUSED_STREAM);
return 0; }
      case 17: { it->second->LogStreamError(ERR_HTTP_1_1_REQUIRED,"Closing session because server reset stream with ERR_HTTP_1_1_REQUIRED.");
return 0; }
      case 18: { DoDrainSession(ERR_HTTP_1_1_REQUIRED,"HTTP_1_1_REQUIRED for stream.");
return 0; }
      case 19: { RecordProtocolErrorHistogram(PROTOCOL_ERROR_RST_STREAM_FOR_NON_ACTIVE_STREAM);
return 0; }
      case 20: { it->second->LogStreamError(ERR_HTTP2_PROTOCOL_ERROR,"Server reset stream.");
return 0; }
      case 21: { CloseActiveStreamIterator(it,ERR_HTTP2_PROTOCOL_ERROR);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnGoAway(spdy::SpdyStreamId last_accepted_stream_id,
                           spdy::SpdyErrorCode error_code,
                           std::string_view debug_data) {
Error net_error=OK;
  RunSpdySessionRust(75,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); base::UmaHistogramSparse("Net.SpdySession.GoAwayReceived",error_code); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_GOAWAY,[&](NetLogCaptureMode mode) { return NetLogSpdyRecvGoAwayParams(last_accepted_stream_id,active_streams_.size(),error_code,debug_data,mode); }); go_away_error_=error_code; go_away_debug_data_=std::string(debug_data);
return 0; }
      case 1: { return error_code; }
      case 2: { return spdy::ERROR_CODE_HTTP_1_1_REQUIRED; }
      case 3: { return spdy::ERROR_CODE_NO_ERROR; }
      case 4: { net_error=ERR_HTTP_1_1_REQUIRED;
return 0; }
      case 5: { net_error=ERR_HTTP2_SERVER_REFUSED_STREAM;
return 0; }
      case 6: { net_error=ERR_HTTP2_PROTOCOL_ERROR;
return 0; }
      case 7: { MakeUnavailable(net_error);
return 0; }
      case 8: { DoDrainSession(net_error,"HTTP_1_1_REQUIRED for stream.");
return 0; }
      case 9: { StartGoingAway(last_accepted_stream_id,net_error);
return 0; }
      case 10: { MaybeFinishGoingAway();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnDataFrameHeader(spdy::SpdyStreamId stream_id,
                                    size_t length,
                                    bool fin) {
ActiveStreamMap::iterator it;
  RunSpdySessionRust(76,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_DATA,[&] { return NetLogSpdyDataParams(stream_id,length,fin); });
return 0; }
      case 1: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 2: { CHECK_EQ(it->second->stream_id(),stream_id); DCHECK(buffered_spdy_framer_); it->second->AddRawReceivedBytes(spdy::kDataFrameMinimumSize);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnStreamFrameData(spdy::SpdyStreamId stream_id,
                                    const char* data,
                                    size_t len) {
std::unique_ptr<SpdyBuffer> buffer;
  ActiveStreamMap::iterator it;
  SpdyStream* stream=nullptr;

  RunSpdySessionRust(77,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); DCHECK_LT(len,1u<<24);
return 0; }
      case 1: { return data!=nullptr; }
      case 2: { DCHECK_GT(len,0u); CHECK_LE(len,static_cast<size_t>(kReadBufferSize)); buffer=std::make_unique<SpdyBuffer>(base::as_byte_span(UNSAFE_TODO(base::span(data,len))));
return 0; }
      case 3: { DecreaseRecvWindowSize(static_cast<int32_t>(len));
return 0; }
      case 4: { buffer->AddConsumeCallback(base::BindRepeating(&SpdySession::OnReadBufferConsumed,weak_factory_.GetWeakPtr()));
return 0; }
      case 5: { DCHECK_EQ(len,0u);
return 0; }
      case 6: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 7: { stream=it->second; CHECK_EQ(stream->stream_id(),stream_id);
return 0; }
      case 8: { stream->AddRawReceivedBytes(len);
return 0; }
      case 9: { stream->OnDataReceived(std::move(buffer));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnStreamEnd(spdy::SpdyStreamId stream_id) {
ActiveStreamMap::iterator it;
  RunSpdySessionRust(78,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 1: { CHECK_EQ(it->second->stream_id(),stream_id); it->second->OnDataReceived(std::unique_ptr<SpdyBuffer>());
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnStreamPadding(spdy::SpdyStreamId stream_id, size_t len) {
ActiveStreamMap::iterator it;
  RunSpdySessionRust(79,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_);
return 0; }
      case 1: { DecreaseRecvWindowSize(static_cast<int32_t>(len));
return 0; }
      case 2: { IncreaseRecvWindowSize(static_cast<int32_t>(len));
return 0; }
      case 3: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 4: { it->second->OnPaddingConsumed(len);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnSettings() {
std::unique_ptr<spdy::SpdySerializedFrame> frame;
std::optional<spdy::SpdySettingsIR> settings_ir;
  RunSpdySessionRust(80,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_SETTINGS); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_SEND_SETTINGS_ACK);
return 0; }
      case 1: { return settings_frame_received_; }
      case 2: { base::UmaHistogramCounts1000("Net.SpdySession.OnSettings.CreatedStreamCount2",created_streams_.size()); base::UmaHistogramCounts1000("Net.SpdySession.OnSettings.ActiveStreamCount2",active_streams_.size()); base::UmaHistogramCounts1000("Net.SpdySession.OnSettings.CreatedAndActiveStreamCount2",created_streams_.size()+active_streams_.size()); base::UmaHistogramCounts1000("Net.SpdySession.OnSettings.PendingStreamCount2",GetTotalSize(pending_create_stream_queues_));
return 0; }
      case 3: { settings_ir.emplace(); settings_ir->set_is_ack(true); frame=std::make_unique<spdy::SpdySerializedFrame>(buffered_spdy_framer_->SerializeFrame(*settings_ir));
return 0; }
      case 4: { EnqueueSessionWrite(HIGHEST,spdy::SpdyFrameType::SETTINGS,std::move(frame));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnSettingsAck() {
RunSpdySessionRust(119,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_);
return 0; }
      case 1: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_SETTINGS_ACK);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnSetting(spdy::SpdySettingsId id, uint32_t value) {
RunSpdySessionRust(120,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_);
return 0; }
      case 1: { HandleSetting(id,value);
return 0; }
      case 2: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_SETTING,[&] { return NetLogSpdyRecvSettingParams(id,value); });
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnSettingsEnd() {
  settings_frame_received_ = true;
}

void SpdySession::OnWindowUpdate(spdy::SpdyStreamId stream_id,
                                 int delta_window_size) {
ActiveStreamMap::iterator it;
  RunSpdySessionRust(81,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_WINDOW_UPDATE,[&] { return NetLogSpdyWindowUpdateFrameParams(stream_id,delta_window_size); });
return 0; }
      case 1: { return stream_id==spdy::kSessionFlowControlStreamId; }
      case 2: { return delta_window_size; }
      case 3: { RecordProtocolErrorHistogram(PROTOCOL_ERROR_INVALID_WINDOW_UPDATE_SIZE);
return 0; }
      case 4: { DoDrainSession(ERR_HTTP2_PROTOCOL_ERROR,"Received WINDOW_UPDATE with an invalid delta_window_size "+base::NumberToString(delta_window_size));
return 0; }
      case 5: { IncreaseSendWindowSize(delta_window_size);
return 0; }
      case 6: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 7: { LOG(WARNING)<<"Received WINDOW_UPDATE for invalid stream "<<stream_id;
return 0; }
      case 8: { CHECK_EQ(it->second->stream_id(),stream_id);
return 0; }
      case 9: { ResetStreamIterator(it,ERR_HTTP2_FLOW_CONTROL_ERROR,"Received WINDOW_UPDATE with an invalid delta_window_size.");
return 0; }
      case 10: { CHECK_EQ(it->second->stream_id(),stream_id); it->second->IncreaseSendWindowSize(delta_window_size);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnPushPromise(spdy::SpdyStreamId /*stream_id*/,
                                spdy::SpdyStreamId /*promised_stream_id*/,
                                quiche::HttpHeaderBlock /*headers*/) {
RunSpdySessionRust(121,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_);
return 0; }
      case 1: { DoDrainSession(ERR_HTTP2_PROTOCOL_ERROR,"PUSH_PROMISE received");
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnHeaders(spdy::SpdyStreamId stream_id,
                            bool has_priority,
                            int weight,
                            spdy::SpdyStreamId parent_stream_id,
                            bool exclusive,
                            bool fin,
                            quiche::HttpHeaderBlock headers,
                            base::TimeTicks recv_first_byte_time) {
ActiveStreamMap::iterator it;
SpdyStream* stream=nullptr;
  RunSpdySessionRust(82,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_RECV_HEADERS,[&](NetLogCaptureMode mode) { return NetLogSpdyHeadersReceivedParams(&headers,fin,stream_id,mode); });
return 0; }
      case 1: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 2: { LOG(WARNING)<<"Received HEADERS for invalid stream "<<stream_id;
return 0; }
      case 3: { stream=it->second; CHECK_EQ(stream->stream_id(),stream_id); stream->AddRawReceivedBytes(last_compressed_frame_len_); last_compressed_frame_len_=0;
return 0; }
      case 4: { const base::Time response_time=base::Time::Now(); stream->OnHeadersReceived(headers,response_time,recv_first_byte_time);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnAltSvc(
    spdy::SpdyStreamId stream_id,
    std::string_view origin,
    const spdy::SpdyAltSvcWireFormat::AlternativeServiceVector& altsvc_vector) {
std::optional<SSLInfo> ssl_info;
  url::SchemeHostPort scheme_host_port;
  std::optional<GURL> gurl;
  ActiveStreamMap::iterator it;

  RunSpdySessionRust(83,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return spdy_session_key_.session_usage()==SessionUsage::kProxy; }
      case 1: { return stream_id; }
      case 2: { ssl_info.emplace(); return GetSSLInfo(&*ssl_info); }
      case 3: { return ssl_info->is_valid(); }
      case 4: { return IsCertStatusError(ssl_info->cert_status); }
      case 5: { return origin.empty(); }
      case 6: { gurl.emplace(origin); return gurl->is_valid(); }
      case 7: { return gurl->GetHost().empty(); }
      case 8: { return gurl->SchemeIs(url::kHttpsScheme); }
      case 9: { return CanPool(transport_security_state_,*ssl_info,*ssl_config_service_,host_port_pair().host(),gurl->host()); }
      case 10: { scheme_host_port=url::SchemeHostPort(*gurl);
return 0; }
      case 11: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 12: { return it->second->url().SchemeIs(url::kHttpsScheme); }
      case 13: { scheme_host_port=url::SchemeHostPort(it->second->url());
return 0; }
      case 14: { http_server_properties_->SetAlternativeServices(scheme_host_port,spdy_session_key_.network_anonymization_key(),ProcessAlternativeServices(altsvc_vector,is_http2_enabled_,is_quic_enabled_,quic_supported_versions_));
return 0; }
    }
    NOTREACHED();
  });
}

bool SpdySession::OnUnknownFrame(spdy::SpdyStreamId stream_id,
                                 uint8_t frame_type) {
const int64_t rust_result = RunSpdySessionRust(84,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return stream_id; }
      case 1: { return stream_hi_water_mark_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

void SpdySession::OnSendCompressedFrame(spdy::SpdyStreamId stream_id,
                                        spdy::SpdyFrameType type,
                                        size_t payload_len,
                                        size_t frame_len) {
RunSpdySessionRust(85,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return type==spdy::SpdyFrameType::HEADERS; }
      case 1: { DCHECK(buffered_spdy_framer_.get());
return 0; }
      case 2: { return frame_len; }
      case 3: { return spdy::kFrameMinimumSize; }
      case 4: { return payload_len; }
      case 5: { UMA_HISTOGRAM_PERCENTAGE("Net.SpdyHeadersCompressionPercentage",static_cast<int>(a));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnReceiveCompressedFrame(spdy::SpdyStreamId stream_id,
                                           spdy::SpdyFrameType type,
                                           size_t frame_len) {
  last_compressed_frame_len_ = frame_len;
}

void SpdySession::OnWriteBufferConsumed(
    size_t frame_payload_size,
    size_t consume_size,
    SpdyBuffer::ConsumeSource consume_source) {
int remaining_payload_bytes=0;
  RunSpdySessionRust(35,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return consume_source==SpdyBuffer::DISCARD; }
      case 1: { return consume_size; }
      case 2: { return frame_payload_size; }
      case 3: { remaining_payload_bytes=static_cast<int>(a); DCHECK_GT(remaining_payload_bytes,0);
return 0; }
      case 4: { IncreaseSendWindowSize(remaining_payload_bytes);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::IncreaseSendWindowSize(int delta_window_size) {
RunSpdySessionRust(36,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GE(delta_window_size,1); return delta_window_size; }
      case 1: { return session_send_window_size_; }
      case 2: { RecordProtocolErrorHistogram(PROTOCOL_ERROR_INVALID_WINDOW_UPDATE_SIZE);
return 0; }
      case 3: { DoDrainSessionAsync(ERR_HTTP2_PROTOCOL_ERROR,base::StrCat({"Received WINDOW_UPDATE [delta: ",base::NumberToString(delta_window_size),"] for session overflows session_send_window_size_ [current: ",base::NumberToString(session_send_window_size_),"]"}));
return 0; }
      case 4: { session_send_window_size_=static_cast<int32_t>(a);
return 0; }
      case 5: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_UPDATE_SEND_WINDOW,[&] { return NetLogSpdySessionWindowUpdateParams(delta_window_size,session_send_window_size_); });
return 0; }
      case 6: { DCHECK(!IsSendStalled());
return 0; }
      case 7: { ResumeSendStalledStreams();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::DecreaseSendWindowSize(int32_t delta_window_size) {
RunSpdySessionRust(37,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GE(delta_window_size,1); DCHECK_LE(delta_window_size,kMaxSpdyFrameChunkSize); DCHECK_GE(session_send_window_size_,delta_window_size); return delta_window_size; }
      case 1: { return session_send_window_size_; }
      case 2: { session_send_window_size_=static_cast<int32_t>(a);
return 0; }
      case 3: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_UPDATE_SEND_WINDOW,[&] { return NetLogSpdySessionWindowUpdateParams(-delta_window_size,session_send_window_size_); });
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnReadBufferConsumed(
    size_t consume_size,
    SpdyBuffer::ConsumeSource consume_source) {
RunSpdySessionRust(122,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GE(consume_size,1u); DCHECK_LE(consume_size,static_cast<size_t>(std::numeric_limits<int32_t>::max()));
return 0; }
      case 1: { IncreaseRecvWindowSize(static_cast<int32_t>(consume_size));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::IncreaseRecvWindowSize(int32_t delta_window_size) {
base::TimeDelta elapsed;
  RunSpdySessionRust(38,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GE(session_unacked_recv_window_bytes_,0); DCHECK_GE(session_recv_window_size_,session_unacked_recv_window_bytes_); DCHECK_GE(delta_window_size,1); DCHECK_LE(delta_window_size,std::numeric_limits<int32_t>::max()-session_recv_window_size_); return delta_window_size; }
      case 1: { return session_recv_window_size_; }
      case 2: { session_recv_window_size_=static_cast<int32_t>(a);
return 0; }
      case 3: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_UPDATE_RECV_WINDOW,[&] { return NetLogSpdySessionWindowUpdateParams(delta_window_size,session_recv_window_size_); });
return 0; }
      case 4: { return session_unacked_recv_window_bytes_; }
      case 5: { session_unacked_recv_window_bytes_=static_cast<int32_t>(a);
return 0; }
      case 6: { elapsed=base::TimeTicks::Now()-last_recv_window_update_;
return 0; }
      case 7: { return session_max_recv_window_size_; }
      case 8: { return elapsed>=time_to_buffer_small_window_updates_; }
      case 9: { last_recv_window_update_=base::TimeTicks::Now();
return 0; }
      case 10: { SendWindowUpdateFrame(spdy::kSessionFlowControlStreamId,session_unacked_recv_window_bytes_,HIGHEST);
return 0; }
      case 11: { session_unacked_recv_window_bytes_=0;
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::DecreaseRecvWindowSize(int32_t delta_window_size) {
int32_t receiving_window_size=0;
  RunSpdySessionRust(39,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { CHECK(in_io_loop_); DCHECK_GE(delta_window_size,1); return delta_window_size; }
      case 1: { return session_recv_window_size_; }
      case 2: { return session_unacked_recv_window_bytes_; }
      case 3: { receiving_window_size=static_cast<int32_t>(a);
return 0; }
      case 4: { RecordProtocolErrorHistogram(PROTOCOL_ERROR_RECEIVE_WINDOW_VIOLATION);
return 0; }
      case 5: { DoDrainSession(ERR_HTTP2_FLOW_CONTROL_ERROR,base::StrCat({"delta_window_size is ",base::NumberToString(delta_window_size)," in DecreaseRecvWindowSize, which is larger than the receive window size of ",base::NumberToString(receiving_window_size)}));
return 0; }
      case 6: { session_recv_window_size_=static_cast<int32_t>(a);
return 0; }
      case 7: { net_log_.AddEvent(NetLogEventType::HTTP2_SESSION_UPDATE_RECV_WINDOW,[&] { return NetLogSpdySessionWindowUpdateParams(-delta_window_size,session_recv_window_size_); });
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::QueueSendStalledStream(const SpdyStream& stream) {
RequestPriority priority=MINIMUM_PRIORITY;
  RunSpdySessionRust(123,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK(stream.send_stalled_by_flow_control() || IsSendStalled()); priority=stream.priority(); CHECK_GE(priority,MINIMUM_PRIORITY); CHECK_LE(priority,MAXIMUM_PRIORITY);
return 0; }
      case 1: { base::span(stream_send_unstall_queue_)[priority].push_back(stream.stream_id());
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::ResumeSendStalledStreams() {
base::circular_deque<SpdyStream*> streams_to_requeue;
  size_t old_size=0;
  spdy::SpdyStreamId stream_id=0;
  ActiveStreamMap::const_iterator it;

  RunSpdySessionRust(40,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return availability_state_==STATE_DRAINING; }
      case 1: { return drain_session_pending_; }
      case 2: { return IsSendStalled(); }
      case 3: { 
#if DCHECK_IS_ON()
       old_size=GetTotalSize(stream_send_unstall_queue_);
#endif
    
return 0; }
      case 4: { stream_id=PopStreamToPossiblyResume(); return stream_id; }
      case 5: { it=active_streams_.find(stream_id); return it!=active_streams_.end(); }
      case 6: { return it->second->PossiblyResumeIfSendStalled()==SpdyStream::Requeue; }
      case 7: { streams_to_requeue.push_back(it->second);
return 0; }
      case 8: { DCHECK_LT(GetTotalSize(stream_send_unstall_queue_),old_size);
return 0; }
      case 9: { return !streams_to_requeue.empty(); }
      case 10: { SpdyStream* stream=streams_to_requeue.front(); streams_to_requeue.pop_front(); QueueSendStalledStream(*stream);
return 0; }
    }
    NOTREACHED();
  });
}

spdy::SpdyStreamId SpdySession::PopStreamToPossiblyResume() {
const int64_t rust_result = RunSpdySessionRust(41,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return MAXIMUM_PRIORITY; }
      case 1: { return MINIMUM_PRIORITY; }
      case 2: { return base::span(stream_send_unstall_queue_)[a].empty(); }
      case 3: { auto& queue=base::span(stream_send_unstall_queue_)[a]; const auto id=queue.front(); queue.pop_front(); return id; }
    }
    NOTREACHED();
  });
return static_cast<spdy::SpdyStreamId>(rust_result);
}

void SpdySession::CheckConnectionStatus() {
RunSpdySessionRust(124,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { MaybeSendPrefacePing();
return 0; }
      case 1: { heartbeat_timer_.Start(FROM_HERE,heartbeat_interval_,base::BindOnce(&SpdySession::MaybeCheckConnectionStatus,weak_factory_.GetWeakPtr()));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::OnDefaultNetworkActive() {
RunSpdySessionRust(42,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return check_connection_on_radio_wakeup_; }
      case 1: { check_connection_on_radio_wakeup_=false;
return 0; }
      case 2: { CheckConnectionStatus();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySession::MaybeDisableBrokenConnectionDetection() {
RunSpdySessionRust(43,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { DCHECK_GT(broken_connection_detection_requests_,0); DCHECK(IsBrokenConnectionDetectionEnabled()); return broken_connection_detection_requests_; }
      case 1: { broken_connection_detection_requests_=static_cast<int>(a);
return 0; }
      case 2: { heartbeat_timer_.Stop();
return 0; }
      case 3: { NetworkChangeNotifier::RemoveDefaultNetworkActiveObserver(this);
return 0; }
      case 4: { check_connection_on_radio_wakeup_=false;
return 0; }
    }
    NOTREACHED();
  });
}

bool SpdySession::IsAvailable() const {

  const int64_t rust_result = RunSpdySessionRust(130,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { static_assert(STATE_AVAILABLE==0 && STATE_GOING_AWAY==1 && STATE_DRAINING==2); return availability_state_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

bool SpdySession::IsGoingAway() const {

  const int64_t rust_result = RunSpdySessionRust(131,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { static_assert(STATE_AVAILABLE==0 && STATE_GOING_AWAY==1 && STATE_DRAINING==2); return availability_state_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

bool SpdySession::IsDraining() const {

  const int64_t rust_result = RunSpdySessionRust(132,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { static_assert(STATE_AVAILABLE==0 && STATE_GOING_AWAY==1 && STATE_DRAINING==2); return availability_state_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

bool SpdySession::WasEverUsedToCreateStreams() const {

  const int64_t rust_result = RunSpdySessionRust(133,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return streams_initiated_count_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

bool SpdySession::IsSendStalled() const {

  const int64_t rust_result = RunSpdySessionRust(134,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return session_send_window_size_; }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

bool SpdySession::is_active() const {

  const int64_t rust_result = RunSpdySessionRust(135,[&](uint32_t operation,[[maybe_unused]] uint64_t a,[[maybe_unused]] uint64_t b,[[maybe_unused]] uint64_t c)->int64_t {
    switch(operation) {
      case 0: { return active_streams_.empty(); }
      case 1: { return created_streams_.empty(); }
    }
    NOTREACHED();
  });
return rust_result!=0;
}

}  // namespace net
