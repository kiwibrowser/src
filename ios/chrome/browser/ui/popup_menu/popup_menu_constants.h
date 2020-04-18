// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_CONSTANTS_H_

#import <UIKit/UIKit.h>

// Accessibility IDs for the table view in various kinds of popup menus.
extern NSString* const kPopupMenuToolsMenuTableViewId;
extern NSString* const kPopupMenuNavigationTableViewId;

// Alpha for the background color of the highlighted items.
extern const CGFloat kSelectedItemBackgroundAlpha;
// Duration of the highlight animation of the popup menu.
extern const CGFloat kHighlightAnimationDuration;

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_CONSTANTS_H_
