// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Warm same-process paired public API construction/parse/ownership/destruction.
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/i18n/icu_util.h"
#include "net/http/content_disposition_reference.h"
#include "net/http/http_content_disposition.h"
namespace {
struct Case {
  const char* name;
  std::string input;
  std::string charset;
  size_t iterations;
};
volatile uint64_t sink = 0;
template <class Parser>
uint64_t Run(const Case& c, size_t iterations) {
  uint64_t sum = 0;
  for (size_t i = 0; i < iterations; ++i) {
    Parser value(c.input, c.charset);
    sum += value.filename().size() + value.parse_result_flags() +
           value.is_attachment();
    if (!value.filename().empty()) {
      sum += static_cast<unsigned char>(value.filename().front());
    }
  }
  return sum;
}
template <class Parser>
double Measure(const Case& c) {
  auto start = std::chrono::steady_clock::now();
  sink = Run<Parser>(c, c.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         static_cast<double>(c.iterations);
}
}  // namespace
int main() {
  if (!base::i18n::InitializeICU()) {
    return 2;
  }
  std::string many = "attachment";
  for (int i = 0; i < 64; ++i) {
    many += ";unknown=\"a;b,c\"";
  }
  many += ";filename=final";
  const std::vector<Case> cases = {
      {"empty", "", "", 200000},
      {"inline", "inline", "", 200000},
      {"ascii", "attachment; filename=report.pdf", "", 100000},
      {"quoted", "attachment; filename=\"a,b;c\\\"d.pdf\"", "", 100000},
      {"priority", "attachment;filename=backup;filename*=UTF-8''caf%C3%A9.pdf",
       "", 20000},
      {"q-word", "filename==?UTF-8?Q?caf=C3=A9_report.pdf?=", "", 20000},
      {"b-word", "filename==?UTF-8?B?Y2Fmw6kucGRm?=", "", 20000},
      {"adjacent", "filename=\"=?UTF-8?Q?caf=C3=A9?= =?UTF-8?Q?_report.pdf?=\"",
       "", 10000},
      {"percent", "filename=caf%C3%A9%20report.pdf", "", 50000},
      {"normalize", "filename*=UTF-8''cafe%CC%81.pdf", "", 20000},
      {"legacy-charset", "filename=caf\xe9.pdf", "ISO-8859-1", 20000},
      {"os-fallback", "filename=caf\xe9.pdf", "unknown-charset", 10000},
      {"malformed", "attachment;filename==?UTF-8?Q?bad=XY?=;filename=backup",
       "", 50000},
      {"long-quoted", "filename=\"" + std::string(4096, 'x') + "\"", "", 5000},
      {"many-parameters", many, "", 5000},
  };
  std::mt19937 rng(20261112);
  for (const auto& c : cases) {
    const auto a = Run<net::ContentDispositionReference>(c, c.iterations / 5);
    const auto b = Run<net::HttpContentDisposition>(c, c.iterations / 5);
    if (a != b) {
      return 1;
    }
    sink = a + b;
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      double cpp_ns, rust_ns;
      const bool rust_first = (rng() & 1) != 0;
      if (rust_first) {
        rust_ns = Measure<net::HttpContentDisposition>(c);
        cpp_ns = Measure<net::ContentDispositionReference>(c);
      } else {
        cpp_ns = Measure<net::ContentDispositionReference>(c);
        rust_ns = Measure<net::HttpContentDisposition>(c);
      }
      // SAFETY: Format and case names are null-terminated literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          c.name, repeat, cpp_ns, rust_ns, c.iterations,
          rust_first ? "true" : "false"));
    }
  }
}
