// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/byte_size.h"
#include "base/pickle.h"
#include "net/base/proxy_chain.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/signed_certificate_timestamp_and_status.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info_rust_reference.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "net/test/cert_test_util.h"
#include "net/test/ct_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

class HttpResponseInfoOriginalTest : public testing::Test {
 protected:
  void SetUp() override {
    response_info_.headers = base::MakeRefCounted<HttpResponseHeaders>("");
  }

  void PickleAndRestore(
      const HttpResponseInfoOriginal& response_info,
      HttpResponseInfoOriginal* restored_response_info) const {
    std::unique_ptr<base::Pickle> pickle = response_info.MakePickle(
        /*skip_transient_headers=*/false, /*response_truncated=*/false);
    bool truncated = false;
    EXPECT_TRUE(restored_response_info->InitFromPickle(
        base::PickleIterator(*pickle), &truncated));
  }

  HttpResponseInfoOriginal response_info_;
};

TEST_F(HttpResponseInfoOriginalTest, UnusedSincePrefetchDefault) {
  EXPECT_FALSE(response_info_.unused_since_prefetch);
}

TEST_F(HttpResponseInfoOriginalTest, UnusedSincePrefetchCopy) {
  response_info_.unused_since_prefetch = true;
  HttpResponseInfoOriginal response_info_clone(response_info_);
  EXPECT_TRUE(response_info_clone.unused_since_prefetch);
}

TEST_F(HttpResponseInfoOriginalTest, UnusedSincePrefetchPersistFalse) {
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_FALSE(restored_response_info.unused_since_prefetch);
}

TEST_F(HttpResponseInfoOriginalTest, UnusedSincePrefetchPersistTrue) {
  response_info_.unused_since_prefetch = true;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_TRUE(restored_response_info.unused_since_prefetch);
}

TEST_F(HttpResponseInfoOriginalTest, ProxyChainDefault) {
  EXPECT_FALSE(response_info_.proxy_chain.IsValid());
  EXPECT_FALSE(response_info_.WasFetchedViaProxy());
}

TEST_F(HttpResponseInfoOriginalTest, ProxyChainCopy) {
  response_info_.proxy_chain =
      proxy_value_rust_reference::ProxyChain::FromSchemeHostAndPort(
          proxy_value_rust_reference::ProxyServer::SCHEME_HTTP, "foo", 80);
  HttpResponseInfoOriginal response_info_clone(response_info_);
  EXPECT_TRUE(response_info_clone.proxy_chain.IsValid());
  EXPECT_TRUE(response_info_clone.WasFetchedViaProxy());
}

TEST_F(HttpResponseInfoOriginalTest, ProxyChainPersistDirect) {
  response_info_.proxy_chain = proxy_value_rust_reference::ProxyChain::Direct();
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_TRUE(restored_response_info.proxy_chain.IsValid());
  EXPECT_FALSE(restored_response_info.WasFetchedViaProxy());
}

TEST_F(HttpResponseInfoOriginalTest, ProxyChainPersistProxy) {
  response_info_.proxy_chain =
      proxy_value_rust_reference::ProxyChain::FromSchemeHostAndPort(
          proxy_value_rust_reference::ProxyServer::SCHEME_HTTP, "foo", 80);
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_TRUE(restored_response_info.proxy_chain.IsValid());
  EXPECT_TRUE(restored_response_info.WasFetchedViaProxy());
}

TEST_F(HttpResponseInfoOriginalTest, PKPBypassPersistTrue) {
  response_info_.ssl_info.pkp_bypassed = true;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_TRUE(restored_response_info.ssl_info.pkp_bypassed);
}

TEST_F(HttpResponseInfoOriginalTest, PKPBypassPersistFalse) {
  response_info_.ssl_info.pkp_bypassed = false;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_FALSE(restored_response_info.ssl_info.pkp_bypassed);
}

TEST_F(HttpResponseInfoOriginalTest, AsyncRevalidationRequestedDefault) {
  EXPECT_FALSE(response_info_.async_revalidation_requested);
}

TEST_F(HttpResponseInfoOriginalTest, AsyncRevalidationRequestedCopy) {
  response_info_.async_revalidation_requested = true;
  HttpResponseInfoOriginal response_info_clone(response_info_);
  EXPECT_TRUE(response_info_clone.async_revalidation_requested);
}

TEST_F(HttpResponseInfoOriginalTest, AsyncRevalidationRequestedAssign) {
  response_info_.async_revalidation_requested = true;
  HttpResponseInfoOriginal response_info_clone;
  response_info_clone = response_info_;
  EXPECT_TRUE(response_info_clone.async_revalidation_requested);
}

