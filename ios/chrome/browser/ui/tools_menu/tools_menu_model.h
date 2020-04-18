// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_MODEL_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_MODEL_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"

// Total number of possible menu items.
const int kToolsMenuNumberOfItems = 15;

// Initialization table for all possible commands to initialize the
// tools menu at run time. Data initialized into this structure is not mutable.
struct MenuItemInfo {
  int title_id;
  NSString* accessibility_id;
  int command_id;
  SEL selector;
  int toolbar_types;
  // |visibility| is applied if a menu item is included for a given
  // |toolbar_types|. A value of 0 means the menu item is always visible for
  // the given |toolbar_types|.
  int visibility;
  // Custom class, if any, for the menu item, or |nil|.
  Class item_class;
};

// Flags for different toolbar types
typedef NS_OPTIONS(NSUInteger, ToolbarType) {
  // clang-format off
  ToolbarTypeNone            = 0,
  ToolbarTypeWebiPhone       = 1 << 0,
  ToolbarTypeWebiPad         = 1 << 1,
  ToolbarTypeNoTabsiPad      = 1 << 2,
  ToolbarTypeSwitcheriPhone  = 1 << 3,
  ToolbarTypeWebAll          = ToolbarTypeWebiPhone | ToolbarTypeWebiPad,
  ToolbarTypeAll             = ToolbarTypeWebAll |
                               ToolbarTypeSwitcheriPhone |
                               ToolbarTypeNoTabsiPad,
  // clang-format on
};

// All possible items.
extern const MenuItemInfo itemInfoList[kToolsMenuNumberOfItems];

// Returns true if a given item should be visible based on the Toolbar type
// and configuration.
bool ToolsMenuItemShouldBeVisible(const MenuItemInfo& item,
                                  ToolbarType toolbarType,
                                  ToolsMenuConfiguration* configuration);

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_MODEL_H_
