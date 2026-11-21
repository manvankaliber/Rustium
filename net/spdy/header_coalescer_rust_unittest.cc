// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <bit>
#include <random>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/strings/string_view_util.h"
#include "net/log/test_net_log.h"
#include "net/spdy/header_coalescer_rust_ffi.h"
#include "net/spdy/header_coalescer_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
// Test-only private snapshots preserve the release ABI and avoid making a
// nested NetLog call while the observer-dispatch lock is held.
struct HeaderCoalescerRustTestAccess {
  template <class Coalescer>
  static std::array<size_t, 4> Snapshot(const Coalescer& c) {
    return {c.header_list_size_, c.headers_valid_, c.error_seen_,
            c.regular_header_seen_};
  }
  template <class Coalescer>
  static std::string_view LowSizeByte(Coalescer& c) {
    return base::as_string_view(base::byte_span_from_ref(c.header_list_size_))
        .substr(
            std::endian::native == std::endian::little ? 0 : sizeof(size_t) - 1,
            1);
  }
  template <class Coalescer>
  static std::string_view RegularByte(Coalescer& c) {
    return base::as_string_view(
        base::byte_span_from_ref(c.regular_header_seen_));
  }
};
}  // namespace net

namespace net::header_coalescer_rust_test {
namespace {
struct Transcript {
  std::vector<bool> errors;
  Headers headers;
  std::vector<base::DictValue> logs;
  bool operator==(const Transcript&) const = default;
};
template <class Coalescer>
Transcript RunTranscript(const Headers& headers,
                         uint32_t limit,
                         NetLogCaptureMode mode) {
  NetLog* log = NetLog::Get();
  const auto source = NetLogWithSource::Make(log, NetLogSourceType::NONE);
  RecordingNetLogObserver observer(log, mode);
  Coalescer coalescer(limit, source);
  Transcript out;
  coalescer.OnHeaderBlockStart();
  for (const auto& [key, value] : headers) {
    coalescer.OnHeader(key, value);
    out.errors.push_back(coalescer.error_seen());
  }
  coalescer.OnHeaderBlockEnd(0, SIZE_MAX);
  out.headers = Copy(coalescer.release_headers());
  for (const auto& entry : observer.GetEntries()) {
    CHECK(entry.source.id == source.source().id);
    CHECK(entry.type == NetLogEventType::HTTP2_SESSION_RECV_INVALID_HEADER);
    CHECK(entry.phase == NetLogEventPhase::NONE);
    out.logs.push_back(entry.params.Clone());
  }
  return out;
}
void Compare(const Headers& headers, uint32_t limit = UINT32_MAX) {
  for (auto mode :
       {NetLogCaptureMode::kHeavilyRedacted, NetLogCaptureMode::kDefault,
        NetLogCaptureMode::kIncludeSensitive, NetLogCaptureMode::kEverything}) {
    EXPECT_EQ(RunTranscript<HeaderCoalescer>(headers, limit, mode),
              RunTranscript<HeaderCoalescerRustOriginal>(headers, limit, mode));
  }
}
TEST(HeaderCoalescerRustTest, ExhaustiveBytesAndPrecedence) {
  for (unsigned c = 0; c <= 255; ++c) {
    const std::string byte(1, static_cast<char>(c));
    Compare({{byte, "ok"}, {":late", ""}});
    Compare({{":" + byte, "ok"}});
    Compare({{"a", byte}, {"a", "ignored"}});
    Compare({{"A" + byte, "\r\n"}}, 0);
    Compare({{"a", byte}}, 33);
  }
  Compare({{"", "x"}, {"a", "ignored"}});
  Compare({{":", ""}});
  Compare({{":status", "200"}, {"a", "b"}, {":method", "GET"}});
  Compare({{"foo", "bar"}, {"foo", "quux"}, {"cookie", "a"}, {"cookie", "b"}});
}
TEST(HeaderCoalescerRustTest, ExactLimitsAndFirstInvalidByte) {
  for (uint32_t limit : {0u, 32u, 33u, 34u, 35u, 66u, 67u, UINT32_MAX}) {
    Compare({{"a", ""}, {"b", "x"}, {":late", "\n"}}, limit);
    Compare({{"a", "\r\n\x7f"}}, limit);
  }
  const auto out = RunTranscript<HeaderCoalescer>(
      {{"a", "\r\n"}}, 100, NetLogCaptureMode::kEverything);
  ASSERT_EQ(out.logs.size(), 1u);
  EXPECT_EQ(*out.logs[0].FindString("error"),
            "Invalid character 0x0D in header value.");
  EXPECT_EQ(out.headers.size(), 0u);
}
TEST(HeaderCoalescerRustTest, FrozenLoggingClosure) {
  for (std::string name :
       {"cookie", "set-cookie", "set-cookie2", "authorization",
        "proxy-authorization", "www-authenticate", "proxy-authenticate"}) {
    for (std::string value :
         {"secret\r", "Negotiate abc\n", " Basic abc\n ", "Digest realm=x\r",
          "NTLM abc,def\n", "\t\n", "%\x80\xff\n", "Negotiate\t abc\n"}) {
      Compare({{name, value}});
      Compare({{name, value}}, 0);
    }
  }
  const auto out = RunTranscript<HeaderCoalescer>(
      {{"cookie", "private\n"}}, 100, NetLogCaptureMode::kDefault);
  EXPECT_EQ(*out.logs[0].FindString("header_value"), "[8 bytes were stripped]");
}
template <class Coalescer>
std::pair<bool, Headers> Reentrant() {
  NetLog* log = NetLog::Get();
  const auto source = NetLogWithSource::Make(log, NetLogSourceType::NONE);
  RecordingNetLogObserver observer(log, NetLogCaptureMode::kEverything);
  Coalescer coalescer(1024, source);
  bool during_error = true;
  Headers released;
  observer.SetThreadsafeAddEntryCallback(base::BindRepeating(
      [](Coalescer* c, bool* during, Headers* out) {
        *during = c->error_seen();
        c->OnHeader("callback", "ok");
        *out = Copy(c->release_headers());
      },
      base::Unretained(&coalescer), base::Unretained(&during_error),
      base::Unretained(&released)));
  coalescer.OnHeader("a", "first");
  coalescer.OnHeader("", "error");
  EXPECT_TRUE(coalescer.error_seen());
  coalescer.OnHeader("after", "ignored");
  EXPECT_EQ(observer.GetSize(), 1u);
  return {during_error, released};
}
TEST(HeaderCoalescerRustTest, CallbackSeesPreLatchStateAndCanRelease) {
  const auto actual = Reentrant<HeaderCoalescer>();
  EXPECT_EQ(actual, Reentrant<HeaderCoalescerRustOriginal>());
  EXPECT_FALSE(actual.first);
  EXPECT_EQ(actual.second, (Headers{{"a", "first"}, {"callback", "ok"}}));
}
template <class Coalescer>
std::array<size_t, 4> CallbackSnapshot(std::string_view key,
                                       std::string_view value,
                                       uint32_t limit) {
  const auto source =
      NetLogWithSource::Make(NetLog::Get(), NetLogSourceType::NONE);
  RecordingNetLogObserver observer(NetLog::Get(),
                                   NetLogCaptureMode::kEverything);
  Coalescer coalescer(limit, source);
  std::array<size_t, 4> observed{};
  observer.SetThreadsafeAddEntryCallback(base::BindRepeating(
      [](Coalescer* c, std::array<size_t, 4>* out) {
        *out = HeaderCoalescerRustTestAccess::Snapshot(*c);
      },
      base::Unretained(&coalescer), base::Unretained(&observed)));
  coalescer.OnHeader(key, value);
  EXPECT_TRUE(coalescer.error_seen());
  return observed;
}
TEST(HeaderCoalescerRustTest, CallbacksObserveFailurePartialState) {
  struct Case {
    const char* key;
    const char* value;
    uint32_t limit;
    size_t size;
    size_t regular;
  };
  for (const auto& c : std::array<Case, 4>{{{"?", "", 100, 0, 1},
                                            {"a", "\r", 70, 34, 1},
                                            {"a", "too large", 40, 42, 1},
                                            {"", "", 100, 0, 0}}}) {
    const auto actual =
        CallbackSnapshot<HeaderCoalescer>(c.key, c.value, c.limit);
    EXPECT_EQ(actual, CallbackSnapshot<HeaderCoalescerRustOriginal>(
                          c.key, c.value, c.limit));
    EXPECT_EQ(actual, (std::array<size_t, 4>{c.size, 1, 0, c.regular}));
  }
}
TEST(HeaderCoalescerRustTest, AliasedInputsOwnershipAndRelease) {
  const NetLogWithSource log;
  HeaderCoalescer coalescer(1024, log);
  std::string backing = "cookie";
  coalescer.OnHeader(backing, backing);
  coalescer.OnHeader(std::string_view(backing).substr(0, 3),
                     std::string_view(backing).substr(1, 3));
  backing.assign(100, 'z');
  const auto released = coalescer.release_headers();
  EXPECT_EQ(Copy(released), (Headers{{"cookie", "cookie"}, {"coo", "ook"}}));
  coalescer.OnHeader("a", "new");
  EXPECT_FALSE(coalescer.error_seen());
  EXPECT_EQ(Copy(released), (Headers{{"cookie", "cookie"}, {"coo", "ook"}}));
}
TEST(HeaderCoalescerRustTest, ReleaseGuard) {
  const NetLogWithSource log;
  HeaderCoalescer coalescer(1024, log);
  coalescer.release_headers();
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(coalescer.release_headers(), "");
#else
  EXPECT_TRUE(coalescer.release_headers().empty());
#endif
}
TEST(HeaderCoalescerRustTest, ValueSnapshotAbiAndWrapping) {
  EXPECT_EQ(sizeof(HeaderCoalescer), sizeof(HeaderCoalescerRustOriginal));
  HeaderCoalescerRustState state{SIZE_MAX - 34, 1, 0, 0};
  auto d = header_coalescer_rust_header(
      state, reinterpret_cast<const uint8_t*>("a"), 1,
      reinterpret_cast<const uint8_t*>("bc"), 2, 0);
  EXPECT_EQ(d.state.header_list_size, 0u);
  EXPECT_EQ(d.action, HeaderCoalescerRustAction::kAppend);
  d = header_coalescer_rust_header({0, 1, 0, 0}, nullptr, 0, nullptr, 0, 0);
  EXPECT_EQ(d.action, HeaderCoalescerRustAction::kEmptyName);
  EXPECT_EQ(d.state.regular_header_seen, 0);
  d = header_coalescer_rust_header(
      {0, 1, 0, 0}, reinterpret_cast<const uint8_t*>("?"), 1, nullptr, 0, 100);
  EXPECT_EQ(d.state.regular_header_seen, 1);
  EXPECT_EQ(d.state.header_list_size, 0u);
  d = header_coalescer_rust_header({123, 0, 1, 1}, nullptr, SIZE_MAX, nullptr,
                                   SIZE_MAX, 0);
  EXPECT_EQ(d.action, HeaderCoalescerRustAction::kIgnore);
  EXPECT_EQ(d.state.header_list_size, 123u);
  d = header_coalescer_rust_release(d.state);
  EXPECT_EQ(d.action, HeaderCoalescerRustAction::kReleaseInvalid);
  d = header_coalescer_rust_release({0, 1, 0, 0});
  EXPECT_EQ(d.action, HeaderCoalescerRustAction::kReleaseValid);
  EXPECT_EQ(header_coalescer_rust_release(d.state).action,
            HeaderCoalescerRustAction::kReleaseInvalid);
}
template <class Coalescer>
Headers CounterAliasedValue() {
  const NetLogWithSource log;
  Coalescer coalescer(UINT32_MAX, log);
  coalescer.OnHeader("a", std::string(223, 'v'));  // Counter is now 256.
  coalescer.OnHeader("b",
                     HeaderCoalescerRustTestAccess::LowSizeByte(coalescer));
  EXPECT_FALSE(coalescer.error_seen());
  return Copy(coalescer.release_headers());
}
TEST(HeaderCoalescerRustTest, CounterAliasedValueReadsCommittedSize) {
  const Headers expected{{"a", std::string(223, 'v')}, {"b", "\""}};
  EXPECT_EQ(CounterAliasedValue<HeaderCoalescerRustOriginal>(), expected);
  EXPECT_EQ(CounterAliasedValue<HeaderCoalescer>(), expected);
}
template <class Coalescer>
Headers CounterAliasedName() {
  const NetLogWithSource log;
  Coalescer coalescer(UINT32_MAX, log);
  coalescer.OnHeader("a", std::string(64, 'v'));  // Counter is now 97 ('a').
  coalescer.OnHeader(HeaderCoalescerRustTestAccess::LowSizeByte(coalescer),
                     "x");
  EXPECT_FALSE(coalescer.error_seen());
  return Copy(coalescer.release_headers());
}
TEST(HeaderCoalescerRustTest, CounterAliasedNameEndsValidationBeforeSizeWrite) {
  const Headers expected{{"a", std::string(64, 'v')},
                         {std::string(1, static_cast<char>(131)), "x"}};
  EXPECT_EQ(CounterAliasedName<HeaderCoalescerRustOriginal>(), expected);
  EXPECT_EQ(CounterAliasedName<HeaderCoalescer>(), expected);
}
template <class Coalescer>
std::string FlagAliasedValue() {
  const auto source =
      NetLogWithSource::Make(NetLog::Get(), NetLogSourceType::NONE);
  RecordingNetLogObserver observer(NetLog::Get(),
                                   NetLogCaptureMode::kEverything);
  Coalescer coalescer(UINT32_MAX, source);
  coalescer.OnHeader("a",
                     HeaderCoalescerRustTestAccess::RegularByte(coalescer));
  EXPECT_TRUE(coalescer.error_seen());
  const auto events = observer.GetEntries();
  EXPECT_EQ(events.size(), 1u);
  return *events.front().params.FindString("error");
}
TEST(HeaderCoalescerRustTest, FlagAliasedValueSeesPreValidationTransition) {
  const char* const expected = "Invalid character 0x01 in header value.";
  EXPECT_EQ(FlagAliasedValue<HeaderCoalescerRustOriginal>(), expected);
  EXPECT_EQ(FlagAliasedValue<HeaderCoalescer>(), expected);
}
TEST(HeaderCoalescerRustTest, MillionDeterministicBoundedInputs) {
  std::mt19937 random(0x42c0a1);
  for (size_t i = 0; i < 1048576; ++i) {
    std::string input(1 + random() % 256, '\0');
    for (auto& c : input) {
      c = static_cast<char>(random());
    }
    Differential(input);
  }
}
}  // namespace
}  // namespace net::header_coalescer_rust_test
