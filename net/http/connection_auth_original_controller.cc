#include "net/log/connection_original_log.h"
#include "net/base/net_error_rust_reference.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/log/connection_original_log.h"
// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/platform_thread.h"
#include "base/values.h"
#include "net/base/auth.h"
#include "net/base/url_util.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/ssl/ssl_info.h"
#include "url/scheme_host_port.h"

namespace net {

namespace {

bool ServerCertMatches(const X509Certificate* handler_cert,
                       const X509Certificate* challenge_cert) {
  if (handler_cert == challenge_cert) {
    return true;
  }
  if (!handler_cert || !challenge_cert) {
    return false;
  }
  return handler_cert->EqualsExcludingChain(challenge_cert);
}

enum AuthTarget {
  AUTH_TARGET_PROXY = 0,
  AUTH_TARGET_SECURE_PROXY,
  AUTH_TARGET_SERVER,
  AUTH_TARGET_SECURE_SERVER,
  AUTH_TARGET_MAX,
};

AuthTarget DetermineAuthTarget(const HttpAuthHandler* handler) {
  switch (handler->target()) {
    case HttpAuth::AUTH_PROXY:
      if (GURL::SchemeIsCryptographic(handler->scheme_host_port().scheme())) {
        return AUTH_TARGET_SECURE_PROXY;
      } else {
        return AUTH_TARGET_PROXY;
      }
    case HttpAuth::AUTH_SERVER:
      if (GURL::SchemeIsCryptographic(handler->scheme_host_port().scheme())) {
        return AUTH_TARGET_SECURE_SERVER;
      } else {
        return AUTH_TARGET_SERVER;
      }
    default:
      NOTREACHED();
  }
}

base::DictValue ControllerParamsToValue(HttpAuth::Target target,
                                        const GURL& url,
                                        NetLogCaptureMode capture_mode) {
  base::DictValue params;
  params.Set("target", HttpAuth::GetAuthTargetString(target));
  params.Set("url", SanitizeUrlForNetLog(url, capture_mode));
  return params;
}

}  // namespace

void ConnectionAuthOriginalPolicy::BindToCallingNetLog(HttpAuthController* self,
    const NetLogWithSource& caller_net_log) {
  if (!self->net_log_.source().IsValid()) {
    self->net_log_ = NetLogWithSource::Make(caller_net_log.net_log(),
                                      NetLogSourceType::HTTP_AUTH_CONTROLLER);
    self->net_log_.BeginEvent(
        NetLogEventType::AUTH_CONTROLLER, [&](NetLogCaptureMode capture_mode) {
          return ControllerParamsToValue(self->target_, self->auth_url_, capture_mode);
        });
  }
  caller_net_log.AddEventReferencingSource(
      NetLogEventType::AUTH_BOUND_TO_CONTROLLER, self->net_log_.source());
}

int ConnectionAuthOriginalPolicy::MaybeGenerateAuthToken(HttpAuthController* self,
    const HttpRequestInfo* request,
    const SSLInfo& ssl_info,
    CompletionOnceCallback callback,
    const NetLogWithSource& caller_net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  DCHECK(!self->auth_info_);

  // A connection-based handler derives per-connection state (such as channel
  // bindings) from the server certificate at creation time. If the
  // underlying connection has been replaced with one that presents a
  // different certificate since the last challenge, that state is no longer
  // valid for the current connection, so drop the handler instead of
  // generating a token from it. The next challenge starts the authentication
  // protocol over on the current connection.
  InvalidateIfServerCertChanged(self, ssl_info.cert.get());

  bool needs_auth = HaveAuth(self) || SelectPreemptiveAuth(self, caller_net_log);
  if (!needs_auth) {
    return OK;
  }
  self->net_log_.BeginEventReferencingSource(NetLogEventType::AUTH_GENERATE_TOKEN,
                                       caller_net_log.source());
  const AuthCredentials* credentials = nullptr;
  if (self->identity_.source != HttpAuth::IDENT_SRC_DEFAULT_CREDENTIALS) {
    credentials = &self->identity_.credentials;
  }
  DCHECK(self->auth_token_.empty());
  DCHECK(self->callback_.is_null());
  int rv = ConnectionAuthOriginalHandlerPolicy::GenerateAuthToken(self->handler_.get(),
      credentials, request,
      base::BindOnce(&ConnectionAuthOriginalPolicy::OnGenerateAuthTokenDone,
                     base::Unretained(self)),
      &self->auth_token_);

  if (rv == ERR_IO_PENDING) {
    self->callback_ = std::move(callback);
    return rv;
  }

  return HandleGenerateTokenResult(self, rv);
}

bool ConnectionAuthOriginalPolicy::SelectPreemptiveAuth(HttpAuthController* self,
    const NetLogWithSource& caller_net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  DCHECK(!HaveAuth(self));
  DCHECK(self->identity_.invalid);

  // Don't do preemptive authorization if the URL contains a username:password,
  // since we must first be challenged in order to use the URL's identity.
  if (self->auth_url_.has_username()) {
    return false;
  }

  // SelectPreemptiveAuth(self) is on the critical path for each request, so it
  // is expected to be fast. LookupByPath() is fast in the common case, since
  // the number of http auth cache entries is expected to be very small.
  // (For most users in fact, it will be 0.)
  HttpAuthCache::Entry* entry = ConnectionAuthOriginalCache::LookupByPath(self->http_auth_cache_,
      self->auth_scheme_host_port_, self->target_, self->network_anonymization_key_, self->auth_path_);
  if (!entry) {
    return false;
  }

  BindToCallingNetLog(self, caller_net_log);

  // Try to create a handler using the previous auth challenge.
  std::unique_ptr<HttpAuthHandler> handler_preemptive;
  int rv_create =
      ConnectionAuthOriginalFactoryPolicy::CreatePreemptiveAuthHandlerFromString(self->http_auth_handler_factory_,
          entry->auth_challenge(), self->target_, self->network_anonymization_key_,
          self->auth_scheme_host_port_, ConnectionAuthOriginalCache::IncrementNonceCount(entry), self->net_log_,
          self->host_resolver_, &handler_preemptive);
  if (rv_create != OK)
    return false;

  // Set the state
  self->identity_.source = HttpAuth::IDENT_SRC_PATH_LOOKUP;
  self->identity_.invalid = false;
  self->identity_.credentials = entry->credentials();
  self->handler_.swap(handler_preemptive);
  self->handler_server_cert_.reset();
  return true;
}

int ConnectionAuthOriginalPolicy::HandleAuthChallenge(HttpAuthController* self,
    scoped_refptr<HttpResponseHeaders> headers,
    const SSLInfo& ssl_info,
    bool do_not_send_server_auth,
    bool establishing_tunnel,
    const NetLogWithSource& caller_net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  DCHECK(headers.get());
  DCHECK(self->auth_scheme_host_port_.IsValid());
  DCHECK(!self->auth_info_);

  self->has_handled_challenge_ = true;
  BindToCallingNetLog(self, caller_net_log);
  self->net_log_.BeginEventReferencingSource(NetLogEventType::AUTH_HANDLE_CHALLENGE,
                                       caller_net_log.source());

  // A connection-based handler derives channel bindings from the server
  // certificate at creation time. If the underlying connection has been
  // replaced with one that presents a different certificate, that state is
  // no longer valid for the current connection, so drop the handler and
  // start the authentication protocol over.
  InvalidateIfServerCertChanged(self, ssl_info.cert.get());

  // Give the existing auth handler first try at the authentication headers.
  // This will also evict the entry in the HttpAuthCache if the previous
  // challenge appeared to be rejected, or is using a stale nonce in the Digest
  // case.
  if (HaveAuth(self)) {
    std::string challenge_used;
    HttpAuth::AuthorizationResult result = ConnectionAuthOriginalFactoryPolicy::HandleChallengeResponse(
        self->handler_.get(), *headers, self->target_, self->disabled_schemes_, &challenge_used);
    switch (result) {
      case HttpAuth::AUTHORIZATION_RESULT_ACCEPT:
        break;
      case HttpAuth::AUTHORIZATION_RESULT_INVALID:
        InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS);
        break;
      case HttpAuth::AUTHORIZATION_RESULT_REJECT:
        HistogramAuthEvent(self, AUTH_EVENT_REJECT);
        InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS);
        break;
      case HttpAuth::AUTHORIZATION_RESULT_STALE:
        if (ConnectionAuthOriginalCache::UpdateStaleChallenge(self->http_auth_cache_,
                self->auth_scheme_host_port_, self->target_, self->handler_->realm(),
                self->handler_->auth_scheme(), self->network_anonymization_key_,
                challenge_used)) {
          InvalidateCurrentHandler(self, INVALIDATE_HANDLER);
        } else {
          // It's possible that a server could incorrectly issue a stale
          // response when the entry is not in the cache. Just evict the
          // current value from the cache.
          InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS);
        }
        break;
      case HttpAuth::AUTHORIZATION_RESULT_DIFFERENT_REALM:
        // If the server changes the authentication realm in a
        // subsequent challenge, invalidate cached credentials for the
        // previous realm.  If the server rejects a preemptive
        // authorization and requests credentials for a different
        // realm, we keep the cached credentials.
        InvalidateCurrentHandler(self,
            (self->identity_.source == HttpAuth::IDENT_SRC_PATH_LOOKUP) ?
            INVALIDATE_HANDLER :
            INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS);
        break;
      default:
        NOTREACHED();
    }
  }

  self->identity_.invalid = true;
  bool can_send_auth = (self->target_ != HttpAuth::AUTH_SERVER ||
                        !do_not_send_server_auth);

  do {
    if (!self->handler_.get() && can_send_auth) {
      // Find the best authentication challenge that we support.
      ConnectionAuthOriginalFactoryPolicy::ChooseBestChallenge(
          self->http_auth_handler_factory_, *headers, ssl_info,
          self->network_anonymization_key_, self->target_, self->auth_scheme_host_port_,
          self->disabled_schemes_, self->net_log_, self->host_resolver_, &self->handler_);
      if (self->handler_.get()) {
        self->handler_server_cert_ = ssl_info.cert;
        HistogramAuthEvent(self, AUTH_EVENT_START);
      }
    }

    if (!self->handler_.get()) {
      if (establishing_tunnel) {
        // We are establishing a tunnel, we can't show the error page because an
        // active network attacker could control its contents.  Instead, we just
        // fail to establish the tunnel.
        DCHECK_EQ(self->target_, HttpAuth::AUTH_PROXY);
        ConnectionOriginalLog::EndEventWithNetErrorCode(self->net_log_,
            NetLogEventType::AUTH_HANDLE_CHALLENGE, ERR_PROXY_AUTH_UNSUPPORTED);
        return ERR_PROXY_AUTH_UNSUPPORTED;
      }
      // We found no supported challenge -- let the transaction continue so we
      // end up displaying the error page.
      self->net_log_.EndEvent(NetLogEventType::AUTH_HANDLE_CHALLENGE);
      return OK;
    }

    if (self->handler_->NeedsIdentity()) {
      // Pick a new auth identity to try, by looking to the URL and auth cache.
      // If an identity to try is found, it is saved to identity_.
      SelectNextAuthIdentityToTry(self);
    } else {
      // Proceed with the existing identity or a null identity.
      self->identity_.invalid = false;
    }

    // From this point on, we are restartable.

    if (self->identity_.invalid) {
      // We have exhausted all identity possibilities.
      if (!self->handler_->AllowsExplicitCredentials()) {
        // If the handler doesn't accept explicit credentials, then we need to
        // choose a different auth scheme.
        HistogramAuthEvent(self, AUTH_EVENT_REJECT);
        InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_DISABLE_SCHEME);
      } else {
        // Pass the challenge information back to the client.
        PopulateAuthChallenge(self);
      }
    }

    // If we get here and we don't have a handler_, that's because we
    // invalidated it due to not having any viable identities to use with it. Go
    // back and try again.
    // TODO(asanka): Instead we should create a priority list of
    //     <handler,identity> and iterate through that.
  } while(!self->handler_.get());
  self->net_log_.EndEvent(NetLogEventType::AUTH_HANDLE_CHALLENGE);
  return OK;
}

