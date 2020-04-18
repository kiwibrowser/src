// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller_test.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

using ::testing::NiceMock;

namespace {

class ConstrainedWindowDelegateMock : public ConstrainedWindowMacDelegate {
 public:
  MOCK_METHOD1(OnConstrainedWindowClosed, void(ConstrainedWindowMac*));
};

}  // namespace

class ConstrainedWindowMacTest : public InProcessBrowserTest {
 public:
  ConstrainedWindowMacTest()
      : InProcessBrowserTest(),
        tab0_(NULL),
        tab1_(NULL),
        controller_(NULL),
        tab_view0_(NULL),
        tab_view1_(NULL) {
    sheet_window_.reset([[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 30, 30)
                  styleMask:NSTitledWindowMask
                    backing:NSBackingStoreBuffered
                      defer:NO]);
    [sheet_window_ setReleasedWhenClosed:NO];
    sheet_.reset([[CustomConstrainedWindowSheet alloc]
        initWithCustomWindow:sheet_window_]);
    [sheet_ hideSheet];
  }

  void SetUpOnMainThread() override {
    AddTabAtIndex(1, GURL("about:blank"), ui::PAGE_TRANSITION_LINK);
    tab0_ = browser()->tab_strip_model()->GetWebContentsAt(0);
    tab1_ = browser()->tab_strip_model()->GetWebContentsAt(1);
    EXPECT_EQ(tab1_, browser()->tab_strip_model()->GetActiveWebContents());

    controller_ = [BrowserWindowController browserWindowControllerForWindow:
        browser()->window()->GetNativeWindow()];
    EXPECT_TRUE(controller_);
    tab_view0_ = [[controller_ tabStripController] viewAtIndex:0];
    EXPECT_TRUE(tab_view0_);
    tab_view1_ = [[controller_ tabStripController] viewAtIndex:1];
    EXPECT_TRUE(tab_view1_);
  }

 protected:
  base::scoped_nsobject<CustomConstrainedWindowSheet> sheet_;
  base::scoped_nsobject<NSWindow> sheet_window_;
  content::WebContents* tab0_;
  content::WebContents* tab1_;
  BrowserWindowController* controller_;
  NSView* tab_view0_;
  NSView* tab_view1_;
};

// Test that a sheet added to a inactive tab is not shown until the
// tab is activated.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowMacTest, ShowInInactiveTab) {
  // Show dialog in non active tab.
  NiceMock<ConstrainedWindowDelegateMock> delegate;
  std::unique_ptr<ConstrainedWindowMac> dialog =
      CreateAndShowWebModalDialogMac(&delegate, tab0_, sheet_);
  EXPECT_EQ(0.0, [sheet_window_ alphaValue]);

  // Switch to inactive tab.
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);

  dialog->CloseWebContentsModalDialog();
}

// If a tab has never been shown then the associated tab view for the web
// content will not be created. Verify that adding a constrained window to such
// a tab works correctly.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowMacTest, ShowInUninitializedTab) {
  content::WebContents::CreateParams create_params(browser()->profile());
  create_params.initially_hidden = true;
  std::unique_ptr<content::WebContents> web_contents(
      content::WebContents::Create(create_params));
  chrome::AddWebContents(browser(), NULL, std::move(web_contents),
                         WindowOpenDisposition::NEW_BACKGROUND_TAB, gfx::Rect(),
                         false);
  content::WebContents* tab2 =
      browser()->tab_strip_model()->GetWebContentsAt(2);
  ASSERT_TRUE(tab2);
  EXPECT_FALSE([tab2->GetNativeView() superview]);

  // Show dialog and verify that it's not visible yet.
  NiceMock<ConstrainedWindowDelegateMock> delegate;
  std::unique_ptr<ConstrainedWindowMac> dialog =
      CreateAndShowWebModalDialogMac(&delegate, tab2, sheet_);
  EXPECT_FALSE([sheet_window_ isVisible]);

  // Activate the tab and verify that the constrained window is shown.
  browser()->tab_strip_model()->ActivateTabAt(2, true);
  EXPECT_TRUE([tab2->GetNativeView() superview]);
  EXPECT_TRUE([sheet_window_ isVisible]);
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);

  dialog->CloseWebContentsModalDialog();
}

// Test that adding a sheet disables tab dragging.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowMacTest, TabDragging) {
  NiceMock<ConstrainedWindowDelegateMock> delegate;
  std::unique_ptr<ConstrainedWindowMac> dialog =
      CreateAndShowWebModalDialogMac(&delegate, tab1_, sheet_);

  // Verify that the dialog disables dragging.
  EXPECT_TRUE([controller_ isTabDraggable:tab_view0_]);
  EXPECT_FALSE([controller_ isTabDraggable:tab_view1_]);

  dialog->CloseWebContentsModalDialog();
}

// Test that closing a browser window with a sheet works.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowMacTest, BrowserWindowClose) {
  NiceMock<ConstrainedWindowDelegateMock> delegate;
  std::unique_ptr<ConstrainedWindowMac> dialog(
      CreateAndShowWebModalDialogMac(&delegate, tab1_, sheet_));
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);

  // Close the browser window.
  base::scoped_nsobject<NSWindow> browser_window(
      [browser()->window()->GetNativeWindow() retain]);
  EXPECT_TRUE([browser_window isVisible]);
  [browser()->window()->GetNativeWindow() performClose:nil];
  EXPECT_FALSE([browser_window isVisible]);
}

// Test that closing a tab with a sheet works.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowMacTest, TabClose) {
  NiceMock<ConstrainedWindowDelegateMock> delegate;
  std::unique_ptr<ConstrainedWindowMac> dialog(
      CreateAndShowWebModalDialogMac(&delegate, tab1_, sheet_));
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);

  // Close the tab.
  TabStripModel* tab_strip = browser()->tab_strip_model();
  EXPECT_EQ(2, tab_strip->count());
  EXPECT_TRUE(tab_strip->CloseWebContentsAt(1,
                                            TabStripModel::CLOSE_USER_GESTURE));
  EXPECT_EQ(1, tab_strip->count());
}

// Test that the sheet is still visible after entering and exiting fullscreen.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowMacTest, BrowserWindowFullscreen) {
  NiceMock<ConstrainedWindowDelegateMock> delegate;
  std::unique_ptr<ConstrainedWindowMac> dialog(
      CreateAndShowWebModalDialogMac(&delegate, tab1_, sheet_));
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);

  // Toggle fullscreen twice: one for entering and one for exiting.
  // Check to see if the sheet is visible.
  for (int i = 0; i < 2; i++) {
    {
      // NOTIFICATION_FULLSCREEN_CHANGED is sent asynchronously. Wait for the
      // notification before testing the sheet's visibility.
      std::unique_ptr<FullscreenNotificationObserver> waiter(
          new FullscreenNotificationObserver());
      browser()
          ->exclusive_access_manager()
          ->fullscreen_controller()
          ->ToggleBrowserFullscreenMode();
      waiter->Wait();
    }
    EXPECT_EQ(1.0, [sheet_window_ alphaValue]);
  }

  dialog->CloseWebContentsModalDialog();
}
