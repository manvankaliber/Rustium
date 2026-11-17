// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <cstdio>
#include <random>

#include "base/strings/string_number_conversions.h"
#include "net/cookies/parsed_cookie_rust_test_support.h"
int main(int argc, char** argv) {
  using namespace net::parsed_cookie_rust_test;
  size_t count = 1000000;
  if (argc > 1) {
    CHECK(base::StringToSizeT(argv[1], &count));
  }
  std::mt19937_64 rng(20261112);
  const std::array<std::string_view, 17> seeds = {
      "a=b",
      "",
      "secure",
      "=a=b",
      "a=b; Path=/one; PATH=/two; Secure=no; Secure=yes; HttpOnly=yes; "
      "partitioned=no",
      "a=b; domain=; path=; samesite=; priority=low; unknown=value",
      "a=b; samesite=NoNe",
      "a=b; samesite=lAx",
      "a=b; samesite=STRICT",
      "a=b; samesite=extended",
      "a=b; samesite=garbage",
      "a=b; priority=HIGH",
      "a=b; priority=medium",
      "a=b; priority=unknown",
      "a=b; \t ; =empty; httponly",
      " a \t= b \t; path= / ",
      "a\tb=c\td"};
  std::fprintf(stderr, "Starting boundaries\n");
  Statics({});
  for (unsigned byte = 0; byte != 256; ++byte) {
    for (const auto prefix : {"", "n", "n=v; PATH=", "\t"}) {
      std::string s(prefix);
      s += static_cast<char>(byte);
      Exercise(s, s, rng());
    }
  }
  std::fprintf(stderr, "Starting sizes\n");
  for (size_t length : {1023u, 1024u, 1025u, 4095u, 4096u, 4097u, 8192u}) {
    std::string value(length, 'a');
    for (const auto prefix :
         {"", "=", "n=", "n=v; path=", "n=v; path=one; path="}) {
      Exercise(std::string(prefix) + value + "; secure", " " + value + " ",
               rng());
    }
  }
  std::fprintf(stderr, "Starting state fuzz\n");
  for (size_t i = 0; i < count; ++i) {
    std::string line(seeds[rng() % seeds.size()]);
    std::string mutation;
    const size_t len = rng() % 100;
    for (size_t j = 0; j < len; ++j) {
      const auto c = static_cast<char>(rng());
      mutation += c;
    }
    if (i % 4 == 0) {
      line += mutation;
    } else if (i % 4 == 1) {
      line = mutation;
    } else if (i % 4 == 2) {
      mutation = std::string(seeds[rng() % seeds.size()]);
    }
    Exercise(line, mutation, rng());
  }
  std::printf(
      "Passed %zu seeded whole-public-API state fuzz inputs plus byte/size "
      "boundaries.\n",
      count);
}