void ConnectionAuthOriginalPolicy::ResetAuth(HttpAuthController* self,
    const AuthCredentials& credentials) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  DCHECK(self->identity_.invalid || credentials.Empty());

  if (self->identity_.invalid) {
    // Update the credentials.
    self->identity_.source = HttpAuth::IDENT_SRC_EXTERNAL;
    self->identity_.invalid = false;
    self->identity_.credentials = credentials;

    // auth_info_ is no longer necessary.
    self->auth_info_ = std::nullopt;
  }

  DCHECK(self->identity_.source != HttpAuth::IDENT_SRC_PATH_LOOKUP);

  // Add the auth entry to the cache before restarting. We don't know whether
  // the identity is valid yet, but if it is valid we want other transactions
  // to know about it. If an entry for (origin, handler->realm()) already
  // exists, we update it.
  //
  // If identity_.source is HttpAuth::IDENT_SRC_NONE or
  // HttpAuth::IDENT_SRC_DEFAULT_CREDENTIALS, identity_ contains no
  // identity because identity is not required yet or we're using default
  // credentials.
  //
  // TODO(wtc): For NTLM_SSPI, we add the same auth entry to the cache in
  // round 1 and round 2, which is redundant but correct.  It would be nice
  // to add an auth entry to the cache only once, preferrably in round 1.
  // See http://crbug.com/21015.
  switch (self->identity_.source) {
    case HttpAuth::IDENT_SRC_NONE:
    case HttpAuth::IDENT_SRC_DEFAULT_CREDENTIALS:
      break;
    default:
      ConnectionAuthOriginalCache::Add(self->http_auth_cache_, self->auth_scheme_host_port_, self->target_, self->handler_->realm(),
                            self->handler_->auth_scheme(), self->network_anonymization_key_,
                            self->handler_->challenge(), self->identity_.credentials,
                            self->auth_path_);
      break;
  }
}