TEST_F(HttpResponseInfoOriginalTest, AsyncRevalidationRequestedNotPersisted) {
  response_info_.async_revalidation_requested = true;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_FALSE(restored_response_info.async_revalidation_requested);
}

TEST_F(HttpResponseInfoOriginalTest, StaleRevalidationTimeoutDefault) {
  EXPECT_TRUE(response_info_.stale_revalidate_timeout.is_null());
}

TEST_F(HttpResponseInfoOriginalTest, StaleRevalidationTimeoutCopy) {
  base::Time test_time = base::Time::FromSecondsSinceUnixEpoch(1000);
  response_info_.stale_revalidate_timeout = test_time;
  HttpResponseInfoOriginal response_info_clone(response_info_);
  EXPECT_EQ(test_time, response_info_clone.stale_revalidate_timeout);
}

TEST_F(HttpResponseInfoOriginalTest, StaleRevalidationTimeoutRestoreValue) {
  base::Time test_time = base::Time::FromSecondsSinceUnixEpoch(1000);
  response_info_.stale_revalidate_timeout = test_time;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_EQ(test_time, restored_response_info.stale_revalidate_timeout);
}

TEST_F(HttpResponseInfoOriginalTest, StaleRevalidationTimeoutRestoreNoValue) {
  EXPECT_TRUE(response_info_.stale_revalidate_timeout.is_null());
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_TRUE(restored_response_info.stale_revalidate_timeout.is_null());
}

// Test that key_exchange_group is preserved for ECDHE ciphers.
TEST_F(HttpResponseInfoOriginalTest, KeyExchangeGroupECDHE) {
  response_info_.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem");
  SSLConnectionStatusSetVersion(SSL_CONNECTION_VERSION_TLS1_2,
                                &response_info_.ssl_info.connection_status);
  SSLConnectionStatusSetCipherSuite(
      0xcca8 /* TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256 */,
      &response_info_.ssl_info.connection_status);
  response_info_.ssl_info.key_exchange_group = 23;  // secp256r1
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_EQ(23, restored_response_info.ssl_info.key_exchange_group);
}

// Test that key_exchange_group is preserved for TLS 1.3.
TEST_F(HttpResponseInfoOriginalTest, KeyExchangeGroupTLS13) {
  response_info_.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem");
  SSLConnectionStatusSetVersion(SSL_CONNECTION_VERSION_TLS1_3,
                                &response_info_.ssl_info.connection_status);
  SSLConnectionStatusSetCipherSuite(0x1303 /* TLS_CHACHA20_POLY1305_SHA256 */,
                                    &response_info_.ssl_info.connection_status);
  response_info_.ssl_info.key_exchange_group = 23;  // secp256r1
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_EQ(23, restored_response_info.ssl_info.key_exchange_group);
}

// Test that key_exchange_group is discarded for non-ECDHE ciphers prior to TLS
// 1.3, to account for the historical key_exchange_info field. See
// https://crbug.com/639421.
TEST_F(HttpResponseInfoOriginalTest, LegacyKeyExchangeInfoDHE) {
  response_info_.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem");
  SSLConnectionStatusSetVersion(SSL_CONNECTION_VERSION_TLS1_2,
                                &response_info_.ssl_info.connection_status);
  SSLConnectionStatusSetCipherSuite(
      0x0093 /* TLS_DHE_RSA_WITH_AES_128_GCM_SHA256 */,
      &response_info_.ssl_info.connection_status);
  response_info_.ssl_info.key_exchange_group = 1024;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_EQ(0, restored_response_info.ssl_info.key_exchange_group);
}

// Test that key_exchange_group is discarded for unknown ciphers prior to TLS
// 1.3, to account for the historical key_exchange_info field. See
// https://crbug.com/639421.
TEST_F(HttpResponseInfoOriginalTest, LegacyKeyExchangeInfoUnknown) {
  response_info_.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem");
  SSLConnectionStatusSetVersion(SSL_CONNECTION_VERSION_TLS1_2,
                                &response_info_.ssl_info.connection_status);
  SSLConnectionStatusSetCipherSuite(0xffff,
                                    &response_info_.ssl_info.connection_status);
  response_info_.ssl_info.key_exchange_group = 1024;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_EQ(0, restored_response_info.ssl_info.key_exchange_group);
}

// Test that peer_signature_algorithm is preserved.
TEST_F(HttpResponseInfoOriginalTest, PeerSignatureAlgorithm) {
  response_info_.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem");
  response_info_.ssl_info.peer_signature_algorithm =
      0x0804;  // rsa_pss_rsae_sha256
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_EQ(0x0804, restored_response_info.ssl_info.peer_signature_algorithm);
}

