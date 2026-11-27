// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
#ifndef NET_DNS_DNS_RESOLUTION_POLICY_FFI_H_
#define NET_DNS_DNS_RESOLUTION_POLICY_FFI_H_
#include <cstdint>
#include <memory>
#include <type_traits>
#include <string_view>
#include <compare>

struct DnsResolutionWords {
  uint64_t a;
  uint64_t b;
  int64_t x;
  int64_t y;
  static constexpr DnsResolutionWords Bool(bool value) { return {.a = value}; }
  static constexpr DnsResolutionWords Unsigned(uint64_t value) { return {.a = value}; }
  static constexpr DnsResolutionWords Signed(int64_t value) { return {.x = value}; }
  static DnsResolutionWords Bytes(std::string_view value) {
    return {.a = reinterpret_cast<uintptr_t>(value.data()), .b = value.size()};
  }
};
static_assert(sizeof(DnsResolutionWords) == 32);
static_assert(std::is_trivial_v<DnsResolutionWords>);
static_assert(std::is_standard_layout_v<DnsResolutionWords>);
static_assert(std::is_trivially_copyable_v<DnsResolutionWords>);
using DnsResolutionService = DnsResolutionWords (*)(void*, uint32_t, DnsResolutionWords);
extern "C" DnsResolutionWords rust_dns_resolution_run(
    uint32_t domain, uint32_t method, void* context, DnsResolutionService service);
template <typename Service>
DnsResolutionWords DnsResolutionRun(uint32_t domain, uint32_t method, Service& service) {
  return rust_dns_resolution_run(domain, method, std::addressof(service),
    [](void* context, uint32_t operation, DnsResolutionWords input) {
      return (*static_cast<Service*>(context))(operation, input);
    });
}
#endif
