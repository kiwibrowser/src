// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

@class BubbleViewControllerPresenter;
@class CommandDispatcher;
@protocol PopupMenuUIUpdating;
class WebStateList;

// Coordinator for the popup menu, handling the commands.
@interface PopupMenuCoordinator : ChromeCoordinator

// Dispatcher used by this coordinator to receive the PopupMenuCommands.
@property(nonatomic, weak) CommandDispatcher* dispatcher;
// The WebStateList this coordinator is handling.
@property(nonatomic, assign) WebStateList* webStateList;
// UI updater.
@property(nonatomic, weak) id<PopupMenuUIUpdating> UIUpdater;
// Bubble view presenter for the incognito tip.
@property(nonatomic, weak)
    BubbleViewControllerPresenter* incognitoTabTipPresenter;

// Returns whether this coordinator is showing a popup menu.
- (BOOL)isShowingPopupMenu;

@end

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_COORDINATOR_H_
