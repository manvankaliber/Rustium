// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NET_ERROR_RUST_REFERENCE_H_
#define NET_BASE_NET_ERROR_RUST_REFERENCE_H_

#include <string>

#include "base/component_export.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "net/base/net_export.h"

namespace net_error_rust_reference {

// Error values are negative.
enum Error {
  // No error. Change NetError.template after changing value.
  OK = 0,

#define NET_ERROR(label, value) ERR_##label = value,
#include "net/base/net_error_rust_reference_list.h"
#undef NET_ERROR

  // The value of the first certificate error code.
  ERR_CERT_BEGIN = ERR_CERT_COMMON_NAME_INVALID,
};

// Returns a textual representation of the error code for logging purposes.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE) std::string ErrorToString(int error);

// Same as above, but leaves off the leading "net::".
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE)
std::string ErrorToShortString(int error);

// Returns a textual representation of the error code and the extended eror
// code.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE)
std::string ExtendedErrorToString(int error, int extended_error_code);

// Returns true if |error| is a certificate error code. Note this does not
// include errors for client certificates.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE) bool IsCertificateError(int error);

// Returns true if |error| is a client certificate authentication error. This
// does not include ERR_SSL_PROTOCOL_ERROR which may also signal a bad client
// certificate.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE)
bool IsClientCertificateError(int error);

// Returns true if |error| is an error from hostname resolution.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE)
bool IsHostnameResolutionError(int error);

// Returns true if |error| means that the request has been blocked.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE)
bool IsRequestBlockedError(int error);

// Returns true if `error` is OK or is listed in net_error_list.h.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE) bool IsOkOrDefinedError(int error);

// Map system error code to Error.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE)
Error MapSystemError(logging::SystemErrorCode os_error);

// A convenient function to translate file error to net error code.
COMPONENT_EXPORT(NET_ERROR_RUST_REFERENCE)
Error FileErrorToNetError(base::File::Error file_error);

}  // namespace net_error_rust_reference

#endif  // NET_BASE_NET_ERROR_RUST_REFERENCE_H_