bool ConnectionAuthOriginalPolicy::HaveAuthHandler(const HttpAuthController* self) {
  return self->handler_.get() != nullptr;
}

bool ConnectionAuthOriginalPolicy::HaveAuth(const HttpAuthController* self) {
  return self->handler_.get() && !self->identity_.invalid;
}

bool ConnectionAuthOriginalPolicy::NeedsHTTP11(const HttpAuthController* self) {
  return self->handler_ && self->handler_->is_connection_based();
}

void ConnectionAuthOriginalPolicy::InvalidateCurrentHandler(HttpAuthController* self,
    InvalidateHandlerAction action) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  DCHECK(self->handler_.get());

  switch (action) {
    case INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS:
      InvalidateRejectedAuthFromCache(self);
      break;

    case INVALIDATE_HANDLER_AND_DISABLE_SCHEME:
      DisableAuthScheme(self, self->handler_->auth_scheme());
      break;

    case INVALIDATE_HANDLER:
      PrepareIdentityForReuse(self);
      break;
  }

  self->handler_.reset();
  self->handler_server_cert_.reset();
  self->identity_ = HttpAuth::Identity();
}

void ConnectionAuthOriginalPolicy::InvalidateRejectedAuthFromCache(HttpAuthController* self) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  DCHECK(HaveAuth(self));

  // Clear the cache entry for the identity we just failed on.
  // Note: we require the credentials to match before invalidating
  // since the entry in the cache may be newer than what we used last time.
  ConnectionAuthOriginalCache::Remove(self->http_auth_cache_, self->auth_scheme_host_port_, self->target_, self->handler_->realm(),
                           self->handler_->auth_scheme(), self->network_anonymization_key_,
                           self->identity_.credentials);
}

