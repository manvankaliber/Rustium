// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See "SSPI Sample Application" at
// http://msdn.microsoft.com/en-us/library/aa918273.aspx

#include "net/http/connection_auth_original_sspi.h"

#include <utility>

#include "base/base64.h"
#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth.h"
#include "net/http/connection_auth_original_rounds.h"
#include "net/http/auth_challenge_reference.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_values.h"
#include "net/log/net_log_with_source.h"


namespace net {
using DelegationType = HttpAuth::DelegationType;
namespace {

base::DictValue SecurityStatusToValue(Error mapped_error,
                                      SECURITY_STATUS status) {
  base::DictValue params;
  params.Set("net_error", mapped_error);
  params.Set("security_status", static_cast<int>(status));
  return params;
}

base::DictValue AcquireCredentialsHandleParams(const std::u16string* domain,
                                               const std::u16string* user,
                                               Error result,
                                               SECURITY_STATUS status) {
  base::DictValue params;
  if (domain && user) {
    params.Set("domain", base::UTF16ToUTF8(*domain));
    params.Set("user", base::UTF16ToUTF8(*user));
  }
  params.Set("status", SecurityStatusToValue(result, status));
  return params;
}

base::DictValue ContextFlagsToValue(DWORD flags) {
  base::DictValue params;
  params.Set("value", base::StringPrintf("0x%08lx", flags));
  params.Set("delegated", (flags & ISC_RET_DELEGATE) == ISC_RET_DELEGATE);
  params.Set("mutual", (flags & ISC_RET_MUTUAL_AUTH) == ISC_RET_MUTUAL_AUTH);
  return params;
}

base::DictValue ContextAttributesToValue(SSPILibrary* library,
                                         PCtxtHandle handle,
                                         DWORD attributes) {
  base::DictValue params;

  SecPkgContext_NativeNames native_names = {0};
  auto qc_result = library->QueryContextAttributesEx(
      handle, SECPKG_ATTR_NATIVE_NAMES, &native_names, sizeof(native_names));
  if (qc_result == SEC_E_OK && native_names.sClientName &&
      native_names.sServerName) {
    params.Set("source", base::as_u16cstr(native_names.sClientName));
    params.Set("target", base::as_u16cstr(native_names.sServerName));
  }

  SecPkgContext_NegotiationInfo negotiation_info = {0};
  qc_result = library->QueryContextAttributesEx(
      handle, SECPKG_ATTR_NEGOTIATION_INFO, &negotiation_info,
      sizeof(negotiation_info));
  if (qc_result == SEC_E_OK && negotiation_info.PackageInfo &&
      negotiation_info.PackageInfo->Name) {
    params.Set("mechanism",
               base::as_u16cstr(negotiation_info.PackageInfo->Name));
    params.Set("open", negotiation_info.NegotiationState !=
                           SECPKG_NEGOTIATION_COMPLETE);
  }

  SecPkgContext_Authority authority = {0};
  qc_result = library->QueryContextAttributesEx(handle, SECPKG_ATTR_AUTHORITY,
                                                &authority, sizeof(authority));
  if (qc_result == SEC_E_OK && authority.sAuthorityName) {
    params.Set("authority", base::as_u16cstr(authority.sAuthorityName));
  }

  params.Set("flags", ContextFlagsToValue(attributes));
  return params;
}

base::DictValue InitializeSecurityContextParams(SSPILibrary* library,
                                                PCtxtHandle handle,
                                                Error result,
                                                SECURITY_STATUS status,
                                                DWORD attributes) {
  base::DictValue params;
  params.Set("status", SecurityStatusToValue(result, status));
  if (result == OK) {
    params.Set("context",
               ContextAttributesToValue(library, handle, attributes));
  }
  return params;
}

Error MapAcquireCredentialsStatusToError(SECURITY_STATUS status) {
  switch (status) {
    case SEC_E_OK:
      return OK;
    case SEC_E_INSUFFICIENT_MEMORY:
      return ERR_OUT_OF_MEMORY;
    case SEC_E_INTERNAL_ERROR:
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case SEC_E_NO_CREDENTIALS:
    case SEC_E_NOT_OWNER:
    case SEC_E_UNKNOWN_CREDENTIALS:
      return ERR_INVALID_AUTH_CREDENTIALS;
    case SEC_E_SECPKG_NOT_FOUND:
      // This indicates that the SSPI configuration does not match expectations
      return ERR_UNSUPPORTED_AUTH_SCHEME;
    default:
      return ERR_UNDOCUMENTED_SECURITY_LIBRARY_STATUS;
  }
}

Error AcquireExplicitCredentials(SSPILibrary* library,
                                 const std::u16string& domain,
                                 const std::u16string& user,
                                 const std::u16string& password,
                                 const NetLogWithSource& net_log,
                                 CredHandle* cred) {
  SEC_WINNT_AUTH_IDENTITY identity;
  identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
  identity.User = reinterpret_cast<unsigned short*>(
      const_cast<wchar_t*>(base::as_wcstr(user)));
  identity.UserLength = user.size();
  identity.Domain = reinterpret_cast<unsigned short*>(
      const_cast<wchar_t*>(base::as_wcstr(domain)));
  identity.DomainLength = domain.size();
  identity.Password = reinterpret_cast<unsigned short*>(
      const_cast<wchar_t*>(base::as_wcstr(password)));
  identity.PasswordLength = password.size();

  TimeStamp expiry;

  net_log.BeginEvent(NetLogEventType::AUTH_LIBRARY_ACQUIRE_CREDS);

  // Pass the username/password to get the credentials handle.
  SECURITY_STATUS status = library->AcquireCredentialsHandle(
      nullptr,                          // pszPrincipal
      SECPKG_CRED_OUTBOUND,             // fCredentialUse
      nullptr,                          // pvLogonID
      &identity,                        // pAuthData
      nullptr,                          // pGetKeyFn (not used)
      nullptr,                          // pvGetKeyArgument (not used)
      cred,                             // phCredential
      &expiry);                         // ptsExpiry

  auto result = MapAcquireCredentialsStatusToError(status);
  net_log.EndEvent(NetLogEventType::AUTH_LIBRARY_ACQUIRE_CREDS, [&] {
    return AcquireCredentialsHandleParams(&domain, &user, result, status);
  });
  return result;
}

Error AcquireDefaultCredentials(SSPILibrary* library,
                                const NetLogWithSource& net_log,
                                CredHandle* cred) {
  TimeStamp expiry;
  net_log.BeginEvent(NetLogEventType::AUTH_LIBRARY_ACQUIRE_CREDS);

  // Pass the username/password to get the credentials handle.
  // Note: Since the 5th argument is nullptr, it uses the default
  // cached credentials for the logged in user, which can be used
  // for a single sign-on.
  SECURITY_STATUS status = library->AcquireCredentialsHandle(
      nullptr,                          // pszPrincipal
      SECPKG_CRED_OUTBOUND,             // fCredentialUse
      nullptr,                          // pvLogonID
      nullptr,                          // pAuthData
      nullptr,                          // pGetKeyFn (not used)
      nullptr,                          // pvGetKeyArgument (not used)
      cred,                             // phCredential
      &expiry);                         // ptsExpiry

  auto result = MapAcquireCredentialsStatusToError(status);
  net_log.EndEvent(NetLogEventType::AUTH_LIBRARY_ACQUIRE_CREDS, [&] {
    return AcquireCredentialsHandleParams(nullptr, nullptr, result, status);
  });
  return result;
}

Error MapInitializeSecurityContextStatusToError(SECURITY_STATUS status) {
  switch (status) {
    case SEC_E_OK:
    case SEC_I_CONTINUE_NEEDED:
      return OK;
    case SEC_I_COMPLETE_AND_CONTINUE:
    case SEC_I_COMPLETE_NEEDED:
    case SEC_I_INCOMPLETE_CREDENTIALS:
    case SEC_E_INCOMPLETE_MESSAGE:
    case SEC_E_INTERNAL_ERROR:
      // These are return codes reported by InitializeSecurityContext
      // but not expected by Chrome (for example, INCOMPLETE_CREDENTIALS
      // and INCOMPLETE_MESSAGE are intended for schannel).
      return ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS;
    case SEC_E_INSUFFICIENT_MEMORY:
      return ERR_OUT_OF_MEMORY;
    case SEC_E_UNSUPPORTED_FUNCTION:
      DLOG(DFATAL);
      return ERR_UNEXPECTED;
    case SEC_E_INVALID_HANDLE:
      DLOG(DFATAL);
      return ERR_INVALID_HANDLE;
    case SEC_E_INVALID_TOKEN:
      return ERR_INVALID_RESPONSE;
    case SEC_E_LOGON_DENIED:
    case SEC_E_NO_CREDENTIALS:
    case SEC_E_WRONG_PRINCIPAL:
      return ERR_INVALID_AUTH_CREDENTIALS;
    case SEC_E_NO_AUTHENTICATING_AUTHORITY:
    case SEC_E_TARGET_UNKNOWN:
      return ERR_MISCONFIGURED_AUTH_ENVIRONMENT;
    default:
      return ERR_UNDOCUMENTED_SECURITY_LIBRARY_STATUS;
  }
}

// Each HttpAuthSSPI gets its own sequence so a handshake stalled inside
// InitializeSecurityContext() does not delay unrelated handshakes.
//
// CONTINUE_ON_SHUTDOWN because shutdown must not wait on an unreachable KDC.
// If shutdown lands mid-handshake, the SspiContext teardown posted by
// base::SequenceBound may never run and the SSPI handles are leaked; that is
// acceptable at process exit.
scoped_refptr<base::SequencedTaskRunner> CreateSspiTaskRunner() {
  return base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN});
}

}  // anonymous namespace