// Test that encrypted_client_hello is preserved.
TEST_F(HttpResponseInfoOriginalTest, EncryptedClientHello) {
  response_info_.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem");
  {
    HttpResponseInfoOriginal restored_response_info;
    PickleAndRestore(response_info_, &restored_response_info);
    EXPECT_FALSE(restored_response_info.ssl_info.encrypted_client_hello);
  }

  response_info_.ssl_info.encrypted_client_hello = true;
  {
    HttpResponseInfoOriginal restored_response_info;
    PickleAndRestore(response_info_, &restored_response_info);
    EXPECT_TRUE(restored_response_info.ssl_info.encrypted_client_hello);
  }
}

// Tests that cache entries loaded over SSLv3 (no longer supported) are dropped.
TEST_F(HttpResponseInfoOriginalTest, FailsInitFromPickleWithSSLV3) {
  // A valid certificate is needed for ssl_info.is_valid() to be true.
  response_info_.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem");

  // Non-SSLv3 versions should succeed.
  SSLConnectionStatusSetVersion(SSL_CONNECTION_VERSION_TLS1_2,
                                &response_info_.ssl_info.connection_status);
  std::unique_ptr<base::Pickle> tls12_pickle = response_info_.MakePickle(
      /*skip_transient_headers=*/false, /*response_truncated=*/false);
  bool truncated = false;
  HttpResponseInfoOriginal restored_tls12_response_info;
  EXPECT_TRUE(restored_tls12_response_info.InitFromPickle(
      base::PickleIterator(*tls12_pickle), &truncated));
  EXPECT_EQ(SSL_CONNECTION_VERSION_TLS1_2,
            SSLConnectionStatusToVersion(
                restored_tls12_response_info.ssl_info.connection_status));
  EXPECT_FALSE(truncated);

  // SSLv3 should fail.
  SSLConnectionStatusSetVersion(SSL_CONNECTION_VERSION_SSL3,
                                &response_info_.ssl_info.connection_status);
  std::unique_ptr<base::Pickle> ssl3_pickle = response_info_.MakePickle(
      /*skip_transient_headers=*/false, /*response_truncated=*/false);
  HttpResponseInfoOriginal restored_ssl3_response_info;
  EXPECT_FALSE(restored_ssl3_response_info.InitFromPickle(
      base::PickleIterator(*ssl3_pickle), &truncated));
}

// Test that `dns_aliases` is preserved.
TEST_F(HttpResponseInfoOriginalTest, DnsAliases) {
  response_info_.dns_aliases = {"alias1", "alias2", "alias3"};
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_THAT(restored_response_info.dns_aliases,
              testing::ElementsAre("alias1", "alias2", "alias3"));
}

// Test that an empty `dns_aliases` is preserved and doesn't throw an error.
TEST_F(HttpResponseInfoOriginalTest, EmptyDnsAliases) {
  response_info_.dns_aliases = {};
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_TRUE(restored_response_info.dns_aliases.empty());
}

// Test that did_send_available_dictionary is NOT preserved .
TEST_F(HttpResponseInfoOriginalTest, DidSendAvailableDictionary) {
  response_info_.did_send_available_dictionary = true;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_FALSE(restored_response_info.did_send_available_dictionary);
}

// Test that did_use_shared_dictionary is NOT preserved .
TEST_F(HttpResponseInfoOriginalTest, DidUseSharedDictionary) {
  response_info_.did_use_shared_dictionary = true;
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_FALSE(restored_response_info.did_use_shared_dictionary);
}

TEST_F(HttpResponseInfoOriginalTest, EncodedBodySize) {
  response_info_.encoded_body_size = base::ByteSize(12345u);
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  ASSERT_TRUE(restored_response_info.encoded_body_size.has_value());
  EXPECT_EQ(12345u, restored_response_info.encoded_body_size->InBytes());
}

TEST_F(HttpResponseInfoOriginalTest, EmptyEncodedBodySize) {
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_FALSE(restored_response_info.encoded_body_size.has_value());
}

TEST_F(HttpResponseInfoOriginalTest, NegativeEncodedBodySize) {
  std::unique_ptr<base::Pickle> pickle =
      response_info_.MakePickleWithSignedBodySizeForTesting(
          /*skip_transient_headers=*/false, /*response_truncated=*/false, -1);
  bool truncated = false;
  HttpResponseInfoOriginal restored_response_info;
  EXPECT_TRUE(restored_response_info.InitFromPickle(
      base::PickleIterator(*pickle), &truncated));
  EXPECT_FALSE(restored_response_info.encoded_body_size.has_value());
}

// Rollback safety: older cache entries predating bit 1<<4 must deserialize
// as uncompressed. Persist-true is covered by
// HttpCacheTest.ZstdDecompressHappyPath.
TEST_F(HttpResponseInfoOriginalTest, ZstdUncompressedBodySizeDefault) {
  HttpResponseInfoOriginal restored_response_info;
  PickleAndRestore(response_info_, &restored_response_info);
  EXPECT_FALSE(restored_response_info.zstd_uncompressed_body_size.has_value());
}

