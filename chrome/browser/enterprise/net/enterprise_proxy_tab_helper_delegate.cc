// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/enterprise/net/enterprise_proxy_tab_helper_delegate.h"

#include "build/build_config.h"
#include "content/public/browser/web_contents.h"


namespace enterprise_net {

EnterpriseProxyTabHelperDelegate::EnterpriseProxyTabHelperDelegate() = default;

EnterpriseProxyTabHelperDelegate::~EnterpriseProxyTabHelperDelegate() = default;

void EnterpriseProxyTabHelperDelegate::SignIn(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  // TODO(crbug.com/532559920): Implement desktop re-auth and login flow for
  // enterprise proxy.
}

}  // namespace enterprise_net