class ConnectionAuthOriginalSSPI::SspiContext {
 public:
  SspiContext(scoped_refptr<SSPILibrary> library, HttpAuth::Scheme scheme)
      : library_(std::move(library)), scheme_(scheme) {
    SecInvalidateHandle(&cred_);
    SecInvalidateHandle(&ctxt_);
  }

  SspiContext(const SspiContext&) = delete;
  SspiContext& operator=(const SspiContext&) = delete;

  ~SspiContext();

  TokenResult GenerateToken(std::optional<AuthCredentials> credentials,
                            std::string spn,
                            std::string channel_bindings,
                            std::string decoded_server_auth_token,
                            DelegationType delegation_type,
                            NetLogWithSource net_log);

 private:
  int GenerateTokenImpl(const AuthCredentials* credentials,
                        const std::string& spn,
                        const std::string& channel_bindings,
                        const std::string& decoded_server_auth_token,
                        DelegationType delegation_type,
                        const NetLogWithSource& net_log,
                        std::string* auth_token);

  int OnFirstRound(const AuthCredentials* credentials,
                   const NetLogWithSource& net_log);

  int GetNextSecurityToken(const std::string& spn,
                           const std::string& channel_bindings,
                           const void* in_token,
                           int in_token_len,
                           DelegationType delegation_type,
                           const NetLogWithSource& net_log,
                           void** out_token,
                           int* out_token_len);

