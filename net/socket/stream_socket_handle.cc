// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/stream_socket_handle.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "base/check.h"

#include <memory>

#include "net/base/load_timing_info.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/stream_socket.h"

namespace net {

// Typed storage/provider primitives only. Sequencing/branches live in safe Rust.
struct StreamSocketHandleFamilyRustBridge final
    : connection_jobs_upper_rust::Invocation {
  using Class = StreamSocketHandle;

  explicit StreamSocketHandleFamilyRustBridge(Class* job)
      : Invocation(job), mutable_owner(job), owner(job) { Register(&Clear);  }
  explicit StreamSocketHandleFamilyRustBridge(const Class* job)
      : Invocation(job), mutable_owner(nullptr), owner(job) { Register(&Clear);  }
  StreamSocketHandleFamilyRustBridge()
      : Invocation(this), mutable_owner(nullptr), owner(nullptr) { Register(&Clear); }
  ~StreamSocketHandleFamilyRustBridge() {  Unregister(); }
  Class& M() { CHECK(Live()); CHECK(mutable_owner); return *mutable_owner; }
  const Class& R() const { CHECK(Live()); CHECK(owner); return *owner; }
  int64_t Run(uint32_t operation, int64_t argument = 0) {
    return connection_jobs_upper_rust::connection_jobs_rust_run(
        13, operation, argument, this, Service);
  }
  raw_ptr<Class> mutable_owner;
  raw_ptr<const Class> owner;


  raw_ptr<std::unique_ptr<StreamSocket>> socket_input=nullptr;
  raw_ptr<std::unique_ptr<StreamSocket>> socket_output=nullptr;
  raw_ptr<LoadTimingInfo> timing_output=nullptr;

  static void Clear(Invocation* value) {
    auto& c = *static_cast<StreamSocketHandleFamilyRustBridge*>(value);
    c.mutable_owner = nullptr;
    c.owner = nullptr;
    c.socket_input = nullptr;
    c.socket_output = nullptr;
    c.timing_output = nullptr;
  }

  static bool Service(void* context, uint32_t module, uint32_t action, int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<StreamSocketHandleFamilyRustBridge*>(context);
    *value = 0;
    
    if (!c.Live() || module != 13) { return false; }
    if(action==UINT32_MAX){*value=1;return true;}
    switch (action) {
      case 0: {  // MOVE_SOCKET
c.M().socket_=std::move(*c.socket_input);
        break;
      }
      case 1: {  // PASS_SOCKET
*c.socket_output=std::move(c.M().socket_);
        break;
      }
      case 2: {  // HAS_SOCKET
*value=c.R().socket_!=nullptr;
        break;
      }
      case 3: {  // STAMP_SOURCE_AND_REUSE
const auto id=c.R().socket_->NetLog().source().id;
c.timing_output->socket_log_id=id;c.timing_output->socket_reused=argument!=0;
        break;
      }
      case 4: {  // COPY_TIMING
c.timing_output->connect_timing=c.R().connect_timing_;
        break;
      }
      default: return false;
    }
    // Only invocation-local liveness is read after any deleting callback.
    return c.Live();
  }
};


StreamSocketHandle::StreamSocketHandle() = default;

StreamSocketHandle::~StreamSocketHandle() { connection_jobs_upper_rust::Invocation::Invalidate(this); }

void StreamSocketHandle::SetSocket(std::unique_ptr<StreamSocket> socket) {
  StreamSocketHandleFamilyRustBridge c(this);c.socket_input=&socket;c.Run(0);
}

std::unique_ptr<StreamSocket> StreamSocketHandle::PassSocket() {
  std::unique_ptr<StreamSocket> result;StreamSocketHandleFamilyRustBridge c(this);c.socket_output=&result;c.Run(1);return result;
}

bool StreamSocketHandle::GetLoadTimingInfo(
    bool is_reused,
    LoadTimingInfo* load_timing_info) const {
  StreamSocketHandleFamilyRustBridge c(this);c.timing_output=load_timing_info;return c.Run(2,is_reused)!=0;
}

void StreamSocketHandle::AddHigherLayeredPool(HigherLayeredPool* pool) {}

void StreamSocketHandle::RemoveHigherLayeredPool(HigherLayeredPool* pool) {}


}  // namespace net
