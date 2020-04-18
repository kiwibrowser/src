// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class PopupMenuController;
@class PopupMenuView;

// A protocol required by delegates of the PopupMenuController.
@protocol PopupMenuDelegate
// Instructs the delegate the PopupMenuController is done and should be
// dismissed.
- (void)dismissPopupMenu:(PopupMenuController*)controller;
@end

// TODO(crbug.com/800266): Remove this class.
// The base view controller for popup menus within the top toolbar like the
// Tools menu.
@interface PopupMenuController : NSObject

// View that contains all subviews. Subclasses should add their views to
// |containerView_|.
@property(nonatomic, readonly, strong) UIView* containerView;
// Displays the background and border of the popup.
@property(nonatomic, readonly, strong) PopupMenuView* popupContainer;
// Button used to dismiss the popup. Covers the entire window behind the popup
// menu. Catches any touch events outside of the popup and invokes
// |-tappedBehindPopup:| if there are any.
@property(nonatomic, readonly, strong) UIButton* backgroundButton;
// Delegate for the popup menu.
@property(nonatomic, weak) id<PopupMenuDelegate> delegate;
// Dispatcher for browser commands.
@property(nonatomic, weak) id<ApplicationCommands, BrowserCommands> dispatcher;

// Initializes the PopupMenuController and adds its views inside of parent.
- (id)initWithParentView:(UIView*)parent;

// The designated initializer.
// Initializes the PopupMenuController and adds its views inside of parent.
// Additional backgroundButton params are used to change the look and behavior
// of |backgroundButton_|.
// backgroundButtonParent is the view to add |backgroundButton_| to. If nil,
// |backgroundButton_| is added to |containerView_|.
// backgroundButtonColor is the backgroundColor to set for |backgroundButton_|.
// backgroundButtonAlpha is the alpha to set for |backgroundButton_|.
// backgroundButtonTag is the tag to set for |backgroundButton_|.
// backgroundButtonSelector is the action to add for touch down events on
// |backgroundButton_|.
- (id)initWithParentView:(UIView*)parent
      backgroundButtonParent:(UIView*)backgroundButtonParent
       backgroundButtonColor:(UIColor*)backgroundButtonColor
       backgroundButtonAlpha:(CGFloat)backgroundButtonAlpha
         backgroundButtonTag:(NSInteger)backgroundButtonTag
    backgroundButtonSelector:(SEL)backgroundButtonSelector;

// Sets the optimal size of the popup needed to display its contents without
// exceeding the bounds of the window. Also positions the arrow to point at
// |origin|.
- (void)setOptimalSize:(CGSize)optimalSize atOrigin:(CGPoint)origin;

// Called when the user taps outside of the popup.
- (void)tappedBehindPopup:(id)sender;

// Called to display the popup with a fade in animation. |completion| is
// executed once the fade animation is complete.
- (void)fadeInPopupFromSource:(CGPoint)source
                toDestination:(CGPoint)destination
                   completion:(ProceduralBlock)completion;

- (void)dismissAnimatedWithCompletion:(ProceduralBlock)completion;

// Called to display the popup with a fade in animation. |completionBlock| is
// executed once the fade animation is complete.
- (void)fadeInPopup:(void (^)(BOOL finished))completionBlock;

@end

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_CONTROLLER_H_