  void ResetSecurityContext();

  scoped_refptr<SSPILibrary> library_;
  HttpAuth::Scheme scheme_;
  CredHandle cred_;
  CtxtHandle ctxt_;
};

ConnectionAuthOriginalSSPI::SspiContext::~SspiContext() {
  ResetSecurityContext();
  if (SecIsValidHandle(&cred_)) {
    library_->FreeCredentialsHandle(&cred_);
    SecInvalidateHandle(&cred_);
  }
}

ConnectionAuthOriginalSSPI::TokenResult ConnectionAuthOriginalSSPI::SspiContext::GenerateToken(
    std::optional<AuthCredentials> credentials,
    std::string spn,
    std::string channel_bindings,
    std::string decoded_server_auth_token,
    DelegationType delegation_type,
    NetLogWithSource net_log) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  TokenResult result;
  result.net_error = GenerateTokenImpl(
      credentials ? &credentials.value() : nullptr, spn, channel_bindings,
      decoded_server_auth_token, delegation_type, net_log, &result.auth_token);
  if (result.net_error != OK) {
    ResetSecurityContext();
  }
  result.has_security_context = SecIsValidHandle(&ctxt_);
  return result;
}

int ConnectionAuthOriginalSSPI::SspiContext::GenerateTokenImpl(
    const AuthCredentials* credentials,
    const std::string& spn,
    const std::string& channel_bindings,
    const std::string& decoded_server_auth_token,
    DelegationType delegation_type,
    const NetLogWithSource& net_log,
    std::string* auth_token) {
  // Initial challenge.
  if (!SecIsValidHandle(&cred_)) {
    // ParseChallenge fails early if a non-empty token is received on the first
    // challenge.
    DCHECK(decoded_server_auth_token.empty());
    int rv = OnFirstRound(credentials, net_log);
    if (rv != OK) {
      return rv;
    }
  }

  DCHECK(SecIsValidHandle(&cred_));
  void* out_buf;
  int out_buf_len;
  int rv = GetNextSecurityToken(
      spn, channel_bindings, decoded_server_auth_token.data(),
      decoded_server_auth_token.length(), delegation_type, net_log, &out_buf,
      &out_buf_len);
  if (rv != OK) {
    return rv;
  }

  // Base64 encode data in output buffer and prepend the scheme.
  std::string encode_input(static_cast<char*>(out_buf), out_buf_len);
  std::string encode_output = auth_challenge_reference::FrozenEncode(encode_input);
  // OK, we are done with |out_buf|
  free(out_buf);
  if (scheme_ == HttpAuth::AUTH_SCHEME_NEGOTIATE) {
    *auth_token = "Negotiate " + encode_output;
  } else {
    *auth_token = "NTLM " + encode_output;
  }
  return OK;
}

