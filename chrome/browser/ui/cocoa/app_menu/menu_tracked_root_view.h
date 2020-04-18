// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_APP_MENU_MENU_TRACKED_ROOT_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_APP_MENU_MENU_TRACKED_ROOT_VIEW_H_

#import <Cocoa/Cocoa.h>

// An instance of MenuTrackedRootView should be the root of the view hierarchy
// of the custom view of NSMenuItems. If the user opens the menu in a non-
// sticky fashion (i.e. clicks, holds, and drags) and then releases the mouse
// over the menu item, it will cancel tracking on the |[menuItem_ menu]|.
@interface MenuTrackedRootView : NSView {
 @private
  // The menu item whose custom view's root view is an instance of this class.
  NSMenuItem* menuItem_;  // weak
}

@property(assign, nonatomic) NSMenuItem* menuItem;

@end

#endif  // CHROME_BROWSER_UI_COCOA_APP_MENU_MENU_TRACKED_ROOT_VIEW_H_