void ConnectionAuthOriginalPolicy::PrepareIdentityForReuse(HttpAuthController* self) {
  if (self->identity_.invalid)
    return;

  switch (self->identity_.source) {
    case HttpAuth::IDENT_SRC_DEFAULT_CREDENTIALS:
      DCHECK(self->default_credentials_used_);
      self->default_credentials_used_ = false;
      break;

    case HttpAuth::IDENT_SRC_URL:
      DCHECK(self->embedded_identity_used_);
      self->embedded_identity_used_ = false;
      break;

    case HttpAuth::IDENT_SRC_NONE:
    case HttpAuth::IDENT_SRC_PATH_LOOKUP:
    case HttpAuth::IDENT_SRC_REALM_LOOKUP:
    case HttpAuth::IDENT_SRC_EXTERNAL:
      break;
  }
}

bool ConnectionAuthOriginalPolicy::SelectNextAuthIdentityToTry(HttpAuthController* self) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  DCHECK(self->handler_.get());
  DCHECK(self->identity_.invalid);

  // Try to use the username:password encoded into the URL first.
  if (self->target_ == HttpAuth::AUTH_SERVER && self->auth_url_.has_username() &&
      !self->embedded_identity_used_) {
    self->identity_.source = HttpAuth::IDENT_SRC_URL;
    self->identity_.invalid = false;
    // Extract the username:password from the URL.
    std::u16string username;
    std::u16string password;
    GetIdentityFromURL(self->auth_url_, &username, &password);
    self->identity_.credentials.Set(username, password);
    self->embedded_identity_used_ = true;
    // TODO(eroman): If the password is blank, should we also try combining
    // with a password from the cache?
    return true;
  }

  // Check the auth cache for a realm entry.
  HttpAuthCache::Entry* entry = ConnectionAuthOriginalCache::Lookup(self->http_auth_cache_,
      self->auth_scheme_host_port_, self->target_, self->handler_->realm(),
      self->handler_->auth_scheme(), self->network_anonymization_key_);

  if (entry) {
    self->identity_.source = HttpAuth::IDENT_SRC_REALM_LOOKUP;
    self->identity_.invalid = false;
    self->identity_.credentials = entry->credentials();
    return true;
  }

  // Use default credentials (single sign-on) if they're allowed and this is the
  // first attempt at using an identity. Do not allow multiple times as it will
  // infinite loop. We use default credentials after checking the auth cache so
  // that if single sign-on doesn't work, we won't try default credentials for
  // future transactions.
  if (!self->default_credentials_used_ && self->handler_->AllowsDefaultCredentials()) {
    self->identity_.source = HttpAuth::IDENT_SRC_DEFAULT_CREDENTIALS;
    self->identity_.invalid = false;
    self->default_credentials_used_ = true;
    return true;
  }

  return false;
}