ConnectionAuthOriginalSSPI::ConnectionAuthOriginalSSPI(scoped_refptr<SSPILibrary> library,
                           HttpAuth::Scheme scheme)
    : library_(std::move(library)),
      scheme_(scheme),
      delegation_type_(DelegationType::kNone) {
  DCHECK(library_);
  DCHECK(scheme_ == HttpAuth::AUTH_SCHEME_NEGOTIATE ||
         scheme_ == HttpAuth::AUTH_SCHEME_NTLM);
}

ConnectionAuthOriginalSSPI::~ConnectionAuthOriginalSSPI() = default;

bool ConnectionAuthOriginalSSPI::Init(const NetLogWithSource&) {
  return true;
}

bool ConnectionAuthOriginalSSPI::NeedsIdentity() const {
  return decoded_server_auth_token_.empty();
}

bool ConnectionAuthOriginalSSPI::AllowsExplicitCredentials() const {
  return true;
}

void ConnectionAuthOriginalSSPI::SetDelegation(DelegationType delegation_type) {
  // Snapshotted when a token is generated, so changing it while a call is
  // outstanding would silently apply only to the next round.
  CHECK(callback_.is_null());
  delegation_type_ = delegation_type;
}

void ConnectionAuthOriginalSSPI::SspiContext::ResetSecurityContext() {
  if (SecIsValidHandle(&ctxt_)) {
    library_->DeleteSecurityContext(&ctxt_);
    SecInvalidateHandle(&ctxt_);
  }
}

HttpAuth::AuthorizationResult ConnectionAuthOriginalSSPI::ParseChallenge(
    HttpAuthChallengeTokenizer* tok) {
  if (!has_security_context_) {
    return connection_auth_original_rounds::First(scheme_, *tok);
  }
  std::string encoded_auth_token;
  return connection_auth_original_rounds::Later(scheme_, *tok, &encoded_auth_token,
                                           &decoded_server_auth_token_);
}

