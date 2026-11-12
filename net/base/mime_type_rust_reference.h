// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_MIME_TYPE_RUST_REFERENCE_H_
#define NET_BASE_MIME_TYPE_RUST_REFERENCE_H_
#include "base/component_export.h"
#include "net/base/mime_util.h"
namespace net::mime_type_test {
COMPONENT_EXPORT(MIME_TYPE_TEST_REFERENCE)
bool LegacyParseMimeType(std::string_view, std::string*, base::StringPairs*);
COMPONENT_EXPORT(MIME_TYPE_TEST_REFERENCE)
bool LegacyWithoutParameter(std::string_view, std::string*, std::string*);
COMPONENT_EXPORT(MIME_TYPE_TEST_REFERENCE)
void LegacyContentType(std::string_view,
                       std::string*,
                       std::string*,
                       bool*,
                       std::string*);
}  // namespace net::mime_type_test
#endif
