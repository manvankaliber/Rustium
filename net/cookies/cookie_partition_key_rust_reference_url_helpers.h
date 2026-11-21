// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_COOKIES_COOKIE_PARTITION_KEY_RUST_REFERENCE_URL_HELPERS_H_
#define NET_COOKIES_COOKIE_PARTITION_KEY_RUST_REFERENCE_URL_HELPERS_H_
#include <string_view>
#include "url/gurl.h"
namespace net::cookie_partition_key_reference {
GURL ChangeWebSocketSchemeToHttpScheme(const GURL& url);
bool IsStandardSchemeWithNetworkHost(std::string_view scheme);
}
#endif