int ConnectionAuthOriginalSSPI::GenerateAuthToken(const AuthCredentials* credentials,
                                    const std::string& spn,
                                    const std::string& channel_bindings,
                                    std::string* auth_token,
                                    const NetLogWithSource& net_log,
                                    CompletionOnceCallback callback) {
  DCHECK(auth_token);
  DCHECK(!callback.is_null());
  CHECK(callback_.is_null());

  auth_token_ = auth_token;
  callback_ = std::move(callback);

  std::optional<AuthCredentials> credentials_copy;
  if (credentials) {
    credentials_copy = *credentials;
  }

  if (context_.is_null()) {
    context_ = base::SequenceBound<SspiContext>(CreateSspiTaskRunner(),
                                                library_, scheme_);
  }

  context_.AsyncCall(&SspiContext::GenerateToken)
      .WithArgs(std::move(credentials_copy), spn, channel_bindings,
                decoded_server_auth_token_, delegation_type_, net_log)
      .Then(base::BindOnce(&ConnectionAuthOriginalSSPI::OnTokenGenerated,
                           weak_factory_.GetWeakPtr()));
  return ERR_IO_PENDING;
}

void ConnectionAuthOriginalSSPI::OnTokenGenerated(TokenResult result) {
  DCHECK(!callback_.is_null());

  has_security_context_ = result.has_security_context;
  if (result.net_error == OK) {
    *auth_token_ = std::move(result.auth_token);
  }
  auth_token_ = nullptr;

  std::move(callback_).Run(result.net_error);
}

int ConnectionAuthOriginalSSPI::SspiContext::OnFirstRound(const AuthCredentials* credentials,
                                            const NetLogWithSource& net_log) {
  DCHECK(!SecIsValidHandle(&cred_));
  int rv = OK;
  if (credentials) {
    std::u16string domain;
    std::u16string user;
    SplitDomainAndUser(credentials->username(), &domain, &user);
    rv = AcquireExplicitCredentials(library_.get(), domain, user,
                                    credentials->password(), net_log, &cred_);
    if (rv != OK)
      return rv;
  } else {
    rv = AcquireDefaultCredentials(library_.get(), net_log, &cred_);
    if (rv != OK)
      return rv;
  }

  return rv;
}

