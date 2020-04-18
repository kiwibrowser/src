// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_POPUP_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_POPUP_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_controller.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class ToolsMenuConfiguration;

// The view controller for the tools menu within the top toolbar.
// The menu is composed of two main view: a top view with icons and a bottom
// view with a table view of menu items.
@interface ToolsPopupController : PopupMenuController

@property(nonatomic, assign) BOOL isCurrentPageBookmarked;

// Initializes the popup with the given |configuration|, a set of information
// used to determine the appearance of the menu and the entries displayed.
// The popup will be presented immediately with an animation, and the
// |animationCompletion| block will be called when the presentation animation
// is finished.
- (instancetype)
initAndPresentWithConfiguration:(ToolsMenuConfiguration*)configuration
                     dispatcher:
                         (id<ApplicationCommands, BrowserCommands>)dispatcher
                     completion:(ProceduralBlock)animationCompletion;

// Called when the current tab loading state changes.
- (void)setIsTabLoading:(BOOL)isTabLoading;

// TODO(stuartmorgan): Should the set of options that are passed in to the
// constructor just have the ability to specify whether commands should be
// enabled or disabled rather than having these individual setters? b/6048639
// Informs tools popup menu whether "Find In Page..." command should be
// enabled.
- (void)setCanShowFindBar:(BOOL)enabled;

// Informs tools popup menu whether "Share..." command should be enabled.
- (void)setCanShowShareMenu:(BOOL)enabled;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_POPUP_CONTROLLER_H_
