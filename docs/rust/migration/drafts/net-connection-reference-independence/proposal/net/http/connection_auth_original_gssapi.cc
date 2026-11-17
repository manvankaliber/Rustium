// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Named adaptation of the hash-bound GSS provider; not compiled or run.
#include "net/http/connection_auth_original_providers.h"
#if BUILDFLAG(USE_KERBEROS) && BUILDFLAG(IS_POSIX)
#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include "base/base64.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_values_rust_reference.h"
#include "net/log/net_log_with_source.h"
namespace net {
using DelegationType = HttpAuth::DelegationType;
namespace connection_original_gss {
base::DictValue GetGssStatusValue(GSSAPILibrary*, std::string_view, OM_uint32, OM_uint32);
OM_uint32 DelegationTypeToFlag(DelegationType delegation_type) {
  switch (delegation_type) {
    case DelegationType::kNone:
      return 0;
    case DelegationType::kByKdcPolicy:
      return GSS_C_DELEG_POLICY_FLAG;
    case DelegationType::kUnconstrained:
      return GSS_C_DELEG_FLAG;
  }
}

class ScopedBuffer {
 public:
  ScopedBuffer(gss_buffer_t buffer, GSSAPILibrary* gssapi_lib)
      : buffer_(buffer), gssapi_lib_(gssapi_lib) {
    DCHECK(gssapi_lib_);
  }

  ScopedBuffer(const ScopedBuffer&) = delete;
  ScopedBuffer& operator=(const ScopedBuffer&) = delete;

  ~ScopedBuffer() {
    if (buffer_ != GSS_C_NO_BUFFER) {
      OM_uint32 minor_status = 0;
      OM_uint32 major_status =
          gssapi_lib_->release_buffer(&minor_status, buffer_);
      DLOG_IF(WARNING, major_status != GSS_S_COMPLETE)
          << "Problem releasing buffer. major=" << major_status
          << ", minor=" << minor_status;
      buffer_ = GSS_C_NO_BUFFER;
    }
  }

 private:
  gss_buffer_t buffer_;
  raw_ptr<GSSAPILibrary> gssapi_lib_;
};

// ScopedName releases a gss_name_t when it goes out of scope.
class ScopedName {
 public:
  ScopedName(gss_name_t name, GSSAPILibrary* gssapi_lib)
      : name_(name), gssapi_lib_(gssapi_lib) {
    DCHECK(gssapi_lib_);
  }

  ScopedName(const ScopedName&) = delete;
  ScopedName& operator=(const ScopedName&) = delete;

  ~ScopedName() {
    if (name_ != GSS_C_NO_NAME) {
      OM_uint32 minor_status = 0;
      OM_uint32 major_status = gssapi_lib_->release_name(&minor_status, &name_);
      if (major_status != GSS_S_COMPLETE) {
        DLOG_IF(WARNING, major_status != GSS_S_COMPLETE)
            << "Problem releasing name. "
            << GetGssStatusValue(nullptr, "gss_release_name", major_status,
                                 minor_status);
      }
      name_ = GSS_C_NO_NAME;
    }
  }

