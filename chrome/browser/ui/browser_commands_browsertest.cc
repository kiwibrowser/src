// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_commands.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"

namespace chrome {

using BrowserCommandsTest = InProcessBrowserTest;

// Verify that calling BookmarkCurrentPageIgnoringExtensionOverrides() just
// after closing all tabs doesn't cause a crash. https://crbug.com/799668
IN_PROC_BROWSER_TEST_F(BrowserCommandsTest, BookmarkCurrentPageAfterCloseTabs) {
  browser()->tab_strip_model()->CloseAllTabs();
  BookmarkCurrentPageIgnoringExtensionOverrides(browser());
}

}  // namespace chrome
