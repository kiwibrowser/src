// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"

#include "build/build_config.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/theme_provider.h"

using BrowserNonClientFrameViewBrowserTest = extensions::ExtensionBrowserTest;

// Test is Flaky on Windows see crbug.com/600201.
#if defined(OS_WIN)
#define MAYBE_InactiveSeparatorColor DISABLED_InactiveSeparatorColor
#elif defined(OS_MACOSX)
// Widget activation doesn't work on Mac: https://crbug.com/823543
#define MAYBE_InactiveSeparatorColor DISABLED_InactiveSeparatorColor
#else
#define MAYBE_InactiveSeparatorColor InactiveSeparatorColor
#endif

// Tests that the color returned by
// BrowserNonClientFrameView::GetToolbarTopSeparatorColor() tracks the window
// actiavtion state.
IN_PROC_BROWSER_TEST_F(BrowserNonClientFrameViewBrowserTest,
                       MAYBE_InactiveSeparatorColor) {
  // In the default theme, the active and inactive separator colors may be the
  // same.  Install a custom theme where they are different.
  InstallExtension(test_data_dir_.AppendASCII("theme"), 1);
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  const BrowserNonClientFrameView* frame_view =
      browser_view->frame()->GetFrameView();
  const ui::ThemeProvider* theme_provider = frame_view->GetThemeProvider();
  const SkColor theme_active_color =
      theme_provider->GetColor(ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR);
  const SkColor theme_inactive_color =
      theme_provider->GetColor(
          ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_INACTIVE);
  EXPECT_NE(theme_active_color, theme_inactive_color);

  // Check that the separator color is the active color when the window is
  // active.
  browser_view->Activate();
  EXPECT_TRUE(browser_view->IsActive());
  const SkColor frame_active_color = frame_view->GetToolbarTopSeparatorColor();
  EXPECT_EQ(theme_active_color, frame_active_color);

  // Check that the separator color is the inactive color when the window is
  // inactive.
  browser_view->Deactivate();
  EXPECT_FALSE(browser_view->IsActive());
  const SkColor frame_inactive_color =
      frame_view->GetToolbarTopSeparatorColor();
  EXPECT_EQ(theme_inactive_color, frame_inactive_color);
}
