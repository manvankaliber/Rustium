// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTTP3_REFERENCE_STREAM_BASE_H_
#define HTTP3_REFERENCE_STREAM_BASE_H_

#include "net/base/net_export.h"
#include "net/third_party/quiche/src/quiche/quic/core/http/quic_spdy_client_session_base.h"
#include "net/third_party/quiche/src/quiche/quic/core/http/quic_spdy_stream.h"

namespace net::http3_reference {

// Chromium-owned HTTP/3 stream base that carries client-specific hooks used by
// QuicChromiumClientSession when iterating mixed active stream populations.
class COMPONENT_EXPORT(HTTP3_REFERENCE) QuicChromiumClientStreamBase
    : public quic::QuicSpdyStream {
 public:
  QuicChromiumClientStreamBase(quic::QuicStreamId id,
                               quic::QuicSpdyClientSessionBase* session,
                               quic::StreamType type);

  QuicChromiumClientStreamBase(const QuicChromiumClientStreamBase&) = delete;
  QuicChromiumClientStreamBase& operator=(const QuicChromiumClientStreamBase&) =
      delete;
  ~QuicChromiumClientStreamBase() override = default;

  virtual void OnError(int error);

  virtual bool CanMigrateToCellularNetwork() const;

 protected:
  void set_can_migrate_to_cellular_network(bool can_migrate) {
    can_migrate_to_cellular_network_ = can_migrate;
  }

 private:
  bool can_migrate_to_cellular_network_ = true;
};

}  // namespace net::http3_reference

#endif  // HTTP3_REFERENCE_STREAM_BASE_H_
