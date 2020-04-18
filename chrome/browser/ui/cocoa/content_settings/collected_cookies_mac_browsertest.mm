// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/collected_cookies_mac.h"

#include <stddef.h>

#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/test/test_utils.h"

using web_modal::WebContentsModalDialogManager;

class CollectedCookiesMacTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    // Spawn a cookies dialog.  Note that |cookies_dialog_| will delete itself
    // automatically when it closes.
    cookies_dialog_ = new CollectedCookiesMac(
        browser()->tab_strip_model()->GetActiveWebContents());

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    WebContentsModalDialogManager* web_contents_modal_dialog_manager =
        WebContentsModalDialogManager::FromWebContents(web_contents);
    EXPECT_TRUE(web_contents_modal_dialog_manager->IsDialogActive());
  }

  // Shows infobar when dialog is closes.
  void SetDialogChanged() {
    [cookies_dialog_->sheet_controller() blockOrigin:nullptr];
  }

  void CloseCookiesDialog() {
    cookies_dialog_->PerformClose();
    content::RunAllPendingInMessageLoop();
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    WebContentsModalDialogManager* web_contents_modal_dialog_manager =
        WebContentsModalDialogManager::FromWebContents(web_contents);
    EXPECT_FALSE(web_contents_modal_dialog_manager->IsDialogActive());
  }

  size_t infobar_count() const {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    return web_contents ?
        InfoBarService::FromWebContents(web_contents)->infobar_count() : 0;
  }

  CollectedCookiesWindowController* sheet_controller() {
    return cookies_dialog_->sheet_controller();
  }

 private:
  CollectedCookiesMac* cookies_dialog_ = nullptr;
};

// Tests closing dialog without changing data.
IN_PROC_BROWSER_TEST_F(CollectedCookiesMacTest, Close) {
  CloseCookiesDialog();
  EXPECT_EQ(0u, infobar_count());
}

// Tests closing dialog with changing data. Dialog will show infobar.
// TODO(vitalybuka): Fix and re-enable http://crbug.com/450295
IN_PROC_BROWSER_TEST_F(CollectedCookiesMacTest, DISABLED_ChangeAndClose) {
  SetDialogChanged();
  CloseCookiesDialog();
  EXPECT_EQ(1u, infobar_count());
}

// Tests closing tab after changing dialog data. Changed dialog should not
// show infobar or crash because InfoBarService is gone.
// TODO(vitalybuka): Fix and re-enable http://crbug.com/450295
IN_PROC_BROWSER_TEST_F(CollectedCookiesMacTest, DISABLED_ChangeAndCloseTab) {
  SetDialogChanged();

  // Closing the owning tab will close dialog.
  browser()->tab_strip_model()->GetActiveWebContents()->Close();

  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(0u, infobar_count());
}

IN_PROC_BROWSER_TEST_F(CollectedCookiesMacTest, Outlets) {
  EXPECT_TRUE([sheet_controller() allowedTreeController]);
  EXPECT_TRUE([sheet_controller() blockedTreeController]);
  EXPECT_TRUE([sheet_controller() allowedOutlineView]);
  EXPECT_TRUE([sheet_controller() blockedOutlineView]);
  EXPECT_TRUE([sheet_controller() infoBar]);
  EXPECT_TRUE([sheet_controller() infoBarIcon]);
  EXPECT_TRUE([sheet_controller() infoBarText]);
  EXPECT_TRUE([sheet_controller() tabView]);
  EXPECT_TRUE([sheet_controller() blockedScrollView]);
  EXPECT_TRUE([sheet_controller() blockedCookiesText]);
  EXPECT_TRUE([sheet_controller() cookieDetailsViewPlaceholder]);

  [sheet_controller() closeSheet:nil];
  content::RunAllPendingInMessageLoop();
}