void ConnectionAuthOriginalPolicy::PopulateAuthChallenge(HttpAuthController* self) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);

  // Populates response_.auth_challenge with the authentication challenge info.
  // This info is consumed by URLRequestHttpJob::GetAuthChallengeInfo().

  self->auth_info_ = AuthChallengeInfo();
  self->auth_info_->is_proxy = (self->target_ == HttpAuth::AUTH_PROXY);
  self->auth_info_->challenger = self->auth_scheme_host_port_;
  self->auth_info_->scheme = HttpAuth::SchemeToString(self->handler_->auth_scheme());
  self->auth_info_->realm = self->handler_->realm();
  self->auth_info_->path = self->auth_path_;
  self->auth_info_->challenge = self->handler_->challenge();
}

int ConnectionAuthOriginalPolicy::HandleGenerateTokenResult(HttpAuthController* self,
    int result) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  ConnectionOriginalLog::EndEventWithNetErrorCode(self->net_log_, NetLogEventType::AUTH_GENERATE_TOKEN,
                                    result);
  switch (result) {
    // Occurs if the credential handle is found to be invalid at the point it is
    // exercised (i.e. GenerateAuthToken stage). We are going to consider this
    // to be an error that invalidates the identity but not necessarily the
    // scheme. Doing so allows a different identity to be used with the same
    // scheme. See https://crbug.com/648366.
    case ERR_INVALID_HANDLE:

    // If the GenerateAuthToken call fails with this error, this means that the
    // handler can no longer be used. However, the authentication scheme is
    // considered still usable. This allows a scheme that attempted and failed
    // to use default credentials to recover and use explicit credentials.
    //
    // The current handler may be tied to external state that is no longer
    // valid, hence should be discarded. Since the scheme is still valid, a new
    // handler can be created for the current scheme.
    case ERR_INVALID_AUTH_CREDENTIALS:
      InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS);
      self->auth_token_.clear();
      return OK;

    // Occurs with GSSAPI, if the user has not already logged in.
    case ERR_MISSING_AUTH_CREDENTIALS:
      // Usually, GSSAPI doesn't allow explicit credentials and the scheme
      // cannot succeed anymore hence it gets disabled. However, on ChromeOS
      // it's not the case so we invalidate the current handler and can ask for
      // explicit credentials later. (See b/260522530).
      if (!self->handler_->AllowsExplicitCredentials()) {
        InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_DISABLE_SCHEME);
      } else {
        InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_CACHED_CREDENTIALS);
      }
      self->auth_token_.clear();
      return OK;

    // Can occur with GSSAPI or SSPI if the underlying library reports
    // a permanent error.
    case ERR_UNSUPPORTED_AUTH_SCHEME:

    // These two error codes represent failures we aren't handling.
    case ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS:
    case ERR_UNDOCUMENTED_SECURITY_LIBRARY_STATUS:

    // Can be returned by SSPI if the authenticating authority or
    // target is not known.
    case ERR_MISCONFIGURED_AUTH_ENVIRONMENT:

      // In these cases, disable the current scheme as it cannot
      // succeed.
      InvalidateCurrentHandler(self, INVALIDATE_HANDLER_AND_DISABLE_SCHEME);
      self->auth_token_.clear();
      return OK;

    default:
      return result;
  }
}

