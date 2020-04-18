// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_views_mac.h"

#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_window_controller.h"

BrowserWindowController* BrowserWindowControllerForWindow(NSWindow* window) {
  return [BrowserWindowController browserWindowControllerForWindow:window];
}

TabWindowController* TabWindowControllerForWindow(NSWindow* window) {
  return [TabWindowController tabWindowControllerForWindow:window];
}
