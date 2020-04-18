// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/collected_cookies_views.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class CollectedCookiesViewsTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->Start());

    // Disable cookies.
    CookieSettingsFactory::GetForProfile(browser()->profile())
        ->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);

    // Load a page with cookies.
    ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/cookie1.html"));

    // Spawn a cookies dialog.  Note that |cookies_dialog_| will delete itself
    // automatically when it closes.
    cookies_dialog_ = new CollectedCookiesViews(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  // Closing dialog with modified data will shows infobar.
  void SetDialogChanged() { cookies_dialog_->status_changed_ = true; }

  void CloseCookiesDialog() { cookies_dialog_->Close(); }

  size_t infobar_count() const {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    return web_contents ?
        InfoBarService::FromWebContents(web_contents)->infobar_count() : 0;
  }

 private:
  CollectedCookiesViews* cookies_dialog_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(CollectedCookiesViewsTest, CloseDialog) {
  // Test closing dialog without changing data.
  CloseCookiesDialog();
  EXPECT_EQ(0u, infobar_count());
}

IN_PROC_BROWSER_TEST_F(CollectedCookiesViewsTest, ChangeAndCloseDialog) {
  // Test closing dialog with changing data. Dialog will show infobar.
  SetDialogChanged();
  CloseCookiesDialog();
  EXPECT_EQ(1u, infobar_count());
}

IN_PROC_BROWSER_TEST_F(CollectedCookiesViewsTest, ChangeAndNavigateAway) {
  // Test navigation after changing dialog data. Changed dialog should not show
  // infobar or crash because InfoBarService is gone.

  SetDialogChanged();

  // Navigation in the owning tab will close dialog.
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/cookie2.html"));

  EXPECT_EQ(0u, infobar_count());
}

IN_PROC_BROWSER_TEST_F(CollectedCookiesViewsTest, ChangeAndCloseTab) {
  // Test closing tab after changing dialog data. Changed dialog should not
  // show infobar or crash because InfoBarService is gone.

  SetDialogChanged();

  // Closing the owning tab will close dialog.
  browser()->tab_strip_model()->GetActiveWebContents()->Close();

  EXPECT_EQ(0u, infobar_count());
}