void ConnectionAuthOriginalPolicy::OnGenerateAuthTokenDone(HttpAuthController* self,
    int result) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  result = HandleGenerateTokenResult(self, result);
  if (!self->callback_.is_null()) {
    std::move(self->callback_).Run(result);
  }
}

void ConnectionAuthOriginalPolicy::HistogramAuthEvent(HttpAuthController* self,
    AuthEvent auth_event) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);

  HttpAuth::Scheme auth_scheme = self->handler_->auth_scheme();
  DCHECK(auth_scheme >= 0 && auth_scheme < HttpAuth::AUTH_SCHEME_MAX);

  // Record start and rejection events for authentication.
  //
  // The results map to:
  //   Basic Start: 0
  //   Basic Reject: 1
  //   Digest Start: 2
  //   Digest Reject: 3
  //   NTLM Start: 4
  //   NTLM Reject: 5
  //   Negotiate Start: 6
  //   Negotiate Reject: 7
  static constexpr int kEventBucketsEnd =
      int{HttpAuth::AUTH_SCHEME_MAX} * AUTH_EVENT_MAX;
  int event_bucket = int{auth_scheme} * AUTH_EVENT_MAX + auth_event;
  DCHECK(event_bucket >= 0 && event_bucket < kEventBucketsEnd);
  UMA_HISTOGRAM_ENUMERATION("Net.HttpAuthCount", event_bucket,
                            kEventBucketsEnd);

  // Record the target of the authentication.
  //
  // The results map to:
  //   Basic Proxy: 0
  //   Basic Secure Proxy: 1
  //   Basic Server: 2
  //   Basic Secure Server: 3
  //   Digest Proxy: 4
  //   Digest Secure Proxy: 5
  //   Digest Server: 6
  //   Digest Secure Server: 7
  //   NTLM Proxy: 8
  //   NTLM Secure Proxy: 9
  //   NTLM Server: 10
  //   NTLM Secure Server: 11
  //   Negotiate Proxy: 12
  //   Negotiate Secure Proxy: 13
  //   Negotiate Server: 14
  //   Negotiate Secure Server: 15
  if (auth_event != AUTH_EVENT_START) {
    return;
  }
  static constexpr int kTargetBucketsEnd =
      int{HttpAuth::AUTH_SCHEME_MAX} * AUTH_TARGET_MAX;
  AuthTarget auth_target = DetermineAuthTarget(self->handler_.get());
  int target_bucket = int{auth_scheme} * AUTH_TARGET_MAX + auth_target;
  DCHECK(target_bucket >= 0 && target_bucket < kTargetBucketsEnd);
  UMA_HISTOGRAM_ENUMERATION("Net.HttpAuthTarget", target_bucket,
                            kTargetBucketsEnd);
}

void ConnectionAuthOriginalPolicy::TakeAuthInfo(HttpAuthController* self,
    std::optional<AuthChallengeInfo>* other) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  self->auth_info_.swap(*other);
}

bool ConnectionAuthOriginalPolicy::IsAuthSchemeDisabled(const HttpAuthController* self,
    HttpAuth::Scheme scheme) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  return self->disabled_schemes_.contains(scheme);
}

void ConnectionAuthOriginalPolicy::DisableAuthScheme(HttpAuthController* self,
    HttpAuth::Scheme scheme) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  self->disabled_schemes_.insert(scheme);
}

void ConnectionAuthOriginalPolicy::DisableEmbeddedIdentity(HttpAuthController* self) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  self->embedded_identity_used_ = true;
}

void ConnectionAuthOriginalPolicy::OnConnectionClosed(HttpAuthController* self) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  InvalidateCurrentHandler(self, INVALIDATE_HANDLER);
}

void ConnectionAuthOriginalPolicy::InvalidateIfServerCertChanged(HttpAuthController* self,
    const X509Certificate* cert) {
  DCHECK_CALLED_ON_VALID_THREAD(self->thread_checker_);
  if (self->handler_ && self->handler_->is_connection_based() &&
      !ServerCertMatches(self->handler_server_cert_.get(), cert)) {
    InvalidateCurrentHandler(self, INVALIDATE_HANDLER);
  }
}
}  // namespace net
