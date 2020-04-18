// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/buildflag.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "ui/base/ui_features.h"

// Create the controller for the Browser, which handles loading the browser
// window from the nib. The controller takes ownership of |browser|.
// static
BrowserWindow* BrowserWindow::CreateBrowserWindowCocoa(Browser* browser,
                                                       bool user_gesture) {
  BrowserWindowController* controller =
      [[BrowserWindowController alloc] initWithBrowser:browser];
  return [controller browserWindow];
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
// Cocoa-only builds always use Cocoa browser windows.
// static
BrowserWindow* BrowserWindow::CreateBrowserWindow(Browser* browser,
                                                  bool user_gesture) {
  return CreateBrowserWindowCocoa(browser, user_gesture);
}
#endif
