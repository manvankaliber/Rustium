// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/content_browser_client.h"

#include "base/test/scoped_feature_list.h"
#include "base/unguessable_token.h"
#include "build/build_config.h"
#include "content/public/common/content_features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

TEST(ContentBrowserClientTest, IsFullCookieAccessAllowed) {
  ContentBrowserClient client;
  const GURL url = GURL("https://example.com");
  const url::Origin origin = url::Origin::Create(url);

  // Without a nonce, full cookie access is allowed by default.
  EXPECT_TRUE(client.IsFullCookieAccessAllowed(
      nullptr, nullptr, url, blink::StorageKey::CreateFirstParty(origin), {}));

  // With a nonce, full cookie access is not allowed.
  EXPECT_FALSE(client.IsFullCookieAccessAllowed(
      nullptr, nullptr, url,
      blink::StorageKey::CreateWithNonce(origin,
                                         base::UnguessableToken::Create()),
      {}));
}


}  // namespace content
