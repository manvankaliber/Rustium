// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECTION_JOBS_RUST_FFI_H_
#define NET_SOCKET_CONNECTION_JOBS_RUST_FFI_H_
#include "net/base/net_export.h"
#include <stdint.h>
extern "C" {
using ConnectionJobsRustService =
    bool (*)(void*, uint32_t module, uint32_t action, int64_t argument, int64_t*);
// Services write initialized scalar output and report receiver liveness after
// every potentially reentrant native call. No pointer/ownership is retained.
NET_EXPORT_PRIVATE int64_t connection_jobs_rust_run(uint32_t module, uint32_t operation,
                                int64_t argument, void*, ConnectionJobsRustService);
}
#endif
