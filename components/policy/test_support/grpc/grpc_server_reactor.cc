// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/test_support/grpc/grpc_server_reactor.h"

#include "base/no_destructor.h"

namespace policy {
namespace test {

const grpc::Status& GetStandardReadsFailedError() {
  static const base::NoDestructor<grpc::Status> kReadsFailedError(
      grpc::StatusCode::ABORTED, "Reads failed");
  return *kReadsFailedError;
}

const grpc::Status& GetStandardWritesFailedError() {
  static const base::NoDestructor<grpc::Status> kWritesFailedError(
      grpc::StatusCode::ABORTED, "Writes failed");
  return *kWritesFailedError;
}

}  // namespace test
}  // namespace policy