 private:
  gss_name_t name_;
  raw_ptr<GSSAPILibrary> gssapi_lib_;
};

base::DictValue GetGssStatusCodeValue(GSSAPILibrary* gssapi_lib,
                                      OM_uint32 status,
                                      OM_uint32 status_code_type) {
  base::DictValue rv;

  rv.Set("status", static_cast<int>(status));

  // Message lookups aren't performed if there's no library or if the status
  // indicates success.
  if (!gssapi_lib || status == GSS_S_COMPLETE)
    return rv;

  // gss_display_status() can potentially return multiple strings by sending
  // each string on successive invocations. State is maintained across these
  // invocations in a caller supplied OM_uint32.  After each successful call,
  // the context is set to a non-zero value that should be passed as a message
  // context to each successive gss_display_status() call.  The initial and
  // terminal values of this context storage is 0.
  OM_uint32 message_context = 0;

  // To account for the off chance that gss_display_status() misbehaves and gets
  // into an infinite loop, we'll artificially limit the number of iterations to
  // |kMaxDisplayIterations|. This limit is arbitrary.
  constexpr size_t kMaxDisplayIterations = 8;
  size_t iterations = 0;

  // In addition, each message string is again arbitrarily limited to
  // |kMaxMsgLength|. There's no real documented limit to work with here.
  constexpr size_t kMaxMsgLength = 4096;

  base::ListValue messages;
  do {
    gss_buffer_desc_struct message_buffer = GSS_C_EMPTY_BUFFER;
    ScopedBuffer message_buffer_releaser(&message_buffer, gssapi_lib);

    OM_uint32 minor_status = 0;
    OM_uint32 major_status = gssapi_lib->display_status(
        &minor_status, status, status_code_type, GSS_C_NO_OID, &message_context,
        &message_buffer);

    if (major_status != GSS_S_COMPLETE || message_buffer.length == 0 ||
        !message_buffer.value) {
      continue;
    }

    std::string_view message_string{
        static_cast<const char*>(message_buffer.value),
        std::min(kMaxMsgLength, message_buffer.length)};

    // The returned string is almost assuredly ASCII, but be defensive.
    if (!base::IsStringUTF8(message_string))
      continue;

    messages.Append(message_string);
  } while (message_context != 0 && ++iterations < kMaxDisplayIterations);

  if (!messages.empty())
    rv.Set("message", std::move(messages));
  return rv;
}

base::DictValue GetGssStatusValue(GSSAPILibrary* gssapi_lib,
                                  std::string_view method,
                                  OM_uint32 major_status,
                                  OM_uint32 minor_status) {
  base::DictValue params;
  params.Set("function", method);
  params.Set("major_status",
             GetGssStatusCodeValue(gssapi_lib, major_status, GSS_C_GSS_CODE));
  params.Set("minor_status",
             GetGssStatusCodeValue(gssapi_lib, minor_status, GSS_C_MECH_CODE));
  return params;
}

base::DictValue OidToValue(gss_OID oid) {
  base::DictValue params;

  if (!oid || oid->length == 0) {
    params.Set("oid", "<Empty OID>");
    return params;
  }

  params.Set("length", static_cast<int>(oid->length));
  if (!oid->elements)
    return params;

  // Cap OID content at arbitrary limit 1k.
  constexpr OM_uint32 kMaxOidDataSize = 1024;
  params.Set("bytes", net_log_values_rust_reference::NetLogBinaryValue(oid->elements, std::min(kMaxOidDataSize,
                                                                oid->length)));

  // Based on RFC 2744 Appendix A. Hardcoding the OIDs in the list below to
  // avoid having a static dependency on the library. The OIDs are stored as
  // bytes rather than as gss_OID_desc, see the comment on PaddedOidDesc.
  static const struct {
    const char* symbolic_name;
    base::span<const char> oid;
  } kWellKnownOIDs[] = {
      {"GSS_C_NT_USER_NAME",
       base::span_from_cstring("\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x01")},
      {"GSS_C_NT_MACHINE_UID_NAME",
       base::span_from_cstring("\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x02")},
      {"GSS_C_NT_STRING_UID_NAME",
       base::span_from_cstring("\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x03")},
      {"GSS_C_NT_HOSTBASED_SERVICE_X",
       base::span_from_cstring("\x2b\x06\x01\x05\x06\x02")},
      {"GSS_C_NT_HOSTBASED_SERVICE",
       base::span_from_cstring("\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x04")},
      {"GSS_C_NT_ANONYMOUS",
       base::span_from_cstring("\x2b\x06\01\x05\x06\x03")},
      {"GSS_C_NT_EXPORT_NAME",
       base::span_from_cstring("\x2b\x06\x01\x05\x06\x04")}};

  // SAFETY: The GSS-API contract is that `elements` points to `length` bytes.
  auto oid_bytes = UNSAFE_BUFFERS(
      base::span(static_cast<const char*>(oid->elements), oid->length));
  for (auto& well_known_oid : kWellKnownOIDs) {
    if (well_known_oid.oid == oid_bytes) {
      params.Set("oid", well_known_oid.symbolic_name);
    }
  }

  return params;
}

base::DictValue GetDisplayNameValue(GSSAPILibrary* gssapi_lib,
                                    const gss_name_t gss_name) {
  OM_uint32 major_status = 0;
  OM_uint32 minor_status = 0;
  gss_buffer_desc_struct name = GSS_C_EMPTY_BUFFER;
  gss_OID name_type = GSS_C_NO_OID;

  base::DictValue rv;
  major_status =
      gssapi_lib->display_name(&minor_status, gss_name, &name, &name_type);
  ScopedBuffer scoped_output_name(&name, gssapi_lib);
  if (major_status != GSS_S_COMPLETE) {
    rv.Set("error", GetGssStatusValue(gssapi_lib, "gss_display_name",
                                      major_status, minor_status));
    return rv;
  }
  auto name_string =
      std::string_view(reinterpret_cast<const char*>(name.value), name.length);
  rv.Set("name", base::IsStringUTF8(name_string)
                     ? net_log_values_rust_reference::NetLogStringValue(name_string)
                     : net_log_values_rust_reference::NetLogBinaryValue(name.value, name.length));
  rv.Set("type", OidToValue(name_type));
  return rv;
}

base::DictValue ContextFlagsToValue(OM_uint32 flags) {
  base::DictValue rv;
  rv.Set("value", base::StringPrintf("0x%08x", flags));
  rv.Set("delegated", (flags & GSS_C_DELEG_FLAG) == GSS_C_DELEG_FLAG);
  rv.Set("mutual", (flags & GSS_C_MUTUAL_FLAG) == GSS_C_MUTUAL_FLAG);
  return rv;
}

base::DictValue GetContextStateAsValue(GSSAPILibrary* gssapi_lib,
                                       const gss_ctx_id_t context_handle) {
  base::DictValue rv;
  if (context_handle == GSS_C_NO_CONTEXT) {
    rv.Set("error", GetGssStatusValue(nullptr, "<none>", GSS_S_NO_CONTEXT, 0));
    return rv;
  }

  OM_uint32 major_status = 0;
  OM_uint32 minor_status = 0;
  gss_name_t src_name = GSS_C_NO_NAME;
  gss_name_t targ_name = GSS_C_NO_NAME;
  OM_uint32 lifetime_rec = 0;
  gss_OID mech_type = GSS_C_NO_OID;
  OM_uint32 ctx_flags = 0;
  int locally_initiated = 0;
  int open = 0;
  major_status = gssapi_lib->inquire_context(&minor_status,
                                             context_handle,
                                             &src_name,
                                             &targ_name,
                                             &lifetime_rec,
                                             &mech_type,
                                             &ctx_flags,
                                             &locally_initiated,
                                             &open);
  if (major_status != GSS_S_COMPLETE) {
    rv.Set("error", GetGssStatusValue(gssapi_lib, "gss_inquire_context",
                                      major_status, minor_status));
    return rv;
  }
  ScopedName scoped_src_name(src_name, gssapi_lib);
  ScopedName scoped_targ_name(targ_name, gssapi_lib);

  rv.Set("source", GetDisplayNameValue(gssapi_lib, src_name));
  rv.Set("target", GetDisplayNameValue(gssapi_lib, targ_name));
  // lifetime_rec is a uint32, while base::Value only takes ints. On 32 bit
  // platforms uint32 doesn't fit on an int.
  rv.Set("lifetime", base::NumberToString(lifetime_rec));
  rv.Set("mechanism", OidToValue(mech_type));
  rv.Set("flags", ContextFlagsToValue(ctx_flags));
  rv.Set("open", !!open);
  return rv;
}

// GSSAPI status codes consist of a calling error (essentially, a programmer
// bug), a routine error (defined by the RFC), and supplementary information,
// all bitwise-or'ed together in different regions of the 32 bit return value.
// This means a simple switch on the return codes is not sufficient.

int MapImportNameStatusToError(OM_uint32 major_status) {
  if (major_status == GSS_S_COMPLETE)
    return OK;
  if (GSS_CALLING_ERROR(major_status) != 0)
    return ERR_UNEXPECTED;
  OM_uint32 routine_error = GSS_ROUTINE_ERROR(major_status);
  switch (routine_error) {
    case GSS_S_FAILURE:
      // Looking at the MIT Kerberos implementation, this typically is returned
      // when memory allocation fails. However, the API does not guarantee
      // that this is the case, so using ERR_UNEXPECTED rather than
      // ERR_OUT_OF_MEMORY.
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case GSS_S_BAD_NAME:
    case GSS_S_BAD_NAMETYPE:
      return ERR_MALFORMED_IDENTITY;
    case GSS_S_DEFECTIVE_TOKEN:
      // Not mentioned in the API, but part of code.
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case GSS_S_BAD_MECH:
      return ERR_UNSUPPORTED_AUTH_SCHEME;
    default:
      return ERR_UNDOCUMENTED_SECURITY_LIBRARY_STATUS;
  }
}

int MapInitSecContextStatusToError(OM_uint32 major_status) {
  // Although GSS_S_CONTINUE_NEEDED is an additional bit, it seems like
  // other code just checks if major_status is equivalent to it to indicate
  // that there are no other errors included.
  if (major_status == GSS_S_COMPLETE || major_status == GSS_S_CONTINUE_NEEDED)
    return OK;
  if (GSS_CALLING_ERROR(major_status) != 0)
    return ERR_UNEXPECTED;
  OM_uint32 routine_status = GSS_ROUTINE_ERROR(major_status);
  switch (routine_status) {
    case GSS_S_DEFECTIVE_TOKEN:
      return ERR_INVALID_RESPONSE;
    case GSS_S_DEFECTIVE_CREDENTIAL:
      // Not expected since this implementation uses the default credential.
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case GSS_S_BAD_SIG:
      // Probably won't happen, but it's a bad response.
      return ERR_INVALID_RESPONSE;
    case GSS_S_NO_CRED:
      return ERR_INVALID_AUTH_CREDENTIALS;
    case GSS_S_CREDENTIALS_EXPIRED:
      return ERR_INVALID_AUTH_CREDENTIALS;
    case GSS_S_BAD_BINDINGS:
      // This only happens with mutual authentication.
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case GSS_S_NO_CONTEXT:
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case GSS_S_BAD_NAMETYPE:
      return ERR_UNSUPPORTED_AUTH_SCHEME;
    case GSS_S_BAD_NAME:
      return ERR_UNSUPPORTED_AUTH_SCHEME;
    case GSS_S_BAD_MECH:
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case GSS_S_FAILURE:
      // This should be an "Unexpected Security Status" according to the
      // GSSAPI documentation, but it's typically used to indicate that
      // credentials are not correctly set up on a user machine, such
      // as a missing credential cache or hitting this after calling
      // kdestroy.
      // TODO(cbentzel): Use minor code for even better mapping?
      return ERR_MISSING_AUTH_CREDENTIALS;
    default:
      if (routine_status != 0)
        return ERR_UNDOCUMENTED_SECURITY_LIBRARY_STATUS;
      break;
  }
  OM_uint32 supplemental_status = GSS_SUPPLEMENTARY_INFO(major_status);
  // Replays could indicate an attack.
  if (supplemental_status & (GSS_S_DUPLICATE_TOKEN | GSS_S_OLD_TOKEN |
                             GSS_S_UNSEQ_TOKEN | GSS_S_GAP_TOKEN))
    return ERR_INVALID_RESPONSE;

  // At this point, every documented status has been checked.
  return ERR_UNDOCUMENTED_SECURITY_LIBRARY_STATUS;
}

base::DictValue ImportNameErrorParams(GSSAPILibrary* library,
                                      std::string_view spn,
                                      OM_uint32 major_status,
                                      OM_uint32 minor_status) {
  base::DictValue params;
  params.Set("spn", spn);
  if (major_status != GSS_S_COMPLETE)
    params.Set("status", GetGssStatusValue(library, "import_name", major_status,
                                           minor_status));
  return params;
}

base::DictValue InitSecContextErrorParams(GSSAPILibrary* library,
                                          gss_ctx_id_t context,
                                          OM_uint32 major_status,
                                          OM_uint32 minor_status) {
  base::DictValue params;
  if (major_status != GSS_S_COMPLETE)
    params.Set("status", GetGssStatusValue(library, "gss_init_sec_context",
                                           major_status, minor_status));
  if (context != GSS_C_NO_CONTEXT)
    params.Set("context", GetContextStateAsValue(library, context));
  return params;
}


}  // namespace connection_original_gss

int ConnectionAuthOriginalGSSAPI::GenerateAuthToken(const AuthCredentials* credentials,
                                      const std::string& spn,
                                      const std::string& channel_bindings,
                                      std::string* auth_token,
                                      const NetLogWithSource& net_log,
                                      CompletionOnceCallback /*callback*/) {
  DCHECK(auth_token);

  gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
  input_token.length = decoded_server_auth_token_.length();
  input_token.value = (input_token.length > 0)
                          ? const_cast<char*>(decoded_server_auth_token_.data())
                          : nullptr;
  gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
  connection_original_gss::ScopedBuffer scoped_output_token(&output_token, library_.get());
  int rv = GetNextSecurityToken(spn, channel_bindings, &input_token,
                                &output_token, net_log);
  if (rv != OK)
    return rv;

  // Base64 encode data in output buffer and prepend the scheme.
  std::string encode_input(static_cast<char*>(output_token.value),
                           output_token.length);
  std::string encode_output = base::Base64Encode(encode_input);
  *auth_token = "Negotiate " + encode_output;
  return OK;
}

int ConnectionAuthOriginalGSSAPI::GetNextSecurityToken(const std::string& spn,
                                         const std::string& channel_bindings,
                                         gss_buffer_t in_token,
                                         gss_buffer_t out_token,
                                         const NetLogWithSource& net_log) {
  // GSSAPI header files, to this day, require OIDs passed in as non-const
  // pointers. Rather than const casting, let's just leave this as non-const.
  // Even if the OID pointer is const, the inner |elements| pointer is still
  // non-const.
  //
  // This is deliberately not static: a static gss_OID_desc would be statically
  // initialized data with a 4-byte-aligned pointer on x86 macOS (see the
  // comment on PaddedOidDesc), while a local is built on the stack by code.
  gss_OID_desc kGSS_C_NT_HOSTBASED_SERVICE = {
      10, const_cast<char*>("\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x04")};

  // Create a name for the principal
  // TODO(cbentzel): Just do this on the first pass?
  std::string spn_principal = spn;
  gss_buffer_desc spn_buffer = GSS_C_EMPTY_BUFFER;
  spn_buffer.value = const_cast<char*>(spn_principal.c_str());
  spn_buffer.length = spn_principal.size() + 1;
  OM_uint32 minor_status = 0;
  gss_name_t principal_name = GSS_C_NO_NAME;

  OM_uint32 major_status =
      library_->import_name(&minor_status, &spn_buffer,
                            &kGSS_C_NT_HOSTBASED_SERVICE, &principal_name);
  net_log.AddEvent(NetLogEventType::AUTH_LIBRARY_IMPORT_NAME, [&] {
    return connection_original_gss::ImportNameErrorParams(library_.get(), spn, major_status,
                                 minor_status);
  });
  int rv = connection_original_gss::MapImportNameStatusToError(major_status);
  if (rv != OK)
    return rv;
  connection_original_gss::ScopedName scoped_name(principal_name, library_.get());

  // Continue creating a security context.
  net_log.BeginEvent(NetLogEventType::AUTH_LIBRARY_INIT_SEC_CTX);
  major_status = library_->init_sec_context(
      &minor_status, GSS_C_NO_CREDENTIAL, scoped_sec_context_.receive(),
      principal_name, gss_oid_, connection_original_gss::DelegationTypeToFlag(delegation_type_),
      GSS_C_INDEFINITE, GSS_C_NO_CHANNEL_BINDINGS, in_token,
      nullptr,  // actual_mech_type
      out_token,
      nullptr,  // ret flags
      nullptr);
  net_log.EndEvent(NetLogEventType::AUTH_LIBRARY_INIT_SEC_CTX, [&] {
    return connection_original_gss::InitSecContextErrorParams(library_.get(), scoped_sec_context_.get(),
                                     major_status, minor_status);
  });
  return connection_original_gss::MapInitSecContextStatusToError(major_status);
}

}  // namespace net
#endif
