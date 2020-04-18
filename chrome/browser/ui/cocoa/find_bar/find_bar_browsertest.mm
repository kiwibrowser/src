// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/foundation_util.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"
#include "chrome/browser/ui/cocoa/find_bar/find_bar_cocoa_controller.h"
#include "chrome/browser/ui/cocoa/find_bar/find_bar_text_field.h"
#import "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller_test.h"
#include "chrome/browser/ui/find_bar/find_bar.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "ui/base/test/ui_controls.h"
#import "ui/events/test/cocoa_test_event_utils.h"

// Expose private variables to make testing easier.
@implementation FindBarCocoaController (Testing)
- (NSButton*)nextButton {
  return nextButton_;
}

- (NSButton*)previousButton {
  return previousButton_;
}
@end

using content::WebContents;
using base::ASCIIToUTF16;

namespace {

const char kSimplePage[] = "simple.html";

bool FindBarHasFocus(Browser* browser) {
  NSWindow* window = browser->window()->GetNativeWindow();
  NSText* text_view = base::mac::ObjCCast<NSText>([window firstResponder]);
  return [[text_view delegate] isKindOfClass:[FindBarTextField class]];
}

GURL GetTestURL(const std::string& filename) {
  return ui_test_utils::GetTestUrl(base::FilePath().AppendASCII("find_in_page"),
                                   base::FilePath().AppendASCII(filename));
}

FindBarCocoaController* GetFindBarCocoaController(Browser* browser) {
  FindBarBridge* bridge =
      static_cast<FindBarBridge*>(browser->GetFindBarController()->find_bar());
  return bridge->find_bar_cocoa_controller();
}

NSButton* GetFindBarControllerNextButton(Browser* browser) {
  FindBarCocoaController* cocoacontroller = GetFindBarCocoaController(browser);
  return [cocoacontroller nextButton];
}

NSButton* GetFindBarControllerPreviousButton(Browser* browser) {
  FindBarCocoaController* cocoacontroller = GetFindBarCocoaController(browser);
  return [cocoacontroller previousButton];
}

int GetFindInContentsMatchCount(WebContents* contents,
                                const base::string16& search_string) {
  return ui_test_utils::FindInPage(contents, search_string, true, false, NULL,
                                   NULL);
}

void SimulateKeyPress(NSWindow* window, ui::KeyboardCode key) {
  base::RunLoop run_loop;
  ui_controls::EnableUIControls();
  ui_controls::SendKeyPressNotifyWhenDone(window, key, false, false, false,
                                          false, run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace

class FindBarBrowserTest : public InProcessBrowserTest {
 private:
  test::ScopedMacViewsBrowserMode cocoa_browser_mode_{false};
};

// Disabled. See https://crbug.com/845389 - this regressed somewhere between
// r545258 and r559030, but it may be obsolete soon.
IN_PROC_BROWSER_TEST_F(FindBarBrowserTest, DISABLED_FocusOnTabSwitch) {
  AddTabAtIndex(1, GURL("about:blank"), ui::PAGE_TRANSITION_LINK);
  browser()->GetFindBarController()->Show();

  // Verify that if the find bar has focus then switching tabs and changing
  // back sets focus back to the find bar.
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE(FindBarHasFocus(browser()));
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  EXPECT_FALSE(FindBarHasFocus(browser()));
  browser()->tab_strip_model()->ActivateTabAt(1, true);
  EXPECT_TRUE(FindBarHasFocus(browser()));

  // Verify that if the find bar does not have focus then switching tabs and
  // changing does not set focus to the find bar.
  browser()->window()->GetLocationBar()->FocusLocation(true);
  EXPECT_FALSE(FindBarHasFocus(browser()));
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  EXPECT_FALSE(FindBarHasFocus(browser()));
  browser()->tab_strip_model()->ActivateTabAt(1, true);
  EXPECT_FALSE(FindBarHasFocus(browser()));
}

IN_PROC_BROWSER_TEST_F(FindBarBrowserTest,
                       NextPreviousButtonsDisabledAfterFocusChange) {
  ui_test_utils::NavigateToURL(browser(), GetTestURL(kSimplePage));
  browser()->GetFindBarController()->Show();
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE(FindBarHasFocus(browser()));

  // Simulate key press for character not in page contents.
  SimulateKeyPress(browser()->window()->GetNativeWindow(), ui::VKEY_Z);
  WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(GetFindInContentsMatchCount(contents, ASCIIToUTF16("Z")), 0);

  // Buttons should be disabled as there were no results.
  NSButton* nextButton = GetFindBarControllerNextButton(browser());
  NSButton* previousButton = GetFindBarControllerPreviousButton(browser());
  EXPECT_FALSE([nextButton isEnabled]);
  EXPECT_FALSE([previousButton isEnabled]);

  // Move focus to the location bar then back to the find bar.
  browser()->window()->GetLocationBar()->FocusLocation(true);
  EXPECT_FALSE(FindBarHasFocus(browser()));
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE(FindBarHasFocus(browser()));

  // Buttons should remain disabled.
  EXPECT_FALSE([nextButton isEnabled]);
  EXPECT_FALSE([previousButton isEnabled]);
}

IN_PROC_BROWSER_TEST_F(FindBarBrowserTest,
                       NextPreviousButtonsEnabledAfterFocusChange) {
  ui_test_utils::NavigateToURL(browser(), GetTestURL(kSimplePage));
  browser()->GetFindBarController()->Show();
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE(FindBarHasFocus(browser()));

  // Simulate key press for character in page contents.
  SimulateKeyPress(browser()->window()->GetNativeWindow(), ui::VKEY_A);
  WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_GT(GetFindInContentsMatchCount(contents, ASCIIToUTF16("A")), 0);

  // Buttons should be enabled because we had results.
  NSButton* nextButton = GetFindBarControllerNextButton(browser());
  NSButton* previousButton = GetFindBarControllerPreviousButton(browser());
  EXPECT_TRUE([nextButton isEnabled]);
  EXPECT_TRUE([previousButton isEnabled]);

  // Move focus to the location bar then back to the find bar.
  browser()->window()->GetLocationBar()->FocusLocation(true);
  EXPECT_FALSE(FindBarHasFocus(browser()));
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE(FindBarHasFocus(browser()));

  // Buttons should remain enabled.
  EXPECT_TRUE([nextButton isEnabled]);
  EXPECT_TRUE([previousButton isEnabled]);
}

IN_PROC_BROWSER_TEST_F(FindBarBrowserTest,
                       NextPreviousButtonsEnabledAfterCloseAndTabSwitch) {
  AddTabAtIndex(1, GURL("about:blank"), ui::PAGE_TRANSITION_LINK);
  ui_test_utils::NavigateToURL(browser(), GetTestURL(kSimplePage));
  browser()->GetFindBarController()->Show();
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE(FindBarHasFocus(browser()));

  // Simulate key press for character in page contents.
  SimulateKeyPress(browser()->window()->GetNativeWindow(), ui::VKEY_A);
  WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_GT(GetFindInContentsMatchCount(contents, ASCIIToUTF16("A")), 0);

  // Close find bar, switch to another tab then switch back and reopen find bar.
  FindBarCocoaController* controller = GetFindBarCocoaController(browser());
  [controller close:nil];
  browser()->tab_strip_model()->ActivateTabAt(1, true);
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  browser()->GetFindBarController()->Show();
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();

  NSButton* nextButton = GetFindBarControllerNextButton(browser());
  NSButton* previousButton = GetFindBarControllerPreviousButton(browser());
  EXPECT_TRUE([nextButton isEnabled]);
  EXPECT_TRUE([previousButton isEnabled]);

  // Switch to another tab again and reopen find bar.
  [controller close:nil];
  browser()->tab_strip_model()->ActivateTabAt(1, true);
  browser()->GetFindBarController()->Show();
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE([nextButton isEnabled]);
  EXPECT_TRUE([previousButton isEnabled]);
}

// Disabled. See https://crbug.com/845389 - this regressed somewhere between
// r545258 and r559030, but it may be obsolete soon.
IN_PROC_BROWSER_TEST_F(FindBarBrowserTest, DISABLED_EscapeKey) {
  // Enter fullscreen.
  std::unique_ptr<FullscreenNotificationObserver> waiter(
      new FullscreenNotificationObserver());
  browser()
      ->exclusive_access_manager()
      ->fullscreen_controller()
      ->ToggleBrowserFullscreenMode();
  waiter->Wait();

  NSWindow* window = browser()->window()->GetNativeWindow();
  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:window];
  EXPECT_TRUE([bwc isInAppKitFullscreen]);

  // Show and focus on the find bar.
  browser()->GetFindBarController()->Show();
  browser()->GetFindBarController()->find_bar()->SetFocusAndSelection();
  EXPECT_TRUE(FindBarHasFocus(browser()));

  // Simulate a key press with the ESC key.
  SimulateKeyPress(window, ui::VKEY_ESCAPE);

  // Check that the browser is still in fullscreen and that the find bar has
  // lost focus.
  EXPECT_FALSE(FindBarHasFocus(browser()));
  EXPECT_TRUE([bwc isInAppKitFullscreen]);
}
