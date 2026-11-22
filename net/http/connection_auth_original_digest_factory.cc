// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/connection_auth_original_digest_factory.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/net_errors.h"
#include "net/base/net_string_util.h"
#include "net/base/url_util.h"
#include "net/dns/host_resolver.h"
#include "net/http/connection_auth_original_parameters.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/http/http_auth.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_request_info.h"
#include "third_party/boringssl/src/include/openssl/digest.h"
#include "url/gurl.h"

namespace net {
namespace {

using auth_challenge_reference::frozen::QuoteParameter;

// Frozen handler policy from net/http/http_auth_handler_digest.{h,cc} at
// 478cd0e02a3a22af2ce2b64bd5c440ba87c5943c. Nonce generation, URL access,
// conversion, and the BoringSSL digest primitive are shared providers.
class OriginalDigestHandler final : public HttpAuthHandler {
 public:
  OriginalDigestHandler(
      int nonce_count,
      const HttpAuthHandlerDigest::NonceGenerator* nonce_generator);
  ~OriginalDigestHandler() override;

 private:
  using NonceGenerator = HttpAuthHandlerDigest::NonceGenerator;
  // HttpAuthHandler
  bool Init(HttpAuthChallengeTokenizer* challenge,
            const SSLInfo& ssl_info,
            const NetworkAnonymizationKey& network_anonymization_key) override;
  int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                            const HttpRequestInfo* request,
                            CompletionOnceCallback callback,
                            std::string* auth_token) override;
  HttpAuth::AuthorizationResult HandleAnotherChallengeImpl(
      HttpAuthChallengeTokenizer* challenge) override;

  // Possible values for the "algorithm" property.
  enum class Algorithm {
    // No algorithm was specified. According to RFC 2617 this means
    // we should default to MD5.
    UNSPECIFIED,

    // Hashes are run for every request.
    MD5,

    // Hash is run only once during the first WWW-Authenticate handshake.
    // (SESS means session).
    MD5_SESS,

    // SHA256 variants of the above.
    SHA256,
    SHA256_SESS,
  };

  // Possible values for QualityOfProtection.
  // auth-int is not supported, see http://crbug.com/62890 for justification.
  enum QualityOfProtection {
    QOP_UNSPECIFIED,
    QOP_AUTH,
  };

  // Parse the challenge, saving the results into this instance.
  // Returns true on success.
  bool ParseChallenge(HttpAuthChallengeTokenizer* challenge);

  // Parse an individual property. Returns true on success.
  bool ParseChallengeProperty(std::string_view name, std::string_view value);

  // Convert enum value back to string.
  static std::string QopToString(QualityOfProtection qop);
  static std::string AlgorithmToString(Algorithm algorithm);

  // Extract the method and path of the request, as needed by
  // the 'A2' production. (path may be a hostname for proxy).
  void GetRequestMethodAndPath(const HttpRequestInfo* request,
                               std::string* method,
                               std::string* path) const;

  // Build up  the 'response' production.
  std::string AssembleResponseDigest(const std::string& method,
                                     const std::string& path,
                                     const AuthCredentials& credentials,
                                     const std::string& cnonce,
                                     const std::string& nc) const;

  // Build up  the value for (Authorization/Proxy-Authorization).
  std::string AssembleCredentials(const std::string& method,
                                  const std::string& path,
                                  const AuthCredentials& credentials,
                                  const std::string& cnonce,
                                  int nonce_count) const;

  // Information parsed from the challenge.
  std::string nonce_;
  std::string domain_;
  std::string opaque_;
  bool stale_ = false;
  Algorithm algorithm_ = Algorithm::UNSPECIFIED;
  QualityOfProtection qop_ = QOP_UNSPECIFIED;
  bool userhash_ = false;

  // The realm as initially encoded over-the-wire. This is used in the
  // challenge text, rather than |realm_| which has been converted to
  // UTF-8.
  std::string original_realm_;

  int nonce_count_;
  raw_ptr<const NonceGenerator> nonce_generator_;