int ConnectionAuthOriginalSSPI::SspiContext::GetNextSecurityToken(
    const std::string& spn,
    const std::string& channel_bindings,
    const void* in_token,
    int in_token_len,
    DelegationType delegation_type,
    const NetLogWithSource& net_log,
    void** out_token,
    int* out_token_len) {
  ULONG max_token_length = 0;
  // Microsoft SDKs have a loose relationship with const.
  Error rv = library_->DetermineMaxTokenLength(&max_token_length);
  if (rv != OK)
    return rv;

  CtxtHandle* ctxt_ptr = nullptr;
  SecBufferDesc in_buffer_desc, out_buffer_desc;
  SecBufferDesc* in_buffer_desc_ptr = nullptr;
  std::array<SecBuffer, 2> in_buffers;
  SecBuffer out_buffer;

  in_buffer_desc.ulVersion = SECBUFFER_VERSION;
  in_buffer_desc.cBuffers = 0;
  in_buffer_desc.pBuffers = in_buffers.data();
  if (in_token_len > 0) {
    // Prepare input buffer.
    SecBuffer& sec_buffer = in_buffers[in_buffer_desc.cBuffers++];
    sec_buffer.BufferType = SECBUFFER_TOKEN;
    sec_buffer.cbBuffer = in_token_len;
    sec_buffer.pvBuffer = const_cast<void*>(in_token);
    ctxt_ptr = &ctxt_;
  } else {
    // If there is no input token, then we are starting a new authentication
    // sequence.  If we have already initialized our security context, then
    // we're incorrectly reusing the auth handler for a new sequence.
    if (SecIsValidHandle(&ctxt_)) {
      return ERR_UNEXPECTED;
    }
  }

  std::vector<char> sec_channel_bindings_buffer;
  if (!channel_bindings.empty()) {
    sec_channel_bindings_buffer.reserve(sizeof(SEC_CHANNEL_BINDINGS) +
                                        channel_bindings.size());
    sec_channel_bindings_buffer.resize(sizeof(SEC_CHANNEL_BINDINGS));
    // SAFETY: `sec_channel_bindings_buffer` was allocated to be long enough to
    // hold a SEC_CHANNEL_BINDINGS object above.
    SEC_CHANNEL_BINDINGS* bindings_desc =
        UNSAFE_BUFFERS(reinterpret_cast<SEC_CHANNEL_BINDINGS*>(
            sec_channel_bindings_buffer.data()));
    bindings_desc->cbApplicationDataLength = channel_bindings.size();
    bindings_desc->dwApplicationDataOffset = sizeof(SEC_CHANNEL_BINDINGS);
    sec_channel_bindings_buffer.insert(sec_channel_bindings_buffer.end(),
                                       channel_bindings.begin(),
                                       channel_bindings.end());
    DCHECK_EQ(sizeof(SEC_CHANNEL_BINDINGS) + channel_bindings.size(),
              sec_channel_bindings_buffer.size());

    SecBuffer& sec_buffer = in_buffers[in_buffer_desc.cBuffers++];
    sec_buffer.BufferType = SECBUFFER_CHANNEL_BINDINGS;
    sec_buffer.cbBuffer = sec_channel_bindings_buffer.size();
    sec_buffer.pvBuffer = sec_channel_bindings_buffer.data();
  }

  if (in_buffer_desc.cBuffers > 0)
    in_buffer_desc_ptr = &in_buffer_desc;

  // Prepare output buffer.
  out_buffer_desc.ulVersion = SECBUFFER_VERSION;
  out_buffer_desc.cBuffers = 1;
  out_buffer_desc.pBuffers = &out_buffer;
  out_buffer.BufferType = SECBUFFER_TOKEN;
  out_buffer.cbBuffer = max_token_length;
  out_buffer.pvBuffer = malloc(out_buffer.cbBuffer);
  if (!out_buffer.pvBuffer)
    return ERR_OUT_OF_MEMORY;

  DWORD context_flags = 0;
  // Firefox only sets ISC_REQ_DELEGATE, but MSDN documentation indicates that
  // ISC_REQ_MUTUAL_AUTH must also be set. On Windows delegation by KDC policy
  // is always respected.
  if (scheme_ == HttpAuth::AUTH_SCHEME_NEGOTIATE &&
      delegation_type != DelegationType::kNone) {
    context_flags |= (ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH);
  }

  net_log.BeginEvent(NetLogEventType::AUTH_LIBRARY_INIT_SEC_CTX, [&] {
    base::DictValue params;
    params.Set("spn", spn);
    params.Set("flags", ContextFlagsToValue(context_flags));
    return params;
  });

  // This returns a token that is passed to the remote server.
  DWORD context_attributes = 0;
  std::u16string spn16 = base::ASCIIToUTF16(spn);
  SECURITY_STATUS status = library_->InitializeSecurityContext(
      &cred_,                          // phCredential
      ctxt_ptr,                        // phContext
      base::as_writable_wcstr(spn16),  // pszTargetName
      context_flags,                   // fContextReq
      0,                               // Reserved1 (must be 0)
      SECURITY_NATIVE_DREP,            // TargetDataRep
      in_buffer_desc_ptr,              // pInput
      0,                               // Reserved2 (must be 0)
      &ctxt_,                          // phNewContext
      &out_buffer_desc,                // pOutput
      &context_attributes,             // pfContextAttr
      nullptr);                        // ptsExpiry
  rv = MapInitializeSecurityContextStatusToError(status);
  net_log.EndEvent(NetLogEventType::AUTH_LIBRARY_INIT_SEC_CTX, [&] {
    return InitializeSecurityContextParams(library_.get(), &ctxt_, rv, status,
                                           context_attributes);
  });

  if (rv != OK) {
    ResetSecurityContext();
    free(out_buffer.pvBuffer);
    return rv;
  }
  if (!out_buffer.cbBuffer) {
    free(out_buffer.pvBuffer);
    out_buffer.pvBuffer = nullptr;
  }
  *out_token = out_buffer.pvBuffer;
  *out_token_len = out_buffer.cbBuffer;
  return OK;
}


}  // namespace net
