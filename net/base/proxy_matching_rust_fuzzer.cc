// Copyright 2026 The Chromium Authors
#include <cstdio>
#include <fstream>

#include "base/i18n/icu_util.h"
#include "net/base/proxy_matching_rust_differential.h"
int main(int argc, char** argv) {
  base::i18n::InitializeICU();
  uint64_t count = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 1000000,
           seed = 0x30fe34ab1829ULL;
  auto random = [&] {
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    return seed;
  };
  using namespace net::proxy_matching_rust_test;
  for (uint64_t n = 0; n < count; ++n) {
    std::string raw(kRules[random() % kRules.size()]);
    std::string url(kUrls[random() % kUrls.size()]);
    auto mode = random() % 5;
    if (mode == 0) {
      raw.clear();
      size_t len = random() % 64;
      for (size_t i = 0; i < len; ++i) {
        raw.push_back(static_cast<char>(random()));
      }
    } else if (mode == 1) {
      size_t at = random() % (raw.size() + 1);
      raw.insert(at, 1, static_cast<char>(random()));
    } else if (mode == 2) {
      raw += ';';
      raw += kRules[random() % kRules.size()];
    } else if (mode == 3) {
      url = "http://";
      for (unsigned i = 0, l = random() % 24; i < l; ++i) {
        url.push_back(static_cast<char>(random()));
      }
    }
    // Persist the current reproducer before checking; overwritten only on
    // success.
    if (n % 10000 == 0) {
      std::printf("inputs=%llu\n", static_cast<unsigned long long>(n));
      std::fflush(stdout);
    }
    Differential(raw, url, n);
  }
  std::printf(
      "passed=%llu seed=0x30fe34ab1829 whole-API differential state inputs\n",
      static_cast<unsigned long long>(count));
}
