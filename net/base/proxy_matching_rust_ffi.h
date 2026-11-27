// Copyright 2026 The Chromium Authors
#ifndef NET_BASE_PROXY_MATCHING_RUST_FFI_H_
#define NET_BASE_PROXY_MATCHING_RUST_FFI_H_
#include <memory>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "net/base/scheme_host_port_matcher.h"
#include "third_party/rust/cxx/v1/cxx.h"
namespace net::proxy_matching_rust {
struct Authority;
struct Rule;
struct Matcher;
struct RuleStorage {
  explicit RuleStorage(rust::Box<Rule> r);
  ~RuleStorage();
  rust::Box<Rule> rule;
};
struct MatcherStorage {
  MatcherStorage();
  ~MatcherStorage();
  rust::Box<Matcher> matcher;
};
rust::Vec<uint8_t> ip_service(rust::Slice<const uint8_t> raw);
rust::Vec<uint8_t> canonical_service(rust::Slice<const uint8_t> ip);
Authority authority_service(rust::Slice<const uint8_t> raw);
rust::Slice<const uint8_t> Bytes(std::string_view value);
std::string String(rust::Vec<uint8_t> value);
std::unique_ptr<SchemeHostPortMatcherRule> Wrap(rust::Box<Rule> rule);
NET_EXPORT bool UnspecifiedEnabled();
uint8_t Evaluate(const Rule&, const GURL&);
}  // namespace net::proxy_matching_rust
#endif
