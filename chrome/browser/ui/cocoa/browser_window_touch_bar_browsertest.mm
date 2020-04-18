// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_touch_bar.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_test_util.cc"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest_mac.h"

@interface TestingBrowserWindowTouchBar : BrowserWindowTouchBar

@property(nonatomic, assign) BOOL hasUpdatedReloadStop;

@end

@implementation TestingBrowserWindowTouchBar

@synthesize hasUpdatedReloadStop = hasUpdatedReloadStop_;

- (void)updateReloadStopButton {
  [super updateReloadStopButton];
  hasUpdatedReloadStop_ = YES;
}

@end

class BrowserWindowTouchBarTest : public InProcessBrowserTest {
 public:
  BrowserWindowTouchBarTest() : InProcessBrowserTest() {}

  void SetUpOnMainThread() override {
    // Ownership is passed to BrowserWindowController in
    // -setBrowserWindowTouchBar:
    browser_touch_bar_ = [[TestingBrowserWindowTouchBar alloc]
                initWithBrowser:browser()
        browserWindowController:browser_window_controller()];
    [browser_window_controller() setBrowserWindowTouchBar:browser_touch_bar_];
  }

  BrowserWindowController* browser_window_controller() {
    return [BrowserWindowController
        browserWindowControllerForWindow:browser()
                                             ->window()
                                             ->GetNativeWindow()];
  }

  TestingBrowserWindowTouchBar* browser_touch_bar() const {
    return browser_touch_bar_;
  }

 private:
  TestingBrowserWindowTouchBar* browser_touch_bar_;

  test::ScopedMacViewsBrowserMode cocoa_browser_mode_{false};

  DISALLOW_COPY_AND_ASSIGN(BrowserWindowTouchBarTest);
};

// Test if the proper controls gets updated when the page loads.
IN_PROC_BROWSER_TEST_F(BrowserWindowTouchBarTest, PageLoadInvalidate) {
  if (@available(macOS 10.12.2, *)) {
    NSButton* reload_stop = [browser_touch_bar() reloadStopButton];
    EXPECT_TRUE(reload_stop);

    browser()->window()->UpdateReloadStopState(true, false);
    EXPECT_TRUE([browser_touch_bar() hasUpdatedReloadStop]);
    EXPECT_EQ(IDC_STOP, [reload_stop tag]);

    // Reset the flag.
    [browser_touch_bar() setHasUpdatedReloadStop:NO];

    browser()->window()->UpdateReloadStopState(false, false);
    EXPECT_TRUE([browser_touch_bar() hasUpdatedReloadStop]);
    EXPECT_EQ(IDC_RELOAD, [reload_stop tag]);
  }
}

// Test if the touch bar gets invalidated when the active tab is changed.
IN_PROC_BROWSER_TEST_F(BrowserWindowTouchBarTest, TabChanges) {
  if (@available(macOS 10.12.2, *)) {
    NSWindow* window = [browser_window_controller() window];
    NSTouchBar* touch_bar = [browser_touch_bar() makeTouchBar];
    [window setTouchBar:touch_bar];
    EXPECT_TRUE([window touchBar]);

    // The window should have a new touch bar.
    [browser_window_controller() onActiveTabChanged:nullptr to:nullptr];
    EXPECT_NE(touch_bar, [window touchBar]);
  }
}

// Test if the touch bar gets invalidated when the starred state is changed.
IN_PROC_BROWSER_TEST_F(BrowserWindowTouchBarTest, StarredChanges) {
  if (@available(macOS 10.12.2, *)) {
    NSWindow* window = [browser_window_controller() window];
    NSTouchBar* touch_bar = [browser_touch_bar() makeTouchBar];
    [window setTouchBar:touch_bar];
    EXPECT_TRUE([window touchBar]);

    // The window should have a new touch bar.
    [browser_window_controller() setStarredState:YES];
    EXPECT_NE(touch_bar, [window touchBar]);
  }
}

// Tests if the touch bar gets invalidated if the default search engine has
// changed.
IN_PROC_BROWSER_TEST_F(BrowserWindowTouchBarTest, SearchEngineChanges) {
  if (@available(macOS 10.12.2, *)) {
    PrefService* prefs = browser()->profile()->GetPrefs();
    DCHECK(prefs);

    NSWindow* window = [browser_window_controller() window];
    NSTouchBar* touch_bar = [browser_touch_bar() makeTouchBar];
    [window setTouchBar:touch_bar];
    EXPECT_TRUE([window touchBar]);

    // Change the default search engine.
    std::unique_ptr<TemplateURLData> data =
        GenerateDummyTemplateURLData("poutine");
    prefs->Set(DefaultSearchManager::kDefaultSearchProviderDataPrefName,
               *TemplateURLDataToDictionary(*data));

    // The window should have a new touch bar.
    EXPECT_NE(touch_bar, [window touchBar]);
  }
}
