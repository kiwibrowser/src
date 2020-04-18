// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "extensions/browser/app_window/native_app_window.h"

namespace extensions {

namespace {

typedef PlatformAppBrowserTest AppWindowBrowserTest;

// This test is disabled on Linux because of the unpredictable nature of native
// windows. We cannot assume that the window manager will insert any title bar
// at all, so the test may fail on certain window managers.
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
#define MAYBE_FrameInsetsForDefaultFrame DISABLED_FrameInsetsForDefaultFrame
#else
#define MAYBE_FrameInsetsForDefaultFrame FrameInsetsForDefaultFrame
#endif

// Verifies that the NativeAppWindows implement GetFrameInsets() correctly.
// See http://crbug.com/346115
IN_PROC_BROWSER_TEST_F(AppWindowBrowserTest, MAYBE_FrameInsetsForDefaultFrame) {
  AppWindow* app_window = CreateTestAppWindow("{}");
  NativeAppWindow* native_window = app_window->GetBaseWindow();
  gfx::Insets insets = native_window->GetFrameInsets();

  // It is a reasonable assumption that the top padding must be greater than
  // the bottom padding due to the title bar.
  EXPECT_GT(insets.top(), insets.bottom());

  CloseAppWindow(app_window);
}

// Verifies that the NativeAppWindows implement GetFrameInsets() correctly.
// See http://crbug.com/346115
IN_PROC_BROWSER_TEST_F(AppWindowBrowserTest, FrameInsetsForColoredFrame) {
  AppWindow* app_window =
      CreateTestAppWindow("{ \"frame\": { \"color\": \"#ffffff\" } }");
  NativeAppWindow* native_window = app_window->GetBaseWindow();
  gfx::Insets insets = native_window->GetFrameInsets();

  // It is a reasonable assumption that the top padding must be greater than
  // the bottom padding due to the title bar.
  EXPECT_GT(insets.top(), insets.bottom());

  CloseAppWindow(app_window);
}

// Verifies that the NativeAppWindows implement GetFrameInsets() correctly for
// frameless windows.
IN_PROC_BROWSER_TEST_F(AppWindowBrowserTest, FrameInsetsForNoFrame) {
  AppWindow* app_window = CreateTestAppWindow("{ \"frame\": \"none\" }");
  NativeAppWindow* native_window = app_window->GetBaseWindow();
  gfx::Insets insets = native_window->GetFrameInsets();

  // All insets must be zero.
  EXPECT_EQ(0, insets.top());
  EXPECT_EQ(0, insets.bottom());
  EXPECT_EQ(0, insets.left());
  EXPECT_EQ(0, insets.right());

  CloseAppWindow(app_window);
}

}  // namespace

}  // namespace extensions
