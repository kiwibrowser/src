// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TABBED_BROWSER_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_TABBED_BROWSER_WINDOW_H_

#import <AppKit/AppKit.h>

#include "chrome/browser/ui/cocoa/framed_browser_window.h"

// Represents a browser window with tabs and customized window controls.
@interface TabbedBrowserWindow : FramedBrowserWindow
@end

#endif  // CHROME_BROWSER_UI_COCOA_TABBED_BROWSER_WINDOW_H_
