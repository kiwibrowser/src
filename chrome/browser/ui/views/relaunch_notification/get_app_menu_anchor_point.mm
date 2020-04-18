// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/views/relaunch_notification/get_app_menu_anchor_point.h"

#import "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/gfx/mac/coordinate_conversion.h"

gfx::Point GetAppMenuAnchorPoint(Browser* browser) {
  NSWindow* parent_window = browser->window()->GetNativeWindow();
  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:parent_window];
  ToolbarController* tbc = [bwc toolbarController];
  NSPoint ns_point = [tbc appMenuBubblePoint];
  return gfx::ScreenPointFromNSPoint(
      ui::ConvertPointFromWindowToScreen(parent_window, ns_point));
}