  class DigestContext;
};

bool OriginalDigestHandler::Init(
    HttpAuthChallengeTokenizer* challenge,
    const SSLInfo& ssl_info,
    const NetworkAnonymizationKey& network_anonymization_key) {
  return ParseChallenge(challenge);
}

int OriginalDigestHandler::GenerateAuthTokenImpl(
    const AuthCredentials* credentials,
    const HttpRequestInfo* request,
    CompletionOnceCallback callback,
    std::string* auth_token) {
  // Generate a random client nonce.
  std::string cnonce = nonce_generator_->GenerateNonce();

  // Extract the request method and path -- the meaning of 'path' is overloaded
  // in certain cases, to be a hostname.
  std::string method;
  std::string path;
  GetRequestMethodAndPath(request, &method, &path);

  *auth_token =
      AssembleCredentials(method, path, *credentials, cnonce, nonce_count_);
  return OK;
}

HttpAuth::AuthorizationResult OriginalDigestHandler::HandleAnotherChallengeImpl(
    HttpAuthChallengeTokenizer* challenge) {
  // Even though Digest is not connection based, a "second round" is parsed
  // to differentiate between stale and rejected responses.
  // Note that the state of the current handler is not mutated - this way if
  // there is a rejection the realm hasn't changed.
  if (challenge->auth_scheme() != kDigestAuthScheme) {
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  }

  auth_challenge_reference::frozen::Parameters parameters(challenge->params());

  // Try to find the "stale" value, and also keep track of the realm
  // for the new challenge.
  std::string original_realm;
  while (parameters.GetNext()) {
    if (base::EqualsCaseInsensitiveASCII(parameters.name(), "stale")) {
      if (base::EqualsCaseInsensitiveASCII(parameters.value(), "true")) {
        return HttpAuth::AUTHORIZATION_RESULT_STALE;
      }
    } else if (base::EqualsCaseInsensitiveASCII(parameters.name(), "realm")) {
      // This has to be a copy, since value_piece() may point to an internal
      // buffer of `parameters`.
      original_realm = parameters.value();
    }
  }
  return (original_realm_ != original_realm)
             ? HttpAuth::AUTHORIZATION_RESULT_DIFFERENT_REALM
             : HttpAuth::AUTHORIZATION_RESULT_REJECT;
}

OriginalDigestHandler::OriginalDigestHandler(
    int nonce_count,
    const NonceGenerator* nonce_generator)
    : nonce_count_(nonce_count), nonce_generator_(nonce_generator) {
  DCHECK(nonce_generator_);
}

OriginalDigestHandler::~OriginalDigestHandler() = default;

// The digest challenge header looks like:
//   WWW-Authenticate: Digest
//     [realm="<realm-value>"]
//     nonce="<nonce-value>"
//     [domain="<list-of-URIs>"]
//     [opaque="<opaque-token-value>"]
//     [stale="<true-or-false>"]
//     [algorithm="<digest-algorithm>"]
//     [qop="<list-of-qop-values>"]
//     [<extension-directive>]
//
// Note that according to RFC 2617 (section 1.2) the realm is required.
// However we allow it to be omitted, in which case it will default to the
// empty string.
//
// This allowance is for better compatibility with webservers that fail to
// send the realm (See http://crbug.com/20984 for an instance where a
// webserver was not sending the realm with a BASIC challenge).
bool OriginalDigestHandler::ParseChallenge(
    HttpAuthChallengeTokenizer* challenge) {
  auth_scheme_ = HttpAuth::AUTH_SCHEME_DIGEST;
  score_ = 2;
  properties_ = ENCRYPTS_IDENTITY;

  // Initialize to defaults.
  stale_ = false;
  algorithm_ = Algorithm::UNSPECIFIED;
  qop_ = QOP_UNSPECIFIED;
  realm_ = original_realm_ = nonce_ = domain_ = opaque_ = std::string();

  // FAIL -- Couldn't match auth-scheme.
  if (challenge->auth_scheme() != kDigestAuthScheme) {
    return false;
  }

  auth_challenge_reference::frozen::Parameters parameters(challenge->params());

  // Loop through all the properties.
  while (parameters.GetNext()) {
    // FAIL -- couldn't parse a property.
    if (!ParseChallengeProperty(parameters.name(), parameters.value())) {
      return false;
    }
  }

  // Check if tokenizer failed.
  if (!parameters.valid()) {
    return false;
  }

  // Check that a minimum set of properties were provided.
  if (nonce_.empty()) {
    return false;
  }

  return true;
}

bool OriginalDigestHandler::ParseChallengeProperty(std::string_view name,
                                                   std::string_view value) {
  if (base::EqualsCaseInsensitiveASCII(name, "realm")) {
    std::string realm;
    if (!ConvertToUtf8AndNormalize(value, kCharsetLatin1, &realm)) {
      return false;
    }
    realm_ = realm;
    original_realm_ = std::string(value);
  } else if (base::EqualsCaseInsensitiveASCII(name, "nonce")) {
    nonce_ = std::string(value);
  } else if (base::EqualsCaseInsensitiveASCII(name, "domain")) {
    domain_ = std::string(value);
  } else if (base::EqualsCaseInsensitiveASCII(name, "opaque")) {
    opaque_ = std::string(value);
  } else if (base::EqualsCaseInsensitiveASCII(name, "stale")) {
    // Parse the stale boolean.
    stale_ = base::EqualsCaseInsensitiveASCII(value, "true");
  } else if (base::EqualsCaseInsensitiveASCII(name, "algorithm")) {
    // Parse the algorithm.
    if (base::EqualsCaseInsensitiveASCII(value, "md5")) {
      algorithm_ = Algorithm::MD5;
    } else if (base::EqualsCaseInsensitiveASCII(value, "md5-sess")) {
      algorithm_ = Algorithm::MD5_SESS;
    } else if (base::EqualsCaseInsensitiveASCII(value, "sha-256")) {
      algorithm_ = Algorithm::SHA256;
    } else if (base::EqualsCaseInsensitiveASCII(value, "sha-256-sess")) {
      algorithm_ = Algorithm::SHA256_SESS;
    } else {
      DVLOG(1) << "Unknown value of algorithm";
      return false;  // FAIL -- unsupported value of algorithm.
    }
  } else if (base::EqualsCaseInsensitiveASCII(name, "userhash")) {
    userhash_ = base::EqualsCaseInsensitiveASCII(value, "true");
  } else if (base::EqualsCaseInsensitiveASCII(name, "qop")) {
    // Parse the comma separated list of qops.
    // auth is the only supported qop, and all other values are ignored.
    auth_challenge_reference::frozen::ParameterValues qop_values(value);
    qop_ = QOP_UNSPECIFIED;
    while (qop_values.GetNext()) {
      if (base::EqualsCaseInsensitiveASCII(qop_values.value(), "auth")) {
        qop_ = QOP_AUTH;
        break;
      }
    }
  } else {
    DVLOG(1) << "Skipping unrecognized digest property";
    // TODO(eroman): perhaps we should fail instead of silently skipping?
  }

  return true;
}

// static
std::string OriginalDigestHandler::QopToString(QualityOfProtection qop) {
  switch (qop) {
    case QOP_UNSPECIFIED:
      return std::string();
    case QOP_AUTH:
      return "auth";
    default:
      NOTREACHED();
  }
}

// static
std::string OriginalDigestHandler::AlgorithmToString(Algorithm algorithm) {
  switch (algorithm) {
    case Algorithm::UNSPECIFIED:
      return std::string();
    case Algorithm::MD5:
      return "MD5";
    case Algorithm::MD5_SESS:
      return "MD5-sess";
    case Algorithm::SHA256:
      return "SHA-256";
    case Algorithm::SHA256_SESS:
      return "SHA-256-sess";
    default:
      NOTREACHED();
  }
}

void OriginalDigestHandler::GetRequestMethodAndPath(
    const HttpRequestInfo* request,
    std::string* method,
    std::string* path) const {
  DCHECK(request);

  const GURL& url = request->url;

  if (target_ == HttpAuth::AUTH_PROXY &&
      (url.SchemeIs("https") || url.SchemeIsWSOrWSS())) {
    *method = "CONNECT";
    *path = GetHostAndPort(url);
  } else {
    *method = request->method;
    *path = url.PathForRequest();
  }
}

class OriginalDigestHandler::DigestContext {
 public:
  explicit DigestContext(OriginalDigestHandler::Algorithm algo) {
    switch (algo) {
      case OriginalDigestHandler::Algorithm::MD5:
      case OriginalDigestHandler::Algorithm::MD5_SESS:
      case OriginalDigestHandler::Algorithm::UNSPECIFIED:
        CHECK(EVP_DigestInit(md_ctx_.get(), EVP_md5()));
        out_len_ = 16;
        break;
      case OriginalDigestHandler::Algorithm::SHA256:
      case OriginalDigestHandler::Algorithm::SHA256_SESS:
        CHECK(EVP_DigestInit(md_ctx_.get(), EVP_sha256()));
        out_len_ = 32;
        break;
    }
  }
  void Update(std::string_view s) {
    CHECK(EVP_DigestUpdate(md_ctx_.get(), s.data(), s.size()));
  }
  void Update(std::initializer_list<std::string_view> sps) {
    for (const auto sp : sps) {
      Update(sp);
    }
  }
  std::string HexDigest() {
    uint8_t md_value[EVP_MAX_MD_SIZE] = {};
    unsigned int md_len = sizeof(md_value);
    CHECK(EVP_DigestFinal_ex(md_ctx_.get(), md_value, &md_len));
    return base::HexEncodeLower(base::span(md_value).first(out_len_));
  }

