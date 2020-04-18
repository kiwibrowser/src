// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_NEW_TAB_MENU_VIEW_ITEM_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_NEW_TAB_MENU_VIEW_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_item.h"

// Specialization of a ToolsMenuViewItem for opening a new tab.
@interface NewTabMenuViewItem : ToolsMenuViewItem
@end

// Specialization of a ToolsMenuViewItem for opening a new incognito tab.
@interface NewIncognitoTabMenuViewItem : NewTabMenuViewItem
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_NEW_TAB_MENU_VIEW_ITEM_H_
