// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_APPS_APP_SHIM_MENU_CONTROLLER_MAC_H_
#define CHROME_BROWSER_UI_COCOA_APPS_APP_SHIM_MENU_CONTROLLER_MAC_H_

#import <Cocoa/Cocoa.h>
#include <string>

#include "base/mac/scoped_nsobject.h"

@class DoppelgangerMenuItem;

// This controller listens to NSWindowDidBecomeMainNotification and
// NSWindowDidResignMainNotification and modifies the main menu bar to mimic a
// main menu for the app. When an app window becomes main, all Chrome menu items
// are hidden and menu items for the app are appended to the main menu. When the
// app window resigns main, its menu items are removed and all Chrome menu items
// are unhidden.
@interface AppShimMenuController : NSObject {
 @private
  // The extension id of the currently focused packaged app.
  std::string appId_;
  // Items that need a doppelganger.
  base::scoped_nsobject<DoppelgangerMenuItem> aboutDoppelganger_;
  base::scoped_nsobject<DoppelgangerMenuItem> hideDoppelganger_;
  base::scoped_nsobject<DoppelgangerMenuItem> quitDoppelganger_;
  base::scoped_nsobject<DoppelgangerMenuItem> newDoppelganger_;
  base::scoped_nsobject<DoppelgangerMenuItem> openDoppelganger_;
  base::scoped_nsobject<DoppelgangerMenuItem> closeWindowDoppelganger_;
  base::scoped_nsobject<DoppelgangerMenuItem> allToFrontDoppelganger_;
  // Menu items for the currently focused packaged app.
  base::scoped_nsobject<NSMenuItem> appMenuItem_;
  base::scoped_nsobject<NSMenuItem> fileMenuItem_;
  base::scoped_nsobject<NSMenuItem> editMenuItem_;
  base::scoped_nsobject<NSMenuItem> windowMenuItem_;
  // Additional menu items for hosted apps.
  base::scoped_nsobject<NSMenuItem> viewMenuItem_;
  base::scoped_nsobject<NSMenuItem> historyMenuItem_;
}

@end

#endif  // CHROME_BROWSER_UI_COCOA_APPS_APP_SHIM_MENU_CONTROLLER_MAC_H_
