// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/apps/app_shim_menu_controller_mac.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

#include "base/command_line.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/apps/app_shim/extension_app_shim_handler_mac.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "extensions/common/extension.h"
#include "extensions/test/extension_test_message_listener.h"
#import "ui/base/test/windowed_nsnotification_observer.h"

using extensions::AppWindow;
using extensions::Extension;
using ui_test_utils::ShowAndFocusNativeWindow;

namespace {

class AppShimMenuControllerUITest : public extensions::PlatformAppBrowserTest {
 protected:
  AppShimMenuControllerUITest() {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformAppBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableAppWindowCycling);
  }

  void SetUpOnMainThread() override {
    PlatformAppBrowserTest::SetUpOnMainThread();
    const Extension* extension =
        LoadAndLaunchPlatformApp("minimal", "Launched");

    // First create an extra app window and an extra browser window. Only after
    // the first call to ui_test_utils::ShowAndFocusNativeWindow(..) will the
    // windows activate, because the test binary has a default activation policy
    // of "prohibited".
    app1_ = GetFirstAppWindow();
    app2_ = CreateAppWindow(browser()->profile(), extension);
    browser1_ = browser()->window();
    browser2_ = (new Browser(Browser::CreateParams(profile(), true)))->window();
    browser2_->Show();

    // Since a pending key status change on any window could cause the test to
    // become flaky, watch everything closely.
    app1_watcher_.reset([[WindowedNSNotificationObserver alloc]
        initForNotification:NSWindowDidBecomeMainNotification
                     object:app1_->GetNativeWindow()]);
    app2_watcher_.reset([[WindowedNSNotificationObserver alloc]
        initForNotification:NSWindowDidBecomeMainNotification
                     object:app2_->GetNativeWindow()]);
    browser1_watcher_.reset([[WindowedNSNotificationObserver alloc]
        initForNotification:NSWindowDidBecomeMainNotification
                     object:browser1_->GetNativeWindow()]);
    browser2_watcher_.reset([[WindowedNSNotificationObserver alloc]
        initForNotification:NSWindowDidBecomeMainNotification
                     object:browser2_->GetNativeWindow()]);
  }

  void TearDownOnMainThread() override {
    CloseAppWindow(app1_);
    CloseAppWindow(app2_);
    browser2_->Close();
    PlatformAppBrowserTest::TearDownOnMainThread();
  }

  // First wait for and verify the given activation counts on all windows, then
  // expect that |main_window| is the active window.
  void ExpectActiveWithCounts(NSWindow* main_window,
                              int app1_count,
                              int app2_count,
                              int browser1_count,
                              int browser2_count) {
    EXPECT_TRUE([app1_watcher_ waitForCount:app1_count]);
    EXPECT_TRUE([app2_watcher_ waitForCount:app2_count]);
    EXPECT_TRUE([browser1_watcher_ waitForCount:browser1_count]);
    EXPECT_TRUE([browser2_watcher_ waitForCount:browser2_count]);
    EXPECT_TRUE([main_window isMainWindow]);
  }

  // Send Cmd+`. Note that it needs to go into kCGSessionEventTap, so NSEvents
  // and [NSApp sendEvent:] doesn't work.
  void CycleWindows() {
    ui_test_utils::SendGlobalKeyEventsAndWait(kVK_ANSI_Grave,
                                              ui::EF_COMMAND_DOWN);
  }

  AppWindow* app1_;
  AppWindow* app2_;
  BrowserWindow* browser1_;
  BrowserWindow* browser2_;
  base::scoped_nsobject<WindowedNSNotificationObserver> app1_watcher_,
      app2_watcher_, browser1_watcher_, browser2_watcher_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppShimMenuControllerUITest);
};

// Test that switching to a packaged app changes window cycling behavior.
IN_PROC_BROWSER_TEST_F(AppShimMenuControllerUITest, WindowCycling) {
  EXPECT_FALSE([app1_->GetNativeWindow() isMainWindow]);
  EXPECT_TRUE(ShowAndFocusNativeWindow(app1_->GetNativeWindow()));
  ExpectActiveWithCounts(app1_->GetNativeWindow(), 1, 0, 0, 0);

  // Packaged app is active, so Cmd+` should cycle between the two app windows.
  CycleWindows();
  ExpectActiveWithCounts(app2_->GetNativeWindow(), 1, 1, 0, 0);
  CycleWindows();
  ExpectActiveWithCounts(app1_->GetNativeWindow(), 2, 1, 0, 0);

  // Activate one of the browsers. Then cycling should go between the browsers.
  EXPECT_TRUE(ShowAndFocusNativeWindow(browser1_->GetNativeWindow()));
  ExpectActiveWithCounts(browser1_->GetNativeWindow(), 2, 1, 1, 0);
  CycleWindows();
  ExpectActiveWithCounts(browser2_->GetNativeWindow(), 2, 1, 1, 1);
  CycleWindows();
  ExpectActiveWithCounts(browser1_->GetNativeWindow(), 2, 1, 2, 1);
}

}  // namespace
