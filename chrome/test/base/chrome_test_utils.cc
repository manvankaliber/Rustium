// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/chrome_test_utils.h"

#include "build/build_config.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace chrome_test_utils {

content::WebContents* GetActiveWebContents(
    const PlatformBrowserTest* browser_test) {
  return browser_test->browser()->GetTabStripModel()->GetActiveWebContents();
}

tabs::TabInterface* GetActiveTab(const PlatformBrowserTest* browser_test) {
  return browser_test->browser()->GetTabStripModel()->GetActiveTab();
}

content::WebContents* GetWebContentsAt(const PlatformBrowserTest* browser_test,
                                       int index) {
  return browser_test->browser()->GetTabStripModel()->GetWebContentsAt(index);
}

Profile* GetProfile(const PlatformBrowserTest* browser_test) {
  return browser_test->browser()->GetProfile();
}

bool NavigateToURL(content::WebContents* web_contents, const GURL& url) {
  content::TestNavigationObserver observer(web_contents);
  // The return value is ignored because some tests load URLs that cause
  // redirects, or are blocked URLs, which make NavigateToURL return false.
  std::ignore = content::NavigateToURL(web_contents, url);
  // Wait for load to stop.
  observer.Wait();
  return observer.last_navigation_succeeded();
}

}  // namespace chrome_test_utils
