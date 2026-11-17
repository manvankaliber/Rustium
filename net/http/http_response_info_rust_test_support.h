// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_RESPONSE_INFO_RUST_TEST_SUPPORT_H_
#define NET_HTTP_HTTP_RESPONSE_INFO_RUST_TEST_SUPPORT_H_
// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/pickle.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_response_info_rust_reference.h"
#include "net/ssl/ssl_cert_request_info.h"
using net::HttpResponseInfo;
using net::HttpResponseInfoOriginal;
static_assert(sizeof(HttpResponseInfo) == sizeof(HttpResponseInfoOriginal));
static uint64_t rng = 0x179a01dbfae937;
inline uint64_t Next() {
  rng ^= rng << 13;
  rng ^= rng >> 7;
  return rng ^= rng << 17;
}
static net::X509Certificate* certificate = nullptr;
// Frozen public fixture net/data/ssl/certificates/ok_cert.pem; parse once
// before validation or timing. No filesystem or network dependency.
inline scoped_refptr<net::X509Certificate> LoadCertificate() {
  constexpr char pem[] = R"CERT(-----BEGIN PRIVATE KEY-----
MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQDgU/TzmMEUMwLI
pG3+qir3lD2mbwDfO95Mn6PqB9Ss5VsN0azg7fnFmB01LeWzSZcUhUQP3EzSZwiI
AaXYp+uT0Wqh91HnhH5SKn28bw7Y27amPt7c9aRolkQRhQLtRxLfuGBxlXtih2h6
RFYJ1bTI8fbJRpKLaOiD1dWGcSPDgB6/bAHH0qS8QG3g48AuMHi9rd0lZtP1BwdW
187icsUlfQzhp28AqNqrS1RDCWSktlI4L7fMAd0cAycDR7/f5jew7RjcUQvUdSLf
UHs86zc5HJtvCHunBayMQ/fx2lEGs4JFPsiBc56wpc92lq+BLKwBKkpYSx2+/x+F
wife8XgLAgMBAAECggEAO0gUmHdKtvLQDoPdiYogtrKXJC97dILWuTsKzyLoohQu
XtWFMR/SfNQ5C7+oTxvocATTurlGF+ggigidckbV64dQ/aJlI6CQ3VfbSHu02bwe
ZYqBzLShkP382QBkiJ3asAKCgiG1rJEKHB2I+ypdjyjaRdB/k5XStFxDBDdL8zKe
kL1hytf1ALxYBQJ3TXcoIqlpzz6v+JnZTAuu4vrySfffyFTrytIwf/KGlBACBqXe
oV+DK586PiyC0m1Hhy1rLTi2/IT4t4j3KO+c/OnpWkMEg1G/Ojy1zPMsMHr1VO9t
UWxhEIYOLQVUOT+2ltO4bRJcgEPHSxlIY23qalJgqQKBgQD9oZwHPhLZg439Zr9O
Zgl5fFkPe4SxqLY+ggE5O81+3RgL8GyAuy9oDtUGcw8L9tpKcBflqH4jEvUvT1/m
8BGKIQteqcKbY0C+KfPvq77R4k2KqmxpKF5f4BS/9O7cgzJZwhSFKDLtcKtDIPUK
3eoERuPB4/cmhQyb+/TKofZpfwKBgQDibEm1+nYj3tIXDxK3hZHjPz0s9vYV84py
zg3HjuEmdacnhNPQQQpNG83RMjEnnBvHWH2vxRWyD+57r/FWeA60aUJo5WIP0rj8
z6GqFT9IWaBrJtBU1YoxuyTH64yih6ljvm2skIEGlQYS+GBL9K/Tx4mh4lAKVzD5
GXVqtyM/dQKBgFD0Ik8VewK+QLXe87TcUK3cCLkuXZ4vEWxGJonUErUpcKFu7dLw
7CK0iT3zv5u8ANS9joMZEpmzVVryZNPbUF3cSjq+yIS8W0/XKCsZkGCBcOqPlubB
oc3MQhM65HqxzYJkthQCTq8GxUM547zCNA2FavDaCGrdELdA5lM++t2VAoGAMLem
AH68bqlhwM5gc5ZMtn2D1ynn9v8oudz2AAsRDKph5dHhlTx5T+/8j9dh1ijznSfA
G1KngWGGKZzIq5c3ar//Jvy75bWsUdEG8saRkCqgpo16Y9ZyXpLqrg1TfCD+ZFSz
2l5ZNKZZ4TkJ1y31qvaS+X7tQ9xQ0DgXGHgBIIUCgYABBBddUXpOyG54HvFHIgxS
esBBkpM0uy3vOkyfMown2GCjKZaXS5kEmRwU9WQQKLPf2ieALgYEf87YGWmPNZNF
tG5DoNhTMsrYORB11tuuBR1e6lrG/aoOkP/4udWC8d4Yp9wnkEu0l6X/kMfq2lb1
67bBVmHqKJ0t8rytHzkP0Q==
-----END PRIVATE KEY-----
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            af:64:47:fb:c3:1d:4e:66:e4:b3:23:2d:75:d0:f8:21
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: C=US, ST=California, L=Mountain View, O=Test CA, CN=Test Root CA
        Validity
            Not Before: Oct 12 21:58:57 2026 GMT
            Not After : Oct 11 21:58:57 2028 GMT
        Subject: C=US, ST=California, L=Mountain View, O=Test CA, CN=127.0.0.1
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (2048 bit)
                Modulus:
                    00:e0:53:f4:f3:98:c1:14:33:02:c8:a4:6d:fe:aa:
                    2a:f7:94:3d:a6:6f:00:df:3b:de:4c:9f:a3:ea:07:
                    d4:ac:e5:5b:0d:d1:ac:e0:ed:f9:c5:98:1d:35:2d:
                    e5:b3:49:97:14:85:44:0f:dc:4c:d2:67:08:88:01:
                    a5:d8:a7:eb:93:d1:6a:a1:f7:51:e7:84:7e:52:2a:
                    7d:bc:6f:0e:d8:db:b6:a6:3e:de:dc:f5:a4:68:96:
                    44:11:85:02:ed:47:12:df:b8:60:71:95:7b:62:87:
                    68:7a:44:56:09:d5:b4:c8:f1:f6:c9:46:92:8b:68:
                    e8:83:d5:d5:86:71:23:c3:80:1e:bf:6c:01:c7:d2:
                    a4:bc:40:6d:e0:e3:c0:2e:30:78:bd:ad:dd:25:66:
                    d3:f5:07:07:56:d7:ce:e2:72:c5:25:7d:0c:e1:a7:
                    6f:00:a8:da:ab:4b:54:43:09:64:a4:b6:52:38:2f:
                    b7:cc:01:dd:1c:03:27:03:47:bf:df:e6:37:b0:ed:
                    18:dc:51:0b:d4:75:22:df:50:7b:3c:eb:37:39:1c:
                    9b:6f:08:7b:a7:05:ac:8c:43:f7:f1:da:51:06:b3:
                    82:45:3e:c8:81:73:9e:b0:a5:cf:76:96:af:81:2c:
                    ac:01:2a:4a:58:4b:1d:be:ff:1f:85:c2:27:de:f1:
                    78:0b
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints: critical
                CA:FALSE
            X509v3 Subject Key Identifier: 
                E2:E0:A4:73:95:9B:E9:6E:FD:CE:29:C4:6F:07:81:0B:96:BD:47:BA
            X509v3 Authority Key Identifier: 
                9B:26:0B:8A:98:A9:BB:1D:B9:1F:1C:E3:1A:40:33:ED:8E:17:88:AB
            X509v3 Extended Key Usage: 
                TLS Web Server Authentication, TLS Web Client Authentication
            X509v3 Subject Alternative Name: 
                IP Address:127.0.0.1
    Signature Algorithm: sha256WithRSAEncryption
    Signature Value:
        3d:91:aa:75:e2:2f:04:88:af:4e:7d:c0:d0:41:ed:9e:e4:d7:
        f7:52:23:f1:73:33:0e:ff:93:03:27:14:4f:f7:5a:43:b1:4b:
        54:f5:ec:76:70:d0:13:2f:c8:ae:e3:97:fe:a2:0d:18:4a:25:
        0b:ba:e7:c8:19:6f:5d:cc:5c:14:b4:2c:da:c6:41:d2:ed:cb:
        24:b2:a2:a8:ea:18:72:86:3e:f9:4e:0a:b8:bb:5a:a5:8a:7d:
        4d:84:ba:55:6f:42:c9:6d:0e:d7:4b:83:69:dd:a3:eb:24:d7:
        d6:92:a1:f0:b5:b6:0e:3d:cb:3a:e3:0b:d8:c3:a5:47:98:8b:
        2c:49:92:0c:a5:fa:88:33:59:25:9d:49:96:91:84:df:54:fb:
        f4:44:9f:53:a7:aa:0b:8d:ff:d4:4e:89:14:b6:ea:e6:bb:df:
        42:60:98:19:22:08:08:f2:fb:35:81:11:7b:cf:6c:b3:a2:26:
        0f:06:62:5f:a0:a2:f7:5f:bf:a7:76:33:0b:ee:6d:fd:99:13:
        59:29:f9:1c:50:69:63:fb:5c:b1:b6:7b:7d:34:4b:fc:d7:f7:
        0f:ae:ea:36:05:68:85:09:e2:0a:e8:86:b9:db:83:d6:1b:2d:
        17:0d:fc:a1:c2:69:f1:9a:4f:ac:fe:66:5f:3b:86:cf:12:d3:
        fe:09:74:0d
-----BEGIN CERTIFICATE-----
MIIDzzCCAregAwIBAgIRAK9kR/vDHU5m5LMjLXXQ+CEwDQYJKoZIhvcNAQELBQAw
YzELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDU1v
dW50YWluIFZpZXcxEDAOBgNVBAoMB1Rlc3QgQ0ExFTATBgNVBAMMDFRlc3QgUm9v
dCBDQTAeFw0yNjEwMTIyMTU4NTdaFw0yODEwMTEyMTU4NTdaMGAxCzAJBgNVBAYT
AlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1Nb3VudGFpbiBWaWV3
MRAwDgYDVQQKDAdUZXN0IENBMRIwEAYDVQQDDAkxMjcuMC4wLjEwggEiMA0GCSqG
SIb3DQEBAQUAA4IBDwAwggEKAoIBAQDgU/TzmMEUMwLIpG3+qir3lD2mbwDfO95M
n6PqB9Ss5VsN0azg7fnFmB01LeWzSZcUhUQP3EzSZwiIAaXYp+uT0Wqh91HnhH5S
Kn28bw7Y27amPt7c9aRolkQRhQLtRxLfuGBxlXtih2h6RFYJ1bTI8fbJRpKLaOiD
1dWGcSPDgB6/bAHH0qS8QG3g48AuMHi9rd0lZtP1BwdW187icsUlfQzhp28AqNqr
S1RDCWSktlI4L7fMAd0cAycDR7/f5jew7RjcUQvUdSLfUHs86zc5HJtvCHunBayM
Q/fx2lEGs4JFPsiBc56wpc92lq+BLKwBKkpYSx2+/x+Fwife8XgLAgMBAAGjgYAw
fjAMBgNVHRMBAf8EAjAAMB0GA1UdDgQWBBTi4KRzlZvpbv3OKcRvB4ELlr1HujAf
BgNVHSMEGDAWgBSbJguKmKm7HbkfHOMaQDPtjheIqzAdBgNVHSUEFjAUBggrBgEF
BQcDAQYIKwYBBQUHAwIwDwYDVR0RBAgwBocEfwAAATANBgkqhkiG9w0BAQsFAAOC
AQEAPZGqdeIvBIivTn3A0EHtnuTX91Ij8XMzDv+TAycUT/daQ7FLVPXsdnDQEy/I
ruOX/qINGEolC7rnyBlvXcxcFLQs2sZB0u3LJLKiqOoYcoY++U4KuLtapYp9TYS6
VW9CyW0O10uDad2j6yTX1pKh8LW2Dj3LOuML2MOlR5iLLEmSDKX6iDNZJZ1JlpGE
31T79ESfU6eqC43/1E6JFLbq5rvfQmCYGSIICPL7NYERe89ss6ImDwZiX6Ci91+/
p3YzC+5t/ZkTWSn5HFBpY/tcsbZ7fTRL/Nf3D67qNgVohQniCuiGuduD1hstFw38
ocJp8ZpPrP5mXzuGzxLT/gl0DQ==
-----END CERTIFICATE-----
)CERT";
  auto certs = net::X509Certificate::CreateCertificateListFromBytes(
      base::as_byte_span(pem).first(sizeof(pem) - 1),
      net::X509Certificate::FORMAT_AUTO);
  CHECK(!certs.empty());
  return certs[0];
}