// Malformed entries intentionally leave earlier fields updated. These are
// observable even when InitFromPickle returns false.
TEST_F(HttpResponseInfoOriginalTest,
       RequestTimeCommitsBeforeResponseTimeFailure) {
  base::Pickle pickle;
  pickle.WriteInt(3);
  pickle.WriteInt64(1234);
  response_info_.response_time = base::Time::Max();
  bool truncated = true;
  EXPECT_FALSE(
      response_info_.InitFromPickle(base::PickleIterator(pickle), &truncated));
  EXPECT_EQ(base::Time::FromInternalValue(1234), response_info_.request_time);
  EXPECT_TRUE(response_info_.was_cached);
  EXPECT_EQ(base::Time::Max(), response_info_.response_time);
  EXPECT_TRUE(truncated);
}

TEST_F(HttpResponseInfoOriginalTest, InvalidVersionDoesNotMutateDestination) {
  base::Pickle pickle;
  pickle.WriteInt(2);
  response_info_.request_time = base::Time::Max();
  bool truncated = true;
  EXPECT_FALSE(
      response_info_.InitFromPickle(base::PickleIterator(pickle), &truncated));
  EXPECT_EQ(base::Time::Max(), response_info_.request_time);
  EXPECT_FALSE(response_info_.was_cached);
  EXPECT_TRUE(truncated);
}

void WriteResponseInfoPrefix(base::Pickle& pickle,
                             uint32_t flags,
                             int extra_flags) {
  pickle.WriteInt(static_cast<int>(flags));
  if (flags & (1u << 31)) {
    pickle.WriteInt(extra_flags);
  }
  pickle.WriteInt64(1);
  pickle.WriteInt64(2);
  pickle.WriteString(
      std::string("HTTP/1.1 200 OK\0\0", sizeof("HTTP/1.1 200 OK\0\0") - 1));
  pickle.WriteString("");
  pickle.WriteUInt16(0);
}

TEST_F(HttpResponseInfoOriginalTest, InvalidSignatureCommitsPriorFlags) {
  base::Pickle pickle;
  WriteResponseInfoPrefix(pickle, 3u | (1u << 25) | (1u << 13), 0);
  pickle.WriteInt(65536);
  response_info_.ssl_info.peer_signature_algorithm = 777;
  response_info_.did_use_shared_dictionary = true;
  bool truncated = true;
  EXPECT_FALSE(
      response_info_.InitFromPickle(base::PickleIterator(pickle), &truncated));
  EXPECT_TRUE(response_info_.was_fetched_via_spdy);
  EXPECT_FALSE(truncated);
  EXPECT_EQ(777, response_info_.ssl_info.peer_signature_algorithm);
  EXPECT_TRUE(response_info_.did_use_shared_dictionary);
}

TEST_F(HttpResponseInfoOriginalTest, NegativeZstdCommitsEncodedBodyOnly) {
  base::Pickle pickle;
  WriteResponseInfoPrefix(pickle, 3u | (1u << 31), (1 << 3) | (1 << 4));
  pickle.WriteInt64(123);
  pickle.WriteInt64(-1);
  response_info_.zstd_uncompressed_body_size = 91;
  bool truncated = true;
  EXPECT_FALSE(
      response_info_.InitFromPickle(base::PickleIterator(pickle), &truncated));
  EXPECT_EQ(base::ByteSize(123u), response_info_.encoded_body_size);
  EXPECT_EQ(91, response_info_.zstd_uncompressed_body_size);
  EXPECT_FALSE(truncated);
}

TEST_F(HttpResponseInfoOriginalTest, EmptyLegacyFlagsPreserveAbsentMetadata) {
  base::Pickle pickle;
  WriteResponseInfoPrefix(pickle, 3, 0);
  response_info_.original_response_time = base::Time::Max();
  response_info_.did_send_available_dictionary = true;
  response_info_.did_use_shared_dictionary = true;
  response_info_.ssl_info.peer_signature_algorithm = 777;
  bool truncated = true;
  EXPECT_TRUE(
      response_info_.InitFromPickle(base::PickleIterator(pickle), &truncated));
  EXPECT_EQ(base::Time::Max(), response_info_.original_response_time);
  EXPECT_EQ(777, response_info_.ssl_info.peer_signature_algorithm);
  EXPECT_TRUE(response_info_.did_send_available_dictionary);
  EXPECT_FALSE(response_info_.did_use_shared_dictionary);
  EXPECT_FALSE(truncated);
}

}  // namespace

}  // namespace net
