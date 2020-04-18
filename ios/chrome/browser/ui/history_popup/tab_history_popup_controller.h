// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_POPUP_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_POPUP_CONTROLLER_H_

#import "ios/chrome/browser/ui/popup_menu/popup_menu_controller.h"

#include "ios/web/public/navigation_item_list.h"

@protocol TabHistoryPopupCommands;

// The view controller for the tab history menu that appears when the user long
// presses the back or forward button.
@interface TabHistoryPopupController : PopupMenuController

// Initializes the popup to display |items| with the given |origin| that is
// relevant to the |parent|'s coordinate system.
- (id)initWithOrigin:(CGPoint)origin
          parentView:(UIView*)parent
               items:(const web::NavigationItemList&)items
          dispatcher:(id<TabHistoryPopupCommands>)dispatcher
    NS_DESIGNATED_INITIALIZER;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_POPUP_CONTROLLER_H_