inline std::string Bytes(const base::Pickle& p) {
  return std::string(reinterpret_cast<const char*>(p.data()), p.size());
}
template <class T>
void Fill(T& s, uint64_t n) {
  s.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      std::string("HTTP/1.1 200 OK\0Set-Cookie: a=b\0Connection: "
                  "close\0Content-Type: text/plain\0\0",
                  sizeof("HTTP/1.1 200 OK\0Set-Cookie: a=b\0Connection: "
                         "close\0Content-Type: text/plain\0\0") -
                      1));
  s.was_cached = n & 1;
  s.network_accessed = n & 2;
  s.was_fetched_via_spdy = n & 4;
  s.was_alpn_negotiated = n & 8;
  s.did_use_server_http_auth = n & 16;
  s.unused_since_prefetch = n & 32;
  s.restricted_prefetch = n & 64;
  s.async_revalidation_requested = n & 128;
  s.cache_entry_status =
      static_cast<typename T::CacheEntryStatus>((n >> 8) % 8);
  if (n & 256) {
    using Chain = std::remove_cvref_t<decltype(s.proxy_chain)>;
    using Server = typename std::remove_cvref_t<
        decltype(s.proxy_chain.proxy_servers())>::value_type;
    s.proxy_chain = (n & 512) ? Chain::Direct()
                              : Chain::FromSchemeHostAndPort(
                                    Server::SCHEME_HTTP, "proxy.example", 8080);
  }
  s.stale_revalidate_timeout =
      base::Time::FromInternalValue((n & 512) ? static_cast<int64_t>(n) : 0);
  s.request_time = base::Time::FromInternalValue(static_cast<int64_t>(n));
  s.response_time = base::Time::FromInternalValue(static_cast<int64_t>(~n));
  s.original_response_time = base::Time::Max();
  s.remote_endpoint = net::IPEndPoint(net::IPAddress::IPv6Localhost(), 443);
  s.alpn_negotiated_protocol = std::string("h2\0x", sizeof("h2\0x") - 1);
  s.connection_info = static_cast<net::HttpConnectionInfo>((n >> 16) % 43);
  s.resolve_error_info = net::ResolveErrorInfo(-105, true);
  s.auth_challenge.emplace();
  s.auth_challenge->realm = "original";
  s.cert_request_info = base::MakeRefCounted<net::SSLCertRequestInfo>();
  if (n & 1024) {
    s.ssl_info.cert = certificate;
  }
  s.ssl_info.unverified_cert = certificate;
  s.ssl_info.cert_status = static_cast<uint32_t>(n);
  s.ssl_info.connection_status = static_cast<int>((n & 4095) | (5 << 20));
  s.ssl_info.key_exchange_group = static_cast<uint16_t>(n);
  s.ssl_info.peer_signature_algorithm = static_cast<uint16_t>(n >> 32);
  s.ssl_info.pkp_bypassed = n & 2048;
  s.ssl_info.encrypted_client_hello = n & 4096;
  s.ssl_info.client_cert_sent = true;
  s.ssl_info.is_issued_by_known_root = n & 1;
  s.ssl_info.early_data_received = n & 2;
  s.ssl_info.early_data_accepted = n & 4;
  s.ssl_info.handshake_type = static_cast<net::SSLInfo::HandshakeType>(n % 3);
  s.ssl_info.is_fatal_cert_error = n & 8;
  s.ssl_info.server_padding_requested = n & 16;
  s.ssl_info.server_padding_received = n & 32;
  net::SHA256HashValue hash{};
  hash[0] = static_cast<uint8_t>(n);
  s.ssl_info.public_key_hashes = {hash};
  auto sct = base::MakeRefCounted<net::ct::SignedCertificateTimestamp>();
  sct->log_id = "original SCT";
  s.ssl_info.signed_certificate_timestamps.emplace_back(sct,
                                                        net::ct::SCT_STATUS_OK);

  s.did_send_available_dictionary = n & 8192;
  s.did_use_shared_dictionary = n & 16384;
  if (n & 32768) {
    s.encoded_body_size = base::ByteSize(n >> 1);
  }
  if (n & 65536) {
    s.zstd_uncompressed_body_size = static_cast<int64_t>(n >> 1);
  }
  if (n & 131072) {
    s.dns_aliases = {"a", "b", std::string("n\0x", sizeof("n\0x") - 1)};
  }
  if (n & 262144) {
    base::Pickle p;
    p.WriteInt((n >> 20) & 1);
    std::array<uint8_t, 32> digest{};
    p.WriteBytes(base::span(digest).first(((n >> 20) & 1) ? 32u : 16u));
    auto it = base::PickleIterator(p);
    CHECK(s.vary_data.InitFromPickle(&it));
  }
}
template <class A, class B>
void Equal(const A& a, const B& b) {
#define EQ(f) CHECK(a.f == b.f) << #f
  EQ(was_cached);
  CHECK(static_cast<int>(a.cache_entry_status) ==
        static_cast<int>(b.cache_entry_status));
  EQ(network_accessed);
  EQ(was_fetched_via_spdy);
  EQ(was_alpn_negotiated);
  CHECK(a.proxy_chain.IsValid() == b.proxy_chain.IsValid());
  CHECK(a.proxy_chain.opaque_data() == b.proxy_chain.opaque_data());
  if (a.proxy_chain.IsValid()) {
    CHECK(a.proxy_chain.length() == b.proxy_chain.length());
    for (size_t i = 0; i < a.proxy_chain.length(); ++i) {
      const auto& left = a.proxy_chain.proxy_servers()[i];
      const auto& right = b.proxy_chain.proxy_servers()[i];
      CHECK(static_cast<int>(left.scheme()) ==
            static_cast<int>(right.scheme()));
      CHECK(left.host_port_pair().host() == right.host_port_pair().host());
      CHECK(left.host_port_pair().port() == right.host_port_pair().port());
    }
  }
  EQ(did_use_server_http_auth);
  EQ(unused_since_prefetch);
  EQ(restricted_prefetch);
  EQ(async_revalidation_requested);
  EQ(stale_revalidate_timeout);
  EQ(remote_endpoint);
  EQ(alpn_negotiated_protocol);
  EQ(alternate_protocol_usage);
  EQ(connection_info);
  EQ(request_time);
  EQ(response_time);
  EQ(original_response_time);
  EQ(resolve_error_info);
  EQ(dns_aliases);
  EQ(did_send_available_dictionary);
  EQ(did_use_shared_dictionary);
  EQ(encoded_body_size);
  EQ(zstd_uncompressed_body_size);
  CHECK(a.auth_challenge.has_value() == b.auth_challenge.has_value());
  if (a.auth_challenge) {
    CHECK(a.auth_challenge->realm == b.auth_challenge->realm);
  }
  CHECK(bool(a.cert_request_info) == bool(b.cert_request_info));
  CHECK(bool(a.headers) == bool(b.headers));
  if (a.headers) {
    CHECK(a.headers->raw_headers() == b.headers->raw_headers());
  }
  CHECK(bool(a.ssl_info.cert) == bool(b.ssl_info.cert));
  if (a.ssl_info.cert) {
    CHECK(a.ssl_info.cert->EqualsIncludingChain(b.ssl_info.cert.get()));
  }
  EQ(ssl_info.unverified_cert);
  EQ(ssl_info.cert_status);
  EQ(ssl_info.connection_status);
  EQ(ssl_info.key_exchange_group);
  EQ(ssl_info.peer_signature_algorithm);
  EQ(ssl_info.pkp_bypassed);
  EQ(ssl_info.encrypted_client_hello);
  EQ(ssl_info.client_cert_sent);
  EQ(ssl_info.is_issued_by_known_root);
  EQ(ssl_info.early_data_received);
  EQ(ssl_info.early_data_accepted);
  EQ(ssl_info.handshake_type);
  EQ(ssl_info.public_key_hashes);
  CHECK(a.ssl_info.signed_certificate_timestamps.size() ==
        b.ssl_info.signed_certificate_timestamps.size());
  for (size_t i = 0; i < a.ssl_info.signed_certificate_timestamps.size(); ++i) {
    base::Pickle left_sct, right_sct;
    a.ssl_info.signed_certificate_timestamps[i].sct->Persist(&left_sct);
    b.ssl_info.signed_certificate_timestamps[i].sct->Persist(&right_sct);
    CHECK(Bytes(left_sct) == Bytes(right_sct));
    CHECK(a.ssl_info.signed_certificate_timestamps[i].status ==
          b.ssl_info.signed_certificate_timestamps[i].status);
  }
#if BUILDFLAG(CHROME_ROOT_STORE_SUPPORTED)
  EQ(ssl_info.crs_root_id);
#endif
  EQ(ssl_info.ct_policy_compliance);
  EQ(ssl_info.is_fatal_cert_error);
  EQ(ssl_info.server_padding_requested);
  EQ(ssl_info.server_padding_received);
  CHECK(a.vary_data.is_valid() == b.vary_data.is_valid());
  CHECK(static_cast<int>(a.vary_data.hash_type()) ==
        static_cast<int>(b.vary_data.hash_type()));
  if (a.vary_data.is_valid()) {
    base::Pickle p, q;
    a.vary_data.Persist(&p);
    b.vary_data.Persist(&q);
    CHECK(Bytes(p) == Bytes(q));
  }
  CHECK(a.DidUseQuic() == b.DidUseQuic());
  CHECK(a.WasFetchedViaProxy() == b.WasFetchedViaProxy());
#undef EQ
}
// All legacy fields, in the frozen original wire order. Counts stay bounded
// because the C++ reference speculatively reserves untrusted DNS counts.
inline base::Pickle Legacy(uint32_t flags, uint32_t extra, uint64_t n) {
  base::Pickle p;
  p.WriteInt(static_cast<int>(flags));
  if (flags & (1u << 31)) {
    p.WriteInt(static_cast<int>(extra));
  }
  p.WriteInt64(static_cast<int64_t>(n));
  p.WriteInt64(static_cast<int64_t>(~n));
  if (extra & 4) {
    p.WriteInt64((n & 1) ? INT64_MAX : INT64_MIN);
  }
  p.WriteString(std::string("HTTP/1.1 200 OK\0X-Test: yes\0\0",
                            sizeof("HTTP/1.1 200 OK\0X-Test: yes\0\0") - 1));
  if (flags & (1 << 8)) {
    certificate->Persist(&p);
  }
  if (flags & (1 << 10)) {
    p.WriteUInt32(static_cast<uint32_t>(n));
  }
  if (flags & (1 << 9)) {
    p.WriteInt(-1);
  }
  if (flags & (1 << 16)) {
    p.WriteInt(static_cast<int>((n & 0xffff) | (((n >> 24) % 8) << 20)));
  }
  if (flags & (1 << 20)) {
    int count = static_cast<int>((n >> 12) % 4) - 1;
    p.WriteInt(count);
    for (int i = 0; i < count; ++i) {
      auto sct = base::MakeRefCounted<net::ct::SignedCertificateTimestamp>();
      sct->log_id = "log";
      sct->extensions = "ext";
      sct->signature.signature_data = "sig";
      sct->Persist(&p);
      p.WriteUInt16(static_cast<uint16_t>(n));
    }
  }
  if (flags & (1 << 11)) {
    p.WriteInt((n >> 20) & 1);
    std::array<uint8_t, 32> hash{};
    p.WriteBytes(base::span(hash).first(((n >> 20) & 1) ? 32u : 16u));
  }
  p.WriteString((n & 1) ? "[::1]" : ((n & 2) ? "bad.host" : "127.0.0.1"));
  p.WriteUInt16(static_cast<uint16_t>(n));
  if (flags & (1 << 17)) {
    p.WriteString(std::string("h2\0test", sizeof("h2\0test") - 1));
  }
  if (flags & (1 << 18)) {
    p.WriteInt(static_cast<int>(n % 50) - 3);
  }
  if (flags & (1 << 22)) {
    p.WriteInt(static_cast<int>(n));
  }
  if (flags & (1 << 24)) {
    p.WriteInt64((n & 1) ? INT64_MAX : INT64_MIN);
  }
  if (flags & (1 << 25)) {
    p.WriteInt((n & 4) ? -1 : ((n & 8) ? 65536 : static_cast<uint16_t>(n)));
  }
  if (flags & (1 << 27)) {
    int count = static_cast<int>(n % 5) - 1;
    p.WriteInt(count);
    for (int i = 0; i < count; ++i) {
      p.WriteString(i & 1 ? "dup" : "other");
    }
  }
  if (flags & (1 << 30)) {
    p.WriteInt64(INT64_MIN);
  }
  if (extra & 2) {
    net::proxy_value_rust_reference::ProxyChain::Direct().Persist(&p);
  }
  if (extra & 8) {
    p.WriteInt64((n & 4) ? -1 : static_cast<int64_t>(n >> 1));
  }
  if (extra & 16) {
    p.WriteInt64((n & 8) ? -1 : static_cast<int64_t>(n >> 1));
  }
  return p;
}
inline void Parse(base::span<const uint8_t> bytes, uint64_t seed) {
  HttpResponseInfo a;
  HttpResponseInfoOriginal b;
  if (seed & 1) {
    Fill(a, seed);
    Fill(b, seed);
  }
  bool t = (seed & 2) != 0, u = t;
  // Public bool* outputs may alias destination bool members. Each callback
  // operates without a retained Rust borrow, preserving this update order.
  bool* truncated_a = &t;
  bool* truncated_b = &u;
  switch ((seed >> 32) % 7) {
    case 1:
      truncated_a = &a.was_cached;
      truncated_b = &b.was_cached;
      break;
    case 2:
      truncated_a = &a.was_fetched_via_spdy;
      truncated_b = &b.was_fetched_via_spdy;
      break;
    case 3:
      truncated_a = &a.was_alpn_negotiated;
      truncated_b = &b.was_alpn_negotiated;
      break;
    case 4:
      truncated_a = &a.did_use_shared_dictionary;
      truncated_b = &b.did_use_shared_dictionary;
      break;
    case 5:
      truncated_a = &a.ssl_info.pkp_bypassed;
      truncated_b = &b.ssl_info.pkp_bypassed;
      break;
    case 6:
      truncated_a = &a.ssl_info.encrypted_client_hello;
      truncated_b = &b.ssl_info.encrypted_client_hello;
      break;
  }
  bool x = a.InitFromPickle(base::PickleIterator::WithData(bytes), truncated_a),
       y = b.InitFromPickle(base::PickleIterator::WithData(bytes), truncated_b);
  CHECK(x == y);
  CHECK(t == u);
  Equal(a, b);
  // Partial states remain legal inputs to copy, assign and self assign.
  HttpResponseInfo c(a);
  HttpResponseInfoOriginal d(b);
  Equal(c, d);
  CHECK(c.headers == a.headers);
  CHECK(d.headers == b.headers);
  CHECK(c.cert_request_info == a.cert_request_info);
  CHECK(d.cert_request_info == b.cert_request_info);
  c = a;
  d = b;
  Equal(c, d);
  auto* ac = &c;
  auto* bd = &d;
  c = *ac;
  d = *bd;
  Equal(c, d);
}
// Keep a valid outer Pickle header while truncating the payload. Otherwise
// WithData rejects the envelope before any response-info partial update.
inline void ParsePrefix(std::string bytes, size_t size, uint64_t seed) {
  bytes.resize(size);
  if (size >= sizeof(uint32_t)) {
    uint32_t payload = static_cast<uint32_t>(size - sizeof(uint32_t));
    base::as_writable_byte_span(bytes).first<sizeof(uint32_t)>().copy_from(
        base::byte_span_from_ref(payload));
  }
  Parse(base::as_byte_span(bytes), seed);
}

#endif
