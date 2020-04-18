// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_PRIMARY_TOOLBAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_PRIMARY_TOOLBAR_COORDINATOR_H_

#import "ios/chrome/browser/ui/toolbar/public/fakebox_focuser.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#import "ios/chrome/browser/ui/toolbar/public/side_swipe_toolbar_snapshot_providing.h"

@protocol ActivityServicePositioner;
@protocol QRScannerResultLoading;
@protocol VoiceSearchControllerDelegate;

// Protocol defining a primary toolbar, in a paradigm where the toolbar can be
// split between primary and secondary.
@protocol PrimaryToolbarCoordinator<FakeboxFocuser,
                                    SideSwipeToolbarSnapshotProviding>

@property(nonatomic, strong, readonly) UIViewController* viewController;

// Returns the different protocols and superclass now implemented by the
// internal ViewController.
- (id<VoiceSearchControllerDelegate>)voiceSearchDelegate;
- (id<QRScannerResultLoading>)QRScannerResultLoader;
- (id<ActivityServicePositioner>)activityServicePositioner;
- (id<OmniboxFocuser>)omniboxFocuser;

// Stops the coordinator.
- (void)stop;

// Shows the animation when transitioning to a prerendered page.
- (void)showPrerenderingAnimation;
// Whether the omnibox is currently the first responder.
- (BOOL)isOmniboxFirstResponder;
// Whether the omnibox popup is currently presented.
- (BOOL)showingOmniboxPopup;

// Coordinates the location bar focusing/defocusing. For example, initiates
// transition to the expanded location bar state of the view controller.
- (void)transitionToLocationBarFocusedState:(BOOL)focused;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_PRIMARY_TOOLBAR_COORDINATOR_H_
