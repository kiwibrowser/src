// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_TOOLBAR_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_TOOLBAR_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class NewTabButton;
@protocol ToolbarCommands;

@class StackViewToolbarController;

// The delegate receives callbacks from UI events on the toolbar.
@protocol StackViewToolbarControllerDelegate

// Asks the delegate to dismiss the tab switcher UI.
- (void)stackViewToolbarControllerShouldDismiss:
    (StackViewToolbarController*)stackViewToolbarController;

@end

// Toolbar controller for the card stack view, adding a new tab button.
@interface StackViewToolbarController : ToolbarController

@property(nonatomic, readonly) NewTabButton* openNewTabButton;
@property(nonatomic, weak) id<StackViewToolbarControllerDelegate> delegate;

- (instancetype)initWithDispatcher:
    (id<ApplicationCommands, BrowserCommands, OmniboxFocuser, ToolbarCommands>)
        dispatcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_TOOLBAR_CONTROLLER_H_