 private:
  bssl::ScopedEVP_MD_CTX md_ctx_;
  size_t out_len_;
};

std::string OriginalDigestHandler::AssembleResponseDigest(
    const std::string& method,
    const std::string& path,
    const AuthCredentials& credentials,
    const std::string& cnonce,
    const std::string& nc) const {
  // ha1 = H(A1)
  DigestContext ha1_ctx(algorithm_);
  ha1_ctx.Update({base::UTF16ToUTF8(credentials.username()), ":",
                  original_realm_, ":",
                  base::UTF16ToUTF8(credentials.password())});
  std::string ha1 = ha1_ctx.HexDigest();

  if (algorithm_ == OriginalDigestHandler::Algorithm::MD5_SESS ||
      algorithm_ == OriginalDigestHandler::Algorithm::SHA256_SESS) {
    DigestContext sess_ctx(algorithm_);
    sess_ctx.Update({ha1, ":", nonce_, ":", cnonce});
    ha1 = sess_ctx.HexDigest();
  }

  // ha2 = H(A2)
  // TODO(eroman): need to add H(req-entity-body) for qop=auth-int.
  DigestContext ha2_ctx(algorithm_);
  ha2_ctx.Update({method, ":", path});
  const std::string ha2 = ha2_ctx.HexDigest();

  DigestContext resp_ctx(algorithm_);
  resp_ctx.Update({ha1, ":", nonce_, ":"});

  if (qop_ != OriginalDigestHandler::QOP_UNSPECIFIED) {
    resp_ctx.Update({nc, ":", cnonce, ":", QopToString(qop_), ":"});
  }

  resp_ctx.Update(ha2);

  return resp_ctx.HexDigest();
}

std::string OriginalDigestHandler::AssembleCredentials(
    const std::string& method,
    const std::string& path,
    const AuthCredentials& credentials,
    const std::string& cnonce,
    int nonce_count) const {
  // the nonce-count is an 8 digit hex string.
  std::string nc = base::StringPrintf("%08x", nonce_count);

  // TODO(eroman): is this the right encoding?
  std::string username = base::UTF16ToUTF8(credentials.username());
  if (userhash_) {  // https://www.rfc-editor.org/rfc/rfc7616#section-3.4.4
    DigestContext uh_ctx(algorithm_);
    uh_ctx.Update({username, ":", realm_});
    username = uh_ctx.HexDigest();
  }

  std::string authorization = base::StrCat(
      {"Digest username=", QuoteParameter(username),
       ", realm=", QuoteParameter(original_realm_),
       ", nonce=", QuoteParameter(nonce_), ", uri=", QuoteParameter(path)});

  if (algorithm_ != Algorithm::UNSPECIFIED) {
    base::StrAppend(&authorization,
                    {", algorithm=", AlgorithmToString(algorithm_)});
  }
  std::string response =
      AssembleResponseDigest(method, path, credentials, cnonce, nc);
  // No need to call QuoteParameter() as the response digest cannot contain
  // any characters needing to be escaped.
  base::StrAppend(&authorization, {", response=\"", response, "\""});

  if (!opaque_.empty()) {
    base::StrAppend(&authorization, {", opaque=", QuoteParameter(opaque_)});
  }
  if (qop_ != QOP_UNSPECIFIED) {
    // TODO(eroman): Supposedly IIS server requires quotes surrounding qop.
    base::StrAppend(&authorization, {", qop=", QopToString(qop_), ", nc=", nc,
                                     ", cnonce=", QuoteParameter(cnonce)});
  }
  if (userhash_) {
    base::StrAppend(&authorization, {", userhash=true"});
  }

  return authorization;
}

}  // namespace

ConnectionAuthOriginalDigestFactory::ConnectionAuthOriginalDigestFactory()
    : nonce_generator_(
          std::make_unique<HttpAuthHandlerDigest::DynamicNonceGenerator>()) {}
ConnectionAuthOriginalDigestFactory::~ConnectionAuthOriginalDigestFactory() = default;

int ConnectionAuthOriginalDigestFactory::CreateAuthHandler(
    HttpAuthChallengeTokenizer* challenge, HttpAuth::Target target,
    const SSLInfo& ssl_info, const NetworkAnonymizationKey& key,
    const url::SchemeHostPort& origin, CreateReason, int nonce_count,
    const NetLogWithSource& log, HostResolver*,
    std::unique_ptr<HttpAuthHandler>* handler) {
  auto candidate = std::make_unique<OriginalDigestHandler>(
      nonce_count, nonce_generator_.get());
  if (!ConnectionAuthOriginalHandlerPolicy::InitFromChallenge(
          candidate.get(), challenge, target, ssl_info, key, origin, log))
    return ERR_INVALID_RESPONSE;
  *handler = std::move(candidate);
  return OK;
}

}  // namespace net
