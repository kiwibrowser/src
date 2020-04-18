// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/one_click_signin_dialog_controller.h"

#include "base/mac/foundation_util.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#import "chrome/browser/ui/cocoa/one_click_signin_view_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#import "testing/gtest_mac.h"

class OneClickSigninDialogControllerTest : public InProcessBrowserTest {
 public:
  OneClickSigninDialogControllerTest()
    : controller_(NULL),
      sync_mode_(OneClickSigninSyncStarter::SYNC_WITH_DEFAULT_SETTINGS),
      callback_count_(0) {
  }

 protected:
  void SetUpOnMainThread() override {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetWebContentsAt(0);
    BrowserWindow::StartSyncCallback callback = base::Bind(
        &OneClickSigninDialogControllerTest::OnStartSyncCallback,
        base::Unretained(this));
    controller_ = new OneClickSigninDialogController(
        web_contents, callback, base::string16());
    EXPECT_NSEQ(@"OneClickSigninDialog",
                [controller_->view_controller() nibName]);
  }

  // Weak pointer. Will delete itself when dialog closes.
  OneClickSigninDialogController* controller_;
  OneClickSigninSyncStarter::StartSyncMode sync_mode_;
  int callback_count_;

 private:
  void OnStartSyncCallback(OneClickSigninSyncStarter::StartSyncMode mode) {
    sync_mode_ = mode;
    ++callback_count_;
  }

  DISALLOW_COPY_AND_ASSIGN(OneClickSigninDialogControllerTest);
};

// Test that the dialog calls the callback if the OK button is clicked.
// Callback should be called to setup sync with default settings.
IN_PROC_BROWSER_TEST_F(OneClickSigninDialogControllerTest, OK) {
  [controller_->view_controller() ok:nil];
  EXPECT_EQ(OneClickSigninSyncStarter::SYNC_WITH_DEFAULT_SETTINGS, sync_mode_);
  EXPECT_EQ(1, callback_count_);
}

// Test that the dialog does call the callback if the Undo button
// is clicked. Callback should be called to abort the sync.
IN_PROC_BROWSER_TEST_F(OneClickSigninDialogControllerTest, Undo) {
  [controller_->view_controller() onClickUndo:nil];
  EXPECT_EQ(OneClickSigninSyncStarter::UNDO_SYNC, sync_mode_);
  EXPECT_EQ(1, callback_count_);
}

// Test that the advanced callback is run if its corresponding button
// is clicked.
IN_PROC_BROWSER_TEST_F(OneClickSigninDialogControllerTest, Advanced) {
  [controller_->view_controller() onClickAdvancedLink:nil];
  EXPECT_EQ(OneClickSigninSyncStarter::CONFIGURE_SYNC_FIRST, sync_mode_);
  EXPECT_EQ(1, callback_count_);
}

// Test that the dialog calls the callback if the bubble is closed.
// Callback should be called to setup sync with default settings.
IN_PROC_BROWSER_TEST_F(OneClickSigninDialogControllerTest, Close) {
  controller_->constrained_window()->CloseWebContentsModalDialog();
  EXPECT_EQ(OneClickSigninSyncStarter::UNDO_SYNC, sync_mode_);
  EXPECT_EQ(1, callback_count_);
}

// Test that clicking the learn more link opens a new window.
IN_PROC_BROWSER_TEST_F(OneClickSigninDialogControllerTest, LearnMore) {
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  OneClickSigninViewController* view_controller =
      base::mac::ObjCCastStrict<OneClickSigninViewController>(
          controller_->view_controller());
  [view_controller textView:[view_controller linkViewForTesting]
              clickedOnLink:@(chrome::kChromeSyncLearnMoreURL)
                    atIndex:0];
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());
}
