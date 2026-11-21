// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/cookies/cookie_partition_key_rust_reference_url_helpers.h"
#include "base/check.h"
#include "url/url_constants.h"
#include "url/url_util.h"
namespace net::cookie_partition_key_reference {
GURL ChangeWebSocketSchemeToHttpScheme(const GURL& url) {
  DCHECK(url.SchemeIsWSOrWSS());
  GURL::Replacements replace_scheme;
  replace_scheme.SetSchemeStr(url.SchemeIs(url::kWssScheme) ? url::kHttpsScheme
                                                            : url::kHttpScheme);
  return url.ReplaceComponents(replace_scheme);
}

bool IsStandardSchemeWithNetworkHost(std::string_view scheme) {
  // file scheme is special. Windows file share origins can have network hosts.
  if (scheme == url::kFileScheme) {
    return true;
  }

  url::SchemeType scheme_type;
  if (!url::GetStandardSchemeType(scheme, &scheme_type)) {
    return false;
  }
  return scheme_type == url::SCHEME_WITH_HOST_PORT_AND_USER_INFORMATION ||
         scheme_type == url::SCHEME_WITH_HOST_AND_PORT;
}

}  // namespace net::cookie_partition_key_reference
