// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "base/compiler_specific.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_sniffer_ffi.h"
#include "net/base/mime_sniffer_reference.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {
namespace {

TEST(MimeSnifferRustTest, PrivateBoundaryEmptyViewsAndStaticResults) {
  using namespace mime_sniffer_internal;
  EXPECT_FALSE(rust_mime_looks_binary(nullptr, 0));
  EXPECT_TRUE(rust_mime_should_sniff(nullptr, 0, 1, nullptr, 0));
  EXPECT_FALSE(rust_mime_should_sniff(nullptr, 0, 0, nullptr, 0));
  const auto local = rust_mime_sniff_local(nullptr, 0);
  EXPECT_EQ(local.flags & 2, 0);
  const auto sniffed = rust_mime_sniff(nullptr, 0, nullptr, 0, nullptr, 0, 0, 0,
                                       0, std::numeric_limits<size_t>::max());
  EXPECT_EQ(sniffed.flags, 0);
  // SAFETY: the private ABI returns an immutable static string of this length.
  const auto mime = UNSAFE_BUFFERS(std::string_view(
      reinterpret_cast<const char*>(sniffed.mime_data), sniffed.mime_size));
  EXPECT_EQ(mime, "text/plain");
}

constexpr std::array<std::string_view, 17> kHints = {
    "",
    "text/plain",
    "application/octet-stream",
    "text/xml",
    "application/xml",
    "image/png",
    "unknown/unknown",
    "application/unknown",
    "*/*",
    "noslash",
    "application/msword",
    "application/vnd.ms-excel",
    "application/vnd.ms-powerpoint",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    "application/vnd.ms-word.document.macroenabled.12"};

constexpr std::array<std::string_view, 10> kUrls = {
    "",
    "https://example.test/a",
    "file:///a",
    "data:text/plain,hi",
    "https://example.test/a.crx",
    "https://example.test/a.DOC",
    "https://example.test/a.xls",
    "https://example.test/a.pptx",
    "https://example.test/a.docx",
    "https://example.test/a.xlsx"};

void Compare(std::string_view bytes,
             const GURL& url,
             std::string_view hint,
             ForceSniffFileUrlsForHtml force) {
  const std::string hint_string(hint);
  std::string actual = "unchanged sentinel";
  std::string expected = actual;
  EXPECT_EQ(SniffMimeType(bytes, url, hint_string, force, &actual),
            mime_sniffer_test::SniffMimeType(bytes, url, hint_string, force,
                                             &expected));
  EXPECT_EQ(actual, expected);
  actual = expected = "unchanged sentinel";
  EXPECT_EQ(SniffMimeTypeFromLocalData(bytes, &actual),
            mime_sniffer_test::SniffMimeTypeFromLocalData(bytes, &expected));
  EXPECT_EQ(actual, expected);
  EXPECT_EQ(LooksLikeBinary(bytes), mime_sniffer_test::LooksLikeBinary(bytes));
}

std::vector<std::string> Inputs() {
  return {
      "",
      "<html>",
      "\t\r\n <!DOCTYPE HTML>",
      "<?xml",
      "<?XML",
      "<SCRIPT",
      "<!--",
      "<head",
      "<iframe",
      "<h1",
      "<div",
      "<font",
      "<table",
      "<a",
      "<style",
      "<title",
      "<b",
      "<body",
      "<br",
      "<p",
      "<?xml ?> <!DOCTYPE a> <FEED",
      "<rss",
      "<other",
      "plain text",
      "%PDF-1.7",
      "%!PS-Adobe-",
      "GIF87a",
      "GIF89a",
      std::string("\x89PNG\r\n\x1a\n", 8),
      std::string("\xff\xd8\xff", 3),
      "BM",
      "#!",
      "%!",
      "From",
      ">From",
      std::string("\x1f\x8b\x08", 3),
      "?RMF",
      std::string(
          "\x30\x26\xb2\x75\x8e\x66\xcf\x11\xa6\xd9\x00\xaa\x00\x62\xce\x6c",
          16),
      "I I",
      "II*",
      std::string("MM\0*", 4),
      "ID3",
      "RIFF1234WEBPVP",
      std::string("\x1a\x45\xdf\xa3", 4),
      std::string("PK\3\4", 4),
      std::string("Rar!\x1a\7\0", 7),
      std::string("\xd7\xcd\xc6\x9a", 4),
      "MZ",
      std::string("\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1", 8),
      "#define",
      std::string("\0\0\1\0", 4),
      "RIFF1234WAVEfmt ",
      "RIFF1234AVI LIST",
      std::string("OggS\0", 5),
      std::string("\0\0\1\xbf", 4),
      std::string("\xff\xe7", 2),
      "abcdftyp3g",
      "abcdftypavcl",
      "abcdftyp",
      "abcdmoov",
      "CWS",
      "FWS",
      "FLV",
      "fLaC",
      "#!AMR\n",
      std::string("II\x2a\0\x10\0\0\0CR", 10),
      std::string("II\x1a\0\0\0HEAPCCDR", 14),
      std::string("\0MRM", 4),
      "MMOR",
      "IIRO",
      "IIRS",
      "FUJIFILMCCD-RAW ",
      std::string("IIU\0\10\0\0\0", 8),
      std::string("IIU\0\30\0\0\0", 8),
      "MMMMRaw",
      "FOVb",
      std::string("\xfe\xff", 2),
      std::string("\xff\xfe", 2),
      std::string("\xef\xbb\xbf", 3),
      std::string("Cr24\2\0\0\0", 8),
      std::string("Cr24\3\0\0\0", 8)};
}

TEST(MimeSnifferRustTest, SignaturesAcrossPolicyAndExtensions) {
  for (const auto& bytes : Inputs()) {
    for (const auto& hint : kHints) {
      for (const auto& url : kUrls) {
        for (const auto force : {ForceSniffFileUrlsForHtml::kDisabled,
                                 ForceSniffFileUrlsForHtml::kEnabled}) {
          Compare(bytes, GURL(url), hint, force);
        }
      }
    }
  }
}

TEST(MimeSnifferRustTest, EverySignatureTruncation) {
  for (const auto& bytes : Inputs()) {
    for (size_t size = 0; size <= bytes.size(); ++size) {
      for (const auto& hint : kHints) {
        Compare(std::string_view(bytes).substr(0, size), GURL(kUrls[4]), hint,
                ForceSniffFileUrlsForHtml::kDisabled);
      }
    }
  }
}

TEST(MimeSnifferRustTest, SingleByteMutations) {
  for (auto bytes : Inputs()) {
    for (size_t pos = 0; pos < bytes.size(); ++pos) {
      const char saved = bytes[pos];
      for (unsigned value = 0; value < 256; ++value) {
        bytes[pos] = static_cast<char>(value);
        Compare(bytes, GURL(kUrls[1]), "",
                ForceSniffFileUrlsForHtml::kDisabled);
      }
      bytes[pos] = saved;
    }
  }
}

TEST(MimeSnifferRustTest, BinaryByteClassificationAndBoundaries) {
  for (unsigned byte = 0; byte < 256; ++byte) {
    for (size_t position : {0u, 7u, 31u, 255u, 511u, 1023u, 1024u}) {
      std::string bytes(1100, 'A');
      bytes[position] = static_cast<char>(byte);
      Compare(bytes, GURL(kUrls[1]), "text/plain",
              ForceSniffFileUrlsForHtml::kDisabled);
    }
  }
  for (size_t size :
       {0, 1, 7, 8, 41, 42, 299, 300, 511, 512, 1023, 1024, 1025}) {
    for (char filler : {'A', ' ', '\t', '\x00'}) {
      Compare(std::string(size, filler), GURL(kUrls[1]), "",
              ForceSniffFileUrlsForHtml::kDisabled);
    }
  }
}

TEST(MimeSnifferRustTest, XmlIterationAndWhitespaceLimits) {
  for (size_t count = 0; count < 8; ++count) {
    std::string declarations;
    for (size_t i = 0; i < count; ++i) {
      declarations += "<?xml> <!DoCtYpE>";
    }
    for (std::string_view tail : {"", "<feed", "<rss", "<other", "none", "<"}) {
      Compare(declarations + std::string(tail), GURL(kUrls[1]), "text/xml",
              ForceSniffFileUrlsForHtml::kDisabled);
    }
  }
  for (size_t count : {0, 1, 299, 300, 511, 512, 1024}) {
    for (char whitespace : {' ', '\t', '\r', '\n', '\v', '\f'}) {
      Compare(std::string(count, whitespace) + "<html>", GURL(kUrls[1]), "",
              ForceSniffFileUrlsForHtml::kDisabled);
    }
  }
}

TEST(MimeSnifferRustTest, ShouldSniffPolicyAndFirstHeaderOnly) {
  for (const auto& url_string : kUrls) {
    const GURL url(url_string);
    for (const auto& hint : kHints) {
      EXPECT_EQ(ShouldSniffMimeType(url, nullptr, hint),
                mime_sniffer_test::ShouldSniffMimeType(url, nullptr, hint));
      for (std::string_view first :
           {"nosniff", "NoSnIfF", "sniff", "", "nosniff, sniff"}) {
        auto headers = HttpResponseHeaders::Builder({1, 1}, "200 OK")
                           .AddHeader("x-content-type-options", first)
                           .AddHeader("x-content-type-options", "nosniff")
                           .Build();
        EXPECT_EQ(
            ShouldSniffMimeType(url, headers.get(), hint),
            mime_sniffer_test::ShouldSniffMimeType(url, headers.get(), hint));
      }
    }
  }
}

TEST(MimeSnifferRustTest, EmbeddedNulAndHighBitHints) {
  for (const auto& bytes : Inputs()) {
    for (const std::string& hint :
         {std::string("text/plain\0", 11), std::string("\xff/a", 3),
          std::string("unknown\0/unknown", 16)}) {
      Compare(bytes, GURL(kUrls[1]), hint,
              ForceSniffFileUrlsForHtml::kDisabled);
    }
  }
}

TEST(MimeSnifferRustTest, HintMayAliasOutput) {
  for (const auto& bytes : Inputs()) {
    for (const auto hint : kHints) {
      std::string actual(hint), expected(hint);
      EXPECT_EQ(SniffMimeType(bytes, GURL(kUrls[1]), actual,
                              ForceSniffFileUrlsForHtml::kDisabled, &actual),
                mime_sniffer_test::SniffMimeType(
                    bytes, GURL(kUrls[1]), expected,
                    ForceSniffFileUrlsForHtml::kDisabled, &expected));
      EXPECT_EQ(actual, expected);
    }
  }
}

TEST(MimeSnifferRustTest, ContentMayAliasOutputWithoutReallocation) {
  for (const auto& bytes : Inputs()) {
    for (const auto hint : kHints) {
      for (size_t offset = 0; offset <= std::min(bytes.size(), size_t{3});
           ++offset) {
        std::string actual(bytes), expected(bytes);
        actual.reserve(2048);
        expected.reserve(2048);
        const std::string_view a = std::string_view(actual).substr(offset);
        const std::string_view e = std::string_view(expected).substr(offset);
        EXPECT_EQ(SniffMimeType(a, GURL(kUrls[1]), std::string(hint),
                                ForceSniffFileUrlsForHtml::kDisabled, &actual),
                  mime_sniffer_test::SniffMimeType(
                      e, GURL(kUrls[1]), std::string(hint),
                      ForceSniffFileUrlsForHtml::kDisabled, &expected));
        EXPECT_EQ(actual, expected);
      }
    }
  }
}

TEST(MimeSnifferRustTest, HintAndContentMayBothAliasOutput) {
  for (const auto& bytes : Inputs()) {
    for (size_t offset = 0; offset <= std::min(bytes.size(), size_t{3});
         ++offset) {
      std::string actual(bytes), expected(bytes);
      actual.reserve(2048);
      expected.reserve(2048);
      const std::string_view a = std::string_view(actual).substr(offset);
      const std::string_view b = std::string_view(expected).substr(offset);
      EXPECT_EQ(SniffMimeType(a, GURL(kUrls[8]), actual,
                              ForceSniffFileUrlsForHtml::kEnabled, &actual),
                mime_sniffer_test::SniffMimeType(
                    b, GURL(kUrls[8]), expected,
                    ForceSniffFileUrlsForHtml::kEnabled, &expected));
      EXPECT_EQ(actual, expected);
    }
  }
}

TEST(MimeSnifferRustTest, AliasedViewsAcrossScratchAndSniffingLimits) {
  for (size_t length :
       {63u, 64u, 65u, 511u, 512u, 1023u, 1024u, 1025u, 4096u}) {
    for (size_t offset : {0u, 1u, 500u}) {
      if (offset > length) {
        continue;
      }
      for (auto hint : kHints) {
        std::string actual(length, 'A'), expected(length, 'A');
        actual.reserve(length + 2048);
        expected.reserve(length + 2048);
        const std::string_view a = std::string_view(actual).substr(offset);
        const std::string_view b = std::string_view(expected).substr(offset);
        EXPECT_EQ(SniffMimeType(a, GURL(kUrls[1]), std::string(hint),
                                ForceSniffFileUrlsForHtml::kDisabled, &actual),
                  mime_sniffer_test::SniffMimeType(
                      b, GURL(kUrls[1]), std::string(hint),
                      ForceSniffFileUrlsForHtml::kDisabled, &expected));
        EXPECT_EQ(actual, expected);
      }
    }
  }
}

TEST(MimeSnifferRustTest, ShortAliasedContentWithoutReserve) {
  // A known short hint never requires an early allocation. Any later magic
  // result is assigned immediately before return, after all content reads.
  for (const auto& bytes : Inputs()) {
    std::string actual(bytes), expected(bytes);
    const std::string_view a(actual), b(expected);
    EXPECT_EQ(SniffMimeType(a, GURL(kUrls[1]), "image/png",
                            ForceSniffFileUrlsForHtml::kDisabled, &actual),
              mime_sniffer_test::SniffMimeType(
                  b, GURL(kUrls[1]), "image/png",
                  ForceSniffFileUrlsForHtml::kDisabled, &expected));
    EXPECT_EQ(actual, expected);
  }
}

}  // namespace
}  // namespace net
