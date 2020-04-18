// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_positioner.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_ui_element.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_consumer.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@protocol OmniboxFocuser;
@class ToolbarButtonFactory;
@class ToolbarButtonUpdater;
@class ToolbarToolsMenuButton;

// View controller for a toolbar, which will show a horizontal row of
// controls and/or labels.
@interface ToolbarViewController
    : UIViewController<ActivityServicePositioner,
                       FullscreenUIElement,
                       ToolbarConsumer>

- (instancetype)initWithDispatcher:
                    (id<ApplicationCommands, BrowserCommands>)dispatcher
                     buttonFactory:(ToolbarButtonFactory*)buttonFactory
                     buttonUpdater:(ToolbarButtonUpdater*)buttonUpdater
                    omniboxFocuser:(id<OmniboxFocuser>)omniboxFocuser
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;

// The dispatcher for this view controller.
@property(nonatomic, weak) id<ApplicationCommands, BrowserCommands> dispatcher;
// The ToolsMenu button.
@property(nonatomic, strong, readonly) ToolbarToolsMenuButton* toolsMenuButton;
// Whether the toolbar is in the expanded state or not.
@property(nonatomic, assign) BOOL expanded;
// Omnibox focuser.
@property(nonatomic, weak) id<OmniboxFocuser> omniboxFocuser;
// Background color of the toolbar when presented on the NTP.
@property(nonatomic, strong, readonly) UIColor* backgroundColorNTP;

// Sets the location bar view, containing the omnibox.
- (void)setLocationBarView:(UIView*)locationBarView;

// Adds the toolbar expanded state animations to |animator|, and changes the
// toolbar constraints in preparation for the animation.
// Adds the toolbar post-expanded state animations (fading-in the contract
// button) to the |completionAnimator|.
- (void)addToolbarExpansionAnimations:(UIViewPropertyAnimator*)animator
                   completionAnimator:
                       (UIViewPropertyAnimator*)completionAnimator;
// Adds the toolbar contracted state animations to |animator|, and changes the
// toolbar constraints in preparation for the animation.
- (void)addToolbarContractionAnimations:(UIViewPropertyAnimator*)animator;
// Updates the view so a snapshot can be taken. It needs to be adapted,
// depending on if it is a snapshot displayed |onNTP| or not.
- (void)updateForSnapshotOnNTP:(BOOL)onNTP;
// Resets the view after taking a snapshot.
- (void)resetAfterSnapshot;
// Sets the background color of the Toolbar to the one of the Incognito NTP,
// with an |alpha|.
- (void)setBackgroundToIncognitoNTPColorWithAlpha:(CGFloat)alpha;
// Briefly animate the progress bar when a pre-rendered tab is displayed.
- (void)showPrerenderingAnimation;
// IPad only function. iPad doesn't animate when locationBar is first responder,
// but there are small UI changes to the locationBarContainer.
- (void)locationBarIsFirstResonderOnIPad:(BOOL)isFirstResponder;
// Activates constraints to simulate a safe area with |fakeSafeAreaInsets|
// insets. The insets will be used as leading/trailing wrt RTL. Those
// constraints have a higher priority than the one used to respect the safe
// area. They need to be deactivated for the toolbar to respect the safe area
// again. The fake safe area can be bigger or smaller than the real safe area.
- (void)activateFakeSafeAreaInsets:(UIEdgeInsets)fakeSafeAreaInsets;
// Deactivates the constraints used to create a fake safe area.
- (void)deactivateFakeSafeAreaInsets;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_VIEW_CONTROLLER_H_
