// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_CONTROLLER_PRIVATE_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_CONTROLLER_PRIVATE_H_

#import "ios/chrome/browser/ui/stack_view/card_set.h"
#import "ios/chrome/browser/ui/stack_view/stack_view_controller.h"

@class ToolbarController, ToolsMenuCoordinator;

namespace {
// Styles used to specify the transition animation type for presenting and
// dismissing the stack view.
typedef enum {
  STACK_TRANSITION_STYLE_NONE = 0,
  STACK_TRANSITION_STYLE_PRESENTING,
  STACK_TRANSITION_STYLE_DISMISSING
} StackTransitionStyle;
}  // namespace

@interface StackViewController ()<CardSetObserver,
                                  UIGestureRecognizerDelegate,
                                  UIScrollViewDelegate>

// Separated out to allow tests to mock CardSet directly.
- (instancetype)initWithMainCardSet:(CardSet*)mainCardSet
                         otrCardSet:(CardSet*)otrCardSet
                      activeCardSet:(CardSet*)activeCardSet
         applicationCommandEndpoint:
             (id<ApplicationCommands>)applicationCommandEndpoint
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Sets up the display views of the card sets, including initialization,
// initial size configuration, adding as subviews of the scroll view, and
// attaching to the card sets.
- (void)setUpDisplayViews;

// Used during |-dealloc| and |-didReceiveMemoryWaring| to clear any references
// to the cards and removes |self| as an observer to any notifications.
- (void)cleanUpViewsAndNotifications;

// Recenters the scroll view if it gets too close to one of its boundaries.
// Updates the positions of the card stacks' display views to remain visually
// centered.
- (void)recenterScrollViewIfNecessary;

// Returns YES if the currently active card set is the incognito set.
- (BOOL)isCurrentSetIncognito;

// Returns the card set that is not currently active.
- (CardSet*)inactiveCardSet;

// Makes |cardSet| the active set, updating the UI accordingly.
- (void)setActiveCardSet:(CardSet*)cardSet;

// Returns YES if both decks should be visible.
- (BOOL)bothDecksShouldBeDisplayed;

// Returns the region, in scroll view coordinates, of the inactive deck.
- (CGRect)inactiveDeckRegion;

// The currently active card set.
@property(nonatomic, weak, readonly) CardSet* activeCardSet;

// The current transition style.
@property(nonatomic, assign) StackTransitionStyle transitionStyle;

// Will be set to YES when a transition animation is cancelled before it can
// finish.
@property(nonatomic, assign) BOOL transitionWasCancelled;

// The owner of |transitionToolbarController|.
@property(nonatomic, strong) id<ToolbarOwner> transitionToolbarOwner;

// Snapshot of the toolbar, used in transition.
@property(nonatomic, strong) UIView* transitionToolbarSnapshot;

// The dummy view used in the transition animation.
@property(nonatomic, strong) UIView* dummyToolbarBackgroundView;

// Records which card was tapped mid-presentation animation, if any.
// TODO(crbug.com/546209): Implement reversed animations for dismissing with a
// new selected card mid-presentation.
@property(nonatomic, strong) StackCard* transitionTappedCard;

// |YES| if there is card set animation being processed.
@property(nonatomic, readonly) BOOL inActiveDeckChangeAnimation;

// Coordinator for the tools menu UI.
@property(nonatomic, readonly, strong)
    ToolsMenuCoordinator* toolsMenuCoordinator;

@end

@interface StackViewController (Testing)

// The driver of scroll events.
@property(nonatomic, readonly) UIScrollView* scrollView;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_CONTROLLER_PRIVATE_H_
