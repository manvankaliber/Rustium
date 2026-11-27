// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/connect_job_params_rust_original.h"

#include "net/http/http_proxy_connect_job_rust_original.h"
#include "net/socket/socks_connect_job_rust_original.h"
#include "net/socket/ssl_connect_job_rust_original.h"
#include "net/socket/transport_connect_job_rust_original.h"

#include "net/socket/connection_jobs_rust_original_helpers.h"

namespace net {

ConnectJobParamsRustOriginal::ConnectJobParamsRustOriginal() = default;
ConnectJobParamsRustOriginal::ConnectJobParamsRustOriginal(scoped_refptr<HttpProxySocketParamsRustOriginal> params)
    : params_(params) {}
ConnectJobParamsRustOriginal::ConnectJobParamsRustOriginal(scoped_refptr<SOCKSSocketParamsRustOriginal> params)
    : params_(params) {}
ConnectJobParamsRustOriginal::ConnectJobParamsRustOriginal(scoped_refptr<TransportSocketParamsRustOriginal> params)
    : params_(params) {}
ConnectJobParamsRustOriginal::ConnectJobParamsRustOriginal(scoped_refptr<SSLSocketParamsRustOriginal> params)
    : params_(params) {}

ConnectJobParamsRustOriginal::~ConnectJobParamsRustOriginal() = default;

ConnectJobParamsRustOriginal::ConnectJobParamsRustOriginal(ConnectJobParamsRustOriginal&) = default;
ConnectJobParamsRustOriginal& ConnectJobParamsRustOriginal::operator=(ConnectJobParamsRustOriginal&) = default;
ConnectJobParamsRustOriginal::ConnectJobParamsRustOriginal(ConnectJobParamsRustOriginal&&) = default;
ConnectJobParamsRustOriginal& ConnectJobParamsRustOriginal::operator=(ConnectJobParamsRustOriginal&&) = default;

}  // namespace net
