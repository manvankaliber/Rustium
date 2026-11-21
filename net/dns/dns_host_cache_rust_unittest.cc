// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "net/dns/dns_host_cache_rust_reference.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver_internal_result.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
namespace {
HostCache::Key RustKey(std::string h = "host.test") {
  return HostCache::Key(std::move(h), DnsQueryType::A, 0,
                        HostResolverSource::ANY, NetworkAnonymizationKey(),
                        handles::kInvalidNetworkHandle);
}
HostCache::Entry RustEntry() {
  return HostCache::Entry(OK, {}, base::flat_set<std::string>{"host.test"},
                          HostCache::Entry::SOURCE_DNS);
}
class CallbackDelegate : public HostCache::PersistenceDelegate {
 public:
  explicit CallbackDelegate(base::RepeatingClosure callback)
      : callback_(std::move(callback)) {}
  void ScheduleWrite() override { callback_.Run(); }

 private:
  base::RepeatingClosure callback_;
};
TEST(DnsHostCacheRustTest, PreservesUnrelatedMapNodeAndIterator) {
  HostCache c(8);
  auto key = RustKey();
  auto now = base::TimeTicks();
  c.Set(key, RustEntry(), now, base::Seconds(3));
  auto* p = c.Lookup(key, now);
  auto it = c.entries().begin();
  const auto* e = &it->second;
  c.Set(RustKey("other.test"), RustEntry(), now, base::Seconds(5));
  EXPECT_EQ(p, c.Lookup(key, now));
  EXPECT_EQ(e, &it->second);
  c.Invalidate();
  EXPECT_EQ(e, &it->second);
  EXPECT_EQ(p, c.LookupStale(key, now, nullptr));
}
TEST(DnsHostCacheRustTest, AliasedReplacementSnapshotsKeyAndEntry) {
  HostCache c(1);
  auto key = RustKey();
  auto now = base::TimeTicks();
  c.Set(key, RustEntry(), now, base::Seconds(3));
  auto* p = c.Lookup(key, now);
  c.Set(p->first, p->second, now, base::Seconds(5));
  auto* after = c.Lookup(key, now);
  ASSERT_TRUE(after);
  EXPECT_EQ(after->second.aliases(), base::flat_set<std::string>{"host.test"});
  EXPECT_EQ(after->second.expires(), now + base::Seconds(5));
}
TEST(DnsHostCacheRustTest, DelegateMayDeleteOwnerAfterSet) {
  auto c = std::make_unique<HostCache>(2);
  bool called = false;
  CallbackDelegate d(base::BindLambdaForTesting([&]() {
    called = true;
    c.reset();
  }));
  c->set_persistence_delegate(&d);
  c->Set(RustKey(), RustEntry(), base::TimeTicks(), base::Seconds(1));
  EXPECT_TRUE(called);
  EXPECT_FALSE(c);
}
TEST(DnsHostCacheRustTest, DelegateMayReenterSet) {
  HostCache c(3);
  bool called = false;
  CallbackDelegate d(base::BindLambdaForTesting([&]() {
    if (called) {
      return;
    }
    called = true;
    c.Set(RustKey("nested.test"), RustEntry(), base::TimeTicks(),
          base::Seconds(1));
  }));
  c.set_persistence_delegate(&d);
  c.Set(RustKey(), RustEntry(), base::TimeTicks(), base::Seconds(1));
  EXPECT_EQ(2u, c.size());
  c.set_persistence_delegate(nullptr);
}
TEST(DnsHostCacheRustTest, HostFilterMayDeleteOwner) {
  auto c = std::make_unique<HostCache>(2);
  c->Set(RustKey(), RustEntry(), base::TimeTicks(), base::Seconds(1));
  bool called = false;
  c->ClearForHosts(base::BindLambdaForTesting([&](const std::string& host) {
    EXPECT_EQ(host, "host.test");
    called = true;
    c.reset();
    EXPECT_EQ(host, "host.test");
    return true;
  }));
  EXPECT_TRUE(called);
  EXPECT_FALSE(c);
}
TEST(DnsHostCacheRustTest, HostFilterMayClearAndReenter) {
  HostCache c(3);
  c.Set(RustKey(), RustEntry(), base::TimeTicks(), base::Seconds(1));
  c.Set(RustKey("other.test"), RustEntry(), base::TimeTicks(),
        base::Seconds(1));
  bool called = false;
  c.ClearForHosts(base::BindLambdaForTesting([&](const std::string&) {
    if (!called) {
      called = true;
      c.clear();
      c.Set(RustKey("new.test"), RustEntry(), base::TimeTicks(),
            base::Seconds(1));
    }
    return true;
  }));
  EXPECT_EQ(1u, c.size());
  EXPECT_TRUE(c.Lookup(RustKey("new.test"), base::TimeTicks()));
}
TEST(DnsHostCacheRustTest, LookupMetricSuppressionAndStaleCount) {
  base::HistogramTester h;
  HostCache c(1);
  auto key = RustKey();
  auto now = base::TimeTicks();
  c.Lookup(key, now, false, false);
  h.ExpectTotalCount("Net.DNS.HostCache.Lookup", 0);
  c.Lookup(key, now);
  h.ExpectBucketCount("Net.DNS.HostCache.Lookup", 0, 1);
  c.Set(key, RustEntry(), now, base::Microseconds(1));
  c.Lookup(key, now);
  c.Lookup(key, now + base::Microseconds(1));
  HostCache::EntryStaleness stale;
  c.LookupStale(key, now + base::Microseconds(1), &stale);
  EXPECT_EQ(1, stale.stale_hits);
  h.ExpectBucketCount("Net.DNS.HostCache.Lookup", 2, 1);
  h.ExpectBucketCount("Net.DNS.HostCache.Lookup", 1, 1);
  h.ExpectBucketCount("Net.DNS.HostCache.Lookup", 3, 1);
  h.ExpectBucketCount("Net.DNS.HostCache.LookupStale.NetworkChanges", 0, 1);
}
TEST(DnsHostCacheRustTest, EntryCompletionAndContentEquality) {
  auto a = RustEntry();
  auto b = a;
  EXPECT_TRUE(a == b);
  b.set_pinning(true);
  EXPECT_TRUE(a.ContentsEqual(b));
  EXPECT_FALSE(a == b);
  for (int error : {OK, ERR_FAILED, ERR_NAME_NOT_RESOLVED, ERR_NETWORK_CHANGED,
                    ERR_HOST_RESOLVER_QUEUE_TOO_LARGE}) {
    a.set_error(error);
    EXPECT_EQ(error != ERR_NETWORK_CHANGED &&
                  error != ERR_HOST_RESOLVER_QUEUE_TOO_LARGE,
              a.did_complete());
  }
}
TEST(DnsHostCacheRustTest, InvalidRestorationTypedFieldsAndPartialProgress) {
  base::DictValue good;
  good.Set("hostname", "restore.test");
  good.Set("flags", 0);
  good.Set("dns_query_type", 1);
  good.Set("expiration", "0");
  good.Set("net_error", ERR_NAME_NOT_RESOLVED);
  base::Value nak;
  ASSERT_TRUE(NetworkAnonymizationKey().ToValue(&nak));
  good.Set("network_anonymization_key", std::move(nak));
  for (const char* name :
       {"hostname", "flags", "dns_query_type", "expiration",
        "network_anonymization_key", "ip_endpoints", "endpoint_metadatas",
        "aliases", "addresses", "text_records", "hostname_results",
        "host_ports", "canonical_names", "scheme", "port", "secure", "pinned",
        "net_error"}) {
    for (int type = 0; type < 7; ++type) {
      base::Value bad;
      switch (type) {
        case 1:
          bad = base::Value(false);
          break;
        case 2:
          bad = base::Value(-1);
          break;
        case 3:
          bad = base::Value(0.25);
          break;
        case 4:
          bad = base::Value("garbage");
          break;
        case 5:
          bad = base::Value(base::DictValue());
          break;
        case 6:
          bad = base::Value(base::ListValue());
          break;
      }
      auto dict = good.Clone();
      dict.Set(name, std::move(bad));
      base::ListValue list;
      list.Append(std::move(dict));
      HostCache a(8);
      DnsHostCacheRustReference b(8);
      EXPECT_EQ(a.RestoreFromListValue(list), b.RestoreFromListValue(list))
          << name << type;
      EXPECT_EQ(a.size(), b.size());
      EXPECT_EQ(a.last_restore_size(), b.last_restore_size());
    }
  }
  base::ListValue mixed;
  mixed.Append(good.Clone());
  mixed.Append(42);
  HostCache a(8);
  EXPECT_FALSE(a.RestoreFromListValue(mixed));
  EXPECT_EQ(1u, a.last_restore_size());
  EXPECT_EQ(1u, a.size());
}
}  // namespace
}  // namespace net
// Additional typed nested-Value mutations exercise the actual native parser,
// including legacy schema paths, rather than a simplified serialization model.
namespace net {
namespace {
TEST(DnsHostCacheRustTest, NestedRestorationFailuresAgainstFrozenOriginal) {
  base::DictValue seed;
  seed.Set("hostname", "restore.test");
  seed.Set("flags", 0);
  seed.Set("dns_query_type", 1);
  seed.Set("expiration", "0");
  base::Value nak;
  ASSERT_TRUE(NetworkAnonymizationKey().ToValue(&nak));
  seed.Set("network_anonymization_key", std::move(nak));
  auto compare = [](base::DictValue dict) {
    base::ListValue list;
    list.Append(std::move(dict));
    HostCache a(8);
    DnsHostCacheRustReference b(8);
    EXPECT_EQ(a.RestoreFromListValue(list), b.RestoreFromListValue(list));
    EXPECT_EQ(a.size(), b.size());
    EXPECT_EQ(a.last_restore_size(), b.last_restore_size());
    if (a.size()) {
      EXPECT_EQ(a.entries().begin()->second.error(),
                b.entries().begin()->second.error());
      EXPECT_EQ(a.entries().begin()->second.ip_endpoints(),
                b.entries().begin()->second.ip_endpoints());
      EXPECT_EQ(a.entries().begin()->second.GetMetadatas(),
                b.entries().begin()->second.GetMetadatas());
    }
  };
  for (const char* field :
       {"ip_endpoints", "endpoint_metadatas", "aliases", "addresses",
        "text_records", "hostname_results", "host_ports", "canonical_names"}) {
    for (int type = 0; type < 5; ++type) {
      auto dict = seed.Clone();
      base::ListValue values;
      switch (type) {
        case 0:
          values.Append(42);
          break;
        case 1:
          values.Append("bad-ip-or-string");
          break;
        case 2:
          values.Append(base::DictValue());
          break;
        case 3:
          values.Append(base::ListValue());
          break;
        case 4:
          values.Append(true);
          break;
      }
      dict.Set(field, std::move(values));
      compare(std::move(dict));
    }
  }
  for (int port : {-1, 0, 443, 65535, 65536}) {
    auto dict = seed.Clone();
    dict.Set("scheme", "https");
    dict.Set("port", port);
    compare(std::move(dict));
    dict = seed.Clone();
    base::DictValue ep;
    ep.Set("endpoint_address", "192.0.2.1");
    ep.Set("endpoint_port", port);
    base::ListValue endpoints;
    endpoints.Append(std::move(ep));
    dict.Set("ip_endpoints", std::move(endpoints));
    compare(std::move(dict));
    dict = seed.Clone();
    base::ListValue hosts;
    hosts.Append("srv.test");
    base::ListValue ports;
    ports.Append(port);
    dict.Set("hostname_results", std::move(hosts));
    dict.Set("host_ports", std::move(ports));
    compare(std::move(dict));
  }
  for (const char* expiration : {"", "not-a-time", "9223372036854775808"}) {
    auto dict = seed.Clone();
    dict.Set("expiration", expiration);
    compare(std::move(dict));
  }
  for (const char* host : {"", "127.0.0.1", "[::1]", "::1"}) {
    auto dict = seed.Clone();
    dict.Set("hostname", host);
    compare(std::move(dict));
  }
}
}  // namespace
}  // namespace net
namespace net {
namespace {
TEST(DnsHostCacheRustTest, DelegateMayDeleteOwnerAfterClearAndFilteredClear) {
  for (bool filtered : {false, true}) {
    auto c = std::make_unique<HostCache>(2);
    c->Set(RustKey(), RustEntry(), base::TimeTicks(), base::Seconds(1));
    bool called = false;
    CallbackDelegate d(base::BindLambdaForTesting([&]() {
      called = true;
      c.reset();
    }));
    c->set_persistence_delegate(&d);
    if (filtered) {
      c->ClearForHosts(
          base::BindRepeating([](const std::string&) { return true; }));
    } else {
      c->clear();
    }
    EXPECT_TRUE(called);
    EXPECT_FALSE(c);
  }
}
TEST(DnsHostCacheRustTest, InternalSourcesAndCacheableFailureAgainstOriginal) {
  auto now = base::TimeTicks::FromInternalValue(1000);
  auto wall = base::Time::FromInternalValue(2000);
  for (auto source : {HostResolverInternalResult::Source::kDns,
                      HostResolverInternalResult::Source::kHosts,
                      HostResolverInternalResult::Source::kUnknown}) {
    for (bool failure : {false, true}) {
      std::set<std::unique_ptr<HostResolverInternalResult>> results;
      if (failure) {
        results.insert(std::make_unique<HostResolverInternalErrorResult>(
            "host.test", DnsQueryType::UNSPECIFIED, now + base::Seconds(1),
            wall + base::Seconds(2), source, ERR_FAILED));
      } else {
        results.insert(std::make_unique<HostResolverInternalDataResult>(
            "host.test", DnsQueryType::A, now + base::Seconds(1),
            wall + base::Seconds(2), source, std::vector<IPEndPoint>{},
            std::vector<std::string>{"TXT"}, std::vector<HostPortPair>{}));
      }
      HostCache::Entry a(results, wall, now);
      DnsHostCacheRustReference::Entry b(results, wall, now);
      EXPECT_EQ(a.error(), b.error());
      EXPECT_EQ(static_cast<int>(a.source()), static_cast<int>(b.source()));
      EXPECT_EQ(a.ttl(), b.ttl());
      EXPECT_EQ(a.text_records(), b.text_records());
      EXPECT_EQ(HostCache::Entry::TtlFromInternalResults(results, wall, now),
                DnsHostCacheRustReference::Entry::TtlFromInternalResults(
                    results, wall, now));
    }
  }
}
}  // namespace
}  // namespace net
